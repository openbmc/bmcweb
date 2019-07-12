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
#include <crow/app.h>
#include <crow/websocket.h>

#include <async_resp.hpp>
#include <boost/asio.hpp>
#include <boost/container/flat_map.hpp>
#include <experimental/filesystem>
#include <variant>

namespace crow
{

namespace nbd_proxy
{

using boost::asio::local::stream_protocol;

struct NbdProxyServer : std::enable_shared_from_this<NbdProxyServer>
{
    NbdProxyServer(crow::websocket::Connection& conn,
                   const std::string& socketIdIn, const std::string& filenameIn,
                   std::function<void()>&& closeHandler) :
        socketId(socketIdIn),
        filename(filenameIn), connection(&conn),
        closeHandler(std::move(closeHandler)),
        acceptor(conn.get_io_context(), stream_protocol::endpoint(socketId))
    {
        // Acquire socket
        doAccept();
    }

    ~NbdProxyServer()
    {
        BMCWEB_LOG_DEBUG << "NbdProxyServer destructor";
        close();

        if (peerSocket)
        {
            BMCWEB_LOG_DEBUG << "peerSocket->close()";
            peerSocket->close();
            peerSocket.reset();
            BMCWEB_LOG_DEBUG << "std::remove(" << socketId.c_str() << ")";
            std::remove(socketId.c_str());
        }
    }

    void send(const std::string& data)
    {
        ws2uxBuf.push_back(data);
        doWrite();
    }

    void close()
    {
        if (connection != nullptr)
        {
            closeHandler();
            connection = nullptr; // so nobody would use it anymore
        }
    }

    std::string get_filename() const
    {
        return filename;
    }

  private:
    void doAccept()
    {
        acceptor.async_accept([this](boost::system::error_code ec,
                                     stream_protocol::socket socket) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "Cannot accept new connection: " << ec;
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
            peerSocket =
                std::make_unique<stream_protocol::socket>(std::move(socket));
            doRead();

            // Trigger Write if any data was sent from server
            // Initially this is negotiation chunk
            doWrite();
        });
    }

    void doRead()
    {
        if (!peerSocket)
        {
            // Skip if UX socket is not created yet.
            return;
        }

        // Trigger async read
        peerSocket->async_read_some(
            boost::asio::buffer(ux2wsBuf),
            [this, self(shared_from_this())](boost::system::error_code ec,
                                             std::size_t length) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "UX: async_read_some error = " << ec;
                    // UX socket has been closed by peer, best we can do is to
                    // break all connections
                    close();
                    return;
                }

                // Fetch data from UX socket
                auto d = std::string(std::move(ux2wsBuf.data()), length);

                // Paste it to WS as binary
                if (connection)
                {
                    connection->sendBinary(d);
                }

                // Allow further reads
                doRead();
            });
    }

    void doWrite()
    {
        if (doingWrite)
        {
            // Skip if write is in progress, it will be called again by the
            // handler
            return;
        }
        if (!peerSocket)
        {
            // Skip if UX socket is not created yet. Collect data, and wait for
            // nbd-client connection
            return;
        }
        if (ws2uxBuf.empty())
        {
            // Skip if there are no data to write to UX socket
            return;
        }

        doingWrite = true;
        boost::asio::async_write(
            *peerSocket, boost::asio::buffer(std::move(ws2uxBuf.front())),
            [this, self(shared_from_this())](boost::system::error_code ec,
                                             std::size_t /* length */) {
                doingWrite = false;
                ws2uxBuf.erase(ws2uxBuf.begin());
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "UX: async_write error = " << ec;
                    return;
                }
                // Retrigger doWrite if there is something in buffer
                doWrite();
            });
    }

    // Keeps UX socket endpoint file path
    const std::string socketId;

    // UX => WS buffer
    std::array<char, 0x20000> ux2wsBuf;

    // WS <= UX buffer
    std::vector<std::string> ws2uxBuf;

    // lock for write in progress on UX socket
    bool doingWrite = false;

    // Default acceptor for UX Stream Socket
    stream_protocol::acceptor acceptor;

    // The socket used to communicate with the client.
    std::unique_ptr<stream_protocol::socket> peerSocket;

    // Reference to WS connection
    crow::websocket::Connection* connection;

    std::function<void()> closeHandler;

    const std::string filename;
};

static boost::container::flat_map<std::string, sdbusplus::message::object_path>
    usedSockets;
static boost::container::flat_map<crow::websocket::Connection*,
                                  std::shared_ptr<NbdProxyServer>>
    sessions;

using GetManagedPropertyType = boost::container::flat_map<
    std::string,
    sdbusplus::message::variant<std::string, bool, uint8_t, int16_t, uint16_t,
                                int32_t, uint32_t, int64_t, uint64_t, double>>;

