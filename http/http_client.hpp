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

#include "async_resolve.hpp"
#include "http_response.hpp"
#include "logging.hpp"
#include "ssl_key_handler.hpp"

#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/basic_endpoint.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/flat_static_buffer.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/beast/version.hpp>
#include <boost/container/devector.hpp>
#include <boost/system/error_code.hpp>

#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <queue>
#include <string>

namespace crow
{

// With Redfish Aggregation it is assumed we will connect to another instance
// of BMCWeb which can handle 100 simultaneous connections.
constexpr size_t maxPoolSize = 20;
constexpr size_t maxRequestQueueSize = 500;
constexpr unsigned int httpReadBodyLimit = 131072;
constexpr unsigned int httpReadBufferSize = 4096;

enum class ConnState
{
    initialized,
    resolveInProgress,
    resolveFailed,
    connectInProgress,
    connectFailed,
    connected,
    handshakeInProgress,
    handshakeFailed,
    sendInProgress,
    sendFailed,
    recvInProgress,
    recvFailed,
    idle,
    closed,
    suspended,
    terminated,
    abortConnection,
    sslInitFailed,
    retry
};

static inline boost::system::error_code
    defaultRetryHandler(unsigned int respCode)
{
    // As a default, assume 200X is alright
    BMCWEB_LOG_DEBUG << "Using default check for response code validity";
    if ((respCode < 200) || (respCode >= 300))
    {
        return boost::system::errc::make_error_code(
            boost::system::errc::result_out_of_range);
    }

    // Return 0 if the response code is valid
    return boost::system::errc::make_error_code(boost::system::errc::success);
};

// We need to allow retry information to be set before a message has been sent
// and a connection pool has been created
struct ConnectionPolicy
{
    uint32_t maxRetryAttempts = 5;

    // the max size of requests in bytes.  0 for unlimited
    boost::optional<uint64_t> requestByteLimit = httpReadBodyLimit;

    size_t maxConnections = 1;

    std::string retryPolicyAction = "TerminateAfterRetries";

    std::chrono::seconds retryIntervalSecs = std::chrono::seconds(0);
    std::function<boost::system::error_code(unsigned int respCode)>
        invalidResp = defaultRetryHandler;
};

struct PendingRequest
{
    boost::beast::http::request<boost::beast::http::string_body> req;
    std::function<void(bool, uint32_t, Response&)> callback;
    PendingRequest(
        boost::beast::http::request<boost::beast::http::string_body>&& reqIn,
        const std::function<void(bool, uint32_t, Response&)>& callbackIn) :
        req(std::move(reqIn)),
        callback(callbackIn)
    {}
};

class ConnectionInfo : public std::enable_shared_from_this<ConnectionInfo>
{
  private:
    ConnState state = ConnState::initialized;
    uint32_t retryCount = 0;
    std::string subId;
    std::shared_ptr<ConnectionPolicy> connPolicy;
    std::string host;
    uint16_t port;
    uint32_t connId;

    // Data buffers
    boost::beast::http::request<boost::beast::http::string_body> req;
    std::optional<
        boost::beast::http::response_parser<boost::beast::http::string_body>>
        parser;
    boost::beast::flat_static_buffer<httpReadBufferSize> buffer;
    Response res;

    // Ascync callables
    std::function<void(bool, uint32_t, Response&)> callback;
    crow::async_resolve::Resolver resolver;
    boost::asio::ip::tcp::socket conn;
    std::optional<boost::beast::ssl_stream<boost::asio::ip::tcp::socket&>>
        sslConn;

    boost::asio::steady_timer timer;

    friend class ConnectionPool;

    void doResolve()
    {
        state = ConnState::resolveInProgress;
        BMCWEB_LOG_DEBUG << "Trying to resolve: " << host << ":"
                         << std::to_string(port)
                         << ", id: " << std::to_string(connId);

        resolver.asyncResolve(host, port,
                              std::bind_front(&ConnectionInfo::afterResolve,
                                              this, shared_from_this()));
    }

