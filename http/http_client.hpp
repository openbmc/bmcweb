// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2020 Intel Corporation
#pragma once

#include "bmcweb_config.h"

#include "async_resolve.hpp"
#include "boost_formatters.hpp"
#include "http_body.hpp"
#include "http_response.hpp"
#include "logging.hpp"
#include "ssl_key_handler.hpp"

#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/tls1.h>

#include <boost/asio/connect.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/experimental/channel.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/ssl/stream_base.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/flat_static_buffer.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/optional/optional.hpp>
#include <boost/system/errc.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/host_type.hpp>
#include <boost/url/url.hpp>
#include <boost/url/url_view_base.hpp>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace crow
{
// With Redfish Aggregation it is assumed we will connect to another
// instance of BMCWeb which can handle 100 simultaneous connections.
constexpr size_t maxPoolSize = 20;
constexpr size_t maxRequestQueueSize = 500;
constexpr unsigned int httpReadBodyLimit = 131072;
constexpr unsigned int httpReadBufferSize = 4096;

inline boost::system::error_code defaultRetryHandler(unsigned int respCode)
{
    // As a default, assume 200X is alright
    BMCWEB_LOG_DEBUG("Using default check for response code validity");
    if ((respCode < 200) || (respCode >= 300))
    {
        return boost::system::errc::make_error_code(
            boost::system::errc::result_out_of_range);
    }

    // Return 0 if the response code is valid
    return boost::system::errc::make_error_code(boost::system::errc::success);
};

// We need to allow retry information to be set before a message has been
// sent and a connection pool has been created
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
    boost::beast::http::request<bmcweb::HttpBody> req;
    std::function<void(Response&&)> callback;
    PendingRequest(boost::beast::http::request<bmcweb::HttpBody>&& reqIn,
                   const std::function<void(Response&&)>& callbackIn) :
        req(std::move(reqIn)), callback(callbackIn)
    {}
    PendingRequest() = default;
};

namespace http = boost::beast::http;
using Channel = boost::asio::experimental::channel<void(
    boost::system::error_code, PendingRequest)>;

class ConnectionInfo : public std::enable_shared_from_this<ConnectionInfo>
{
  private:
    uint32_t retryCount = 0;
    std::string subId;
    std::shared_ptr<ConnectionPolicy> connPolicy;
    boost::urls::url host;
    ensuressl::VerifyCertificate verifyCert;
    uint32_t connId;

    std::optional<PendingRequest> pendingReq;

    // Data buffers
    Response res;

    using parser_type = http::response_parser<bmcweb::HttpBody>;
    std::optional<parser_type> parser;
    boost::beast::flat_static_buffer<httpReadBufferSize> buffer;

    // Async callables
    boost::asio::io_context& ioc;

    using Resolver = std::conditional_t<BMCWEB_DNS_RESOLVER == "systemd-dbus",
                                        async_resolve::Resolver,
                                        boost::asio::ip::tcp::resolver>;
    Resolver resolver;

    boost::asio::ip::tcp::socket conn;
    std::optional<boost::asio::ssl::stream<boost::asio::ip::tcp::socket&>>
        sslConn;

    boost::asio::steady_timer timer;

    Channel& channel;

    friend class ConnectionPool;

    void doResolve()
    {
        BMCWEB_LOG_DEBUG("Trying to resolve: {}, id: {}", host, connId);

        resolver.async_resolve(host.encoded_host_address(), host.port(),
                               std::bind_front(&ConnectionInfo::afterResolve,
                                               this, shared_from_this()));
    }

    void afterResolve(const std::shared_ptr<ConnectionInfo>& /*self*/,
                      const boost::system::error_code& ec,
                      const Resolver::results_type& endpointList)
    {
        if (ec || (endpointList.empty()))
        {
            BMCWEB_LOG_ERROR("Resolve failed: {} {}", ec.message(), host);
            return;
        }
        BMCWEB_LOG_DEBUG("Resolved {}, id: {}", host, connId);

        BMCWEB_LOG_DEBUG("Trying to connect to: {}, id: {}", host, connId);

        timer.expires_after(std::chrono::seconds(30));
        timer.async_wait(std::bind_front(onTimeout, weak_from_this()));

        boost::asio::async_connect(
            conn, endpointList,
            std::bind_front(&ConnectionInfo::afterConnect, this,
                            shared_from_this()));
    }

