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

// It is assumed that the BMC should be able to handle 4 parallel connections
constexpr uint8_t maxPoolSize = 4;
constexpr uint8_t maxRequestQueueSize = 50;
constexpr unsigned int httpReadBodyLimit = 8192;

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
    std::chrono::seconds retryIntervalSecs = std::chrono::seconds(0);
    std::string retryPolicyAction = "TerminateAfterRetries";
    std::string name;
};

struct PendingRequest
{
    boost::beast::http::request<boost::beast::http::string_body> req;
    std::function<void(bool, uint32_t, Response&)> callback;
    RetryPolicyData retryPolicy;
    PendingRequest(
        boost::beast::http::request<boost::beast::http::string_body>&& req,
        const std::function<void(bool, uint32_t, Response&)>& callback,
        const RetryPolicyData& retryPolicy) :
        req(std::move(req)),
        callback(callback), retryPolicy(retryPolicy)
    {}
};

class ConnectionInfo : public std::enable_shared_from_this<ConnectionInfo>
{
  private:
    ConnState state = ConnState::initialized;
    uint32_t retryCount = 0;
    bool runningTimer = false;
    std::string subId;
    std::string host;
    uint16_t port;
    uint32_t connId;

    // Retry policy information
    // This should be updated before each message is sent
    RetryPolicyData retryPolicy;

    // Data buffers
    boost::beast::http::request<boost::beast::http::string_body> req;
    std::optional<
        boost::beast::http::response_parser<boost::beast::http::string_body>>
        parser;
    boost::beast::flat_static_buffer<httpReadBodyLimit> buffer;
    Response res;

    // Ascync callables
    std::function<void(bool, uint32_t, Response&)> callback;
    crow::async_resolve::Resolver resolver;
    boost::beast::tcp_stream conn;
    boost::asio::steady_timer timer;

    friend class ConnectionPool;

    void doResolve()
    {
        state = ConnState::resolveInProgress;
        BMCWEB_LOG_DEBUG << "Trying to resolve: " << host << ":"
                         << std::to_string(port)
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
                             << std::to_string(self->port)
                             << ", id: " << std::to_string(self->connId);
            self->doConnect(endpointList);
        };

        resolver.asyncResolve(host, port, std::move(respHandler));
    }

    void doConnect(
        const std::vector<boost::asio::ip::tcp::endpoint>& endpointList)
    {
        state = ConnState::connectInProgress;

        BMCWEB_LOG_DEBUG << "Trying to connect to: " << host << ":"
                         << std::to_string(port)
                         << ", id: " << std::to_string(connId);

        conn.expires_after(std::chrono::seconds(30));
        conn.async_connect(endpointList,
                           [self(shared_from_this())](
                               const boost::beast::error_code ec,
                               const boost::asio::ip::tcp::endpoint& endpoint) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "Connect " << endpoint.address().to_string()
                                 << ":" << std::to_string(endpoint.port())
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

        // Set a timeout on the operation
        conn.expires_after(std::chrono::seconds(30));

        // Send the HTTP request to the remote host
        boost::beast::http::async_write(
            conn, req,
            [self(shared_from_this())](const boost::beast::error_code& ec,
                                       const std::size_t& bytesTransferred) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "sendMessage() failed: " << ec.message();
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
        state = ConnState::recvInProgress;

        parser.emplace(std::piecewise_construct, std::make_tuple());
        parser->body_limit(httpReadBodyLimit);

        // Receive the HTTP response
        boost::beast::http::async_read(
            conn, buffer, *parser,
            [self(shared_from_this())](const boost::beast::error_code& ec,
                                       const std::size_t& bytesTransferred) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "recvMessage() failed: " << ec.message();
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
                BMCWEB_LOG_ERROR << "recvMessage() Listener Failed to "
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

            // Copy the response into a Response object so that it can be
            // processed by the callback function.
            self->res.clear();
            self->res.stringResponse = self->parser->release();
            self->callback(self->parser->keep_alive(), self->connId, self->res);
            });
    }

    void waitAndRetry()
    {
        if (retryCount >= retryPolicy.maxRetryAttempts)
        {
            BMCWEB_LOG_ERROR << "Maximum number of retries reached.";
            BMCWEB_LOG_DEBUG << "Retry policy: "
                             << retryPolicy.retryPolicyAction;

            // We want to return a 502 to indicate there was an error with the
            // external server
            res.clear();
            redfish::messages::operationFailed(res);

            if (retryPolicy.retryPolicyAction == "TerminateAfterRetries")
            {
                // TODO: delete subscription
                state = ConnState::terminated;
                callback(false, connId, res);
            }
            if (retryPolicy.retryPolicyAction == "SuspendRetries")
            {
                state = ConnState::suspended;
                callback(false, connId, res);
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

        BMCWEB_LOG_DEBUG << "Attempt retry after "
                         << std::to_string(
                                retryPolicy.retryIntervalSecs.count())
                         << " seconds. RetryCount = " << retryCount;
        timer.expires_after(retryPolicy.retryIntervalSecs);
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
            BMCWEB_LOG_ERROR << host << ":" << std::to_string(port)
                             << ", id: " << std::to_string(connId)
                             << "shutdown failed: " << ec.message();
            return;
        }
        BMCWEB_LOG_DEBUG << host << ":" << std::to_string(port)
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
            BMCWEB_LOG_ERROR << host << ":" << std::to_string(port)
                             << ", id: " << std::to_string(connId)
                             << "shutdown failed: " << ec.message();
            return;
        }
        BMCWEB_LOG_DEBUG << host << ":" << std::to_string(port)
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
                            const std::string& destIP, const uint16_t destPort,
                            const unsigned int connId) :
        subId(id),
        host(destIP), port(destPort), connId(connId), conn(ioc), timer(ioc)
    {}
};