    void afterResolve(
        const std::shared_ptr<ConnectionInfo>& /*self*/,
        const boost::beast::error_code ec,
        const std::vector<boost::asio::ip::tcp::endpoint>& endpointList)
    {
        if (ec || (endpointList.empty()))
        {
            BMCWEB_LOG_ERROR << "Resolve failed: " << ec.message();
            state = ConnState::resolveFailed;
            waitAndRetry();
            return;
        }
        BMCWEB_LOG_DEBUG << "Resolved " << host << ":" << std::to_string(port)
                         << ", id: " << std::to_string(connId);
        state = ConnState::connectInProgress;

        BMCWEB_LOG_DEBUG << "Trying to connect to: " << host << ":"
                         << std::to_string(port)
                         << ", id: " << std::to_string(connId);

        timer.expires_after(std::chrono::seconds(30));
        timer.async_wait(std::bind_front(onTimeout, weak_from_this()));

        boost::asio::async_connect(
            conn, endpointList,
            std::bind_front(&ConnectionInfo::afterConnect, this,
                            shared_from_this()));
    }

    void afterConnect(const std::shared_ptr<ConnectionInfo>& /*self*/,
                      boost::beast::error_code ec,
                      const boost::asio::ip::tcp::endpoint& endpoint)
    {
        // The operation already timed out.  We don't want do continue down
        // this branch
        if (ec && ec == boost::asio::error::operation_aborted)
        {
            return;
        }

        timer.cancel();
        if (ec)
        {
            BMCWEB_LOG_ERROR << "Connect " << endpoint.address().to_string()
                             << ":" << std::to_string(endpoint.port())
                             << ", id: " << std::to_string(connId)
                             << " failed: " << ec.message();
            state = ConnState::connectFailed;
            waitAndRetry();
            return;
        }
        BMCWEB_LOG_DEBUG << "Connected to: " << endpoint.address().to_string()
                         << ":" << std::to_string(endpoint.port())
                         << ", id: " << std::to_string(connId);
        if (sslConn)
        {
            doSslHandshake();
            return;
        }
        state = ConnState::connected;
        sendMessage();
    }

    void doSslHandshake()
    {
        if (!sslConn)
        {
            return;
        }
        state = ConnState::handshakeInProgress;
        timer.expires_after(std::chrono::seconds(30));
        timer.async_wait(std::bind_front(onTimeout, weak_from_this()));
        sslConn->async_handshake(
            boost::asio::ssl::stream_base::client,
            std::bind_front(&ConnectionInfo::afterSslHandshake, this,
                            shared_from_this()));
    }

    void afterSslHandshake(const std::shared_ptr<ConnectionInfo>& /*self*/,
                           boost::beast::error_code ec)
    {
        // The operation already timed out.  We don't want do continue down
        // this branch
        if (ec && ec == boost::asio::error::operation_aborted)
        {
            return;
        }

        timer.cancel();
        if (ec)
        {
            BMCWEB_LOG_ERROR << "SSL Handshake failed -"
                             << " id: " << std::to_string(connId)
                             << " error: " << ec.message();
            state = ConnState::handshakeFailed;
            waitAndRetry();
            return;
        }
        BMCWEB_LOG_DEBUG << "SSL Handshake successful -"
                         << " id: " << std::to_string(connId);
        state = ConnState::connected;
        sendMessage();
    }

    void sendMessage()
    {
        state = ConnState::sendInProgress;

        // Set a timeout on the operation
        timer.expires_after(std::chrono::seconds(30));
        timer.async_wait(std::bind_front(onTimeout, weak_from_this()));

        // Send the HTTP request to the remote host
        if (sslConn)
        {
            boost::beast::http::async_write(
                *sslConn, req,
                std::bind_front(&ConnectionInfo::afterWrite, this,
                                shared_from_this()));
        }
        else
        {
            boost::beast::http::async_write(
                conn, req,
                std::bind_front(&ConnectionInfo::afterWrite, this,
                                shared_from_this()));
        }
    }

