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
#include <boost/asio/connect.hpp>
#include <boost/asio/experimental/channel.hpp>
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
#include <http/http_response.hpp>
#include <include/async_resolve.hpp>
#include <logging.hpp>
#include <ssl_key_handler.hpp>

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
constexpr unsigned int httpReadBodyLimit = 131072;
constexpr unsigned int httpReadBufferSize = 4096;

struct PendingRequest
{
    boost::beast::http::request<boost::beast::http::string_body> req;
    std::function<void(Response&&)> callback;
    PendingRequest(
        boost::beast::http::request<boost::beast::http::string_body>&& reqIn,
        const std::function<void(Response&&)>& callbackIn) :
        req(std::move(reqIn)),
        callback(callbackIn)
    {}
    PendingRequest() = default;
};

using Channel = boost::asio::experimental::channel<void(
    boost::system::error_code, PendingRequest)>;

class ConnectionInfo : public std::enable_shared_from_this<ConnectionInfo>
{
  private:
    std::string host;
    uint16_t port;
    uint32_t connId;

    // Data buffers
    using BodyType = boost::beast::http::string_body;
    using RequestType = boost::beast::http::request<BodyType>;
    std::optional<RequestType> req;
    std::optional<boost::beast::http::response_parser<BodyType>> parser;
    boost::beast::flat_static_buffer<httpReadBufferSize> buffer;

    // Async callables
    std::function<void(Response&&)> callback;
    crow::async_resolve::Resolver resolver;
    boost::asio::ip::tcp::socket conn;
    std::optional<boost::beast::ssl_stream<boost::asio::ip::tcp::socket&>>
        sslConn;

    boost::asio::steady_timer timer;

    std::shared_ptr<Channel> channel;

    friend class ConnectionPool;

    void doResolve()
    {
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
            return;
        }
        BMCWEB_LOG_DEBUG << "Resolved " << host << ":" << std::to_string(port)
                         << ", id: " << std::to_string(connId);

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
        timer.cancel();
        if (ec)
        {
            BMCWEB_LOG_ERROR << "Connect " << endpoint.address().to_string()
                             << ":" << std::to_string(endpoint.port())
                             << ", id: " << std::to_string(connId)
                             << " failed: " << ec.message();
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
        sendMessage();
    }

    void doSslHandshake()
    {
        if (!sslConn)
        {
            return;
        }
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
        timer.cancel();
        if (ec)
        {
            BMCWEB_LOG_ERROR << "SSL Handshake failed -"
                             << " id: " << std::to_string(connId)
                             << " error: " << ec.message();
            return;
        }
        BMCWEB_LOG_DEBUG << "SSL Handshake successful -"
                         << " id: " << std::to_string(connId);
        sendMessage();
    }

    void sendMessage()
    {
        channel->async_receive(std::bind_front(
            &ConnectionInfo::onMessageReadyToSend, this, shared_from_this()));

        BMCWEB_LOG_DEBUG << "Start monitoring for socket errors";
        conn.async_wait(boost::asio::ip::tcp::socket::wait_error,
                        std::bind_front(&ConnectionInfo::onIdleEvent, this,
                                        weak_from_this()));
    }

    void onMessageReadyToSend(const std::shared_ptr<ConnectionInfo>& /*self*/,
                              boost::system::error_code ec,
                              PendingRequest pending)
    {
        if (ec)
        {
            return;
        }

        // Cancel our idle waiting event
        conn.cancel(ec);
        // intentionally ignore errors here.  It's possible there was nothing in
        // progress to cancel

        req = std::move(pending.req);
        callback = std::move(pending.callback);

        // Set a timeout on the operation
        timer.expires_after(std::chrono::seconds(30));
        timer.async_wait(std::bind_front(onTimeout, weak_from_this()));

        // Send the HTTP request to the remote host
        if (sslConn)
        {
            boost::beast::http::async_write(
                *sslConn, *req,
                std::bind_front(&ConnectionInfo::afterWrite, this,
                                shared_from_this()));
        }
        else
        {
            boost::beast::http::async_write(
                conn, *req,
                std::bind_front(&ConnectionInfo::afterWrite, this,
                                shared_from_this()));
        }
    }

    void onIdleEvent(const std::weak_ptr<ConnectionInfo>& /*self*/,
                     const boost::system::error_code& ec)
    {
        if (ec && ec != boost::asio::error::operation_aborted)
        {
            doClose();
        }
    }

