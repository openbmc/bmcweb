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

namespace crow
{

static constexpr uint8_t maxRequestQueueSize = 50;
static constexpr unsigned int httpReadBodyLimit = 1024;

enum class ConnState
{
    initialized,
    resolved,
    connected,
    idle,
    suspended,
    terminated,
    abortConnection,
    closed,
    retry
};

class HttpClient : public std::enable_shared_from_this<HttpClient>
{
  private:
    boost::asio::ip::tcp::resolver resolver;
    boost::asio::ssl::context ctx{boost::asio::ssl::context::tlsv12_client};
    boost::beast::tcp_stream conn;
    std::optional<boost::beast::ssl_stream<boost::beast::tcp_stream&>> sslConn;
    boost::asio::steady_timer timer;
    boost::beast::flat_static_buffer<httpReadBodyLimit> buffer;
    boost::beast::http::request<boost::beast::http::string_body> req;
    boost::beast::http::response<boost::beast::http::string_body> res;
    boost::asio::ip::tcp::resolver::results_type endpoint;
    std::queue<std::string> requestDataQueue;
    std::string subId;
    std::string host;
    std::string port;
    std::string uri;
    uint32_t retryCount;
    uint32_t maxRetryAttempts;
    uint32_t retryIntervalSecs;
    std::string retryPolicyAction;
    bool runningTimer;
    ConnState state;

    void doResolve()
    {
        BMCWEB_LOG_DEBUG << "Trying to resolve: " << host << ":" << port;

        // TODO: Use async_resolve once the boost crash is resolved
        endpoint = resolver.resolve(host, port);
#if 0
        auto respHandler =
            [self(shared_from_this())](const boost::beast::error_code ec,
                                       const boost::asio::ip::tcp::resolver::results_type ep) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Resolve failed: " << ec.message();
                    self->state = ConnState::retry;
                    self->handleConnState();
                    return;
                }
                self->endpoint = ep;
                self->state = ConnState::resolved;
                self->handleConnState();
            };

