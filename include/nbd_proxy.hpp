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
#include "app.hpp"
#include "dbus_utility.hpp"
#include "privileges.hpp"

#include <boost/asio/buffer.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/write.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <boost/container/flat_map.hpp>
#include <websocket.hpp>

namespace crow
{

namespace nbd_proxy
{

using boost::asio::local::stream_protocol;

static constexpr auto nbdBufferSize = 131088;
constexpr const char* requiredPrivilegeString = "ConfigureManager";

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
    }

    std::string getEndpointId() const
    {
        return endpointId;
    }

    void run()
    {
        acceptor.async_accept([this, self(shared_from_this())](
                                  const boost::system::error_code& ec,
                                  stream_protocol::socket socket) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "UNIX socket: async_accept error = "
                                 << ec.message();
                return;
            }
            if (peerSocket)
            {
                // Something is wrong - socket shouldn't be acquired at this
                // point
                BMCWEB_LOG_ERROR
                    << "Failed to open connection - socket already used";
                return;
            }

            BMCWEB_LOG_DEBUG << "Connection opened";
            peerSocket = std::move(socket);
            doRead();

            // Trigger Write if any data was sent from server
            // Initially this is negotiation chunk
            doWrite();
        });

        auto mountHandler =
            [this, self(shared_from_this())](
                const boost::system::error_code& ec, const bool) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "DBus error: cannot call mount method = "
                                 << ec.message();
                connection.close("Failed to mount media");
                return;
            }
        };

        crow::connections::systemBus->async_method_call(
            std::move(mountHandler), "xyz.openbmc_project.VirtualMedia", path,
            "xyz.openbmc_project.VirtualMedia.Proxy", "Mount");
    }

    void send(std::string_view data)
    {
        boost::asio::buffer_copy(ws2uxBuf.prepare(data.size()),
                                 boost::asio::buffer(data));
        ws2uxBuf.commit(data.size());
        doWrite();
    }

    void close()
    {
        acceptor.close();
        if (peerSocket)
        {
            BMCWEB_LOG_DEBUG << "peerSocket->close()";
            peerSocket->close();
            peerSocket.reset();
            BMCWEB_LOG_DEBUG << "std::remove(" << socketId << ")";
            std::remove(socketId.c_str());
        }
        // The reference to session should exists until unmount is
        // called
        auto unmountHandler = [](const boost::system::error_code& ec) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "DBus error: " << ec
                                 << ", cannot call unmount method";
                return;
            }
        };

        crow::connections::systemBus->async_method_call(
            std::move(unmountHandler), "xyz.openbmc_project.VirtualMedia", path,
            "xyz.openbmc_project.VirtualMedia.Proxy", "Unmount");
    }

  private:
    void doRead()
    {
        if (!peerSocket)
        {
            BMCWEB_LOG_DEBUG << "UNIX socket isn't created yet";
            // Skip if UNIX socket is not created yet.
            return;
        }

        // Trigger async read
        peerSocket->async_read_some(
            ux2wsBuf.prepare(nbdBufferSize),
            [this, self(shared_from_this())](
                const boost::system::error_code& ec, std::size_t bytesRead) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "UNIX socket: async_read_some error = "
                                 << ec.message();
                // UNIX socket has been closed by peer, best we can do is to
                // break all connections
                close();
                return;
            }

            // Fetch data from UNIX socket

            ux2wsBuf.commit(bytesRead);

            // Paste it to WebSocket as binary
            connection.sendBinary(
                boost::beast::buffers_to_string(ux2wsBuf.data()));
            ux2wsBuf.consume(bytesRead);

            // Allow further reads
            doRead();
            });
    }

    void doWrite()
    {
        if (!peerSocket)
        {
            BMCWEB_LOG_DEBUG << "UNIX socket isn't created yet";
            // Skip if UNIX socket is not created yet. Collect data, and wait
            // for nbd-client connection
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
            [this, self(shared_from_this())](
                const boost::system::error_code& ec, std::size_t bytesWritten) {
            ws2uxBuf.consume(bytesWritten);
            uxWriteInProgress = false;
            if (ec)
            {
                BMCWEB_LOG_ERROR << "UNIX: async_write error = "
                                 << ec.message();
                return;
            }
            // Retrigger doWrite if there is something in buffer
            if (ws2uxBuf.size() > 0)
            {
                doWrite();
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

    // WebSocket <= UNIX buffer
    boost::beast::multi_buffer ws2uxBuf;

    // Default acceptor for UNIX socket
    stream_protocol::acceptor acceptor;

    // The socket used to communicate with the client.
    std::optional<stream_protocol::socket> peerSocket;

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

    // If the socket file exists (i.e. after bmcweb crash),
    // we cannot reuse it.
    std::remove((*socketValue).c_str());

    sessions[&conn] = std::make_shared<NbdProxyServer>(
        conn, *socketValue, *endpointValue, *endpointObjectPath);

    sessions[&conn]->run();
};
inline void onOpen(crow::websocket::Connection& conn)
{
    BMCWEB_LOG_DEBUG << "nbd-proxy.onopen(" << &conn << ")";

    auto openHandler =
        [&conn](const boost::system::error_code& ec,
                const dbus::utility::ManagedObjectType& objects) {
        afterGetManagedObjects(conn, ec, objects);
    };

    crow::connections::systemBus->async_method_call(
        std::move(openHandler), "xyz.openbmc_project.VirtualMedia",
        "/xyz/openbmc_project/VirtualMedia",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

inline void onClose(crow::websocket::Connection& conn,
                    const std::string& reason)
{
    BMCWEB_LOG_DEBUG << "nbd-proxy.onclose(reason = '" << reason << "')";
    auto session = sessions.find(&conn);
    if (session == sessions.end())
    {
        BMCWEB_LOG_DEBUG << "No session to close";
        return;
    }
    session->second->close();
    // Remove reference to session in global map
    sessions.erase(session);
}

inline void onMessage(crow::websocket::Connection& conn,
                      const std::string& data, bool /*isBinary*/)
{
    BMCWEB_LOG_DEBUG << "nbd-proxy.onmessage(len = " << data.length() << ")";
    // Acquire proxy from sessions
    auto session = sessions.find(&conn);
    if (session != sessions.end())
    {
        if (session->second)
        {
            session->second->send(data);
            return;
        }
    }
}

inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/nbd/<str>")
        .websocket()
        .onopen(onOpen)
        .onclose(onClose)
        .onmessage(onMessage);
}
} // namespace nbd_proxy
} // namespace crow