using GetManagedObjectsType = boost::container::flat_map<
    sdbusplus::message::object_path,
    boost::container::flat_map<std::string, GetManagedPropertyType>>;

template <typename... Middlewares> void requestRoutes(Crow<Middlewares...>& app)
{
    BMCWEB_ROUTE(app, "/nbd/<str>")
        .websocket()
        .onopen([&app](crow::websocket::Connection& conn) {
            BMCWEB_LOG_DEBUG << "nbd-proxy.onopen(" << &conn << ")";

            const auto usedSocket =
                usedSockets.find(std::string(conn.req.target()));

            if (usedSocket != usedSockets.end())
            {
                BMCWEB_LOG_ERROR
                    << "Cannot open new connection - socket is in use";
                return;
            }

            auto openHandler = [&app, &conn](const boost::system::error_code ec,
                                             GetManagedObjectsType& objects) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "DBus error: " << ec;
                    return;
                }

                for (const auto& objectPath : objects)
                {
                    const auto interfaceMap = objectPath.second.find(
                        "xyz.openbmc_project.VirtualMedia.MountPoint");
                    if (interfaceMap == objectPath.second.end())
                    {
                        BMCWEB_LOG_DEBUG << "Cannot find MountPoint object";
                        continue;
                    }

                    const auto endpoint =
                        interfaceMap->second.find("EndpointId");
                    if (endpoint == interfaceMap->second.end())
                    {
                        BMCWEB_LOG_DEBUG << "Cannot find EndpointId property";
                        continue;
                    }

                    const std::string* endpointValue =
                        std::get_if<std::string>(&endpoint->second);
                    if (endpointValue == nullptr)
                    {
                        BMCWEB_LOG_ERROR << "EndpointId property value is null";
                        continue;
                    }

                    if (*endpointValue != conn.req.target())
                    {
                        continue;
                    }

                    const auto socket = interfaceMap->second.find("Socket");
                    if (socket == interfaceMap->second.end())
                    {
                        BMCWEB_LOG_DEBUG << "Cannot find Socket property";
                        continue;
                    }

                    const std::string* socketValue =
                        std::get_if<std::string>(&socket->second);
                    if (socketValue == nullptr)
                    {
                        BMCWEB_LOG_ERROR << "Socket property value is null";
                        return;
                    }

                    const std::string socketName = *socketValue;

                    // If the socket file exists (i.e. after bmcweb crash), we
                    // cannot reuse it.
                    std::remove(socketName.c_str());

                    const auto filename =
                        std::string(conn.req.getHeaderValue("filename"));

                    sessions[&conn] = std::make_shared<NbdProxyServer>(
                        conn, std::move(socketName), std::move(filename),
                        [&conn]() { conn.close(); });

                    usedSockets.insert(
                        std::make_pair(*endpointValue, objectPath.first));

                    auto mountHandler = [](const boost::system::error_code ec,
                                           const bool status) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR << "DBus error: " << ec
                                             << ", cannot call mount method";
                            return;
                        }
                    };

                    crow::connections::systemBus->async_method_call(
                        std::move(mountHandler),
                        "xyz.openbmc_project.VirtualMedia", objectPath.first,
                        "xyz.openbmc_project.VirtualMedia.Proxy", "Mount");

                    return;
                }
            };
            crow::connections::systemBus->async_method_call(
                std::move(openHandler), "xyz.openbmc_project.VirtualMedia",
                "/xyz/openbmc_project/VirtualMedia",
                "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");

            auto& io = crow::connections::systemBus->get_io_context();
            io.run_for(boost::asio::chrono::seconds(1));
        })
        .onclose([](crow::websocket::Connection& conn,
                    const std::string& reason) {
            BMCWEB_LOG_DEBUG << "nbd-proxy.onclose(reason = '" << reason
                             << "')";
            auto session = sessions.find(&conn);
            if (session == sessions.end())
            {
                BMCWEB_LOG_DEBUG << "No session to close";
                return;
            }

            const auto usedSocket =
                usedSockets.find(std::string(conn.req.target()));
            if (usedSocket != usedSockets.end())
            {
                // The reference to session should exists until unmount is
                // called
                auto unmountHandler =
                    [&conn, &session](const boost::system::error_code ec) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR << "DBus error: " << ec
                                             << ", cannot call unmount method";
                            return;
                        }
                    };

                crow::connections::systemBus->async_method_call(
                    std::move(unmountHandler),
                    "xyz.openbmc_project.VirtualMedia", usedSocket->second,
                    "xyz.openbmc_project.VirtualMedia.Proxy", "Unmount");

                usedSockets.erase(usedSocket);
            }
            // Remove reference to session in global map
            session->second->close();
            sessions.erase(&conn);
        })
        .onmessage([](crow::websocket::Connection& conn,
                      const std::string& data, bool isBinary) {
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

            conn.close();
        });
}
} // namespace nbd_proxy
} // namespace crow