        resolver.async_resolve(host.c_str(), port.c_str(), std::move(respHandler));
#endif
        state = ConnState::resolved;
        handleConnState();
    }

    void doConnect()
    {
        BMCWEB_LOG_DEBUG << "Trying to connect to: " << host << ":" << port;

        auto respHandler =
            [self(shared_from_this())](const boost::beast::error_code ec,
                                       const boost::asio::ip::tcp::resolver::
                                           results_type::endpoint_type& ep) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Connect " << ep
                                     << " failed: " << ec.message();
                    self->state = ConnState::retry;
                    self->handleConnState();
                    return;
                }
                BMCWEB_LOG_DEBUG << "Connected to: " << ep;
                if (self->sslConn)
                {
                    self->performHandshake();
                }
                else
                {
                    self->state = ConnState::connected;
                    self->handleConnState();
                }
            };

        conn.expires_after(std::chrono::seconds(30));
        conn.async_connect(endpoint, std::move(respHandler));
    }

    void performHandshake()
    {
        sslConn->async_handshake(
            boost::asio::ssl::stream_base::client,
            [self(shared_from_this())](const boost::beast::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "SSL handshake failed: "
                                     << ec.message();
                    self->state = ConnState::retry;
                    self->handleConnState();
                    return;
                }
                self->state = ConnState::connected;
                BMCWEB_LOG_DEBUG << "SSL Handshake successfull";

                self->handleConnState();
            });
    }

    void sendMessage(const std::string& data)
    {
        BMCWEB_LOG_DEBUG << "sendMessage() to: " << host << ":" << port;

        req.body() = data;
        req.prepare_payload();

        auto respHandler = [self(shared_from_this())](
                               const boost::beast::error_code ec,
                               const std::size_t& bytesTransferred) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "sendMessage() failed: " << ec.message();
                self->state = ConnState::retry;
                self->handleConnState();
                return;
            }
            BMCWEB_LOG_DEBUG << "sendMessage() bytes transferred: "
                             << bytesTransferred;
            boost::ignore_unused(bytesTransferred);

            self->recvMessage();
        };

        conn.expires_after(std::chrono::seconds(30));
        if (sslConn)
        {
            boost::beast::http::async_write(*sslConn, req,
                                            std::move(respHandler));
        }
        else
        {
            boost::beast::http::async_write(conn, req, std::move(respHandler));
        }
    }

    void recvMessage()
    {
        auto respHandler = [self(shared_from_this())](
                               const boost::beast::error_code ec,
                               const std::size_t& bytesTransferred) {
            if (ec && ec != boost::beast::http::error::partial_message)
            {
                BMCWEB_LOG_ERROR << "recvMessage() failed: " << ec.message();
                self->state = ConnState::retry;
                // The receive failed. Close the connection and
                // retry sending as per the retry policy
                self->handleConnState();
                return;
            }
            BMCWEB_LOG_DEBUG << "recvMessage() bytes transferred: "
                             << bytesTransferred;
            boost::ignore_unused(bytesTransferred);

            BMCWEB_LOG_DEBUG << "recvMessage() data: " << self->res;

            // Send is successful, Lets remove data from queue
            // check for next request data in queue.
            // Even if the error partial_message is received, the subscriber
            // would have received the events. So bmcweb need not attempt send.
            self->requestDataQueue.pop();
            self->state = ConnState::idle;

            // Set the retry count to 0
            self->retryCount = 0;

            // Keep the connection alive if server supports it
            // Else close the connection
            BMCWEB_LOG_DEBUG << "recvMessage() keepalive : "
                             << self->res.keep_alive();
            if (!self->res.keep_alive())
            {
                // Abort the connection since server is not keep-alive enabled
                self->state = ConnState::abortConnection;
                self->handleConnState();
                return;
            }
        };

        conn.expires_after(std::chrono::seconds(30));
        if (sslConn)
        {
            boost::beast::http::async_read(*sslConn, buffer, res,
                                           std::move(respHandler));
        }
        else
        {
            boost::beast::http::async_read(conn, buffer, res,
                                           std::move(respHandler));
        }
    }

    void doClose()
    {
        // Set the timeout on the tcp stream socket for the async operation
        conn.expires_after(std::chrono::seconds(30));
        if (sslConn)
        {
            sslConn->async_shutdown([self = shared_from_this()](
                                        const boost::system::error_code ec) {
                if (ec)
                {
                    // Many https server closes connection abruptly
                    // i.e witnout close_notify. More details are at
                    // https://github.com/boostorg/beast/issues/824
                    if (ec == boost::asio::ssl::error::stream_truncated)
                    {
                        BMCWEB_LOG_ERROR << "doClose(): Connection "
                                            "closed by server. ";
                    }
                    else
                    {
                        BMCWEB_LOG_ERROR << "doClose() failed: "
                                         << ec.message();
                    }
                }
                else
                {
                    BMCWEB_LOG_DEBUG << "Connection closed gracefully...";
                }
                self->conn.close();
            });
        }
        else
        {
            boost::beast::error_code ec;
            conn.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both,
                                   ec);
            if (ec)
            {
                BMCWEB_LOG_ERROR << "doClose() failed: " << ec.message();
            }
            else
            {
                BMCWEB_LOG_DEBUG << "Connection closed gracefully...";
            }
            conn.close();
        }
        state = ConnState::closed;
    }

    void checkQueueAndRetry()
    {
        if (retryCount >= maxRetryAttempts)
        {
            BMCWEB_LOG_ERROR << "Maximum number of retries reached.";

            // Clear queue.
            while (!requestDataQueue.empty())
            {
                requestDataQueue.pop();
            }

            BMCWEB_LOG_DEBUG << "Retry policy: " << retryPolicyAction;
            if (retryPolicyAction == "TerminateAfterRetries")
            {
                // TODO: delete subscription
                state = ConnState::terminated;
            }
            else if (retryPolicyAction == "SuspendRetries")
            {
                state = ConnState::suspended;
            }
            // Reset the retrycount to zero so that client can try connecting
            // again if needed
            retryCount = 0;
            handleConnState();
            return;
        }

        if (runningTimer)
        {
            BMCWEB_LOG_DEBUG << "Retry timer is already running.";
            return;
        }
        runningTimer = true;

        // Close the existing connection to re-establish the new connection
        // fot this retry attempt
        doClose();

        retryCount++;

        BMCWEB_LOG_DEBUG << "Attempt retry after " << retryIntervalSecs
                         << " seconds. RetryCount = " << retryCount;
        timer.expires_after(std::chrono::seconds(retryIntervalSecs));
        timer.async_wait(
            [self = shared_from_this()](const boost::system::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "async_wait failed: " << ec.message();
                    // Ignore the error and continue the retry loop to attempt
                    // sending the event as per the retry policy
                }
                self->runningTimer = false;
                // Set the state to resolved to start reconnecting
                self->state = ConnState::resolved;
                self->handleConnState();
            });
        return;
    }

    void handleConnState()
    {
        switch (state)
        {
            case ConnState::initialized:
            case ConnState::closed:
            {
                // Initial state of connection
                // If the state here is 'closed' it means that the keep alive
                // was not set at the server and the connection was closed
                doResolve();
                break;
            }
            case ConnState::suspended:
            case ConnState::terminated:
                // close the connection
                doClose();
                break;
            case ConnState::retry:
            {
                // In case of failures during connect, handshake, send and
                // receive the retry policy will be applied
                checkQueueAndRetry();
                break;
            }
            case ConnState::resolved:
            {
                // Resolve is successful. Start connecting
                doConnect();
                break;
            }
            case ConnState::connected:
            case ConnState::idle:
            {
                // State idle means, previous attempt is successful
                // State connected means, client connection is established
                // successfully
                if (requestDataQueue.empty())
                {
                    BMCWEB_LOG_DEBUG << "requestDataQueue is empty";
                    return;
                }
                std::string data = requestDataQueue.front();
                sendMessage(data);
                break;
            }
            case ConnState::abortConnection:
            {
                // Server did not want to keep alive the session
                doClose();
                break;
            }
            default:
                break;
        }
    }

  public:
    explicit HttpClient(boost::asio::io_context& ioc, const std::string& id,
                        const std::string& destIP, const std::string& destPort,
                        const std::string& destUri,
                        const std::string& uriProto) :
        resolver(ioc),
        conn(ioc), timer(ioc), subId(id), host(destIP), port(destPort),
        uri(destUri), retryCount(0), maxRetryAttempts(5),
        retryPolicyAction("TerminateAfterRetries"), runningTimer(false),
        state(ConnState::initialized)
    {
        // Set the request header
        req = {};
        req.set(boost::beast::http::field::host, host);
        req.set(boost::beast::http::field::content_type, "application/json");
        req.version(11); // HTTP 1.1
        req.target(uri);
        req.method(boost::beast::http::verb::post);
        req.keep_alive(true);

        if (uriProto == "https")
        {
            sslConn.emplace(conn, ctx);
        }
    }

    void sendData(const std::string& data)
    {
        if (state == ConnState::suspended)
        {
            return;
        }

        if (requestDataQueue.size() <= maxRequestQueueSize)
        {
            requestDataQueue.push(data);
            handleConnState();
        }
        else
        {
            BMCWEB_LOG_ERROR << "Request queue is full. So ignoring data.";
        }

        return;
    }

    void setHeaders(
        const std::vector<std::pair<std::string, std::string>>& httpHeaders)
    {
        // Set custom headers
        for (const auto& [key, value] : httpHeaders)
        {
            req.set(key, value);
        }
    }

    void setRetryConfig(const uint32_t retryAttempts,
                        const uint32_t retryTimeoutInterval)
    {
        maxRetryAttempts = retryAttempts;
        retryIntervalSecs = retryTimeoutInterval;
    }

    void setRetryPolicy(const std::string& retryPolicy)
    {
        retryPolicyAction = retryPolicy;
    }
};

} // namespace crow
