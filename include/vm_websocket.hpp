#pragma once

#include "app.hpp"
#include "dbus_utility.hpp"
#include "privileges.hpp"
#include "websocket.hpp"

#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/readable_pipe.hpp>
#include <boost/asio/writable_pipe.hpp>
#include <boost/asio/write.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/core/flat_static_buffer.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/process/v2/process.hpp>
#include <boost/process/v2/stdio.hpp>

#include <csignal>
#include <string_view>

namespace crow
{

namespace obmc_vm
{

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static crow::websocket::Connection* session = nullptr;

// The max network block device buffer size is 128kb plus 16bytes
// for the message header:
// https://github.com/NetworkBlockDevice/nbd/blob/master/doc/proto.md#simple-reply-message
static constexpr auto nbdBufferSize = (128 * 1024 + 16) * 4;

class Handler : public std::enable_shared_from_this<Handler>
{
  public:
    Handler(const std::string& media, boost::asio::io_context& ios) :
        pipeOut(ios), pipeIn(ios),
        proxy(ios, "/usr/bin/nbd-proxy", {media},
              boost::process::v2::process_stdio{
                  .in = pipeIn, .out = pipeOut, .err = nullptr}),
        outputBuffer(new boost::beast::flat_static_buffer<nbdBufferSize>),
        inputBuffer(new boost::beast::flat_static_buffer<nbdBufferSize>)
    {}

    ~Handler() = default;

    Handler(const Handler&) = delete;
    Handler(Handler&&) = delete;
    Handler& operator=(const Handler&) = delete;
    Handler& operator=(Handler&&) = delete;

    void doClose()
    {
        // boost::process::child::terminate uses SIGKILL, need to send SIGTERM
        // to allow the proxy to stop nbd-client and the USB device gadget.
        int rc = kill(proxy.id(), SIGTERM);
        if (rc != 0)
        {
            BMCWEB_LOG_ERROR("Failed to terminate nbd-proxy: {}", errno);
            return;
        }

        proxy.wait();
    }

    void connect()
    {
        std::error_code ec;
        if (ec)
        {
            BMCWEB_LOG_ERROR("Couldn't connect to nbd-proxy: {}", ec.message());
            if (session != nullptr)
            {
                session->close("Error connecting to nbd-proxy");
            }
            return;
        }
        doWrite();
        doRead();
    }

    void doWrite()
    {
        if (doingWrite)
        {
            BMCWEB_LOG_DEBUG("Already writing.  Bailing out");
            return;
        }

        if (inputBuffer->size() == 0)
        {
            BMCWEB_LOG_DEBUG("inputBuffer empty.  Bailing out");
            return;
        }

        doingWrite = true;
        pipeIn.async_write_some(
            inputBuffer->data(),
            [this, self(shared_from_this())](const boost::beast::error_code& ec,
                                             std::size_t bytesWritten) {
            BMCWEB_LOG_DEBUG("Wrote {}bytes", bytesWritten);
            doingWrite = false;
            inputBuffer->consume(bytesWritten);

            if (session == nullptr)
            {
                return;
            }
            if (ec == boost::asio::error::eof)
            {
                session->close("VM socket port closed");
                return;
            }
            if (ec)
            {
                session->close("Error in writing to proxy port");
                BMCWEB_LOG_ERROR("Error in VM socket write {}", ec);
                return;
            }
            doWrite();
        });
    }

    void doRead()
    {
        std::size_t bytes = outputBuffer->capacity() - outputBuffer->size();

        pipeOut.async_read_some(
            outputBuffer->prepare(bytes),
            [this, self(shared_from_this())](
                const boost::system::error_code& ec, std::size_t bytesRead) {
            BMCWEB_LOG_DEBUG("Read done.  Read {} bytes", bytesRead);
            if (ec)
            {
                BMCWEB_LOG_ERROR("Couldn't read from VM port: {}", ec);
                if (session != nullptr)
                {
                    session->close("Error in connecting to VM port");
                }
                return;
            }
            if (session == nullptr)
            {
                return;
            }

            outputBuffer->commit(bytesRead);
            std::string_view payload(
                static_cast<const char*>(outputBuffer->data().data()),
                bytesRead);
            session->sendBinary(payload);
            outputBuffer->consume(bytesRead);

            doRead();
        });
    }

    boost::asio::readable_pipe pipeOut;
    boost::asio::writable_pipe pipeIn;
    boost::process::v2::process proxy;
    bool doingWrite{false};

