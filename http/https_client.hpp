/*
// Copyright (c) 2020 Intel Corporation
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
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <queue>
#include <string>

//#define BMCWEB_LOG_DEBUG std::cerr
//#define BMCWEB_LOG_ERROR std::cerr

namespace crow
{

enum class ConnState
{
    initializing,
    connected,
    closed
};

inline boost::asio::ssl::context getSSLCtx(const std::string& certFile)
{
    // The SSL context is required, and holds certificates
    boost::asio::ssl::context ctx{boost::asio::ssl::context::tlsv12_client};

    ctx.set_default_verify_paths();
    ctx.load_verify_file(certFile);

    // Verify the remote server's certificate
    ctx.set_verify_mode(boost::asio::ssl::verify_peer);

    return ctx;
}

class HttpsClient : public std::enable_shared_from_this<HttpsClient>
{
  private:
    boost::beast::ssl_stream<boost::beast::tcp_stream> conn;
    boost::beast::http::request<boost::beast::http::string_body> req;
    boost::beast::http::response<boost::beast::http::string_body> res;
    boost::asio::ip::tcp::resolver::results_type endpoint;
    std::vector<std::pair<std::string, std::string>> headers;
    std::queue<std::string> requestDataQueue;
    ConnState state;
    std::string host;
    std::string port;
    std::string uri;
    int retry;

    void prepareRequestAndSend(const std::string& data)
    {
        BMCWEB_LOG_DEBUG << __FUNCTION__ <<  "(): " << host << ":" << port;

        req.version(static_cast<int>(11)); // HTTP 1.1
        req.target(uri);
        req.method(boost::beast::http::verb::post);

        // Set headers
        for (const auto& [key, value] : headers)
        {
            req.set(key, value);
        }
        req.set(boost::beast::http::field::host, host);
        req.keep_alive(true);

        req.body() = data;
        req.prepare_payload();

        sendMessage();
    }

    void checkQueue()
    {
        if (requestDataQueue.empty())
        {
            // TODO: Having issue in keeping connection alive. So lets close if
            // nothing to be trasferred.
            doClose();

            BMCWEB_LOG_DEBUG << "requestDataQueue is empty\n";
            return;
        }

        if (state != ConnState::connected)
        {
            // After establishing the conenction, checkQueue() will
            // get called and it will attempt to send data.
            doConnect();
            return;
        }

        std::string data = requestDataQueue.front();
        prepareRequestAndSend(data);

        return;
    }

    void doConnect()
    {
        // Set a timeout on the operation
        boost::beast::get_lowest_layer(conn).expires_after(
            std::chrono::seconds(30));

        boost::beast::get_lowest_layer(conn).async_connect(
            endpoint,
            [self(shared_from_this())](const boost::beast::error_code& ec,
                                       const boost::asio::ip::tcp::resolver::
                                           results_type::endpoint_type& ep) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Connect " << ep
                                     << " failed: " << ec.message();
                    return;
                }
                BMCWEB_LOG_DEBUG << "Connected to: " << ep;

                self->performHandshake();
            });
    }

    void performHandshake()
    {
        conn.async_handshake(
            boost::asio::ssl::stream_base::client,
            [self(shared_from_this())](const boost::beast::error_code& ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "SSL handshake failed: "
                                     << ec.message();
                    return;
                }
                self->state = ConnState::connected;
                BMCWEB_LOG_DEBUG << "SSL Handshake successfull\n";

                self->checkQueue();
                // self->sendMessage();
            });
    }

    void sendMessage()
    {
        if (state != ConnState::connected)
        {
            BMCWEB_LOG_DEBUG << "Not connected to: " << host;
            return;
        }

        // Set a timeout on the operation
        boost::beast::get_lowest_layer(conn).expires_after(
            std::chrono::seconds(30));

        // Send the HTTP request to the remote host
        boost::beast::http::async_write(
            conn, req,
            [self(shared_from_this())](const boost::beast::error_code& ec,
                                       const std::size_t& bytesTransferred) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "sendMessage() failed: "
                                     << ec.message();
                    self->doClose();
                    return;
                }
                BMCWEB_LOG_DEBUG << "sendMessage() bytes trasferred: "
                                 << bytesTransferred;
                boost::ignore_unused(bytesTransferred);

                self->recvMessage();
            });
    }

    void recvMessage()
    {
        if (state != ConnState::connected)
        {
            BMCWEB_LOG_DEBUG << "Not connected to: " << host;
            return;
        }

        // Set a timeout on the operation
        boost::beast::get_lowest_layer(conn).expires_after(
            std::chrono::seconds(30));

        // Receive the HTTP response
        auto buffer = std::make_unique<std::vector<char>>(1024);
        conn.async_read_some(
            boost::asio::buffer(buffer->data(), buffer->size()),
            [self(shared_from_this()),
             buffer{std::move(buffer)}](const boost::beast::error_code& ec,
                                        const std::size_t& bytesTransferred) {
                if (ec)
                {
                    // Many https server closes connection abruptly
                    // i.e witnout close_notify. More details are at
                    // https://github.com/boostorg/beast/issues/824
                    if (ec != boost::asio::ssl::error::stream_truncated)
                    {
                        BMCWEB_LOG_ERROR << "recvMessage() failed: "
                                         << ec.message();
                        self->doClose();
                    }
                    return;
                }
                BMCWEB_LOG_DEBUG << "recvMessage() bytes trasferred: "
                                 << bytesTransferred;
                boost::ignore_unused(bytesTransferred);

                // Discard received data. We are not interested.
                BMCWEB_LOG_DEBUG << "recvMessage() data: " << self->res;

                // self->doClose();
                // Send is successful. Reset the retryCount;
                self->retry = 0;

                // Send is successful, Lets remove data from queue
                // check for next request data in queue.
                self->requestDataQueue.pop();
                self->checkQueue();
            });
    }

    void doClose()
    {
        // Set a timeout on the operation
        boost::beast::get_lowest_layer(conn).expires_after(
            std::chrono::seconds(30));

        conn.async_shutdown([self(shared_from_this())](
                                const boost::beast::error_code& ec) {
            if (ec && ec != boost::asio::error::eof)
            {
                // Many https server closes connection abruptly
                // i.e witnout close_notify. More details are at
                // https://github.com/boostorg/beast/issues/824
                if (ec == boost::asio::ssl::error::stream_truncated)
                {
                    BMCWEB_LOG_DEBUG
                        << "doClose(): Connection closed by server.";
                }
                {
                    BMCWEB_LOG_ERROR << "doClose() failed: " << ec.message();
                }
                return;
            }
            boost::beast::get_lowest_layer(self->conn).cancel();
            BMCWEB_LOG_DEBUG << "Connection closed gracefully";
        });

        state = ConnState::closed;
    }

  public:
    explicit HttpsClient(boost::asio::io_context& ioc,
                         boost::asio::ssl::context& ctx,
                         const std::string& destIP, const std::string& destPort,
                         const std::string& destUri) :
        conn(ioc, ctx),
        host(destIP), port(destPort), uri(destUri)
    {
        boost::asio::ip::tcp::resolver resolver(ioc);
        endpoint = resolver.resolve(host, port);
        state = ConnState::initializing;

        // Set SNI Hostname (many hosts need this to handshake successfully)
        char* hostName = host.data();
        if (!SSL_set_tlsext_host_name(conn.native_handle(),
                                      reinterpret_cast<const void*>(hostName)))
        {
            // We can ignore the error and handle it in handshake failure.
            boost::beast::error_code ec{static_cast<int>(::ERR_get_error()),
                                        boost::asio::error::get_ssl_category()};
            BMCWEB_LOG_DEBUG
                << "SSL_set_tlsext_host_name failed: " << ec.message() << "\n";
            return;
        }
    }

    void sendData(const std::string& data)
    {
        requestDataQueue.push(data);
        checkQueue();

        return;
    }

    void setHeaders(
        const std::vector<std::pair<std::string, std::string>>& httpHeaders)
    {
        headers = httpHeaders;
    }

    ConnState getState()
    {
        return state;
    }
};

} // namespace crow
