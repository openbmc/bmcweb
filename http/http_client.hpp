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
#include <boost/circular_buffer.hpp>
#include <include/async_resolve.hpp>

#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <queue>
#include <string>

namespace crow
{

static constexpr uint32_t maxPoolSize = 50;
static constexpr uint8_t maxRequestQueueSize = 50;
static constexpr unsigned int httpReadBodyLimit = 8192;

// TODO ME: Add retry variables

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

class ConnectionInfo
// class ConnectionInfo : public std::enable_shared_from_this<ConnectionInfo>
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

    // TODO ME: Can these be moved up a level?
    std::string subId;
    std::string host;
    std::string port;

    unsigned int connId;

    friend class ConnectionPool;

    void doResolve(const std::string& data)
    {
        state = ConnState::resolveInProgress;
        BMCWEB_LOG_DEBUG << "Trying to resolve: " << host << ":" << port
                         << ", id: " << std::to_string(connId);

        auto respHandler =
            [this, data](const boost::beast::error_code ec,
                         const std::vector<boost::asio::ip::tcp::endpoint>&
                             endpointList) {
                if (ec || (endpointList.empty()))
                {
                    BMCWEB_LOG_ERROR << "Resolve failed: " << ec.message();
                    // self->state = ConnState::resolveFailed;
                    this->state = ConnState::resolveFailed;

                    // TODO ME: Handle retry
                    // self->handleConnState();
                    return;
                }
                BMCWEB_LOG_DEBUG << "Resolved";
                // self->doConnect(data, endpointList);
                this->doConnect(data, endpointList);
            };

        resolver.asyncResolve(host, port, std::move(respHandler));
    }