    void afterWrite(const std::shared_ptr<ConnectionInfo>& /*self*/,
                    const boost::beast::error_code& ec, size_t bytesTransferred)
    {
        // The operation already timed out.  We don't want do continue down
        // this branch
        if (ec && ec == boost::asio::error::operation_aborted)
        {
            return;
        }

        timer.cancel();
        if (ec)
        {
            BMCWEB_LOG_ERROR << "sendMessage() failed: " << ec.message();
            state = ConnState::sendFailed;
            waitAndRetry();
            return;
        }
        BMCWEB_LOG_DEBUG << "sendMessage() bytes transferred: "
                         << bytesTransferred;

        recvMessage();
    }

    void recvMessage()
    {
        state = ConnState::recvInProgress;

        parser.emplace(std::piecewise_construct, std::make_tuple());

        parser->body_limit(connPolicy->requestByteLimit);

        timer.expires_after(std::chrono::seconds(30));
        timer.async_wait(std::bind_front(onTimeout, weak_from_this()));

        // Receive the HTTP response
        if (sslConn)
        {
            boost::beast::http::async_read(
                *sslConn, buffer, *parser,
                std::bind_front(&ConnectionInfo::afterRead, this,
                                shared_from_this()));
        }
        else
        {
            boost::beast::http::async_read(
                conn, buffer, *parser,
                std::bind_front(&ConnectionInfo::afterRead, this,
                                shared_from_this()));
        }
    }

    void afterRead(const std::shared_ptr<ConnectionInfo>& /*self*/,
                   const boost::beast::error_code& ec,
                   const std::size_t& bytesTransferred)
    {
        // The operation already timed out.  We don't want do continue down
        // this branch
        if (ec && ec == boost::asio::error::operation_aborted)
        {
            return;
        }

        timer.cancel();
        if (ec && ec != boost::asio::ssl::error::stream_truncated)
        {
            BMCWEB_LOG_ERROR << "recvMessage() failed: " << ec.message();
            state = ConnState::recvFailed;
            waitAndRetry();
            return;
        }
        BMCWEB_LOG_DEBUG << "recvMessage() bytes transferred: "
                         << bytesTransferred;
        BMCWEB_LOG_DEBUG << "recvMessage() data: " << parser->get().body();

        unsigned int respCode = parser->get().result_int();
        BMCWEB_LOG_DEBUG << "recvMessage() Header Response Code: " << respCode;

        // Make sure the received response code is valid as defined by
        // the associated retry policy
        if (connPolicy->invalidResp(respCode))
        {
            // The listener failed to receive the Sent-Event
            BMCWEB_LOG_ERROR << "recvMessage() Listener Failed to "
                                "receive Sent-Event. Header Response Code: "
                             << respCode;
            state = ConnState::recvFailed;
            waitAndRetry();
            return;
        }

        // Send is successful
        // Reset the counter just in case this was after retrying
        retryCount = 0;

        // Keep the connection alive if server supports it
        // Else close the connection
        BMCWEB_LOG_DEBUG << "recvMessage() keepalive : "
                         << parser->keep_alive();

        // Copy the response into a Response object so that it can be
        // processed by the callback function.
        res.stringResponse = parser->release();
        callback(parser->keep_alive(), connId, res);
        res.clear();
    }

    static void onTimeout(const std::weak_ptr<ConnectionInfo>& weakSelf,
                          const boost::system::error_code& ec)
    {
        if (ec == boost::asio::error::operation_aborted)
        {
            BMCWEB_LOG_DEBUG
                << "async_wait failed since the operation is aborted";
            return;
        }
        if (ec)
        {
            BMCWEB_LOG_ERROR << "async_wait failed: " << ec.message();
            // If the timer fails, we need to close the socket anyway, same as
            // if it expired.
        }
        std::shared_ptr<ConnectionInfo> self = weakSelf.lock();
        if (self == nullptr)
        {
            return;
        }
        self->waitAndRetry();
    }