class ConnectionPool : public std::enable_shared_from_this<ConnectionPool>
{
  private:
    boost::asio::io_context& ioc;
    const std::string id;
    const std::string destIP;
    const uint16_t destPort;
    std::vector<std::shared_ptr<ConnectionInfo>> connections;
    boost::container::devector<PendingRequest> requestQueue;

    friend class HttpClient;

    // Configure a connections's request, callback, and retry info in
    // preparation to begin sending the request
    void setConnProps(ConnectionInfo& conn)
    {
        if (requestQueue.empty())
        {
            BMCWEB_LOG_ERROR
                << "setConnProps() should not have been called when requestQueue is empty";
            return;
        }

        auto nextReq = requestQueue.front();
        conn.retryPolicy = std::move(nextReq.retryPolicy);
        conn.req = std::move(nextReq.req);
        conn.callback = std::move(nextReq.callback);

        BMCWEB_LOG_DEBUG << "Setting properties for connection " << conn.host
                         << ":" << std::to_string(conn.port)
                         << ", id: " << std::to_string(conn.connId)
                         << ", retry policy is \"" << conn.retryPolicy.name
                         << "\"";

        // We can remove the request from the queue at this point
        requestQueue.pop_front();
    }

    // Configures a connection to use the specific retry policy.
    inline void setConnRetryPolicy(ConnectionInfo& conn,
                                   const RetryPolicyData& retryPolicy)
    {
        BMCWEB_LOG_DEBUG << destIP << ":" << std::to_string(destPort)
                         << ", id: " << std::to_string(conn.connId)
                         << " using retry policy \"" << retryPolicy.name
                         << "\"";

        conn.retryPolicy = retryPolicy;
    }

