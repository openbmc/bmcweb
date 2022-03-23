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
#include <boost/beast/version.hpp>
#include <boost/container/devector.hpp>
#include <include/async_resolve.hpp>

#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <queue>
#include <string>

namespace crow
{

static constexpr uint8_t maxPoolSize = 4;
static constexpr uint8_t maxRequestQueueSize = 50;
static constexpr unsigned int httpReadBodyLimit = 8192;

enum class ConnState
{
    initialized,
    resolveInProgress,
    resolveFailed,
    connectInProgress,
    connectFailed,
    connected,
    sendInProgress,
    sendFailed,
    recvInProgress,
    recvFailed,
    idle,
    closeInProgress,
    closed,
    suspended,
    terminated,
    abortConnection,
    retry
};

// We need to allow retry information to be set before a message has been sent
// and a connection pool has been created
struct RetryPolicyData
{
    uint32_t maxRetryAttempts = 5;
    uint32_t retryIntervalSecs = 0;
    std::string retryPolicyAction = "TerminateAfterRetries";
};
std::unordered_map<std::string, RetryPolicyData> unconnectedRetryInfo;

class ConnectionInfo : public std::enable_shared_from_this<ConnectionInfo>
{
  private:
    boost::beast::tcp_stream conn;
    boost::asio::steady_timer timer;
    boost::beast::http::request<boost::beast::http::string_body> req;
    std::optional<
        boost::beast::http::response_parser<boost::beast::http::string_body>>
        parser;
    crow::async_resolve::Resolver resolver;
    boost::beast::flat_static_buffer<httpReadBodyLimit> buffer;

    ConnState state = ConnState::initialized;
    uint32_t retryCount = 0;
    bool runningTimer = false;
    std::string subId;
    std::string host;
    std::string port;
    uint32_t connId;
    std::string data;
    std::function<void(bool, uint32_t)> callback;

    // Retry policy for the larger connection pool
    const uint32_t& maxRetryAttempts;
    const uint32_t& retryIntervalSecs;
    const std::string& retryPolicyAction;

    friend class ConnectionPool;

    void doResolve()
    {
        state = ConnState::resolveInProgress;
        BMCWEB_LOG_DEBUG << "Trying to resolve: " << host << ":" << port
                         << ", id: " << std::to_string(connId);

        auto respHandler =
            [self(shared_from_this())](
                const boost::beast::error_code ec,
                const std::vector<boost::asio::ip::tcp::endpoint>&
                    endpointList) {
                if (ec || (endpointList.empty()))
                {
                    BMCWEB_LOG_ERROR << "Resolve failed: " << ec.message();
                    self->state = ConnState::resolveFailed;
                    self->waitAndRetry();
                    return;
                }
                BMCWEB_LOG_DEBUG << "Resolved " << self->host << ":"
                                 << self->port
                                 << ", id: " << std::to_string(self->connId);
                self->doConnect(endpointList);
            };

        resolver.asyncResolve(host, port, std::move(respHandler));
    }