    std::unique_ptr<boost::beast::flat_static_buffer<nbdBufferSize>>
        outputBuffer;
    std::unique_ptr<boost::beast::flat_static_buffer<nbdBufferSize>>
        inputBuffer;
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static std::shared_ptr<Handler> handler;

} // namespace obmc_vm

namespace nbd_proxy
{
using boost::asio::local::stream_protocol;

// The max network block device buffer size is 128kb plus 16bytes
// for the message header:
// https://github.com/NetworkBlockDevice/nbd/blob/master/doc/proto.md#simple-reply-message
static constexpr auto nbdBufferSize = (128 * 1024 + 16) * 4;

struct NbdProxyServer : std::enable_shared_from_this<NbdProxyServer>
{
    NbdProxyServer(crow::websocket::Connection& connIn,
                   const std::string& socketIdIn,
                   const std::string& endpointIdIn, const std::string& pathIn) :
        socketId(socketIdIn),
        endpointId(endpointIdIn), path(pathIn),

        peerSocket(connIn.getIoContext()),
        acceptor(connIn.getIoContext(), stream_protocol::endpoint(socketId)),
        connection(connIn)
    {}

    NbdProxyServer(const NbdProxyServer&) = delete;
    NbdProxyServer(NbdProxyServer&&) = delete;
    NbdProxyServer& operator=(const NbdProxyServer&) = delete;
    NbdProxyServer& operator=(NbdProxyServer&&) = delete;

    ~NbdProxyServer()
    {
        BMCWEB_LOG_DEBUG("NbdProxyServer destructor");

        BMCWEB_LOG_DEBUG("peerSocket->close()");
        boost::system::error_code ec;
        peerSocket.close(ec);

        BMCWEB_LOG_DEBUG("std::filesystem::remove({})", socketId);
        std::error_code ec2;
        std::filesystem::remove(socketId.c_str(), ec2);
        if (ec2)
        {
            BMCWEB_LOG_DEBUG("Failed to remove file, ignoring");
        }

        crow::connections::systemBus->async_method_call(
            dbus::utility::logError, "xyz.openbmc_project.VirtualMedia", path,
            "xyz.openbmc_project.VirtualMedia.Proxy", "Unmount");
    }

    std::string getEndpointId() const
    {
        return endpointId;
    }

    void run()
    {
        acceptor.async_accept(
            [weak(weak_from_this())](const boost::system::error_code& ec,
                                     stream_protocol::socket socket) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("UNIX socket: async_accept error = {}",
                                 ec.message());
                return;
            }

            BMCWEB_LOG_DEBUG("Connection opened");
            std::shared_ptr<NbdProxyServer> self = weak.lock();
            if (self == nullptr)
            {
                return;
            }

            self->connection.resumeRead();
            self->peerSocket = std::move(socket);
            //  Start reading from socket
            self->doRead();
        });

        auto mountHandler = [weak(weak_from_this())](
                                const boost::system::error_code& ec, bool) {
            std::shared_ptr<NbdProxyServer> self = weak.lock();
            if (self == nullptr)
            {
                return;
            }
            if (ec)
            {
                BMCWEB_LOG_ERROR("DBus error: cannot call mount method = {}",
                                 ec.message());

                self->connection.close("Failed to mount media");
                return;
            }
        };

        crow::connections::systemBus->async_method_call(
            std::move(mountHandler), "xyz.openbmc_project.VirtualMedia", path,
            "xyz.openbmc_project.VirtualMedia.Proxy", "Mount");
    }

    void send(std::string_view buffer, std::function<void()>&& onDone)
    {
        size_t copied = boost::asio::buffer_copy(
            ws2uxBuf.prepare(buffer.size()), boost::asio::buffer(buffer));
        ws2uxBuf.commit(copied);

        doWrite(std::move(onDone));
    }