    void afterWrite(const std::shared_ptr<ConnectionInfo>& /*self*/,
                    const boost::beast::error_code& ec, size_t bytesTransferred)
    {
        timer.cancel();
        if (ec)
        {
            BMCWEB_LOG_ERROR << "sendMessage() failed: " << ec.message();

            callback(Response(parser->release()));
            return;
        }
        BMCWEB_LOG_DEBUG << "sendMessage() bytes transferred: "
                         << bytesTransferred;

        recvMessage();
    }

    void recvMessage()
    {
        parser.emplace(std::piecewise_construct, std::make_tuple());
        parser->body_limit(httpReadBodyLimit);

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
        timer.cancel();
        if (ec && ec != boost::asio::ssl::error::stream_truncated)
        {
            BMCWEB_LOG_ERROR << "recvMessage() failed: " << ec.message();
            callback(Response(parser->release()));
            return;
        }
        BMCWEB_LOG_DEBUG << "recvMessage() bytes transferred: "
                         << bytesTransferred;
        BMCWEB_LOG_DEBUG << "recvMessage() data: " << parser->get().body();

        unsigned int respCode = parser->get().result_int();
        BMCWEB_LOG_DEBUG << "recvMessage() Header Response Code: " << respCode;

        // Keep the connection alive if server supports it
        // Else close the connection
        BMCWEB_LOG_DEBUG << "recvMessage() keepalive : "
                         << parser->keep_alive();

        // Copy the response into a Response object so that it can be
        // processed by the callback function.
        callback(Response(parser->release()));

        // Callback has served its purpose, let it destruct
        callback = nullptr;

        // Is more data is now loaded for the next request?
        if (parser->keep_alive())
        {
            sendMessage();
        }
        else
        {
            // Server is not keep-alive enabled so we need to close the
            // connection and then start over from resolve
            doClose();
            doResolve();
        }
    }

    static void onTimeout(const std::weak_ptr<ConnectionInfo>& weakSelf,
                          const boost::system::error_code ec)
    {
        if (ec == boost::asio::error::operation_aborted)
        {
            BMCWEB_LOG_DEBUG
                << "async_wait failed since the operation is aborted"
                << ec.message();
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
        self->doClose();
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
        }

        // Let's close the connection
        doClose();
    }

    void shutdownConn()
    {
        boost::beast::error_code ec;
        conn.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        conn.close();

        // not_connected happens sometimes so don't bother reporting it.
        if (ec && ec != boost::beast::errc::not_connected)
        {
            BMCWEB_LOG_ERROR << host << ":" << std::to_string(port)
                             << ", id: " << std::to_string(connId)
                             << "shutdown failed: " << ec.message();
        }
        else
        {
            BMCWEB_LOG_DEBUG << host << ":" << std::to_string(port)
                             << ", id: " << std::to_string(connId)
                             << " closed gracefully";
        }
    }

    void doClose()
    {
        if (!sslConn)
        {
            shutdownConn();
            return;
        }

        sslConn->async_shutdown(std::bind_front(
            &ConnectionInfo::afterSslShutdown, this, shared_from_this()));
    }

    void afterSslShutdown(const std::shared_ptr<ConnectionInfo>& /*self*/,
                          const boost::system::error_code& ec)
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
        shutdownConn();
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
            return;
        }
    }

  public:
    explicit ConnectionInfo(boost::asio::io_context& iocIn,
                            const std::string& destIPIn, uint16_t destPortIn,
                            bool useSSL, unsigned int connIdIn,
                            const std::shared_ptr<Channel>& channelIn) :
        host(destIPIn),
        port(destPortIn), connId(connIdIn), conn(iocIn), timer(iocIn),
        channel(channelIn)
    {
        if (useSSL)
        {
            std::optional<boost::asio::ssl::context> sslCtx =
                ensuressl::getSSLClientContext();

            if (!sslCtx)
            {
                BMCWEB_LOG_ERROR << "prepareSSLContext failed - " << host << ":"
                                 << port << ", id: " << std::to_string(connId);
                return;
            }
            sslConn.emplace(conn, *sslCtx);
            setCipherSuiteTLSext();
        }
        doResolve();
    }
};

class ConnectionPool : public std::enable_shared_from_this<ConnectionPool>
{
  private:
    boost::asio::io_context& ioc;
    std::string id;
    std::string destIP;
    uint16_t destPort;
    bool useSSL;
    std::array<std::weak_ptr<ConnectionInfo>, maxPoolSize> connections;

    // Note, this is sorted by value.attemptAfter, to ensure that we queue
    // operations in the appropriate order
    boost::container::devector<PendingRequest> requestQueue;

    // set to true when we're in process of pushing a message to the queue
    bool pushInProgress = false;
    std::shared_ptr<Channel> channel;

    friend class HttpClient;

