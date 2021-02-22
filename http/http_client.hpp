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
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/basic_endpoint.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/beast/version.hpp>
#include <include/async_resolve.hpp>

#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <queue>
#include <string>

namespace crow
{

static constexpr uint8_t maxRequestQueueSize = 50;
static constexpr unsigned int httpReadBodyLimit = 8000;

enum class ConnState
{
    initialized,
    connected,
    idle,
    closed,
    suspended,
    terminated,
    abortConnection,
    retry
};

class HttpClient : public std::enable_shared_from_this<HttpClient>
{
  private:
    crow::async_resolve::Resolver resolver;
    boost::asio::ssl::context ctx{boost::asio::ssl::context::tlsv12_client};
    boost::beast::tcp_stream conn;
    std::optional<boost::beast::ssl_stream<boost::beast::tcp_stream&>> sslConn;
    boost::asio::steady_timer timer;
    boost::beast::flat_static_buffer<httpReadBodyLimit> buffer;
    boost::beast::http::request<boost::beast::http::string_body> req;
    std::optional<
        boost::beast::http::response_parser<boost::beast::http::string_body>>
        parser;
    boost::asio::ip::tcp::endpoint endpoint;
    boost::circular_buffer_space_optimized<std::string> requestDataQueue{};
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
    bool connBusy;