    void doConnect(
        const std::vector<boost::asio::ip::tcp::endpoint>& endpointList)
    {
        state = ConnState::connectInProgress;

        BMCWEB_LOG_DEBUG << "Trying to connect to: " << host << ":" << port
                         << ", id: " << std::to_string(connId);

        conn.expires_after(std::chrono::seconds(30));
        conn.async_connect(
            endpointList, [self(shared_from_this())](
                              const boost::beast::error_code ec,
                              const boost::asio::ip::tcp::endpoint& endpoint) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Connect "
                                     << endpoint.address().to_string() << ":"
                                     << std::to_string(endpoint.port())
                                     << ", id: " << std::to_string(self->connId)
                                     << " failed: " << ec.message();
                    self->state = ConnState::connectFailed;
                    self->waitAndRetry();
                    return;
                }
                BMCWEB_LOG_DEBUG
                    << "Connected to: " << endpoint.address().to_string() << ":"
                    << std::to_string(endpoint.port())
                    << ", id: " << std::to_string(self->connId);
                self->state = ConnState::connected;
                self->sendMessage();
            });
    }

    void sendMessage()
    {
        state = ConnState::sendInProgress;

        req.body() = data;
        req.prepare_payload();

        // Set a timeout on the operation
        conn.expires_after(std::chrono::seconds(30));

        // Send the HTTP request to the remote host
        boost::beast::http::async_write(
            conn, req,
            [self(shared_from_this())](const boost::beast::error_code& ec,
                                       const std::size_t& bytesTransferred) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "sendMessage() failed: "
                                     << ec.message();
                    self->state = ConnState::sendFailed;
                    self->waitAndRetry();
                    return;
                }
                BMCWEB_LOG_DEBUG << "sendMessage() bytes transferred: "
                                 << bytesTransferred;
                boost::ignore_unused(bytesTransferred);

                self->recvMessage();
            });
    }

    void recvMessage()
    {
        parser.emplace(std::piecewise_construct, std::make_tuple());
        parser->body_limit(httpReadBodyLimit);

        // Receive the HTTP response
        boost::beast::http::async_read(
            conn, buffer, *parser,
            [self(shared_from_this())](const boost::beast::error_code& ec,
                                       const std::size_t& bytesTransferred) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "recvMessage() failed: "
                                     << ec.message();
                    self->state = ConnState::recvFailed;
                    self->waitAndRetry();
                    return;
                }
                BMCWEB_LOG_DEBUG << "recvMessage() bytes transferred: "
                                 << bytesTransferred;
                BMCWEB_LOG_DEBUG << "recvMessage() data: "
                                 << self->parser->get().body();

                unsigned int respCode = self->parser->get().result_int();
                BMCWEB_LOG_DEBUG << "recvMessage() Header Response Code: "
                                 << respCode;

                // 2XX response is considered to be successful
                if ((respCode < 200) || (respCode >= 300))
                {
                    // The listener failed to receive the Sent-Event
                    BMCWEB_LOG_ERROR
                        << "recvMessage() Listener Failed to "
                           "receive Sent-Event. Header Response Code: "
                        << respCode;
                    self->state = ConnState::recvFailed;
                    self->waitAndRetry();
                    return;
                }

                // Send is successful
                // Reset the counter just in case this was after retrying
                self->retryCount = 0;

                // Keep the connection alive if server supports it
                // Else close the connection
                BMCWEB_LOG_DEBUG << "recvMessage() keepalive : "
                                 << self->parser->keep_alive();

                self->callback(self->parser->keep_alive(), self->connId);
            });
    }

    void waitAndRetry()
    {
        if (retryCount >= maxRetryAttempts)
        {
            BMCWEB_LOG_ERROR << "Maximum number of retries reached.";
            BMCWEB_LOG_DEBUG << "Retry policy: " << retryPolicyAction;
            if (retryPolicyAction == "TerminateAfterRetries")
            {
                // TODO: delete subscription
                state = ConnState::terminated;
                callback(false, connId);
            }
            if (retryPolicyAction == "SuspendRetries")
            {
                state = ConnState::suspended;
                callback(false, connId);
            }
            // Reset the retrycount to zero so that client can try connecting
            // again if needed
            retryCount = 0;
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
            [self(shared_from_this())](const boost::system::error_code ec) {
                if (ec == boost::asio::error::operation_aborted)
                {
                    BMCWEB_LOG_DEBUG
                        << "async_wait failed since the operation is aborted"
                        << ec.message();
                }
                else if (ec)
                {
                    BMCWEB_LOG_ERROR << "async_wait failed: " << ec.message();
                    // Ignore the error and continue the retry loop to attempt
                    // sending the event as per the retry policy
                }
                self->runningTimer = false;

                // Let's close the connection and restart from resolve.
                self->doCloseAndRetry();
            });
    }

    void doClose()
    {
        state = ConnState::closeInProgress;
        boost::beast::error_code ec;
        conn.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        conn.close();

        // not_connected happens sometimes so don't bother reporting it.
        if (ec && ec != boost::beast::errc::not_connected)
        {
            BMCWEB_LOG_ERROR << host << ":" << port
                             << ", id: " << std::to_string(connId)
                             << "shutdown failed: " << ec.message();
            return;
        }
        BMCWEB_LOG_DEBUG << host << ":" << port
                         << ", id: " << std::to_string(connId)
                         << " closed gracefully";
        if ((state != ConnState::suspended) && (state != ConnState::terminated))
        {
            state = ConnState::closed;
        }
    }

    void doCloseAndRetry()
    {
        state = ConnState::closeInProgress;
        boost::beast::error_code ec;
        conn.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        conn.close();

        // not_connected happens sometimes so don't bother reporting it.
        if (ec && ec != boost::beast::errc::not_connected)
        {
            BMCWEB_LOG_ERROR << host << ":" << port
                             << ", id: " << std::to_string(connId)
                             << "shutdown failed: " << ec.message();
            return;
        }
        BMCWEB_LOG_DEBUG << host << ":" << port
                         << ", id: " << std::to_string(connId)
                         << " closed gracefully";
        if ((state != ConnState::suspended) && (state != ConnState::terminated))
        {
            // Now let's try to resend the data
            state = ConnState::retry;
            this->doResolve();
        }
    }

  public:
    explicit ConnectionInfo(boost::asio::io_context& ioc, const std::string& id,
                            const std::string& destIP,
                            const std::string& destPort,
                            const std::string& destUri,
                            const boost::beast::http::fields& httpHeader,
                            const unsigned int connId,
                            const uint32_t& maxRetryAttempts,
                            const uint32_t& retryIntervalSecs,
                            const std::string& retryPolicyAction) :
        conn(ioc),
        timer(ioc),
        req(boost::beast::http::verb::post, destUri, 11, "", httpHeader),
        subId(id), host(destIP), port(destPort), connId(connId),
        maxRetryAttempts(maxRetryAttempts),
        retryIntervalSecs(retryIntervalSecs),
        retryPolicyAction(retryPolicyAction)
    {
        req.set(boost::beast::http::field::host, host);
        req.keep_alive(true);
    }
};