    void afterConnect(const std::shared_ptr<ConnectionInfo>& /*self*/,
                      const boost::beast::error_code& ec,
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
            BMCWEB_LOG_ERROR("Connect {}:{}, id: {} failed: {}",
                             host.encoded_host_address(), host.port(), connId,
                             ec.message());
            return;
        }
        BMCWEB_LOG_DEBUG("Connected to: {}:{}, id: {}",
                         endpoint.address().to_string(), endpoint.port(),
                         connId);
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
        auto& ssl = *sslConn;
        timer.expires_after(std::chrono::seconds(30));
        timer.async_wait(std::bind_front(onTimeout, weak_from_this()));
        ssl.async_handshake(boost::asio::ssl::stream_base::client,
                            std::bind_front(&ConnectionInfo::afterSslHandshake,
                                            this, shared_from_this()));
    }

    void afterSslHandshake(const std::shared_ptr<ConnectionInfo>& /*self*/,
                           const boost::beast::error_code& ec)
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
            BMCWEB_LOG_ERROR("SSL Handshake failed - id: {} error: {}", connId,
                             ec.message());
            return;
        }
        BMCWEB_LOG_DEBUG("SSL Handshake successful - id: {}", connId);
        sendMessage();
    }

    void sendMessage()
    {
        channel.async_receive(std::bind_front(
            &ConnectionInfo::onMessageReadyToSend, this, shared_from_this()));

        BMCWEB_LOG_DEBUG("Start monitoring for socket errors");
        conn.async_wait(boost::asio::ip::tcp::socket::wait_error,
                        std::bind_front(&ConnectionInfo::onIdleEvent, this,
                                        weak_from_this()));
    }

    void onMessageReadyToSend(const std::shared_ptr<ConnectionInfo>& /*self*/,
                              const boost::system::error_code& ec,
                              PendingRequest&& pending)
    {
        if (ec)
        {
            return;
        }

        // Cancel our idle waiting event
        boost::beast::error_code cancelEc;
        conn.cancel(cancelEc);
        // intentionally ignore errors here.  It's possible there was nothing in
        // progress to cancel

        PendingRequest& req = pendingReq.emplace(std::move(pending));

        // Set a timeout on the operation
        timer.expires_after(std::chrono::seconds(30));
        timer.async_wait(std::bind_front(onTimeout, weak_from_this()));
        // Send the HTTP request to the remote host
        if (sslConn)
        {
            boost::beast::http::async_write(
                *sslConn, req.req,
                std::bind_front(&ConnectionInfo::afterWrite, this,
                                shared_from_this()));
        }
        else
        {
            boost::beast::http::async_write(
                conn, req.req,
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
        // The operation already timed out.  We don't want do continue down
        // this branch
        if (ec && ec == boost::asio::error::operation_aborted)
        {
            return;
        }

        timer.cancel();
        if (ec)
        {
            BMCWEB_LOG_ERROR("sendMessage() failed: {} {}", ec.message(), host);
            return;
        }
        BMCWEB_LOG_DEBUG("sendMessage() bytes transferred: {}",
                         bytesTransferred);

        recvMessage();
    }

    void recvMessage()
    {
        parser_type& thisParser = parser.emplace();

        thisParser.body_limit(connPolicy->requestByteLimit);

        timer.expires_after(std::chrono::seconds(30));
        timer.async_wait(std::bind_front(onTimeout, weak_from_this()));

        // Receive the HTTP response
        if (sslConn)
        {
            boost::beast::http::async_read(
                *sslConn, buffer, thisParser,
                std::bind_front(&ConnectionInfo::afterRead, this,
                                shared_from_this()));
        }
        else
        {
            boost::beast::http::async_read(
                conn, buffer, thisParser,
                std::bind_front(&ConnectionInfo::afterRead, this,
                                shared_from_this()));
        }
    }

    void afterRead(const std::shared_ptr<ConnectionInfo>& /*self*/,
                   const boost::beast::error_code& ec,
                   const std::size_t bytesTransferred)
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
            BMCWEB_LOG_ERROR("recvMessage() failed: {} from {}", ec.message(),
                             host);
            return;
        }
        BMCWEB_LOG_DEBUG("recvMessage() bytes transferred: {}",
                         bytesTransferred);
        if (!parser)
        {
            return;
        }
        BMCWEB_LOG_DEBUG("recvMessage() data: {}", parser->get().body().str());

        unsigned int respCode = parser->get().result_int();
        BMCWEB_LOG_DEBUG("recvMessage() Header Response Code: {}", respCode);

        // Handle the case of stream_truncated.  Some servers close the ssl
        // connection uncleanly, so check to see if we got a full response
        // before we handle this as an error.
        if (!parser->is_done())
        {
            return;
        }

        // Make sure the received response code is valid as defined by
        // the associated retry policy
        if (connPolicy->invalidResp(respCode))
        {
            // The listener failed to receive the Sent-Event
            BMCWEB_LOG_ERROR(
                "recvMessage() Listener Failed to "
                "receive Sent-Event. Header Response Code: {} from {}",
                respCode, host);
            return;
        }

        // Send is successful
        // Reset the counter just in case this was after retrying
        retryCount = 0;

        // Keep the connection alive if server supports it
        // Else close the connection
        BMCWEB_LOG_DEBUG("recvMessage() keepalive : {}", parser->keep_alive());

        // Copy the response into a Response object so that it can be
        // processed by the callback function.
        res.response = parser->release();
        pendingReq->callback(std::move(res));
        res.clear();
        if (!parser->keep_alive())
        {
            shutdownConn();
        }
        else
        {
            sendMessage();
        }
    }

    static void onTimeout(const std::weak_ptr<ConnectionInfo>& weakSelf,
                          const boost::system::error_code& ec)
    {
        if (ec == boost::asio::error::operation_aborted)
        {
            BMCWEB_LOG_DEBUG(
                "async_wait failed since the operation is aborted");
            return;
        }
        if (ec)
        {
            BMCWEB_LOG_ERROR("async_wait failed: {}", ec.message());
            // If the timer fails, we need to close the socket anyway, same
            // as if it expired.
        }
        std::shared_ptr<ConnectionInfo> self = weakSelf.lock();
        if (self == nullptr)
        {
            return;
        }
    }

    void onTimerDone(const std::shared_ptr<ConnectionInfo>& /*self*/,
                     const boost::system::error_code& ec)
    {
        if (ec == boost::asio::error::operation_aborted)
        {
            BMCWEB_LOG_DEBUG(
                "async_wait failed since the operation is aborted{}",
                ec.message());
        }
        else if (ec)
        {
            BMCWEB_LOG_ERROR("async_wait failed: {}", ec.message());
            // Ignore the error and continue the retry loop to attempt
            // sending the event as per the retry policy
        }

        // Let's close the connection and restart from resolve.
        shutdownConn();
    }

    void shutdownConn()
    {
        boost::beast::error_code ec;
        conn.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        conn.close();

        // not_connected happens sometimes so don't bother reporting it.
        if (ec && ec != boost::beast::errc::not_connected)
        {
            BMCWEB_LOG_ERROR("{}, id: {} shutdown failed: {}", host, connId,
                             ec.message());
        }
        else
        {
            BMCWEB_LOG_DEBUG("{}, id: {} closed gracefully", host, connId);
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
            BMCWEB_LOG_ERROR("{}, id: {} shutdown failed: {}", host, connId,
                             ec.message());
        }
        else
        {
            BMCWEB_LOG_DEBUG("{}, id: {} closed gracefully", host, connId);
        }
        shutdownConn();
    }

    void setCipherSuiteTLSext()
    {
        if (!sslConn)
        {
            return;
        }

        if (host.host_type() != boost::urls::host_type::name)
        {
            // Avoid setting SNI hostname if its IP address
            return;
        }
        // Create a null terminated string for SSL
        std::string hostname(host.encoded_host_address());
        if (SSL_set_tlsext_host_name(sslConn->native_handle(),
                                     hostname.data()) == 0)

        {
            boost::beast::error_code ec{static_cast<int>(::ERR_get_error()),
                                        boost::asio::error::get_ssl_category()};

            BMCWEB_LOG_ERROR("SSL_set_tlsext_host_name {}, id: {} failed: {}",
                             host, connId, ec.message());
            // Set state as sslInit failed so that we close the connection
            // and take appropriate action as per retry configuration.
            return;
        }
    }

    void initializeConnection(bool ssl)
    {
        conn = boost::asio::ip::tcp::socket(ioc);
        if (ssl)
        {
            std::optional<boost::asio::ssl::context> sslCtx =
                ensuressl::getSSLClientContext(verifyCert);

            if (!sslCtx)
            {
                BMCWEB_LOG_ERROR("prepareSSLContext failed - {}, id: {}", host,
                                 connId);
                // Don't retry if failure occurs while preparing SSL context
                // such as certificate is invalid or set cipher failure or
                // set host name failure etc... Setting conn state to
                // sslInitFailed and connection state will be transitioned
                // to next state depending on retry policy set by
                // subscription.
                return;
            }
            sslConn.emplace(conn, *sslCtx);
            setCipherSuiteTLSext();
        }
        doResolve();
    }

  public:
    explicit ConnectionInfo(
        boost::asio::io_context& iocIn, const std::string& idIn,
        const std::shared_ptr<ConnectionPolicy>& connPolicyIn,
        const boost::urls::url_view_base& hostIn,
        ensuressl::VerifyCertificate verifyCertIn, unsigned int connIdIn,
        Channel& channelIn) :
        subId(idIn), connPolicy(connPolicyIn), host(hostIn),
        verifyCert(verifyCertIn), connId(connIdIn), ioc(iocIn), resolver(iocIn),
        conn(iocIn), timer(iocIn), channel(channelIn)
    {
        conn = boost::asio::ip::tcp::socket(ioc);
        if (host.scheme() == "https")
        {
            std::optional<boost::asio::ssl::context> sslCtx =
                ensuressl::getSSLClientContext(verifyCert);

            if (!sslCtx)
            {
                BMCWEB_LOG_ERROR("prepareSSLContext failed - {}, id: {}", host,
                                 connId);
                // Don't retry if failure occurs while preparing SSL context
                // such as certificate is invalid or set cipher failure or
                // set host name failure etc... Setting conn state to
                // sslInitFailed and connection state will be transitioned
                // to next state depending on retry policy set by
                // subscription.
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
  public:
    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;
    ConnectionPool(ConnectionPool&&) = delete;
    ConnectionPool& operator=(ConnectionPool&&) = delete;
    ~ConnectionPool() = default;

  private:
    boost::asio::io_context& ioc;
    std::string id;
    std::shared_ptr<ConnectionPolicy> connPolicy;
    boost::urls::url destIP;
    std::vector<std::shared_ptr<ConnectionInfo>> connections;
    Channel requestQueue;
    ensuressl::VerifyCertificate verifyCert;

    friend class HttpClient;

    void sendData(std::string&& data, const boost::urls::url_view_base& destUri,
                  const boost::beast::http::fields& httpHeader,
                  const boost::beast::http::verb verb,
                  const std::function<void(Response&)>& resHandler)
    {
        // Construct the request to be sent
        boost::beast::http::request<bmcweb::HttpBody> thisReq(
            verb, destUri.encoded_target(), 11, "", httpHeader);
        thisReq.set(boost::beast::http::field::host,
                    destUri.encoded_host_address());
        thisReq.keep_alive(true);
        thisReq.body().str() = std::move(data);
        thisReq.prepare_payload();
        auto cb = std::bind_front(&ConnectionPool::afterSendData,
                                  weak_from_this(), resHandler);
        PendingRequest pending(std::move(thisReq), std::move(cb));

        if (!requestQueue.try_send(boost::system::error_code(),
                                   std::move(pending)))
        {
            BMCWEB_LOG_ERROR("{} request queue full.  Dropping request.", id);
            Response dummyRes;
            dummyRes.result(boost::beast::http::status::too_many_requests);
            resHandler(dummyRes);
        }
    }

    // Callback to be called once the request has been sent
    static void afterSendData(const std::weak_ptr<ConnectionPool>& weakSelf,
                              const std::function<void(Response&)>& resHandler,
                              Response&& res)
    {
        // Allow provided callback to perform additional processing of the
        // request
        resHandler(res);

        // If requests remain in the queue then we want to reuse this
        // connection to send the next request
        std::shared_ptr<ConnectionPool> self = weakSelf.lock();
        if (!self)
        {
            BMCWEB_LOG_CRITICAL("{} Failed to capture connection",
                                logPtr(self.get()));
            return;
        }
    }

    void addConnection()
    {
        while (connections.size() < connPolicy->maxConnections)
        {
            BMCWEB_LOG_DEBUG("Adding new connection to pool {}", id);

            unsigned int newId = static_cast<unsigned int>(connections.size());

            connections.emplace_back(std::make_shared<ConnectionInfo>(
                ioc, id, connPolicy, destIP, verifyCert, newId, requestQueue));

            BMCWEB_LOG_DEBUG("Added connection {} to pool {}",
                             connections.size() - 1, id);
        }
    }

  public:
    explicit ConnectionPool(
        boost::asio::io_context& iocIn, const std::string& idIn,
        const std::shared_ptr<ConnectionPolicy>& connPolicyIn,
        const boost::urls::url_view_base& destIPIn,
        ensuressl::VerifyCertificate verifyCertIn) :
        ioc(iocIn), id(idIn), connPolicy(connPolicyIn), destIP(destIPIn),
        requestQueue(ioc, maxRequestQueueSize), verifyCert(verifyCertIn)
    {
        BMCWEB_LOG_DEBUG("Initializing connection pool for {}", id);

        // Initialize the pool with a single connection
        addConnection();
    }

    // Check whether all connections are terminated
    bool areAllConnectionsTerminated()
    {
        return false;
    }
};

class HttpClient
{
  private:
    std::unordered_map<std::string, std::shared_ptr<ConnectionPool>>
        connectionPools;

    // reference_wrapper here makes HttpClient movable
    std::reference_wrapper<boost::asio::io_context> ioc;
    std::shared_ptr<ConnectionPolicy> connPolicy;

    // Used as a dummy callback by sendData() in order to call
    // sendDataWithCallback()
    static void genericResHandler(const Response& res)
    {
        BMCWEB_LOG_DEBUG("Response handled with return code: {}",
                         res.resultInt());
    }

  public:
    HttpClient() = delete;
    explicit HttpClient(boost::asio::io_context& iocIn,
                        const std::shared_ptr<ConnectionPolicy>& connPolicyIn) :
        ioc(iocIn), connPolicy(connPolicyIn)
    {}

    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;
    HttpClient(HttpClient&& client) = default;
    HttpClient& operator=(HttpClient&& client) = default;
    ~HttpClient() = default;

    // Send a request to destIP where additional processing of the
    // result is not required
    void sendData(std::string&& data, const boost::urls::url_view_base& destUri,
                  ensuressl::VerifyCertificate verifyCert,
                  const boost::beast::http::fields& httpHeader,
                  const boost::beast::http::verb verb)
    {
        const std::function<void(Response&)> cb = genericResHandler;
        sendDataWithCallback(std::move(data), destUri, verifyCert, httpHeader,
                             verb, cb);
    }

    // Send request to destIP and use the provided callback to
    // handle the response
    void sendDataWithCallback(std::string&& data,
                              const boost::urls::url_view_base& destUrl,
                              ensuressl::VerifyCertificate verifyCert,
                              const boost::beast::http::fields& httpHeader,
                              const boost::beast::http::verb verb,
                              const std::function<void(Response&)>& resHandler)
    {
        std::string_view verify = "ssl_verify";
        if (verifyCert == ensuressl::VerifyCertificate::NoVerify)
        {
            verify = "ssl_no_verify";
        }
        std::string clientKey =
            std::format("{}{}://{}", verify, destUrl.scheme(),
                        destUrl.encoded_host_and_port());
        auto pool = connectionPools.try_emplace(clientKey);
        if (pool.first->second == nullptr)
        {
            pool.first->second = std::make_shared<ConnectionPool>(
                ioc, clientKey, connPolicy, destUrl, verifyCert);
        }
        // Send the data using either the existing connection pool or the
        // newly created connection pool
        pool.first->second->sendData(std::move(data), destUrl, httpHeader, verb,
                                     resHandler);
    }

    // Test whether all connections are terminated (after MaxRetryAttempts)
    bool isTerminated()
    {
        for (const auto& pool : connectionPools)
        {
            if (pool.second != nullptr &&
                !pool.second->areAllConnectionsTerminated())
            {
                BMCWEB_LOG_DEBUG(
                    "Not all of client connections are terminated");
                return false;
            }
        }
        BMCWEB_LOG_DEBUG("All client connections are terminated");
        return true;
    }
};
} // namespace crow