    // Gets called as part of callback after request is sent
    // Reuses the connection if there are any requests waiting to be sent
    // Otherwise closes the connection if it is not a keep-alive
    void sendNext(bool keepAlive, uint32_t connId)
    {
        auto conn = connections[connId];
        // Reuse the connection to send the next request in the queue
        if (!requestQueue.empty())
        {
            BMCWEB_LOG_DEBUG << std::to_string(requestQueue.size())
                             << " requests remaining in queue for " << destIP
                             << ":" << std::to_string(destPort)
                             << ", reusing connnection "
                             << std::to_string(connId);

            setConnProps(*conn);

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

    void sendData(std::string& data, const std::string& destUri,
                  const boost::beast::http::fields& httpHeader,
                  const boost::beast::http::verb verb,
                  const RetryPolicyData& retryPolicy,
                  std::function<void(Response&)>& resHandler)
    {
        std::weak_ptr<ConnectionPool> weakSelf = weak_from_this();

        // Callback to be called once the request has been sent
        auto cb = [weakSelf, resHandler](bool keepAlive, uint32_t connId,
                                         Response& res) {
            // Allow provided callback to perform additional processing of the
            // request
            resHandler(res);

            // If requests remain in the queue then we want to reuse this
            // connection to send the next request
            std::shared_ptr<ConnectionPool> self = weakSelf.lock();
            if (!self)
            {
                BMCWEB_LOG_CRITICAL << self << " Failed to capture connection";
                return;
            }

            self->sendNext(keepAlive, connId);
        };

        // Construct the request to be sent
        boost::beast::http::request<boost::beast::http::string_body> thisReq(
            verb, destUri, 11, "", httpHeader);
        thisReq.set(boost::beast::http::field::host, destIP);
        thisReq.keep_alive(true);
        thisReq.body() = std::move(data);
        thisReq.prepare_payload();

        // Reuse an existing connection if one is available
        for (unsigned int i = 0; i < connections.size(); i++)
        {
            auto conn = connections[i];
            if ((conn->state == ConnState::idle) ||
                (conn->state == ConnState::initialized) ||
                (conn->state == ConnState::closed))
            {
                conn->req = std::move(thisReq);
                conn->callback = std::move(cb);
                setConnRetryPolicy(*conn, retryPolicy);
                std::string commonMsg = std::to_string(i) + " from pool " +
                                        destIP + ":" + std::to_string(destPort);

                if (conn->state == ConnState::idle)
                {
                    BMCWEB_LOG_DEBUG << "Grabbing idle connection "
                                     << commonMsg;
                    conn->sendMessage();
                }
                else
                {
                    BMCWEB_LOG_DEBUG << "Reusing existing connection "
                                     << commonMsg;
                    conn->doResolve();
                }
                return;
            }
        }

        // All connections in use so create a new connection or add request to
        // the queue
        if (connections.size() < maxPoolSize)
        {
            BMCWEB_LOG_DEBUG << "Adding new connection to pool " << destIP
                             << ":" << std::to_string(destPort);
            auto conn = addConnection();
            conn->req = std::move(thisReq);
            conn->callback = std::move(cb);
            setConnRetryPolicy(*conn, retryPolicy);
            conn->doResolve();
        }
        else if (requestQueue.size() < maxRequestQueueSize)
        {
            BMCWEB_LOG_ERROR << "Max pool size reached. Adding data to queue.";
            requestQueue.emplace_back(std::move(thisReq), std::move(cb),
                                      retryPolicy);
        }
        else
        {
            BMCWEB_LOG_ERROR << destIP << ":" << std::to_string(destPort)
                             << " request queue full.  Dropping request.";
        }
    }

    std::shared_ptr<ConnectionInfo>& addConnection()
    {
        unsigned int newId = static_cast<unsigned int>(connections.size());

        auto& ret = connections.emplace_back(
            std::make_shared<ConnectionInfo>(ioc, id, destIP, destPort, newId));

        BMCWEB_LOG_DEBUG << "Added connection "
                         << std::to_string(connections.size() - 1)
                         << " to pool " << destIP << ":"
                         << std::to_string(destPort);

        return ret;
    }

  public:
    explicit ConnectionPool(boost::asio::io_context& ioc, const std::string& id,
                            const std::string& destIP,
                            const uint16_t destPort) :
        ioc(ioc),
        id(id), destIP(destIP), destPort(destPort)
    {
        std::string clientKey = destIP + ":" + std::to_string(destPort);
        BMCWEB_LOG_DEBUG << "Initializing connection pool for " << destIP << ":"
                         << std::to_string(destPort);

        // Initialize the pool with a single connection
        addConnection();
    }
};

class HttpClient
{
  private:
    std::unordered_map<std::string, std::shared_ptr<ConnectionPool>>
        connectionPools;
    boost::asio::io_context& ioc =
        crow::connections::systemBus->get_io_context();
    std::unordered_map<std::string, RetryPolicyData> retryInfo;
    HttpClient() = default;

    // Used as a dummy callback by sendData() in order to call
    // sendDataWithCallback()
    static void genericResHandler(Response& res)
    {
        BMCWEB_LOG_DEBUG << "Response handled with return code: "
                         << std::to_string(res.resultInt());
    }

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

    // Send a request to destIP:destPort where additional processing of the
    // result is not required
    void sendData(std::string& data, const std::string& id,
                  const std::string& destIP, const uint16_t destPort,
                  const std::string& destUri,
                  const boost::beast::http::fields& httpHeader,
                  const boost::beast::http::verb verb,
                  const std::string& retryPolicyName)
    {
        std::function<void(Response&)> cb = genericResHandler;
        sendDataWithCallback(data, id, destIP, destPort, destUri, httpHeader,
                             verb, retryPolicyName, cb);
    }

    // Send request to destIP:destPort and use the provided callback to
    // handle the response
    void sendDataWithCallback(std::string& data, const std::string& id,
                              const std::string& destIP,
                              const uint16_t destPort,
                              const std::string& destUri,
                              const boost::beast::http::fields& httpHeader,
                              const boost::beast::http::verb verb,
                              const std::string& retryPolicyName,
                              std::function<void(Response&)>& resHandler)
    {
        std::string clientKey = destIP + ":" + std::to_string(destPort);
        // Use nullptr to avoid creating a ConnectionPool each time
        auto result = connectionPools.try_emplace(clientKey, nullptr);
        if (result.second)
        {
            // Now actually create the ConnectionPool shared_ptr since it does
            // not already exist
            result.first->second =
                std::make_shared<ConnectionPool>(ioc, id, destIP, destPort);
            BMCWEB_LOG_DEBUG << "Created connection pool for " << clientKey;
        }
        else
        {
            BMCWEB_LOG_DEBUG << "Using existing connection pool for "
                             << clientKey;
        }

        // Get the associated retry policy
        auto policy = retryInfo.try_emplace(retryPolicyName);
        if (policy.second)
        {
            BMCWEB_LOG_DEBUG << "Creating retry policy \"" << retryPolicyName
                             << "\" with default values";
            policy.first->second.name = retryPolicyName;
        }

        // Send the data using either the existing connection pool or the newly
        // created connection pool
        result.first->second->sendData(data, destUri, httpHeader, verb,
                                       policy.first->second, resHandler);
    }

    void setRetryConfig(const uint32_t retryAttempts,
                        const uint32_t retryTimeoutInterval,
                        const std::string& retryPolicyName)
    {
        // We need to create the retry policy if one does not already exist for
        // the given retryPolicyName
        auto result = retryInfo.try_emplace(retryPolicyName);
        if (result.second)
        {
            BMCWEB_LOG_DEBUG << "setRetryConfig(): Creating new retry policy \""
                             << retryPolicyName << "\"";
            result.first->second.name = retryPolicyName;
        }
        else
        {
            BMCWEB_LOG_DEBUG << "setRetryConfig(): Updating retry info for \""
                             << retryPolicyName << "\"";
        }

        result.first->second.maxRetryAttempts = retryAttempts;
        result.first->second.retryIntervalSecs =
            std::chrono::seconds(retryTimeoutInterval);
    }

    void setRetryPolicy(const std::string& retryPolicy,
                        const std::string& retryPolicyName)
    {
        // We need to create the retry policy if one does not already exist for
        // the given retryPolicyName
        auto result = retryInfo.try_emplace(retryPolicyName);
        if (result.second)
        {
            BMCWEB_LOG_DEBUG << "setRetryPolicy(): Creating new retry policy \""
                             << retryPolicyName << "\"";
            result.first->second.name = retryPolicyName;
        }
        else
        {
            BMCWEB_LOG_DEBUG << "setRetryPolicy(): Updating retry policy for \""
                             << retryPolicyName << "\"";
        }

        result.first->second.retryPolicyAction = retryPolicy;
    }
};
} // namespace crow
