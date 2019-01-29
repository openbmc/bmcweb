/*
// Copyright (c) 2018 Intel Corporation
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
#include <boost/asio.hpp>
#include <boost/container/flat_map.hpp>
#include <experimental/filesystem>

namespace crow
{

namespace nbd_proxy
{

using boost::asio::local::stream_protocol;

struct NbdProxyServer : std::enable_shared_from_this<NbdProxyServer>
{
    NbdProxyServer(crow::websocket::Connection& conn,
                   std::pair<std::string, bool>& socketId,
                   std::function<void()> close_handler)
        : socketId_(socketId), conn_(&conn),
          closeHandler(std::move(close_handler)),
          acceptor_(conn.getIoService(),
                    stream_protocol::endpoint(socketId_.first))
    {
        // Accquire socket
        socketId_.second = false;
        do_accept();
    }

    ~NbdProxyServer()
    {
        BMCWEB_LOG_DEBUG << "nbd-proxy: destructor()";
        close();
    }

    void send(const std::string& data)
    {
        ws2uxBuf.push_back(data);
        do_write();
    }

    void close()
    {
        BMCWEB_LOG_DEBUG << "nbd-proxy: close()";
        if (socket_)
        {
            BMCWEB_LOG_DEBUG << "nbd-proxy: socket_->close()";
            socket_->close();
            socket_.reset();
        }
        conn_ = nullptr; // so nobody would use it anymore
        closeHandler();
        std::experimental::filesystem::remove(socketId_.first);
        // return socket to the pool
        socketId_.second = true;
    }

  private:
    void do_accept()
    {
        acceptor_.async_accept([this](boost::system::error_code ec,
                                      stream_protocol::socket socket) {
            if (ec)
            {
                BMCWEB_LOG_ERROR
                    << "Error nbd-client did not accept new connection: " << ec;
                return;
            }
            if (socket_)
            {
                // Something is wrong the socket already shall be acquired
                return;
            }

            // nbd-client connected successfully
            BMCWEB_LOG_DEBUG << "Connection nbd-client opened";
            socket_ =
                std::make_unique<stream_protocol::socket>(std::move(socket));
            // Trigger Read on device
            do_read();

            // Trigger Write if any data was sent from server
            // Initially this is negotiation chunk
            do_write();

            // Since one connection is already opened we're not allowing others
            // to connect
        });
    }

    void do_read()
    {
        // Trigger async read
        socket_->async_read_some(boost::asio::buffer(ux2wsBuf), [
            this, self(shared_from_this())
        ](boost::system::error_code ec, std::size_t length) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "UX: async_read_some_cb error = " << ec;
                // UX socket has been closed by peer, best we can do is to break
                // all connections
                close();
                return;
            }

            // Fetch data from UX socket
            auto d = std::string(std::move(ux2wsBuf.data()), length);

            // Paste it to WS as binary
            if (conn_)
            {
                conn_->sendBinary(d);
            }

            // Allow further reads
            do_read();
        });
    }

    void do_write()
    {
        if (doingWrite)
        {
            // Skip if write is in progress, it will be called again by the
            // handler
            return;
        }
        if (!socket_)
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
            *socket_, boost::asio::buffer(std::move(ws2uxBuf.front())),
            [ this, self(shared_from_this()) ](boost::system::error_code ec,
                                               std::size_t /* length */) {
                doingWrite = false;
                ws2uxBuf.erase(ws2uxBuf.begin());
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "UX: async_write_cb error = " << ec;
                    return;
                }
                // Retrigger do_write if there is something in buffer
                do_write();
            });
    }

    // Keeps UX socket endpoint file path
    std::pair<std::string, bool>& socketId_;

    // UX => WS buffer
    std::array<char, 0x20000> ux2wsBuf;

    // WS <= UX buffer
    std::vector<std::string> ws2uxBuf;

    // lock for write in progress on UX socket
    bool doingWrite = false;

    // Default acceptor for UX Stream Socket
    stream_protocol::acceptor acceptor_;

    // The socket used to communicate with the client.
    std::unique_ptr<stream_protocol::socket> socket_;

    // Reference to WS connection
    crow::websocket::Connection* conn_;

    std::function<void()> closeHandler;
};

static boost::container::flat_map<crow::websocket::Connection*,
                                  std::shared_ptr<NbdProxyServer>>
    sessions;

static boost::container::flat_map<std::string, bool> available_sockets = {
    {"/tmp/nbd-proxy.0.sock", true}, {"/tmp/nbd-proxy.1.sock", true},
    {"/tmp/nbd-proxy.2.sock", true}, {"/tmp/nbd-proxy.3.sock", true},
    {"/tmp/nbd-proxy.4.sock", true},
};

template <typename... Middlewares> void requestRoutes(Crow<Middlewares...>& app)
{

    BMCWEB_ROUTE(app, "/nbd")
        .websocket()
        .onopen([&](crow::websocket::Connection& conn) {
            BMCWEB_LOG_DEBUG << "nbd-proxy.onopen(" << &conn << ")";

            auto freeSocket =
                std::find_if(available_sockets.begin(), available_sockets.end(),
                             [](const std::pair<std::string, bool>& item) {
                                 return item.second;
                             });

            if (freeSocket == available_sockets.end())
            {
                // No free socket found, disconnect
                BMCWEB_LOG_DEBUG << "nbd-proxy: no available UX sockets";
                conn.close();
                return;
            }

            sessions[&conn] = std::make_shared<NbdProxyServer>(
                conn, *freeSocket, [&conn]() { conn.close(); });
            BMCWEB_LOG_DEBUG << "nbd-proxy: oppened UX socket: "
                             << freeSocket->first;
            // This is the place where nbd-client shall be started. This may be
            // done in various ways. Perhaps systemd may be configured to watch
            // for /tmp/nbd-proxy.*.sock file appearance, and based on that
            // start nbd-client accordingly. Or there might be separate service
            // for VirtualMedia which will expose interfaces for mounting
            // resources in different ways. In this case starting nbd-client and
            // connect to given Unix socket file.
        })
        .onclose(
            [&](crow::websocket::Connection& conn, const std::string& reason) {
                BMCWEB_LOG_DEBUG << "nbd-proxy.onclose(reason = '" << reason
                                 << "')";
                // Release the connection
                auto session = sessions.find(&conn);
                if (session != sessions.end())
                {
                    session->second->close();
                }
                sessions.erase(&conn);
                // This is the place where nbd-client shall be interrupted.
                // nbd-client makes a lot of retries (about 30seconds) before it
                // will stop working and clean the nbd device properly, this may
                // be unintentional overhead for the system.
            })
        .onmessage([&](crow::websocket::Connection& conn,
                       const std::string& data, bool is_binary) {
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
