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
#include <sdbusplus/asio/property.hpp>

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
                  .in = pipeIn, .out = pipeOut, .err = nullptr})
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
        doRead();
    }

    void doWrite(std::string_view data)
    {
        pipeIn.async_write_some(
            boost::asio::buffer(data),
            [self(shared_from_this())](const boost::beast::error_code& ec,
                                       size_t bytesWritten) {
            BMCWEB_LOG_DEBUG("Wrote {} bytes", bytesWritten);

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
            session->resumeRead();
        });
    }

    void doRead()
    {
        pipeOut.async_read_some(
            boost::asio::buffer(outputBuffer),
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
            std::string_view payload(outputBuffer.data(), bytesRead);
            session->sendEx(crow::websocket::MessageType::Binary, payload,
                            [self2{shared_from_this()}]() { self2->doRead(); });
        });
    }

    boost::asio::readable_pipe pipeOut;
    boost::asio::writable_pipe pipeIn;
    boost::process::v2::process proxy;
    bool doingWrite{false};

    std::array<char, 4096> outputBuffer{};
    std::array<char, nbdBufferSize> inputBuffer{};
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

    static void afterMount(const std::weak_ptr<NbdProxyServer>& weak,
                           const boost::system::error_code& ec,
                           bool /*isBinary*/)
    {
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
    }

    static void afterAccept(const std::weak_ptr<NbdProxyServer>& weak,
                            const boost::system::error_code& ec,
                            stream_protocol::socket socket)
    {
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
    }

    void run()
    {
        acceptor.async_accept(
            std::bind_front(&NbdProxyServer::afterAccept, weak_from_this()));

        crow::connections::systemBus->async_method_call(
            [weak{weak_from_this()}](const boost::system::error_code& ec,
                                     bool isBinary) {
            afterMount(weak, ec, isBinary);
        },
            "xyz.openbmc_project.VirtualMedia", path,
            "xyz.openbmc_project.VirtualMedia.Proxy", "Mount");
    }

    void doWrite(std::string_view data, std::function<void()>&& onDone)
    {
        boost::asio::async_write(
            peerSocket, boost::asio::buffer(data),
            [weak(weak_from_this()),
             onDone(std::move(onDone))](const boost::system::error_code& ec,
                                        size_t /*bytesWritten*/) mutable {
            std::shared_ptr<NbdProxyServer> self = weak.lock();
            if (self == nullptr)
            {
                return;
            }

            if (ec)
            {
                BMCWEB_LOG_ERROR("UNIX: async_write error = {}", ec.message());
                self->connection.close("Internal error");
                return;
            }
            self->connection.resumeRead();
            onDone();
        });
    }

  private:
    static void afterSendEx(const std::weak_ptr<NbdProxyServer>& weak)
    {
        std::shared_ptr<NbdProxyServer> self = weak.lock();
        if (self != nullptr)
        {
            self->doRead();
        }
    }

    inline void afterRead(const std::weak_ptr<NbdProxyServer>& weak,
                          const boost::system::error_code& ec, size_t bytesRead)
    {
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
        self->connection.sendEx(
            crow::websocket::MessageType::Binary,
            std::string_view(self->ux2wsBuf.data(), bytesRead),
            std::bind_front(&NbdProxyServer::afterSendEx, weak_from_this()));
    }

    void doRead()
    {
        // Trigger async read
        peerSocket.async_read_some(boost::asio::buffer(ux2wsBuf),
                                   std::bind_front(&NbdProxyServer::afterRead,
                                                   this, weak_from_this()));
    }

    static void afterWrite(const std::weak_ptr<NbdProxyServer>& weak,
                           std::function<void()>&& onDone,
                           const boost::system::error_code& ec,
                           size_t bytesWritten)
    {
        BMCWEB_LOG_DEBUG("Wrote {} bytes", bytesWritten);
        std::shared_ptr<NbdProxyServer> self = weak.lock();
        if (self == nullptr)
        {
            return;
        }

        if (ec)
        {
            BMCWEB_LOG_ERROR("UNIX: async_write error = {}", ec.message());
            self->connection.close("Internal error");
            return;
        }

        self->connection.resumeRead();
        onDone();
    }

    // Keeps UNIX socket endpoint file path
    const std::string socketId;
    const std::string endpointId;
    const std::string path;

    // UNIX => WebSocket buffer
    std::array<char, nbdBufferSize> ux2wsBuf{};

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
    afterGetSocket(crow::websocket::Connection& conn,
                   const sdbusplus::message::object_path& path,
                   const boost::system::error_code& ec,
                   const dbus::utility::DBusPropertiesMap& propertiesList)
{
    if (ec)
    {
        conn.close("Internal Error");
        return;
    }
    std::string endpointId;
    std::string socket;

    bool success = sdbusplus::unpackPropertiesNoThrow(
        redfish::dbus_utils::UnpackErrorPrinter(), propertiesList, "EndpointId",
        endpointId, "Socket", socket);

    if (!success)
    {
        conn.close("Internal Error");
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
    std::remove(socket.c_str());

    sessions[&conn] = std::make_shared<NbdProxyServer>(conn, socket, endpointId,
                                                       path);
    sessions[&conn]->run();
}

inline void onOpen(crow::websocket::Connection& conn)
{
    BMCWEB_LOG_DEBUG("nbd-proxy.onopen({})", logPtr(&conn));

    sdbusplus::message::object_path path(
        "/xyz/openbmc_project/VirtualMedia/nbd");

    path /= std::to_string(0);

    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, "xyz.openbmc_project.VirtualMedia", path,
        "xyz.openbmc_project.VirtualMedia",
        [&conn, path](const boost::system::error_code& ec,
                      const dbus::utility::DBusPropertiesMap& propertiesList) {
        afterGetSocket(conn, path, ec, propertiesList);
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
                      crow::websocket::MessageType /*type*/, bool /*isDone*/,
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

    conn.deferRead();

    session->second->doWrite(data, std::move(whenComplete));
}
} // namespace nbd_proxy

namespace obmc_vm
{

inline void requestRoutes(App& app)
{
    static_assert(
        !(bmcwebVmWebsocket && bmcwebNbdProxy),
        "nbd proxy cannot be turned on at the same time as vm websocket.");

    if constexpr (bmcwebNbdProxy)
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
    if constexpr (bmcwebVmWebsocket)
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
            handler.reset();
        })
            .onmessage([](crow::websocket::Connection& conn,
                          std::string_view data, bool) {
            conn.deferRead();
            handler->doWrite(data);
        });
    }
}

} // namespace obmc_vm

} // namespace crow