    void doResolve()
    {
        connBusy = true;
        BMCWEB_LOG_DEBUG << "Trying to resolve: " << host << ":" << port;

        auto respHandler =
            [self(shared_from_this())](
                const boost::beast::error_code ec,
                const std::vector<boost::asio::ip::tcp::endpoint>&
                    endpointList) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Resolve failed: " << ec.message();
                    self->state = ConnState::retry;
                    self->connBusy = false;
                    self->handleConnState();
                    return;
                }
                BMCWEB_LOG_DEBUG << "Resolved";
                self->connBusy = false;
                self->doConnect(endpointList);
            };
        resolver.asyncResolve(host, port, std::move(respHandler));
    }

    void doConnect(
        const std::vector<boost::asio::ip::tcp::endpoint>& endpointList)
    {
        connBusy = true;

        sslConn.emplace(conn, ctx);
        BMCWEB_LOG_DEBUG << "Trying to connect to: " << host << ":" << port;

        auto respHandler = [self(shared_from_this()), endpointList](
                               const boost::beast::error_code ec,
                               const boost::asio::ip::tcp::endpoint& endpoint) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "Connect " << endpoint
                                 << " failed: " << ec.message();
                self->state = ConnState::retry;
                self->connBusy = false;
                self->handleConnState();
                return;
            }
            BMCWEB_LOG_DEBUG << "Connected to: " << endpoint;
            if (self->sslConn)
            {
                self->performHandshake();
            }
            else
            {
                self->state = ConnState::connected;
                self->connBusy = false;
                self->handleConnState();
            }
        };

        conn.expires_after(std::chrono::seconds(30));
        conn.async_connect(endpointList, std::move(respHandler));
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
                    self->connBusy = false;
                    self->handleConnState();
                    return;
                }
                self->state = ConnState::connected;
                BMCWEB_LOG_DEBUG << "SSL Handshake successfull";

                self->connBusy = false;
                self->handleConnState();
            });
    }

    void sendMessage(const std::string& data)
    {
        connBusy = true;

        BMCWEB_LOG_DEBUG << __FUNCTION__ << "(): " << host << ":" << port;

        req.body() = data;
        req.prepare_payload();

        auto respHandler = [self(shared_from_this())](
                               const boost::beast::error_code ec,
                               const std::size_t& bytesTransferred) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "sendMessage() failed: " << ec.message();
                // Start fresh connection
                self->state = ConnState::retry;
                self->connBusy = false;
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
            if (ec && ec != boost::asio::ssl::error::stream_truncated)
            {
                BMCWEB_LOG_ERROR << "recvMessage() failed: " << ec.message();
                // Start fresh connection
                self->state = ConnState::retry;
                self->connBusy = false;
                self->handleConnState();
                return;
            }
            BMCWEB_LOG_DEBUG << "recvMessage() bytes transferred: "
                             << bytesTransferred;
            boost::ignore_unused(bytesTransferred);

            // Send is successful, Lets remove data from queue
            // check for next request data in queue.
            if (!self->requestDataQueue.empty())
            {
                self->requestDataQueue.pop_front();
            }
            self->state = ConnState::idle;

            // Keep the connection alive if server supports it
            // Else close the connection
            BMCWEB_LOG_DEBUG << "recvMessage() keepalive : "
                             << self->parser->keep_alive();
            if (!self->parser->keep_alive())
            {
                // Abort the connection since server is not keep-alive enabled
                self->state = ConnState::abortConnection;
            }
            // Transfer ownership of the response
            self->parser->release();

            self->connBusy = false;
            self->handleConnState();
        };

        parser.emplace(std::piecewise_construct, std::make_tuple());
        parser->body_limit(httpReadBodyLimit);
        // Since these are all push style eventing, we are not
        // bothered about response body parsing.
        // Check only for the response header
        parser->skip(true);

        conn.expires_after(std::chrono::seconds(30));
        if (sslConn)
        {
            boost::beast::http::async_read(*sslConn, buffer, *parser,
                                           std::move(respHandler));
        }
        else
        {
            boost::beast::http::async_read(conn, buffer, *parser,
                                           std::move(respHandler));
        }
    }

    void doClose()
    {
        connBusy = true;

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
                        BMCWEB_LOG_INFO << "doClose(): Connection "
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

                if ((self->state != ConnState::suspended) &&
                    (self->state != ConnState::terminated))
                {
                    self->state = ConnState::closed;
                    self->connBusy = false;
                    self->handleConnState();
                }
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

            if ((state != ConnState::suspended) &&
                (state != ConnState::terminated))
            {
                state = ConnState::closed;
                connBusy = false;
                handleConnState();
            }
        }
    }

    void checkQueueAndRetry()
    {
        if (retryCount >= maxRetryAttempts)
        {
            BMCWEB_LOG_ERROR << "Maximum number of retries reached.";

            // Clear queue.
            while (!requestDataQueue.empty())
            {
                requestDataQueue.pop_front();
            }

            BMCWEB_LOG_DEBUG << "Retry policy: " << retryPolicyAction;
            if (retryPolicyAction == "TerminateAfterRetries")
            {
                // TODO: delete subscription
                state = ConnState::terminated;
            }
            if (retryPolicyAction == "SuspendRetries")
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
                self->connBusy = false;
                // Lets close connection and start from resolve.
                self->doClose();
            });
        return;
    }

    void handleConnState()
    {
        if (connBusy)
        {
            BMCWEB_LOG_DEBUG << "Async operation is already in progress";
            return;
        }
        switch (state)
        {
            case ConnState::initialized:
            case ConnState::closed:
            {
                if (requestDataQueue.empty())
                {
                    BMCWEB_LOG_DEBUG << "requestDataQueue is empty";
                    return;
                }
                doResolve();
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
            case ConnState::retry:
            {
                // In case of failures during connect and handshake
                // the retry policy will be applied
                checkQueueAndRetry();
                break;
            }
            case ConnState::suspended:
            case ConnState::terminated:
            case ConnState::abortConnection:
            {
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
        conn(ioc),
        timer(ioc), req(boost::beast::http::verb::post, destUri, 11), subId(id),
        host(destIP), port(destPort), uri(destUri), retryCount(0),
        maxRetryAttempts(5), retryIntervalSecs(0),
        retryPolicyAction("TerminateAfterRetries"), runningTimer(false),
        state(ConnState::initialized), connBusy(false)
    {
        // Set the request header
        req.set(boost::beast::http::field::host, host);
        req.set(boost::beast::http::field::content_type, "application/json");
        req.keep_alive(true);

        requestDataQueue.set_capacity(maxRequestQueueSize);

        if (uriProto == "https")
        {
            sslConn.emplace(conn, ctx);
        }
    }

    void sendData(const std::string& data)
    {
        if ((state == ConnState::suspended) || (state == ConnState::terminated))
        {
            return;
        }
        requestDataQueue.push_back(data);
        handleConnState();
        return;
    }

    void addHeaders(
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