    void waitAndRetry()
    {
        if ((retryCount >= connPolicy->maxRetryAttempts) ||
            (state == ConnState::sslInitFailed))
        {
            BMCWEB_LOG_ERROR << "Maximum number of retries reached.";
            BMCWEB_LOG_DEBUG << "Retry policy: "
                             << connPolicy->retryPolicyAction;

            if (connPolicy->retryPolicyAction == "TerminateAfterRetries")
            {
                // TODO: delete subscription
                state = ConnState::terminated;
            }
            if (connPolicy->retryPolicyAction == "SuspendRetries")
            {
                state = ConnState::suspended;
            }

            // We want to return a 502 to indicate there was an error with
            // the external server
            res.result(boost::beast::http::status::bad_gateway);
            callback(false, connId, res);
            res.clear();

            // Reset the retrycount to zero so that client can try connecting
            // again if needed
            retryCount = 0;
            return;
        }

        retryCount++;

        BMCWEB_LOG_DEBUG << "Attempt retry after "
                         << std::to_string(
                                connPolicy->retryIntervalSecs.count())
                         << " seconds. RetryCount = " << retryCount;
        timer.expires_after(connPolicy->retryIntervalSecs);
        timer.async_wait(std::bind_front(&ConnectionInfo::onTimerDone, this,
                                         shared_from_this()));
    }

    void onTimerDone(const std::shared_ptr<ConnectionInfo>& /*self*/,
                     const boost::system::error_code& ec)
    {
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

        // Let's close the connection and restart from resolve.
        doClose(true);
    }

    void shutdownConn(bool retry)
    {
        boost::beast::error_code ec;
        conn.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        conn.close();

        // not_connected happens sometimes so don't bother reporting it.
        if (ec && ec != boost::beast::errc::not_connected)
        {
            BMCWEB_LOG_ERROR << host << ":" << std::to_string(port)
                             << ", id: " << std::to_string(connId)
                             << " shutdown failed: " << ec.message();
        }
        else
        {
            BMCWEB_LOG_DEBUG << host << ":" << std::to_string(port)
                             << ", id: " << std::to_string(connId)
                             << " closed gracefully";
        }

        if (retry)
        {
            // Now let's try to resend the data
            state = ConnState::retry;
            doResolve();
        }
        else
        {
            state = ConnState::closed;
        }
    }

    void doClose(bool retry = false)
    {
        if (!sslConn)
        {
            shutdownConn(retry);
            return;
        }

        sslConn->async_shutdown(
            std::bind_front(&ConnectionInfo::afterSslShutdown, this,
                            shared_from_this(), retry));
    }

    void afterSslShutdown(const std::shared_ptr<ConnectionInfo>& /*self*/,
                          bool retry, const boost::system::error_code& ec)
    {

        if (ec)
        {
            BMCWEB_LOG_ERROR << host << ":" << std::to_string(port)
                             << ", id: " << std::to_string(connId)
                             << " shutdown failed: " << ec.message();
        }
        else
        {
            BMCWEB_LOG_DEBUG << host << ":" << std::to_string(port)
                             << ", id: " << std::to_string(connId)
                             << " closed gracefully";
        }
        shutdownConn(retry);
    }

    void setCipherSuiteTLSext()
    {
        if (!sslConn)
        {
            return;
        }
        // NOTE: The SSL_set_tlsext_host_name is defined in tlsv1.h header
        // file but its having old style casting (name is cast to void*).
        // Since bmcweb compiler treats all old-style-cast as error, its
        // causing the build failure. So replaced the same macro inline and
        // did corrected the code by doing static_cast to viod*. This has to
        // be fixed in openssl library in long run. Set SNI Hostname (many
        // hosts need this to handshake successfully)
        if (SSL_ctrl(sslConn->native_handle(), SSL_CTRL_SET_TLSEXT_HOSTNAME,
                     TLSEXT_NAMETYPE_host_name,
                     static_cast<void*>(&host.front())) == 0)

        {
            boost::beast::error_code ec{static_cast<int>(::ERR_get_error()),
                                        boost::asio::error::get_ssl_category()};

            BMCWEB_LOG_ERROR << "SSL_set_tlsext_host_name " << host << ":"
                             << port << ", id: " << std::to_string(connId)
                             << " failed: " << ec.message();
            // Set state as sslInit failed so that we close the connection
            // and take appropriate action as per retry configuration.
            state = ConnState::sslInitFailed;
            waitAndRetry();
            return;
        }
    }