    void doConnect(
        const std::string& data,
        const std::vector<boost::asio::ip::tcp::endpoint>& endpointList)
    {
        state = ConnState::connectInProgress;

        BMCWEB_LOG_DEBUG << "Trying to connect to: " << host << ":" << port
                         << ", id: " << std::to_string(connId);

        conn.expires_after(std::chrono::seconds(30));
        conn.async_connect(
            endpointList,
            [this, data](const boost::beast::error_code ec,
                         const boost::asio::ip::tcp::endpoint& endpoint) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Connect "
                                     << endpoint.address().to_string()
                                     << " failed: " << ec.message();
                    // self->state = ConnState::connectFailed;
                    this->state = ConnState::connectFailed;

                    // TODO ME: Configure retry
                    // self->handleConnState();
                    return;
                }
                BMCWEB_LOG_DEBUG << "Connected to: "
                                 << endpoint.address().to_string();
                // self->state = ConnState::connected;
                this->state = ConnState::connected;

                // TODO ME: Make sure this sends correctly
                // self->handleConnState();
                this->sendMessage(data);
            });
    }
    /*
        void doResolve()
        {
            state = ConnState::resolveInProgress;
            BMCWEB_LOG_DEBUG << "Trying to resolve: " << host << ":" << port
                             << ", id: " << std::to_string(connId);

            //auto self(shared_from_this());
            BMCWEB_LOG_DEBUG << "About to create respHandler";

            auto respHandler =
                [this](
                    const boost::beast::error_code ec,
                    const std::vector<boost::asio::ip::tcp::endpoint>&
                        endpointList) {
                    if (ec || (endpointList.empty()))
                    {
                        BMCWEB_LOG_ERROR << "Resolve failed: " << ec.message();
                        //self->state = ConnState::resolveFailed;
                        this->state = ConnState::resolveFailed;

                        //TODO ME: Configure retry
                        //self->handleConnState();
                        return;
                    }
                    BMCWEB_LOG_DEBUG << "Resolved";
                    //self->doConnect(endpointList);
                    this->doConnect(endpointList);
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
                                  const boost::asio::ip::tcp::endpoint&
    endpoint) { if (ec)
                    {
                        BMCWEB_LOG_ERROR << "Connect "
                                         << endpoint.address().to_string()
                                         << " failed: " << ec.message();
                        self->state = ConnState::connectFailed;
    //                    self->handleConnState();
                        return;
                    }
                    BMCWEB_LOG_DEBUG << "Connected to: "
                                     << endpoint.address().to_string();
                    self->state = ConnState::connected;
    //                self->handleConnState();
                });
        }
    */
    void sendMessage(const std::string& data)
    {
        //        boost::beast::http::request<boost::beast::http::string_body>
        //        req = connections[0]->req; boost::beast::tcp_stream* conn =
        //        &connections[0]->conn;

        state = ConnState::sendInProgress;

        req.body() = data;
        req.prepare_payload();

        // Set a timeout on the operation
        conn.expires_after(std::chrono::seconds(30));

        // Send the HTTP request to the remote host
        boost::beast::http::async_write(
            conn, req,
            [this](const boost::beast::error_code& ec,
                   const std::size_t& bytesTransferred) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "sendMessage() failed: "
                                     << ec.message();
                    // self->state = ConnState::sendFailed;
                    this->state = ConnState::sendFailed;
                    // self->handleConnState();
                    return;
                }
                BMCWEB_LOG_DEBUG << "sendMessage() bytes transferred: "
                                 << bytesTransferred;
                boost::ignore_unused(bytesTransferred);

                // self->recvMessage();
                this->recvMessage();
            });
    }

    void recvMessage()
    {
        parser.emplace(std::piecewise_construct, std::make_tuple());
        parser->body_limit(httpReadBodyLimit);

        // Receive the HTTP response
        boost::beast::http::async_read(
            conn, buffer, *parser,
            [this](const boost::beast::error_code& ec,
                   const std::size_t& bytesTransferred) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "recvMessage() failed: "
                                     << ec.message();
                    // self->state = ConnState::recvFailed;
                    this->state = ConnState::recvFailed;
                    // self->handleConnState();
                    return;
                }
                BMCWEB_LOG_DEBUG << "recvMessage() bytes transferred: "
                                 << bytesTransferred;
                BMCWEB_LOG_DEBUG << "recvMessage() data: "
                                 << this->parser->get().body();

                // unsigned int respCode = self->parser->get().result_int();
                unsigned int respCode = this->parser->get().result_int();
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
                    this->state = ConnState::recvFailed;
                    // self->handleConnState();
                    return;
                }

                // Send is successful, Lets remove data from queue
                // check for next request data in queue.
                // if (!self->requestDataQueue.empty())
                //{
                //    self->requestDataQueue.pop_front();
                //}

                // Keep the connection alive if server supports it
                // Else close the connection
                BMCWEB_LOG_DEBUG << "recvMessage() keepalive : "
                                 << this->parser->keep_alive();
                if (!this->parser->keep_alive())
                {
                    // Abort the connection since server is not keep-alive
                    // enabled

                    // TODO ME: Actually close the connection?
                    this->state = ConnState::abortConnection;
                }
                else
                {
                    this->state = ConnState::idle;
                }

                // self->handleConnState();
            });
    }

    //  public:
    explicit ConnectionInfo(boost::asio::io_context& ioc, const std::string& id,
                            const std::string& destIP,
                            const std::string& destPort,
                            const std::string& destUri,
                            const boost::beast::http::fields& httpHeader,
                            const unsigned int connId) :
        conn(ioc),
        timer(ioc),
        req(boost::beast::http::verb::post, destUri, 11, "", httpHeader),
        state(ConnState::initialized), subId(id), host(destIP), port(destPort),
        connId(connId)
    {
        req.set(boost::beast::http::field::host, host);
        req.keep_alive(true);
    }
};