class ConnectionPool
{
  private:
    boost::asio::io_context& ioc;
    const std::string id;
    const std::string destIP;
    const std::string destPort;
    const std::string destUri;
    const boost::beast::http::fields& httpHeader;
    std::vector<std::shared_ptr<ConnectionInfo>> connections;
    boost::container::devector<std::string> requestDataQueue;
    boost::container::devector<std::function<void(bool, uint32_t)>>
        requestCallbackQueue;

    // Retry policy for all connections in the pool
    uint32_t maxRetryAttempts = 5;
    uint32_t retryIntervalSecs = 0;
    std::string retryPolicyAction = "TerminateAfterRetries";

    friend class HttpClient;

    // Gets called as part of callback after request is sent
    // Reuses the connection if there are any requests waiting to be sent
    // Otherwise closes the connection if it is not a keep-alive
    void sendNext(bool keepAlive, uint32_t connId)
    {
        auto conn = connections[connId];
        // Reuse the connection to send the next request in the queue
        if (!requestDataQueue.empty())
        {
            BMCWEB_LOG_DEBUG << std::to_string(requestDataQueue.size())
                             << " requests remaining in queue for " << destIP
                             << ":" << destPort << ", reusing connnection "
                             << std::to_string(connId);
            conn->data = requestDataQueue.front();
            requestDataQueue.pop_front();
            conn->callback = requestCallbackQueue.front();
            requestCallbackQueue.pop_front();
            if (keepAlive)
            {
                conn->sendMessage();
            }
            else
            {
                // Server is not keep-alive enabled so we need to close the
                // connection and then start over from resolve
                conn->doClose();
                conn->doResolve();
            }
            return;
        }

        // No more messages to send so close the connection if necessary
        if (keepAlive)
        {
            conn->state = ConnState::idle;
        }
        else
        {
            // Abort the connection since server is not keep-alive enabled
            conn->state = ConnState::abortConnection;
            conn->doClose();
        }
    }

