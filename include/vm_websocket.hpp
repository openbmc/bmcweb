#pragma once

#include "app.hpp"
#include "dbus_utility.hpp"
#include "privileges.hpp"
#include "websocket.hpp"

#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/write.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/core/flat_static_buffer.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/process/async_pipe.hpp>
#include <boost/process/child.hpp>
#include <boost/process/io.hpp>
#include <sdbusplus/asio/property.hpp>

#include <csignal>
#include <string_view>

namespace crow
{
namespace obmc_vm
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

        BMCWEB_LOG_DEBUG("std::remove({})", socketId);
        std::remove(socketId.c_str());

        crow::connections::systemBus->async_method_call(
            dbus::utility::logError, "xyz.openbmc_project.VirtualMedia", path,
            "xyz.openbmc_project.VirtualMedia.Proxy", "Unmount");
    }

    std::string getEndpointId() const
    {
        return endpointId;
    }

    void afterMount(const std::weak_ptr<NbdProxyServer>& weak,
                    const boost::system::error_code& ec, bool)
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

    void afterAccept(const std::weak_ptr<NbdProxyServer>& weak,
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
        acceptor.async_accept(std::bind_front(&NbdProxyServer::afterAccept,
                                              this, weak_from_this()));

        crow::connections::systemBus->async_method_call(
            [this, weak{weak_from_this()}](const boost::system::error_code& ec,
                                           bool isBinary) {
            afterMount(weak, ec, isBinary);
        },
            "xyz.openbmc_project.VirtualMedia", path,
            "xyz.openbmc_project.VirtualMedia.Proxy", "Mount");
    }

    void send(std::string_view buffer, std::function<void()>&& onDone)
    {
        boost::asio::buffer_copy(ws2uxBuf.prepare(buffer.size()),
                                 boost::asio::buffer(buffer));
        ws2uxBuf.commit(buffer.size());

        doWrite(std::move(onDone));
    }

  private:
    void afterSendEx(const std::weak_ptr<NbdProxyServer>& weak)
    {
        std::shared_ptr<NbdProxyServer> self2 = weak.lock();
        if (self2 != nullptr)
        {
            self2->ux2wsBuf.consume(self2->ux2wsBuf.size());
            self2->doRead();
        }
    }

    void afterRead(const std::weak_ptr<NbdProxyServer>& weak,
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
        self->ux2wsBuf.commit(bytesRead);
        self->connection.sendEx(
            crow::websocket::MessageType::Binary,
            boost::beast::buffers_to_string(self->ux2wsBuf.data()),
            std::bind_front(&NbdProxyServer::afterSendEx, this,
                            weak_from_this()));
    }

    void doRead()
    {
        // Trigger async read
        peerSocket.async_read_some(ux2wsBuf.prepare(nbdBufferSize),
                                   std::bind_front(&NbdProxyServer::afterRead,
                                                   this, weak_from_this()));
    }

    void afterWrite(const std::weak_ptr<NbdProxyServer>& weak,
                    std::function<void()>&& onDone,
                    const boost::system::error_code& ec, size_t bytesWritten)
    {
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
        peerSocket.async_write_some(ws2uxBuf.data(),
                                    std::bind_front(&NbdProxyServer::afterWrite,
                                                    this, weak_from_this(),
                                                    std::move(onDone)));
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

inline void afterGetSocket(crow::websocket::Connection& conn,
                    const sdbusplus::message::object_path& path,
                    const boost::system::error_code& ec,
                    const dbus::utility::DBusPropertiesMap& propertiesList)
{
    if (ec)
    {
        conn.close("Internal Error");
        return;
    }
    std::string EndpointId;
    std::string socket;

    bool success = sdbusplus::unpackPropertiesNoThrow(
        redfish::dbus_utils::UnpackErrorPrinter(), propertiesList, "EndpointId",
        EndpointId, "Socket", socket);

    if (!success)
    {
        conn.close("Internal Error");
        return;
    }

    // If the socket file exists (i.e. after bmcweb crash),
    // we cannot reuse it.
    std::remove(socket.c_str());

    sessions[&conn] = std::make_shared<NbdProxyServer>(conn, socket, EndpointId,
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
    auto sessionIt = sessions.find(&conn);
    if (sessionIt == sessions.end())
    {
        BMCWEB_LOG_DEBUG("No session to close");
        return;
    }
    // Remove reference to session in global map
    sessions.erase(sessionIt);
}

inline void onMessage(crow::websocket::Connection& conn, std::string_view data,
                      crow::websocket::MessageType /*type*/,
                      std::function<void()>&& whenComplete)
{
    BMCWEB_LOG_DEBUG("nbd-proxy.onMessage(len = {})", data.size());

    // Acquire proxy from sessions
    auto sessionIt = sessions.find(&conn);
    if (sessionIt == sessions.end() || sessionIt->second == nullptr)
    {
        whenComplete();
        return;
    }

    sessionIt->second->send(data, std::move(whenComplete));
}

inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/nbd/0")
        .privileges({{"ConfigureComponents", "ConfigureManager"}})
        .websocket()
        .onopen(onOpen)
        .onclose(onClose)
        .onmessageex(onMessage);


    // Note, this route is kept for backward compatibility with the old virtual media implementation.
    BMCWEB_ROUTE(app, "/vm/0/0")
        .privileges({{"ConfigureComponents", "ConfigureManager"}})
        .websocket()
        .onopen(onOpen)
        .onclose(onClose)
        .onmessageex(onMessage);
}

} // namespace obmc_vm
} // namespace crow