  private:
    void doRead()
    {
        // Trigger async read
        peerSocket.async_read_some(
            ux2wsBuf.prepare(nbdBufferSize),
            [weak(weak_from_this())](const boost::system::error_code& ec,
                                     size_t bytesRead) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("UNIX socket: async_read_some error = {}",
                                 ec.message());
                return;
            }
            std::shared_ptr<NbdProxyServer> self = weak.lock();
            if (self == nullptr)
            {
                return;
            }

            // Send to websocket
            self->ux2wsBuf.commit(bytesRead);
            self->connection.sendEx(
                crow::websocket::MessageType::Binary,
                boost::beast::buffers_to_string(self->ux2wsBuf.data()),
                [weak(self->weak_from_this())]() {
                std::shared_ptr<NbdProxyServer> self2 = weak.lock();
                if (self2 != nullptr)
                {
                    self2->ux2wsBuf.consume(self2->ux2wsBuf.size());
                    self2->doRead();
                }
            });
        });
    }

    void doWrite(std::function<void()>&& onDone)
    {
        if (uxWriteInProgress)
        {
            BMCWEB_LOG_ERROR("Write in progress");
            return;
        }

        if (ws2uxBuf.size() == 0)
        {
            BMCWEB_LOG_ERROR("No data to write to UNIX socket");
            return;
        }

        uxWriteInProgress = true;
        peerSocket.async_write_some(
            ws2uxBuf.data(),
            [weak(weak_from_this()),
             onDone(std::move(onDone))](const boost::system::error_code& ec,
                                        size_t bytesWritten) mutable {
            std::shared_ptr<NbdProxyServer> self = weak.lock();
            if (self == nullptr)
            {
                return;
            }

            self->ws2uxBuf.consume(bytesWritten);
            self->uxWriteInProgress = false;

            if (ec)
            {
                BMCWEB_LOG_ERROR("UNIX: async_write error = {}", ec.message());
                self->connection.close("Internal error");
                return;
            }

            // Retrigger doWrite if there is something in buffer
            if (self->ws2uxBuf.size() > 0)
            {
                self->doWrite(std::move(onDone));
                return;
            }
            onDone();
        });
    }

    // Keeps UNIX socket endpoint file path
    const std::string socketId;
    const std::string endpointId;
    const std::string path;

    bool uxWriteInProgress = false;

    // UNIX => WebSocket buffer
    boost::beast::flat_static_buffer<nbdBufferSize> ux2wsBuf;

    // WebSocket => UNIX buffer
    boost::beast::flat_static_buffer<nbdBufferSize> ws2uxBuf;

    // The socket used to communicate with the client.
    stream_protocol::socket peerSocket;

    // Default acceptor for UNIX socket
    stream_protocol::acceptor acceptor;

    crow::websocket::Connection& connection;
};

using SessionMap = boost::container::flat_map<crow::websocket::Connection*,
                                              std::shared_ptr<NbdProxyServer>>;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static SessionMap sessions;

inline void
    afterGetManagedObjects(crow::websocket::Connection& conn,
                           const boost::system::error_code& ec,
                           const dbus::utility::ManagedObjectType& objects)
{
    const std::string* socketValue = nullptr;
    const std::string* endpointValue = nullptr;
    const std::string* endpointObjectPath = nullptr;

    if (ec)
    {
        BMCWEB_LOG_ERROR("DBus error: {}", ec.message());
        conn.close("Failed to create mount point");
        return;
    }

    for (const auto& [objectPath, interfaces] : objects)
    {
        for (const auto& [interface, properties] : interfaces)
        {
            if (interface != "xyz.openbmc_project.VirtualMedia.MountPoint")
            {
                continue;
            }

            for (const auto& [name, value] : properties)
            {
                if (name == "EndpointId")
                {
                    endpointValue = std::get_if<std::string>(&value);

                    if (endpointValue == nullptr)
                    {
                        BMCWEB_LOG_ERROR("EndpointId property value is null");
                    }
                }
                if (name == "Socket")
                {
                    socketValue = std::get_if<std::string>(&value);
                    if (socketValue == nullptr)
                    {
                        BMCWEB_LOG_ERROR("Socket property value is null");
                    }
                }
            }
        }

        if ((endpointValue != nullptr) && (socketValue != nullptr) &&
            *endpointValue == conn.url().path())
        {
            endpointObjectPath = &objectPath.str;
            break;
        }
    }

    if (objects.empty() || endpointObjectPath == nullptr)
    {
        BMCWEB_LOG_ERROR("Cannot find requested EndpointId");
        conn.close("Failed to match EndpointId");
        return;
    }

    for (const auto& session : sessions)
    {
        if (session.second->getEndpointId() == conn.url().path())
        {
            BMCWEB_LOG_ERROR("Cannot open new connection - socket is in use");
            conn.close("Slot is in use");
            return;
        }
    }

    // If the socket file exists (i.e. after bmcweb crash),
    // we cannot reuse it.
    std::remove((*socketValue).c_str());

    sessions[&conn] = std::make_shared<NbdProxyServer>(
        conn, *socketValue, *endpointValue, *endpointObjectPath);

    sessions[&conn]->run();
};