    void sendData(const std::string& data)
    {
        // Callback to be called once the request has been sent
        auto cb = [this](bool keepAlive, uint32_t connId) {
            // If requests remain in the queue then we want to reuse this
            // connection to send the next request
            this->sendNext(keepAlive, connId);
        };

        // Reuse an existing connection if one is available
        for (unsigned int i = 0; i < connections.size(); i++)
        {
            auto conn = connections[i];
            if (conn->state == ConnState::idle)
            {
                BMCWEB_LOG_DEBUG << "Grabbing idle connection "
                                 << std::to_string(i) << " from pool " << destIP
                                 << ":" << destPort;
                conn->data = data;
                conn->callback = std::bind_front(cb);
                conn->sendMessage();
                return;
            }
            if ((conn->state == ConnState::initialized) ||
                (conn->state == ConnState::closed))
            {
                BMCWEB_LOG_DEBUG << "Reusing existing connection "
                                 << std::to_string(i) << " from pool " << destIP
                                 << ":" << destPort;
                conn->data = data;
                conn->callback = std::bind_front(cb);
                conn->doResolve();
                return;
            }
        }

        // All connections in use so create a new connection or add request to
        // the queue
        if (connections.size() < maxPoolSize)
        {
            BMCWEB_LOG_DEBUG << "Adding new connection to pool " << destIP
                             << ":" << destPort;
            this->addConnection();
            connections.back()->data = data;
            connections.back()->callback = std::bind_front(cb);
            connections.back()->doResolve();
        }
        else if (requestDataQueue.size() < maxRequestQueueSize)
        {
            BMCWEB_LOG_ERROR << "Max pool size reached. Adding data to queue.";
            requestDataQueue.push_back(data);
            requestCallbackQueue.push_back(std::bind_front(cb));
        }
        else
        {
            BMCWEB_LOG_ERROR << destIP << ":" << destPort
                             << " request queue full.  Dropping request.";
        }
    }

    void addConnection()
    {
        unsigned int newId = static_cast<unsigned int>(connections.size());

        connections.emplace_back(std::make_shared<ConnectionInfo>(
            ioc, id, destIP, destPort, destUri, httpHeader, newId,
            maxRetryAttempts, retryIntervalSecs, retryPolicyAction));

        BMCWEB_LOG_DEBUG << "Added connection "
                         << std::to_string(connections.size() - 1)
                         << " to pool " << destIP << ":" << destPort;
    }

  public:
    explicit ConnectionPool(boost::asio::io_context& ioc, const std::string& id,
                            const std::string& destIP,
                            const std::string& destPort,
                            const std::string& destUri,
                            const boost::beast::http::fields& httpHeader) :
        ioc(ioc),
        id(id), destIP(destIP), destPort(destPort), destUri(destUri),
        httpHeader(httpHeader)
    {
        std::string clientKey = destIP + ":" + destPort;
        BMCWEB_LOG_DEBUG << "Initializing connection pool for " << destIP << ":"
                         << destPort;

        // Use existing retry info if it was set before connections were created
        auto rp = unconnectedRetryInfo.find(clientKey);
        if (rp != unconnectedRetryInfo.end())
        {
            BMCWEB_LOG_DEBUG << "Using previous retry info for " << clientKey;

            maxRetryAttempts = rp->second.maxRetryAttempts;
            retryIntervalSecs = rp->second.retryIntervalSecs;
            retryPolicyAction = rp->second.retryPolicyAction;

            // We can remove the info now that a connection has been created
            unconnectedRetryInfo.erase(clientKey);
        }

        // Initialize the pool with a single connection
        addConnection();
    }
};

