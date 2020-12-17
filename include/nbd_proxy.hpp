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

#include <variant>

namespace crow
{

namespace nbd_proxy
{

using boost::asio::local::stream_protocol;

static constexpr auto nbdBufferSize = 131088;
static const char* requiredPrivilegeString = "ConfigureManager";

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
        acceptor.async_accept(
            [this, self(shared_from_this())](boost::system::error_code ec,
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

        auto mountHandler = [this, self(shared_from_this())](
                                const boost::system::error_code ec,
                                const bool) {
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

    void send(const std::string_view data)
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
        auto unmountHandler = [](const boost::system::error_code ec) {
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
            [this, self(shared_from_this())](boost::system::error_code ec,
                                             std::size_t bytesRead) {
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
            [this, self(shared_from_this())](boost::system::error_code ec,
                                             std::size_t bytesWritten) {
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

static boost::container::flat_map<crow::websocket::Connection*,
                                  std::shared_ptr<NbdProxyServer>>
    sessions;

inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/nbd/<str>")
        .websocket()
        .onopen([](crow::websocket::Connection& conn,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
            BMCWEB_LOG_DEBUG << "nbd-proxy.onopen(" << &conn << ")";

            auto getUserInfoHandler =
                [&conn, asyncResp](
                    const boost::system::error_code ec,
                    boost::container::flat_map<
                        std::string, std::variant<bool, std::string,
                                                  std::vector<std::string>>>
                        userInfo) {
                    if (ec)
                    {
                        BMCWEB_LOG_ERROR << "GetUserInfo failed...";
                        conn.close("Failed to get user information");
                        return;
                    }

                    const std::string* userRolePtr = nullptr;
                    auto userInfoIter = userInfo.find("UserPrivilege");
                    if (userInfoIter != userInfo.end())
                    {
                        userRolePtr =
                            std::get_if<std::string>(&userInfoIter->second);
                    }

                    std::string userRole{};
                    if (userRolePtr != nullptr)
                    {
                        userRole = *userRolePtr;
                        BMCWEB_LOG_DEBUG << "userName = " << conn.getUserName()
                                         << " userRole = " << *userRolePtr;
                    }

                    // Get the user privileges from the role
                    ::redfish::Privileges userPrivileges =
                        ::redfish::getUserPrivileges(userRole);

                    const ::redfish::Privileges requiredPrivileges{
                        requiredPrivilegeString};

                    if (!userPrivileges.isSupersetOf(requiredPrivileges))
                    {
                        BMCWEB_LOG_DEBUG
                            << "User " << conn.getUserName()
                            << " not authorized for nbd connection";
                        conn.close("Unathourized access");
                        return;
                    }

                    auto openHandler = [&conn, asyncResp](
                                           const boost::system::error_code ec,
                                           const dbus::utility::
                                               ManagedObjectType& objects) {
                        const std::string* socketValue = nullptr;
                        const std::string* endpointValue = nullptr;
                        const std::string* endpointObjectPath = nullptr;

                        if (ec)
                        {
                            BMCWEB_LOG_ERROR << "DBus error: " << ec.message();
                            conn.close("Failed to create mount point");
                            return;
                        }

                        for (const auto& objectPath : objects)
                        {
                            const auto interfaceMap = objectPath.second.find(
                                "xyz.openbmc_project.VirtualMedia.MountPoint");

                            if (interfaceMap == objectPath.second.end())
                            {
                                BMCWEB_LOG_DEBUG
                                    << "Cannot find MountPoint object";
                                continue;
                            }

                            const auto endpoint =
                                interfaceMap->second.find("EndpointId");
                            if (endpoint == interfaceMap->second.end())
                            {
                                BMCWEB_LOG_DEBUG
                                    << "Cannot find EndpointId property";
                                continue;
                            }

                            endpointValue =
                                std::get_if<std::string>(&endpoint->second);

                            if (endpointValue == nullptr)
                            {
                                BMCWEB_LOG_ERROR
                                    << "EndpointId property value is null";
                                continue;
                            }

                            if (*endpointValue == conn.req.target())
                            {
                                const auto socket =
                                    interfaceMap->second.find("Socket");
                                if (socket == interfaceMap->second.end())
                                {
                                    BMCWEB_LOG_DEBUG
                                        << "Cannot find Socket property";
                                    continue;
                                }

                                socketValue =
                                    std::get_if<std::string>(&socket->second);
                                if (socketValue == nullptr)
                                {
                                    BMCWEB_LOG_ERROR
                                        << "Socket property value is null";
                                    continue;
                                }

                                endpointObjectPath = &objectPath.first.str;
                                break;
                            }
                        }

                        if (objects.empty() || endpointObjectPath == nullptr)
                        {
                            BMCWEB_LOG_ERROR
                                << "Cannot find requested EndpointId";
                            conn.close("Failed to match EndpointId");
                            return;
                        }

                        for (const auto& session : sessions)
                        {
                            if (session.second->getEndpointId() ==
                                conn.req.target())
                            {
                                BMCWEB_LOG_ERROR
                                    << "Cannot open new connection - socket is "
                                       "in use";
                                conn.close("Slot is in use");
                                return;
                            }
                        }

                        // If the socket file exists (i.e. after bmcweb crash),
                        // we cannot reuse it.
                        std::remove((*socketValue).c_str());

                        sessions[&conn] = std::make_shared<NbdProxyServer>(
                            conn, *socketValue, *endpointValue,
                            *endpointObjectPath);

                        sessions[&conn]->run();
                    };
                    crow::connections::systemBus->async_method_call(
                        std::move(openHandler),
                        "xyz.openbmc_project.VirtualMedia",
                        "/xyz/openbmc_project/VirtualMedia",
                        "org.freedesktop.DBus.ObjectManager",
                        "GetManagedObjects");
                };

            crow::connections::systemBus->async_method_call(
                std::move(getUserInfoHandler),
                "xyz.openbmc_project.User.Manager", "/xyz/openbmc_project/user",
                "xyz.openbmc_project.User.Manager", "GetUserInfo",
                conn.getUserName());
        })
        .onclose(
            [](crow::websocket::Connection& conn, const std::string& reason) {
                BMCWEB_LOG_DEBUG << "nbd-proxy.onclose(reason = '" << reason
                                 << "')";
                auto session = sessions.find(&conn);
                if (session == sessions.end())
                {
                    BMCWEB_LOG_DEBUG << "No session to close";
                    return;
                }
                // Remove reference to session in global map
                sessions.erase(session);
                session->second->close();
            })
        .onmessage([](crow::websocket::Connection& conn,
                      const std::string& data, bool) {
            BMCWEB_LOG_DEBUG << "nbd-proxy.onmessage(len = " << data.length()
                             << ")";
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
        });
}
} // namespace nbd_proxy
} // namespace crow
