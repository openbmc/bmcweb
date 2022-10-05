/*
// Copyright (c) 2019 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#pragma once
#include <app.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/write.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <boost/container/flat_map.hpp>
#include <dbus_utility.hpp>
#include <privileges.hpp>
#include <websocket.hpp>

#include <string_view>

namespace crow
{

namespace nbd_proxy
{

using boost::asio::local::stream_protocol;

static constexpr auto nbdBufferSize = 131088;

void logDbusError(const boost::system::error_code ec)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR << "DBus error: " << ec
                         << ", cannot call unmount method";
    }
}

struct NbdProxyServer : std::enable_shared_from_this<NbdProxyServer>
{
    NbdProxyServer(crow::websocket::Connection& connIn,
                   const std::string& socketIdIn,
                   const std::string& endpointIdIn, const std::string& pathIn) :
        socketId(socketIdIn),
        endpointId(endpointIdIn), path(pathIn),
        acceptor(connIn.getIoContext(), stream_protocol::endpoint(socketId)),
        connection(connIn)
    {}

    NbdProxyServer(const NbdProxyServer&) = delete;
    NbdProxyServer(NbdProxyServer&&) = delete;
    NbdProxyServer& operator=(const NbdProxyServer&) = delete;
    NbdProxyServer& operator=(NbdProxyServer&&) = delete;

    ~NbdProxyServer()
    {
        BMCWEB_LOG_DEBUG << "NbdProxyServer destructor";

        if (peerSocket)
        {
            BMCWEB_LOG_DEBUG << "peerSocket->close()";
        }

        crow::connections::systemBus->async_method_call(
            logDbusError, "xyz.openbmc_project.VirtualMedia", path,
            "xyz.openbmc_project.VirtualMedia.Proxy", "Unmount");
    }

    std::string getEndpointId() const
    {
        return endpointId;
    }

    void run()
    {
        acceptor.async_accept(
            [weak(weak_from_this())](boost::system::error_code ec,
                                     stream_protocol::socket socket) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "UNIX socket: async_accept error = "
                                 << ec.message();
                return;
            }

            BMCWEB_LOG_DEBUG << "Connection opened";
            std::shared_ptr<NbdProxyServer> self = weak.lock();
            if (self == nullptr)
            {
                return;
            }

            if (self->peerSocket)
            {
                // Something is wrong - socket shouldn't be acquired at this
                // point
                BMCWEB_LOG_ERROR
                    << "Failed to open connection - socket already used";
                return;
            }

            self->peerSocket = std::move(socket);
            self->doRead();

            // Trigger Write if any data was sent from server
            // Initially this is negotiation chunk
            self->doWrite();
        });

        auto mountHandler =
            [weak(weak_from_this())](const boost::system::error_code ec,
                                     const bool) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "DBus error: cannot call mount method = "
                                 << ec.message();
                std::shared_ptr<NbdProxyServer> self = weak.lock();
                if (self == nullptr)
                {
                    return;
                }

                self->connection.close("Failed to mount media");
            }
        };

        crow::connections::systemBus->async_method_call(
            std::move(mountHandler), "xyz.openbmc_project.VirtualMedia", path,
            "xyz.openbmc_project.VirtualMedia.Proxy", "Mount");
    }

    void send(std::string_view buffer, std::function<void(bool)>&& onDone)
    {
        boost::asio::buffer_copy(ws2uxBuf.prepare(buffer.size()),
                                 boost::asio::buffer(buffer));
        ws2uxBuf.commit(buffer.size());
        uxSendDone = std::move(onDone);

        doWrite();
    }

  private:
    void doRead()
    {
        if (!peerSocket)
        {
            BMCWEB_LOG_DEBUG << "UNIX socket isn't created yet";
            return;
        }

        // Trigger async read
        peerSocket->async_read_some(
            ux2wsBuf.prepare(nbdBufferSize),
            [weak(weak_from_this())](boost::system::error_code ec,
                                     std::size_t bytesRead) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "UNIX socket: async_read_some error = "
                                 << ec.message();
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
                    self2->doRead();
                }
                });
            });
    }

    void doWrite()
    {
        if (!peerSocket)
        {
            BMCWEB_LOG_DEBUG << "UNIX socket isn't created yet";
            return;
        }

        if (uxWriteInProgress)
        {
            BMCWEB_LOG_ERROR << "Write in progress";
            return;
        }

        if (ws2uxBuf.size() == 0)
        {
            BMCWEB_LOG_ERROR << "No data to write to UNIX socket";
            return;
        }

        uxWriteInProgress = true;
        boost::asio::async_write(
            *peerSocket, ws2uxBuf.data(),
            [weak(weak_from_this())](boost::system::error_code ec,
                                     std::size_t bytesWritten) {
            std::shared_ptr<NbdProxyServer> self = weak.lock();
            if (self == nullptr)
            {
                return;
            }

            self->ws2uxBuf.consume(bytesWritten);
            self->uxWriteInProgress = false;

            if (ec)
            {
                BMCWEB_LOG_ERROR << "UNIX: async_write error = "
                                 << ec.message();
                self->connection.close("Internal error");
            }

            // Retrigger doWrite if there is something in buffer
            if (self->ws2uxBuf.size() > 0)
            {
                self->doWrite();
            }
            else if (self->uxSendDone)
            {
                self->uxSendDone(true);
            }
            });
    }

    // Keeps UNIX socket endpoint file path
    const std::string socketId;
    const std::string endpointId;
    const std::string path;

    bool uxWriteInProgress = false;

    // UNIX => WebSocket buffer
    boost::beast::multi_buffer ux2wsBuf;

    // WebSocket => UNIX buffer
    boost::beast::multi_buffer ws2uxBuf;

    // The socket used to communicate with the client.
    std::optional<stream_protocol::socket> peerSocket;

    // Default acceptor for UNIX socket
    stream_protocol::acceptor acceptor;

    crow::websocket::Connection& connection;

    std::function<void(bool)> uxSendDone;
};

static boost::container::flat_map<crow::websocket::Connection*,
                                  std::shared_ptr<NbdProxyServer>>
    sessions;

void afterGetManagedObjects(crow::websocket::Connection& conn,
                            const boost::system::error_code ec,
                            const dbus::utility::ManagedObjectType& objects)
{
    const std::string* socketValue = nullptr;
    const std::string* endpointValue = nullptr;
    const std::string* endpointObjectPath = nullptr;

    if (ec)
    {
        BMCWEB_LOG_ERROR << "DBus error: " << ec.message();
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
                        BMCWEB_LOG_ERROR << "EndpointId property value is null";
                    }
                }
                if (name == "Socket")
                {
                    socketValue = std::get_if<std::string>(&value);
                    if (socketValue == nullptr)
                    {
                        BMCWEB_LOG_ERROR << "Socket property value is null";
                    }
                }
            }
        }

        if ((endpointValue != nullptr) && (socketValue != nullptr) &&
            *endpointValue == conn.req.target())
        {
            endpointObjectPath = &objectPath.str;
            break;
        }
    }

    if (objects.empty() || endpointObjectPath == nullptr)
    {
        BMCWEB_LOG_ERROR << "Cannot find requested EndpointId";
        conn.close("Failed to match EndpointId");
        return;
    }

    for (const auto& session : sessions)
    {
        if (session.second->getEndpointId() == conn.req.target())
        {
            BMCWEB_LOG_ERROR << "Cannot open new connection - socket is in use";
            conn.close("Slot is in use");
            return;
        }
    }

    std::shared_ptr<NbdProxyServer>& session = sessions[&conn];
    session = std::make_shared<NbdProxyServer>(
        conn, *socketValue, *endpointValue, *endpointObjectPath);

    session->run();
}

void onOpen(crow::websocket::Connection& conn)
{
    BMCWEB_LOG_DEBUG << "nbd-proxy.onopen(" << &conn << ")";

    auto openHandler =
        [&conn](const boost::system::error_code ec,
                const dbus::utility::ManagedObjectType& objects) {
        afterGetManagedObjects(conn, ec, objects);
    };
    crow::connections::systemBus->async_method_call(
        std::move(openHandler), "xyz.openbmc_project.VirtualMedia",
        "/xyz/openbmc_project/VirtualMedia",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

void onClose(crow::websocket::Connection& conn, const std::string& reason)
{
    BMCWEB_LOG_DEBUG << "nbd-proxy.onclose(reason = '" << reason << "')";
    auto session = sessions.find(&conn);
    if (session == sessions.end())
    {
        BMCWEB_LOG_DEBUG << "No session to close";
        return;
    }
    // Remove reference to session in global map
    sessions.erase(session);
}

void onMessage(crow::websocket::Connection& conn, std::string_view data,
               crow::websocket::MessageType type,
               std::function<void(bool)>&& whenComplete)
{
    BMCWEB_LOG_DEBUG << "nbd-proxy.onMessageEx(len = " << data.size() << ")";
    if (type != crow::websocket::MessageType::Binary)
    {
        conn.close("Bad message");
        whenComplete(true);
        return;
    }
    // Acquire proxy from sessions
    auto session = sessions.find(&conn);
    if (session == sessions.end() || session->second == nullptr)
    {
        whenComplete(false);
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
} // namespace crow