class ConnectionPool
// class ConnectionPool : public std::enable_shared_from_this<ConnectionPool>
{
  private:
    //    boost::asio::io_context& ioc;
    const std::string id;
    const std::string destIP;
    const std::string destPort;
    const std::string destUri;
    const boost::beast::http::fields& httpHeader;
    std::vector<ConnectionInfo*> connections;

    friend class HttpClient;

    void sendData(const std::string& data)
    {
        for (unsigned int i = 0; i < connections.size(); i++)
        {
            auto conn = connections[i];
            if ((conn->state == ConnState::idle) ||
                (conn->state == ConnState::connected))
            {
                BMCWEB_LOG_DEBUG << "Grabbing idle connection "
                                 << std::to_string(i) << " from pool " << destIP
                                 << ":" << destPort;
                conn->sendMessage(data);
                return;
            }
            if ((conn->state == ConnState::suspended) ||
                (conn->state == ConnState::terminated) ||
                (conn->state == ConnState::abortConnection) ||
                (conn->state == ConnState::initialized) ||
                (conn->state == ConnState::closed))
            {
                BMCWEB_LOG_DEBUG << "Reusing connection " << std::to_string(i)
                                 << " from pool " << destIP << ":" << destPort;
                conn->doResolve(data);
                return;
            }
            // TODO ME: In this loop should I handle terminated conns? Restart
            // or close?
        }

        // Create a new connection
        if (connections.size() < maxPoolSize)
        {
            BMCWEB_LOG_DEBUG << "Adding connection to pool " << destIP << ":"
                             << destPort;
            this->addConnection();
            connections.back()->doResolve(data);
        }
        else
        {
            BMCWEB_LOG_ERROR << "Max pool size reached. Ignoring data.";
        }
    }

    void addConnection()
    {
        unsigned int newId = static_cast<unsigned int>(connections.size());
        // ConnectionInfo* ci = new ConnectionInfo(ioc,id,destIP,destPort,
        ConnectionInfo* ci =
            new ConnectionInfo(connections::systemBus->get_io_context(), id,
                               destIP, destPort, destUri, httpHeader, newId);
        connections.push_back(ci);
        BMCWEB_LOG_DEBUG << "Added connection "
                         << std::to_string(connections.size() - 1)
                         << " to pool " << destIP << ":" << destPort;
    }

    //  public:
    // explicit ConnectionPool(boost::asio::io_context& ioc, const std::string&
    // id,
    explicit ConnectionPool(const std::string& id, const std::string& destIP,
                            const std::string& destPort,
                            const std::string& destUri,
                            const boost::beast::http::fields& httpHeader) :
        // ioc(ioc),
        id(id),
        destIP(destIP), destPort(destPort), destUri(destUri),
        httpHeader(httpHeader)
    {
        this->addConnection();
    }
};

// class HttpClient : public std::enable_shared_from_this<HttpClient>
class HttpClient
{
  private:
    // TODO ME: Make retry info into static globals so they are accessible
    // to individual clients
    uint32_t retryCount = 0;
    uint32_t maxRetryAttempts = 5;
    uint32_t retryIntervalSecs = 0;
    std::string retryPolicyAction = "TerminateAfterRetries";
    bool runningTimer = false;
    std::vector<ConnectionPool> connectionPools;

    HttpClient() = default;

    /*
    void doClose()
    {
        boost::beast::tcp_stream* conn = &connections[0]->conn;

        state = ConnState::closeInProgress;
        boost::beast::error_code ec;
        conn->socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both,
    ec); conn->close();

        // not_connected happens sometimes so don't bother reporting it.
        if (ec && ec != boost::beast::errc::not_connected)
        {
            BMCWEB_LOG_ERROR << "shutdown failed: " << ec.message();
            return;
        }
        BMCWEB_LOG_DEBUG << "Connection closed gracefully";
        if ((state != ConnState::suspended) && (state != ConnState::terminated))
        {
            state = ConnState::closed;
            handleConnState();
        }
    }
    */

    /*
    void waitAndRetry()
    {
//        boost::beast::http::request<boost::beast::http::string_body> req =
connections[0]->req;
//        boost::beast::tcp_stream conn = connections[0]->conn;
//        boost::asio::steady_timer* timer = &connections[0]->timer;

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
        timer->expires_after(std::chrono::seconds(retryIntervalSecs));
        timer->async_wait(
            [self = shared_from_this()](const boost::system::error_code ec) {
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

                // Lets close connection and start from resolve.
                self->doClose();
            });
    }
*/

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

    bool connectionExists(const std::string& destIP,
                          const std::string& destPort)
    {
        for (auto& pool : connectionPools)
        {
            if ((pool.destIP == destIP) && (pool.destPort == destPort))
            {
                return true;
            }
        }
        return false;
    }

    // void createConnection(boost::asio::io_context& ioc, const std::string&
    // id,
    void createConnection(const std::string& id, const std::string& destIP,
                          const std::string& destPort,
                          const std::string& destUri,
                          const boost::beast::http::fields& httpHeader)
    {
        // ConnectionPool cp(ioc,id,destIP,destPort,destUri,httpHeader);
        ConnectionPool cp(id, destIP, destPort, destUri, httpHeader);
        connectionPools.push_back(cp);
    }

    void sendData(const std::string& data, const std::string& destIP,
                  const std::string& destPort)
    {
        for (auto& pool : connectionPools)
        {
            if ((pool.destIP == destIP) && (pool.destPort == destPort))
            {
                pool.sendData(data);
                return;
            }
        }

        BMCWEB_LOG_ERROR << "Requested connection " << destIP << ":" << destPort
                         << " does not exist.";
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