class HttpClient
{
  private:
    std::unordered_map<std::string, ConnectionPool> connectionPools;
    boost::asio::io_context& ioc =
        crow::connections::systemBus->get_io_context();
    HttpClient() = default;

  public:
    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;
    HttpClient(HttpClient&&) = delete;
    HttpClient& operator=(HttpClient&&) = delete;
    ~HttpClient() = default;

    static HttpClient& getInstance()
    {
        static HttpClient handler;
        return handler;
    }

    void sendData(const std::string& data, const std::string& id,
                  const std::string& destIP, const std::string& destPort,
                  const std::string& destUri,
                  const boost::beast::http::fields& httpHeader)
    {
        std::string clientKey = destIP + ":" + destPort;
        auto result = connectionPools.try_emplace(
            clientKey, ioc, id, destIP, destPort, destUri, httpHeader);

        if (result.second)
        {
            BMCWEB_LOG_DEBUG << "Created connection pool for " << clientKey;
        }
        else
        {
            BMCWEB_LOG_DEBUG << "Using existing connection pool for "
                             << clientKey;
        }

        // Send the data using either the existing connection pool or the newly
        // created connection pool
        result.first->second.sendData(data);
    }

    void setRetryConfig(const uint32_t retryAttempts,
                        const uint32_t retryTimeoutInterval,
                        const std::string& destIP, const std::string& destPort)
    {
        std::string clientKey = destIP + ":" + destPort;
        auto conn = connectionPools.find(clientKey);
        if (conn == connectionPools.end())
        {
            auto rp = unconnectedRetryInfo.find(clientKey);
            if (rp != unconnectedRetryInfo.end())
            {
                BMCWEB_LOG_DEBUG
                    << "setRetryConfig(): updating existing retry info for unconnected "
                    << clientKey;
                rp->second.maxRetryAttempts = retryAttempts;
                rp->second.retryIntervalSecs = retryTimeoutInterval;
            }
            else
            {
                BMCWEB_LOG_DEBUG
                    << "setRetryConfig(): creating retry info for unconnected "
                    << clientKey;
                RetryPolicyData obj;
                obj.maxRetryAttempts = retryAttempts;
                obj.retryIntervalSecs = retryTimeoutInterval;
                unconnectedRetryInfo[clientKey] = obj;
            }
        }
        else
        {
            BMCWEB_LOG_DEBUG
                << "setRetryConfig(): updating retry info for existing connection "
                << clientKey;
            conn->second.maxRetryAttempts = retryAttempts;
            conn->second.retryIntervalSecs = retryTimeoutInterval;
        }
    }

    void setRetryPolicy(const std::string& retryPolicy,
                        const std::string& destIP, const std::string& destPort)
    {
        std::string clientKey = destIP + ":" + destPort;
        auto conn = connectionPools.find(clientKey);
        if (conn == connectionPools.end())
        {
            auto rp = unconnectedRetryInfo.find(clientKey);
            if (rp != unconnectedRetryInfo.end())
            {
                BMCWEB_LOG_DEBUG
                    << "setRetryPolicy(): updating existing retry info for unconnected "
                    << clientKey;
                rp->second.retryPolicyAction = retryPolicy;
            }
            else
            {
                BMCWEB_LOG_DEBUG
                    << "setRetryPolicy(): creating retry info for unconnected "
                    << clientKey;
                RetryPolicyData obj;
                obj.retryPolicyAction = retryPolicy;
                unconnectedRetryInfo[clientKey] = obj;
            }
        }
        else
        {
            BMCWEB_LOG_DEBUG
                << "setRetryPolicy(): updating retry info for existing connection "
                << clientKey;
            conn->second.retryPolicyAction = retryPolicy;
        }
    }
};

} // namespace crow
