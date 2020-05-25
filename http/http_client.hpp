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

using resolvedIpStruct = std::tuple<int32_t, int32_t, std::vector<uint8_t>>;
using resolvedIp = std::vector<resolvedIpStruct>;

enum class ConnState
{
    initialized,
    resolveInProgress,
    resolved,
    connectInProgress,
    connected,
    sendInProgress,
    idle,
    suspended,
    terminated,
    abortConnection,
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
    boost::asio::ip::tcp::endpoint endpoint;
    boost::asio::ip::tcp::resolver::results_type resEndpoint;
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

    void doInit()
    {
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
                        BMCWEB_LOG_INFO
                            << "doInit: Close the existing connection";
                    }
                    else
                    {
                        BMCWEB_LOG_ERROR << "doInit: failed: " << ec.message();
                    }
                }
                else
                {
                    BMCWEB_LOG_DEBUG << "doInit: Connection closed gracefully...";
                }
                self->sslConn.emplace(self->conn, self->ctx);
                self->doResolve();
            });
        }
        else
        {
            boost::beast::error_code ec;
            conn.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both,
                                   ec);
            if (ec)
            {
                BMCWEB_LOG_ERROR << "doInit failed: " << ec.message();
            }
            else
            {
                BMCWEB_LOG_DEBUG << "doInit: Connection closed gracefully...";
            }
            doResolve();
        }
    }

    void doResolve()
    {
        if (state == ConnState::resolveInProgress)
        {
            return;
        }
        state = ConnState::resolveInProgress;

        BMCWEB_LOG_DEBUG << "Trying to resolve: " << host << ":" << port;
        uint64_t flag = 0;
        crow::connections::systemBus->async_method_call(
            [self(shared_from_this()),
             flag](const boost::system::error_code ec, const resolvedIp& resp,
                   const std::string& hostName, const uint64_t flagNum) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Resolve failed: " << ec.message();
                    self->state = ConnState::retry;
                    self->handleConnState();
                    return;
                }
                BMCWEB_LOG_DEBUG << "ResolveHostname returned: " << hostName
                                 << ":" << flagNum;
                std::string address;
                for (auto i : resp)
                {
                    // Extract the IP address from the response
                    std::vector<uint8_t> ipaddress = std::get<2>(i);
                    uint32_t index = 0;
                    for (auto byte : ipaddress)
                    {
                        address += std::to_string(byte);
                        index++;
                        if (index != 4)
                        {
                            address += ".";
                        }
                    }
                    BMCWEB_LOG_DEBUG << "IP Address: " << address;
                }
                self->endpoint.address(boost::asio::ip::make_address(address));
                self->endpoint.port(uint16_t(std::stoi(self->port.c_str())));
                BMCWEB_LOG_DEBUG << "Resolved endpoint is : " << self->endpoint;
                // TODO: Get the resEndpoint(type-
                // boost::asio::ip::tcp::resolver::results_type) from the
                // endpoint(type- boost::asio::ip::tcp::endpoint).
                // -OR- any other alternatives to this.
                // Till then adding resolver.resolve call to get the resolve
                // working
                self->resEndpoint = self->resolver.resolve(address, self->port);
                self->state = ConnState::resolved;
                self->handleConnState();
            },
            "org.freedesktop.resolve1", "/org/freedesktop/resolve1",
            "org.freedesktop.resolve1.Manager", "ResolveHostname", 0, host, 0,
            flag);
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
        conn.async_connect(resEndpoint, std::move(respHandler));
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
        if (state == ConnState::sendInProgress)
        {
            return;
        }
        state = ConnState::sendInProgress;

        BMCWEB_LOG_DEBUG << "sendMessage() to: " << host << ":" << port;

        req.body() = data;
        req.prepare_payload();

        auto respHandler = [self(shared_from_this())](
                               const boost::beast::error_code ec,
                               const std::size_t& bytesTransferred) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "sendMessage() failed: " << ec.message();
                self->state = ConnState::initialized;
                self->handleConnState();
                return;
            }
            BMCWEB_LOG_DEBUG << "sendMessage() bytes transferred: "
                             << bytesTransferred;
            boost::ignore_unused(bytesTransferred);

            // Send is successful, Lets remove data from queue
            // check for next request data in queue.
            if (!self->requestDataQueue.empty())
            {
                self->requestDataQueue.pop_front();
            }

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
            if (ec)
            {
                BMCWEB_LOG_ERROR << "recvMessage() failed: " << ec.message();
                self->state = ConnState::initialized;
            }
            else
            {
                BMCWEB_LOG_DEBUG << "recvMessage() bytes transferred: "
                                 << bytesTransferred;
                boost::ignore_unused(bytesTransferred);

                BMCWEB_LOG_DEBUG << "recvMessage() data: " << self->res;
                self->state = ConnState::idle;
            }
            // Keep the connection alive if server supports it
            // Else close the connection
            BMCWEB_LOG_DEBUG << "recvMessage() keepalive : "
                             << self->res.keep_alive();
            if (!self->res.keep_alive())
            {
                // Abort the connection since server is not keep-alive enabled
                self->state = ConnState::abortConnection;
            }
            // Handle the failure or success state
            self->handleConnState();
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
                // Set the state to resolved to start reconnecting
                self->state = ConnState::initialized;
                self->handleConnState();
            });
        return;
    }

    void handleConnState()
    {
        switch (state)
        {
            case ConnState::initialized:
            {
                doInit();
                break;
            }
            case ConnState::suspended:
            case ConnState::terminated:
            {
                doClose();
                break;
            }
            case ConnState::retry:
            {
                // In case of failures during connect and handshake
                // the retry policy will be applied
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
            case ConnState::resolveInProgress:
            case ConnState::connectInProgress:
            case ConnState::sendInProgress:
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
        uri(destUri), retryCount(0), maxRetryAttempts(5), retryIntervalSecs(0),
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

        requestDataQueue.set_capacity(maxRequestQueueSize);

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
        requestDataQueue.push_back(data);
        handleConnState();
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