  public:
    explicit ConnectionInfo(
        boost::asio::io_context& iocIn, const std::string& idIn,
        const std::shared_ptr<ConnectionPolicy>& connPolicyIn,
        const std::string& destIPIn, uint16_t destPortIn, bool useSSL,
        unsigned int connIdIn) :
        subId(idIn),
        connPolicy(connPolicyIn), host(destIPIn), port(destPortIn),
        connId(connIdIn), conn(iocIn), timer(iocIn)
    {
        if (useSSL)
        {
            std::optional<boost::asio::ssl::context> sslCtx =
                ensuressl::getSSLClientContext();

            if (!sslCtx)
            {
                BMCWEB_LOG_ERROR << "prepareSSLContext failed - " << host << ":"
                                 << port << ", id: " << std::to_string(connId);
                // Don't retry if failure occurs while preparing SSL context
                // such as certificate is invalid or set cipher failure or set
                // host name failure etc... Setting conn state to sslInitFailed
                // and connection state will be transitioned to next state
                // depending on retry policy set by subscription.
                state = ConnState::sslInitFailed;
                waitAndRetry();
                return;
            }
            sslConn.emplace(conn, *sslCtx);
            setCipherSuiteTLSext();
        }
    }
};

class ConnectionPool : public std::enable_shared_from_this<ConnectionPool>
{
  private:
    boost::asio::io_context& ioc;
    std::string id;
    std::shared_ptr<ConnectionPolicy> connPolicy;
    std::string destIP;
    uint16_t destPort;
    bool useSSL;
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
        conn.req = std::move(nextReq.req);
        conn.callback = std::move(nextReq.callback);

        BMCWEB_LOG_DEBUG << "Setting properties for connection " << conn.host
                         << ":" << std::to_string(conn.port)
                         << ", id: " << std::to_string(conn.connId);

        // We can remove the request from the queue at this point
        requestQueue.pop_front();
    }