inline void onOpen(crow::websocket::Connection& conn)
{
    BMCWEB_LOG_DEBUG("nbd-proxy.onopen({})", logPtr(&conn));

    sdbusplus::message::object_path path("/xyz/openbmc_project/VirtualMedia");
    dbus::utility::getManagedObjects(
        "xyz.openbmc_project.VirtualMedia", path,
        [&conn](const boost::system::error_code& ec,
                const dbus::utility::ManagedObjectType& objects) {
        afterGetManagedObjects(conn, ec, objects);
    });

    // We need to wait for dbus and the websockets to hook up before data is
    // sent/received.  Tell the core to hold off messages until the sockets are
    // up
    conn.deferRead();
}

inline void onClose(crow::websocket::Connection& conn,
                    const std::string& reason)
{
    BMCWEB_LOG_DEBUG("nbd-proxy.onclose(reason = '{}')", reason);
    auto session = sessions.find(&conn);
    if (session == sessions.end())
    {
        BMCWEB_LOG_DEBUG("No session to close");
        return;
    }
    // Remove reference to session in global map
    sessions.erase(session);
}

inline void onMessage(crow::websocket::Connection& conn, std::string_view data,
                      crow::websocket::MessageType /*type*/,
                      std::function<void()>&& whenComplete)
{
    BMCWEB_LOG_DEBUG("nbd-proxy.onMessage(len = {})", data.size());

    // Acquire proxy from sessions
    auto session = sessions.find(&conn);
    if (session == sessions.end() || session->second == nullptr)
    {
        whenComplete();
        return;
    }

    session->second->send(data, std::move(whenComplete));
}

inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/nbd/<str>")
        .websocket()
        .onopen(onOpen)
        .onclose(onClose)
        .onmessageex(onMessage);
}
} // namespace nbd_proxy

namespace obmc_vm
{

inline void requestRoutes(App& app)
{
    static_assert(
        !(BMCWEB_VM_WEBSOCKET && BMCWEB_VM_NBDPROXY),
        "nbd proxy cannot be turned on at the same time as vm websocket.");

    if constexpr (BMCWEB_VM_NBDPROXY)
    {
        BMCWEB_ROUTE(app, "/nbd/<str>")
            .privileges({{"ConfigureComponents", "ConfigureManager"}})
            .websocket()
            .onopen(nbd_proxy::onOpen)
            .onclose(nbd_proxy::onClose)
            .onmessageex(nbd_proxy::onMessage);

        BMCWEB_ROUTE(app, "/vm/0/0")
            .privileges({{"ConfigureComponents", "ConfigureManager"}})
            .websocket()
            .onopen(nbd_proxy::onOpen)
            .onclose(nbd_proxy::onClose)
            .onmessageex(nbd_proxy::onMessage);
    }
    if constexpr (BMCWEB_VM_WEBSOCKET)
    {
        BMCWEB_ROUTE(app, "/vm/0/0")
            .privileges({{"ConfigureComponents", "ConfigureManager"}})
            .websocket()
            .onopen([](crow::websocket::Connection& conn) {
            BMCWEB_LOG_DEBUG("Connection {} opened", logPtr(&conn));

            if (session != nullptr)
            {
                conn.close("Session already connected");
                return;
            }

            if (handler != nullptr)
            {
                conn.close("Handler already running");
                return;
            }

            session = &conn;

            // media is the last digit of the endpoint /vm/0/0. A future
            // enhancement can include supporting different endpoint values.
            const char* media = "0";
            handler = std::make_shared<Handler>(media, conn.getIoContext());
            handler->connect();
        })
            .onclose([](crow::websocket::Connection& conn,
                        const std::string& /*reason*/) {
            if (&conn != session)
            {
                return;
            }

            session = nullptr;
            handler->doClose();
            handler->inputBuffer->clear();
            handler->outputBuffer->clear();
            handler.reset();
        })
            .onmessage([](crow::websocket::Connection& conn,
                          const std::string& data, bool) {
            if (data.length() >
                handler->inputBuffer->capacity() - handler->inputBuffer->size())
            {
                BMCWEB_LOG_ERROR("Buffer overrun when writing {} bytes",
                                 data.length());
                conn.close("Buffer overrun");
                return;
            }

            size_t copied = boost::asio::buffer_copy(
                handler->inputBuffer->prepare(data.size()),
                boost::asio::buffer(data));
            handler->inputBuffer->commit(copied);
            handler->doWrite();
        });
    }
}

} // namespace obmc_vm

} // namespace crow