    void queuePending(PendingRequest&& pending)
    {
        // If we have to queue it, push it into the request queue in time order
        if (pushInProgress)
        {
            if (requestQueue.size() >= maxRequestQueueSize)
            {
                BMCWEB_LOG_ERROR
                    << "Max pool size reached. Adding data to queue.";
                BMCWEB_LOG_ERROR << destIP << ":" << std::to_string(destPort)
                                 << " request queue full.  Dropping request.";
            }
            requestQueue.emplace_back(std::move(pending));
            return;
        }

        // Make sure we have some connections open ready to receive
        for (std::weak_ptr<ConnectionInfo>& weakConn : connections)
        {
            std::shared_ptr<ConnectionInfo> conn = weakConn.lock();
            if (conn == nullptr)
            {
                continue;
            }

            static unsigned int newId = 0;
            newId++;
            conn = std::make_shared<ConnectionInfo>(ioc, destIP, destPort,
                                                    useSSL, newId, channel);
            weakConn = conn->weak_from_this();

            // Only need to construct one extra connection max
            break;
        }
        pushInProgress = true;

        channel->async_send(
            boost::system::error_code(), std::move(pending),
            std::bind_front(&ConnectionPool::channelPushComplete, this));
    }

    void channelPushComplete(boost::system::error_code ec)
    {
        pushInProgress = false;
        if (ec)
        {
            BMCWEB_LOG_ERROR << "Failed to async_send" << ec;
            return;
        }
        if (!requestQueue.empty())
        {
            pushInProgress = true;

            channel->async_send(
                boost::system::error_code(), std::move(requestQueue.front()),
                std::bind_front(&ConnectionPool::channelPushComplete, this));
            requestQueue.pop_front();
        }
    }

  public:
    explicit ConnectionPool(boost::asio::io_context& iocIn,
                            const std::string& idIn,
                            const std::string& destIPIn, uint16_t destPortIn,
                            bool useSSLIn) :
        ioc(iocIn),
        id(idIn), destIP(destIPIn), destPort(destPortIn), useSSL(useSSLIn)
    {
        BMCWEB_LOG_DEBUG << "Initializing connection pool for " << destIP << ":"
                         << std::to_string(destPort);
    }
};

class HttpClient
{
  private:
    std::unordered_map<std::string, std::shared_ptr<ConnectionPool>>
        connectionPools;
    boost::asio::io_context& ioc =
        crow::connections::systemBus->get_io_context();
    HttpClient() = default;

    // Used as a dummy callback by sendData() in order to call
    // sendDataWithCallback()
    static void genericResHandler(const Response& res)
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
    void sendData(std::string&& data, const std::string& id,
                  const std::string& destIP, uint16_t destPort,
                  const std::string& destUri, bool useSSL,
                  const boost::beast::http::fields& httpHeader,
                  const boost::beast::http::verb verb)
    {
        const std::function<void(Response &&)> cb = genericResHandler;
        sendDataWithCallback(std::move(data), id, destIP, destPort, destUri,
                             useSSL, httpHeader, verb, cb);
    }

    // Send request to destIP:destPort and use the provided callback to
    // handle the response
    void sendDataWithCallback(std::string&& data, const std::string& id,
                              const std::string& destIP, uint16_t destPort,
                              const std::string& destUri, bool useSSL,
                              const boost::beast::http::fields& httpHeader,
                              const boost::beast::http::verb verb,
                              const std::function<void(Response&&)>& resHandler)
    {
        std::string clientKey = useSSL ? "https" : "http";
        clientKey += destIP;
        clientKey += ":";
        clientKey += std::to_string(destPort);
        // Use nullptr to avoid creating a ConnectionPool each time
        std::shared_ptr<ConnectionPool>& conn = connectionPools[clientKey];
        if (conn == nullptr)
        {
            // Now actually create the ConnectionPool shared_ptr since it does
            // not already exist
            conn = std::make_shared<ConnectionPool>(ioc, id, destIP, destPort,
                                                    useSSL);
            BMCWEB_LOG_DEBUG << "Created connection pool for " << clientKey;
        }
        else
        {
            BMCWEB_LOG_DEBUG << "Using existing connection pool for "
                             << clientKey;
        }

        // Send the data using either the existing connection pool or the newly
        // created connection pool
        // Construct the request to be sent
        boost::beast::http::request<boost::beast::http::string_body> thisReq(
            verb, destUri, 11, "", httpHeader);
        thisReq.set(boost::beast::http::field::host, destIP);
        thisReq.keep_alive(true);
        thisReq.body() = std::move(data);
        thisReq.prepare_payload();
        conn->queuePending(PendingRequest(std::move(thisReq), resHandler));
    }
};
} // namespace crow