    // Gets called as part of callback after request is sent
    // Reuses the connection if there are any requests waiting to be sent
    // Otherwise closes the connection if it is not a keep-alive
    void sendNext(bool keepAlive, uint32_t connId)
    {
        auto conn = connections[connId];

        // Allow the connection's handler to be deleted
        // This is needed because of Redfish Aggregation passing an
        // AsyncResponse shared_ptr to this callback
        conn->callback = nullptr;

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

    // Callback to be called once the request has been sent
    static void afterSendData(const std::weak_ptr<ConnectionPool>& weakSelf,
                              const std::function<void(Response&)>& resHandler,
                              bool keepAlive, uint32_t connId, Response& res)
    {
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
    }

    std::shared_ptr<ConnectionInfo>& addConnection()
    {
        unsigned int newId = static_cast<unsigned int>(connections.size());

        auto& ret = connections.emplace_back(std::make_shared<ConnectionInfo>(
            ioc, id, connPolicy, destIP, destPort, useSSL, newId));

        BMCWEB_LOG_DEBUG << "Added connection "
                         << std::to_string(connections.size() - 1)
                         << " to pool " << destIP << ":"
                         << std::to_string(destPort);

        return ret;
    }

  public:
    explicit ConnectionPool(
        boost::asio::io_context& iocIn, const std::string& idIn,
        const std::shared_ptr<ConnectionPolicy>& connPolicyIn,
        const std::string& destIPIn, uint16_t destPortIn, bool useSSLIn) :
        ioc(iocIn),
        id(idIn), connPolicy(connPolicyIn), destIP(destIPIn),
        destPort(destPortIn), useSSL(useSSLIn)
    {
        BMCWEB_LOG_DEBUG << "Initializing connection pool for " << destIP << ":"
                         << std::to_string(destPort);

        // Initialize the pool with a single connection
        addConnection();
    }

    void sendData(std::string& data, const std::string& destUri,
                  const boost::beast::http::fields& httpHeader,
                  const boost::beast::http::verb verb,
                  const std::function<void(Response&)>& resHandler)
    {
        // Construct the request to be sent
        boost::beast::http::request<boost::beast::http::string_body> thisReq(
            verb, destUri, 11, "", httpHeader);
        thisReq.set(boost::beast::http::field::host, destIP);
        thisReq.keep_alive(true);
        thisReq.body() = std::move(data);
        thisReq.prepare_payload();
        auto cb = std::bind_front(&ConnectionPool::afterSendData,
                                  weak_from_this(), resHandler);
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
        if (connections.size() < connPolicy->maxConnections)
        {
            BMCWEB_LOG_DEBUG << "Adding new connection to pool " << destIP
                             << ":" << std::to_string(destPort);
            auto conn = addConnection();
            conn->req = std::move(thisReq);
            conn->callback = std::move(cb);
            conn->doResolve();
        }
        else if (requestQueue.size() < maxRequestQueueSize)
        {
            BMCWEB_LOG_ERROR << "Max pool size reached. Adding data to queue.";
            requestQueue.emplace_back(std::move(thisReq), std::move(cb));
        }
        else
        {
            // If we can't buffer the request then we should let the callback
            // handle a 429 Too Many Requests dummy response
            BMCWEB_LOG_ERROR << destIP << ":" << std::to_string(destPort)
                             << " request queue full.  Dropping request.";
            Response dummyRes;
            dummyRes.result(boost::beast::http::status::too_many_requests);
            resHandler(dummyRes);
        }
    }
};

class HttpClient
{
  private:
    std::unordered_map<std::string, ConnectionPool> connectionPools;
    boost::asio::io_context& ioc =
        crow::connections::systemBus->get_io_context();
    std::shared_ptr<ConnectionPolicy> connPolicy;

  public:
    // Used as a dummy callback by sendData() in order to call
    // sendDataWithCallback()
    static void genericResHandler(const Response& res)
    {
        BMCWEB_LOG_DEBUG << "Response handled with return code: "
                         << std::to_string(res.resultInt());
    }

    HttpClient() = delete;
    explicit HttpClient(const std::shared_ptr<ConnectionPolicy>& connPolicyIn) :
        connPolicy(connPolicyIn)
    {}
    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;
    HttpClient(HttpClient&&) = delete;
    HttpClient& operator=(HttpClient&&) = delete;
    ~HttpClient() = default;

    // Send a request to destIP:destPort where additional processing of the
    // result is not required
    void sendData(std::string& data, const std::string& destIP,
                  uint16_t destPort, const std::string& destUri, bool useSSL,
                  const boost::beast::http::fields& httpHeader,
                  const boost::beast::http::verb verb)
    {
        const std::function<void(Response&)> cb = genericResHandler;
        sendDataWithCallback(data, destIP, destPort, destUri, useSSL,
                             httpHeader, verb, cb);
    }

    // Send request to destIP:destPort and use the provided callback to
    // handle the response
    void sendDataWithCallback(std::string& data, const std::string& destIP,
                              uint16_t destPort, const std::string& destUri,
                              bool useSSL,
                              const boost::beast::http::fields& httpHeader,
                              const boost::beast::http::verb verb,
                              const std::function<void(Response&)>& resHandler)
    {
        std::string clientKey = useSSL ? "https" : "http";
        clientKey += destIP;
        clientKey += ":";
        clientKey += std::to_string(destPort);
        auto pool = connectionPools.try_emplace(
            clientKey, ioc, clientKey, connPolicy, destIP, destPort, useSSL);

        // Send the data using either the existing connection pool or the newly
        // created connection pool
        pool.first->second.sendData(data, destUri, httpHeader, verb,
                                    resHandler);
    }
};
} // namespace crow
