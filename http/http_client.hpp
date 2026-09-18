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
#include "parsing.hpp"
#include "ssl_key_handler.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <boost/asio/connect.hpp>
#include <boost/asio/connect_pipe.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/readable_pipe.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/host_name_verification.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/ssl/stream_base.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/writable_pipe.hpp>
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
#include <boost/container/devector.hpp>
#include <boost/optional/optional.hpp>
#include <boost/system/errc.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/host_type.hpp>
#include <boost/url/url.hpp>
#include <boost/url/url_view_base.hpp>

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
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
constexpr unsigned int httpReadBufferSize = 4096 * 8;

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
    std::function<void(bool, uint32_t, Response&)> callback;
    PendingRequest(
        boost::beast::http::request<bmcweb::HttpBody>&& reqIn,
        const std::function<void(bool, uint32_t, Response&)>& callbackIn) :
        req(std::move(reqIn)), callback(callbackIn)
    {}
};

namespace http = boost::beast::http;

struct StreamingState
{
    explicit StreamingState(boost::asio::io_context& ioc) :
        readPipe(ioc), writePipe(ioc), chunkStallTimer(ioc),
        streamingDeadline(ioc)
    {}
    boost::asio::readable_pipe readPipe;
    boost::asio::writable_pipe writePipe;
    boost::asio::steady_timer chunkStallTimer;
    boost::asio::steady_timer streamingDeadline;
    std::function<void()> onRelayDone;
    size_t contentLength = 0;
    size_t byteCount = 0;
};

// Returns 0 if Content-Length is absent or unparseable.
inline size_t parseStreamingContentLength(
    const http::response<bmcweb::HttpBody>& response)
{
    auto contentLengthIt =
        response.find(boost::beast::http::field::content_length);
    if (contentLengthIt == response.end())
    {
        BMCWEB_LOG_DEBUG(
            "afterReadHeader: HMC response has no Content-Length header; "
            "will not stream");
        return 0;
    }
    std::string_view val = contentLengthIt->value();
    size_t parsed = 0;
    auto [ptr, parseEc] = std::from_chars(val.begin(), val.end(), parsed);
    if (parseEc != std::errc{})
    {
        BMCWEB_LOG_WARNING(
            "afterReadHeader: failed to parse Content-Length '{}', "
            "will not stream",
            val);
        return 0;
    }
    return parsed;
}

inline bool isJsonResponse(const http::response<bmcweb::HttpBody>& response)
{
    auto contentTypeIt = response.find(boost::beast::http::field::content_type);
    if (contentTypeIt == response.end())
    {
        return false;
    }
    return isJsonContentType(contentTypeIt->value());
}

inline bool shouldStreamResponse(
    const http::response<bmcweb::HttpBody>& response, size_t contentLength,
    bool responseIsInvalid)
{
    if (isJsonResponse(response))
    {
        return false;
    }
    if (responseIsInvalid ||
        response.result() == boost::beast::http::status::no_content ||
        contentLength == 0)
    {
        return false;
    }
    return true;
}

class ConnectionInfo : public std::enable_shared_from_this<ConnectionInfo>
{
  private:
    ConnState state = ConnState::initialized;
    uint32_t retryCount = 0;
    std::string subId;
    std::shared_ptr<ConnectionPolicy> connPolicy;
    boost::urls::url host;
    ensuressl::VerifyCertificate verifyCert;
    uint32_t connId;
    // Data buffers
    http::request<bmcweb::HttpBody> req;
    using parser_type = http::response_parser<bmcweb::HttpBody>;
    std::optional<parser_type> parser;
    boost::beast::flat_static_buffer<httpReadBufferSize> buffer;
    Response res;

    // Async callables
    std::function<void(bool, uint32_t, Response&)> callback;

    // Invoked by ConnectionPool once a streamed response has finished
    // relaying to the client, so the connection can be returned to the
    // pool for reuse.
    std::function<void()> onRelayDone;

    boost::asio::io_context& ioc;

    using Resolver = std::conditional_t<BMCWEB_DNS_RESOLVER == "systemd-dbus",
                                        async_resolve::Resolver,
                                        boost::asio::ip::tcp::resolver>;
    Resolver resolver;

    boost::asio::ip::tcp::socket conn;
    std::optional<boost::asio::ssl::stream<boost::asio::ip::tcp::socket&>>
        sslConn;

    boost::asio::steady_timer timer;

    // Set only during active streaming.
    static constexpr std::chrono::minutes streamingDeadlineDuration{15};
    std::optional<StreamingState> streaming;

    friend class ConnectionPool;

  public:
    bool isBusyStreaming() const
    {
        return streaming.has_value();
    }

  private:
    void doResolve()
    {
        state = ConnState::resolveInProgress;
        BMCWEB_LOG_DEBUG("Trying to resolve: {}, id: {}", host, connId);
        boost::urls::host_type hostType = host.host_type();
        if (hostType == boost::urls::host_type::name)
        {
            resolver.async_resolve(
                host.encoded_host_address(), host.port(),
                std::bind_front(&ConnectionInfo::afterResolve, this,
                                shared_from_this()));

            return;
        }
        // If we already have an ip address, no need to resolve
        boost::system::error_code ec;
        boost::asio::ip::address addr =
            boost::asio::ip::make_address(host.host_address(), ec);

        if (ec)
        {
            BMCWEB_LOG_ERROR(
                "Failed to parse already-parsed ip address.  This should not happen {}",
                host.host_address());
            return;
        }
        boost::asio::ip::tcp::endpoint end(addr, host.port_number());
        Resolver::results_type ip = Resolver::results_type::create(
            end, host.host_address(), host.port());
        afterResolve(shared_from_this(), boost::system::error_code(), ip);
    }

    void afterResolve(const std::shared_ptr<ConnectionInfo>& /*self*/,
                      const boost::system::error_code& ec,
                      const Resolver::results_type& endpointList)
    {
        if (ec || (endpointList.empty()))
        {
            BMCWEB_LOG_ERROR("Resolve failed: {} {}", ec.message(), host);
            state = ConnState::resolveFailed;
            waitAndRetry();
            return;
        }
        BMCWEB_LOG_DEBUG("Resolved {}, id: {}", host, connId);
        state = ConnState::connectInProgress;

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
            state = ConnState::connectFailed;
            waitAndRetry();
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
        state = ConnState::connected;
        sendMessage();
    }

    void doSslHandshake()
    {
        if (!sslConn)
        {
            return;
        }
        auto& ssl = *sslConn;
        state = ConnState::handshakeInProgress;
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
            state = ConnState::handshakeFailed;
            waitAndRetry();
            return;
        }
        BMCWEB_LOG_DEBUG("SSL Handshake successful - id: {}", connId);
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
            BMCWEB_LOG_ERROR("sendMessage() failed: {} {}", ec.message(), host);
            state = ConnState::sendFailed;
            waitAndRetry();
            return;
        }
        BMCWEB_LOG_DEBUG("sendMessage() bytes transferred: {}",
                         bytesTransferred);

        recvMessage();
    }

    void recvMessage()
    {
        state = ConnState::recvInProgress;

        parser_type& thisParser = parser.emplace();
        // Header-first receive: the body limit is widened here so the
        // header can always be read, then narrowed in readJsonBody() for
        // the buffered-JSON path once we know it isn't a large streamed
        // binary download.
        thisParser.body_limit(std::numeric_limits<std::uint64_t>::max());
        thisParser.get().body().setStreamingReceiver(true);

        timer.expires_after(std::chrono::seconds(30));
        timer.async_wait(std::bind_front(onTimeout, weak_from_this()));

        // Receive the HTTP response headers first; the body is either read
        // in full (readJsonBody) or streamed chunk-by-chunk through a pipe
        // (startStreamingResponse), decided in afterReadHeader().
        if (sslConn)
        {
            boost::beast::http::async_read_header(
                *sslConn, buffer, thisParser,
                std::bind_front(&ConnectionInfo::afterReadHeader, this,
                                shared_from_this()));
        }
        else
        {
            boost::beast::http::async_read_header(
                conn, buffer, thisParser,
                std::bind_front(&ConnectionInfo::afterReadHeader, this,
                                shared_from_this()));
        }
    }

    void handleReadHeaderError(const boost::beast::error_code& ec)
    {
        if (ec == boost::asio::error::operation_aborted ||
            ec == boost::system::errc::operation_canceled)
        {
            return;
        }
        BMCWEB_LOG_ERROR("afterReadHeader error: {} {}", ec, ec.message());
        timer.cancel();
        state = ConnState::recvFailed;
        waitAndRetry();
    }

    void readJsonBody()
    {
        if (!parser)
        {
            BMCWEB_LOG_ERROR("readJsonBody: parser not initialised");
            return;
        }
        // Tighten body limit now that we know it's JSON.
        parser->body_limit(connPolicy->requestByteLimit);
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

    void startStreamingResponse(
        const http::response<bmcweb::HttpBody>& response, size_t contentLength)
    {
        if (!parser)
        {
            BMCWEB_LOG_ERROR("startStreamingResponse: parser not initialised");
            return;
        }
        // Streaming uses per-chunk timeouts; cancel the read-header recv
        // timeout.
        timer.cancel();
        res.response = response;
        if (!createStreamPipe())
        {
            if (callback)
            {
                callback(false, connId, res);
            }
            shutdownConn(false);
            return;
        }
        if (!streaming)
        {
            return;
        }
        streaming->contentLength = contentLength;
        streaming->byteCount = 0;
        if (!openStreamFdAndStart())
        {
            // openStreamFdAndStart already fired callback(false) +
            // shutdownConn.
            return;
        }
        resetChunkStallTimer();
        startStreamingDeadline();
        scheduleStreamBodyRead();
    }

    void afterReadHeader(const std::shared_ptr<ConnectionInfo>& /*self*/,
                         const boost::beast::error_code& ec,
                         const std::size_t& bytesTransferred)
    {
        if (ec)
        {
            handleReadHeaderError(ec);
            return;
        }
        BMCWEB_LOG_DEBUG("afterReadHeader() bytes transferred: {}",
                         bytesTransferred);
        if (!parser)
        {
            BMCWEB_LOG_ERROR("afterReadHeader: parser not initialised");
            return;
        }
        const auto& response = parser->get();
        size_t contentLength = parseStreamingContentLength(response);
        auto ctIt = response.find(boost::beast::http::field::content_type);
        BMCWEB_LOG_DEBUG("afterReadHeader() content_length={} type={}",
                         contentLength,
                         ctIt != response.end() ? ctIt->value() : "(none)");
        // Route to the buffered-JSON path or the streamed-pipe path.
        if (!shouldStreamResponse(response, contentLength,
                                  static_cast<bool>(connPolicy->invalidResp(
                                      response.result_int()))))
        {
            readJsonBody();
            return;
        }
        startStreamingResponse(response, contentLength);
    }

    bool createStreamPipe()
    {
        streaming.emplace(ioc);
        streaming->onRelayDone = onRelayDone;
        boost::system::error_code pipeEc{};
        boost::asio::connect_pipe(streaming->readPipe, streaming->writePipe,
                                  pipeEc);
        if (pipeEc)
        {
            BMCWEB_LOG_ERROR("createStreamPipe: pipe create failed: {}",
                             pipeEc.message());
            streaming.reset();
            return false;
        }
        // Expand the pipe buffer to reduce write stalls under backpressure.
        constexpr int pipeBufferSize = 1 * 1024 * 1024;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
        if (::fcntl(streaming->writePipe.native_handle(), F_SETPIPE_SZ,
                    pipeBufferSize) < 0)
        {
            BMCWEB_LOG_WARNING(
                "createStreamPipe: F_SETPIPE_SZ failed, using default pipe size");
        }
        return true;
    }

    bool openStreamFdAndStart()
    {
        if (!streaming)
        {
            BMCWEB_LOG_ERROR("openStreamFdAndStart: readPipe not initialised");
            streaming.reset();
            if (callback)
            {
                callback(false, connId, res);
            }
            shutdownConn(false);
            return false;
        }
        DuplicatableFileHandle dupHandle(
            ::dup(streaming->readPipe.native_handle()));
        if (!dupHandle.fileHandle.is_open())
        {
            BMCWEB_LOG_ERROR("openStreamFdAndStart: dup() failed: {}",
                             std::generic_category().message(errno));
            streaming.reset();
            if (callback)
            {
                callback(false, connId, res);
            }
            shutdownConn(false);
            return false;
        }
        // Close the original read-end now that the dup'd FD is handed to the
        // connection handler.  This makes the dup'd FD the sole reader: when
        // the downstream session closes it (client disconnect / stream
        // cancel), the kernel delivers EPIPE to writePipe immediately instead
        // of waiting for the 1 MB pipe buffer to fill up.
        boost::system::error_code closeEc;
        streaming->readPipe.close(closeEc);
        if (closeEc)
        {
            BMCWEB_LOG_WARNING(
                "openStreamFdAndStart: readPipe close failed: {}",
                closeEc.message());
        }
        res.openFd(std::move(dupHandle), bmcweb::EncodingType::Raw);
        // fstat on a pipe returns 0; set fileSize from the Content-Length
        // header so Beast emits Content-Length instead of chunked encoding.
        if (streaming->contentLength > 0)
        {
            res.response.body().setFileSize(streaming->contentLength);
        }
        if (callback)
        {
            callback(true, connId, res);
        }
        res.clear();
        return true;
    }

    void resetChunkStallTimer()
    {
        if (!streaming)
        {
            return;
        }
        streaming->chunkStallTimer.cancel();
        streaming->chunkStallTimer.expires_after(std::chrono::seconds(120));
        streaming->chunkStallTimer.async_wait(
            std::bind_front(onChunkStallTimeout, weak_from_this()));
    }

    void startStreamingDeadline()
    {
        if (!streaming)
        {
            return;
        }
        streaming->streamingDeadline.cancel();
        streaming->streamingDeadline.expires_after(streamingDeadlineDuration);
        streaming->streamingDeadline.async_wait(
            std::bind_front(onStreamingDeadlineFired, weak_from_this()));
    }

    [[nodiscard]] static std::shared_ptr<std::string> drainParserBodyChunk(
        parser_type& msgParser)
    {
        auto chunk = std::make_shared<std::string>(
            std::move(msgParser.get().body().str()));
        msgParser.get().body().str().clear();
        return chunk;
    }

    void scheduleStreamBodyRead()
    {
        if (!streaming || !parser)
        {
            return;
        }
        // Drain any bytes already in the read buffer.
        if (!drainPrefetchedStreamBuffer())
        {
            scheduleStreamBodyRawRead();
        }
    }

    void scheduleStreamBodyRawRead()
    {
        if (!streaming || !parser)
        {
            return;
        }
        size_t remaining{streaming->contentLength - streaming->byteCount};
        size_t readLimit{
            std::min(buffer.max_size() - buffer.size(), remaining)};
        if (readLimit == 0)
        {
            // Buffer is full; yield so the write side can drain it.
            boost::asio::post(
                sslConn ? sslConn->get_executor() : conn.get_executor(),
                std::bind_front(&ConnectionInfo::scheduleStreamBodyRead,
                                shared_from_this()));
            return;
        }
        if (sslConn)
        {
            sslConn->async_read_some(
                buffer.prepare(readLimit),
                std::bind_front(&ConnectionInfo::afterStreamBodyRawRead, this,
                                shared_from_this()));
        }
        else
        {
            conn.async_read_some(
                buffer.prepare(readLimit),
                std::bind_front(&ConnectionInfo::afterStreamBodyRawRead, this,
                                shared_from_this()));
        }
    }

    bool drainPrefetchedStreamBuffer()
    {
        if (!parser || buffer.size() == 0)
        {
            return false;
        }
        boost::beast::error_code ec{};
        size_t consumed{parser.value().put(buffer.data(), ec)};
        buffer.consume(consumed);
        if (consumed == 0 && !ec && !parser.value().is_done())
        {
            return false;
        }
        // Post to break synchronous recursion when the full body is already
        // buffered.
        boost::asio::post(
            sslConn ? sslConn->get_executor() : conn.get_executor(),
            [self = shared_from_this(), ec, consumed]() {
                self->afterStreamBodyRead(self, ec, consumed);
            });
        return true;
    }

    void afterStreamBodyRawRead(const std::shared_ptr<ConnectionInfo>& self,
                                boost::beast::error_code ec, size_t bytesRead)
    {
        buffer.commit(bytesRead);
        if (!parser)
        {
            return;
        }
        boost::beast::error_code parseEc{};
        size_t consumed{parser.value().put(buffer.data(), parseEc)};
        buffer.consume(consumed);
        afterStreamBodyRead(self, parseEc.failed() ? parseEc : ec, bytesRead);
    }

    void afterStreamBodyRead(const std::shared_ptr<ConnectionInfo>& /*self*/,
                             const boost::beast::error_code& ec,
                             const std::size_t& bytesTransferred)
    {
        if (ec == boost::asio::error::operation_aborted ||
            ec == boost::system::errc::operation_canceled)
        {
            return;
        }
        if (!streaming || !parser)
        {
            return;
        }
        auto& parserRef = *parser;
        if (ec && ec != boost::asio::error::eof)
        {
            BMCWEB_LOG_ERROR("afterStreamBodyRead upstream error: {} {}", ec,
                             ec.message());
            // Close write-end without flushing so the client receives a
            // short EOF it can detect, rather than silently truncated data.
            auto cb = std::move(streaming->onRelayDone);
            streaming.reset();
            if (cb)
            {
                cb();
            }
            return;
        }
        bool hadEof = (ec == boost::asio::error::eof);
        BMCWEB_LOG_DEBUG(
            "afterStreamBodyRead bytesTransferred={} done={} eof={}",
            bytesTransferred, parserRef.is_done(), hadEof);
        if (!hadEof)
        {
            resetChunkStallTimer();
        }
        auto chunk = drainParserBodyChunk(parserRef);
        bool done = parserRef.is_done();
        if (chunk->empty())
        {
            handleEmptyChunk(done, hadEof);
            return;
        }
        writeChunkToPipe(chunk, done, hadEof);
    }

    void handleEmptyChunk(bool done, bool hadEof)
    {
        if (!streaming)
        {
            return;
        }
        if (done || hadEof)
        {
            if (hadEof && !done)
            {
                BMCWEB_LOG_ERROR(
                    "afterStreamBodyRead: remote server closed connection "
                    "before transfer complete (EOF mid-stream), closing pipe");
            }
            auto cb = std::move(streaming->onRelayDone);
            streaming.reset();
            if (cb)
            {
                cb();
            }
        }
        else
        {
            scheduleStreamBodyRead();
        }
    }

    void writeChunkToPipe(const std::shared_ptr<std::string>& chunk, bool done,
                          bool hadEof)
    {
        if (!streaming)
        {
            BMCWEB_LOG_ERROR("writeChunkToPipe: writePipe not initialised");
            return;
        }
        streaming->byteCount += chunk->size();
        boost::asio::async_write(
            streaming->writePipe, boost::asio::buffer(*chunk),
            std::bind_front(&ConnectionInfo::afterChunkWrite, this,
                            shared_from_this(), chunk, done, hadEof));
    }

    void afterChunkWrite(const std::shared_ptr<ConnectionInfo>& /*self*/,
                         const std::shared_ptr<std::string>& /*chunk*/,
                         bool done, bool hadEof,
                         boost::system::error_code writeEc,
                         size_t /*bytesWritten*/)
    {
        if (!streaming)
        {
            return;
        }
        if (writeEc)
        {
            // operation_canceled/operation_aborted means the stall timer
            // already fired and closed the pipe; don't double-close.
            if (writeEc == boost::system::errc::operation_canceled ||
                writeEc == boost::asio::error::operation_aborted)
            {
                return;
            }
            if (writeEc == boost::system::errc::broken_pipe)
            {
                BMCWEB_LOG_ERROR(
                    "afterChunkWrite: client disconnected mid-download "
                    "(EPIPE after {} bytes); closing HMC relay",
                    streaming->byteCount);
            }
            else
            {
                BMCWEB_LOG_ERROR("afterStreamBodyRead pipe write error: {}",
                                 writeEc);
            }
            auto cb = std::move(streaming->onRelayDone);
            streaming.reset();
            if (cb)
            {
                cb();
            }
            return;
        }
        if (done || hadEof)
        {
            if (done && streaming->contentLength > 0 &&
                streaming->byteCount != streaming->contentLength)
            {
                BMCWEB_LOG_ERROR(
                    "afterStreamBodyRead: pipe byte count mismatch: "
                    "wrote {} bytes, expected {}",
                    streaming->byteCount, streaming->contentLength);
            }
            if (hadEof && !done)
            {
                BMCWEB_LOG_ERROR(
                    "afterStreamBodyRead: upstream disconnected "
                    "mid-transfer (EOF), closing pipe after flushing "
                    "last chunk");
            }
            auto cb = std::move(streaming->onRelayDone);
            streaming.reset();
            if (cb)
            {
                cb();
            }
        }
        else
        {
            scheduleStreamBodyRead();
        }
    }

    static void onChunkStallTimeout(
        const std::weak_ptr<ConnectionInfo>& weakSelf,
        const boost::system::error_code& ec)
    {
        if (ec == boost::asio::error::operation_aborted)
        {
            return;
        }
        if (ec)
        {
            BMCWEB_LOG_ERROR("chunk-stall async_wait failed: {}", ec.message());
        }
        std::shared_ptr<ConnectionInfo> self = weakSelf.lock();
        if (self == nullptr)
        {
            return;
        }
        if (!self->streaming.has_value())
        {
            return;
        }
        BMCWEB_LOG_ERROR("Streaming stall: 120 s without data from upstream {}",
                         self->host);
        auto cb = std::move(self->streaming->onRelayDone);
        self->streaming.reset();
        if (cb)
        {
            cb();
        }
    }

    static void onStreamingDeadlineFired(
        const std::weak_ptr<ConnectionInfo>& weakSelf,
        const boost::system::error_code& ec)
    {
        if (ec == boost::asio::error::operation_aborted)
        {
            return;
        }
        std::shared_ptr<ConnectionInfo> self = weakSelf.lock();
        if (self == nullptr)
        {
            return;
        }
        if (!self->streaming.has_value())
        {
            return;
        }
        BMCWEB_LOG_ERROR(
            "Streaming deadline ({} min) exceeded for {}, closing pipe",
            streamingDeadlineDuration.count(), self->host);
        auto cb = std::move(self->streaming->onRelayDone);
        self->streaming.reset();
        if (cb)
        {
            cb();
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
            state = ConnState::recvFailed;
            waitAndRetry();
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
            state = ConnState::recvFailed;
            waitAndRetry();
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
            state = ConnState::recvFailed;
            waitAndRetry();
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
        if (callback)
        {
            callback(parser->keep_alive(), connId, res);
        }
        res.clear();
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
        self->waitAndRetry();
    }

    void waitAndRetry()
    {
        if ((retryCount >= connPolicy->maxRetryAttempts) ||
            (state == ConnState::sslInitFailed))
        {
            BMCWEB_LOG_ERROR("Maximum number of retries reached. {}", host);
            BMCWEB_LOG_DEBUG("Retry policy: {}", connPolicy->retryPolicyAction);

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
            if (callback)
            {
                callback(false, connId, res);
            }
            res.clear();

            // Reset the retrycount to zero so that client can try
            // connecting again if needed
            retryCount = 0;
            return;
        }

        retryCount++;

        BMCWEB_LOG_DEBUG("Attempt retry after {} seconds. RetryCount = {}",
                         connPolicy->retryIntervalSecs.count(), retryCount);
        timer.expires_after(connPolicy->retryIntervalSecs);
        timer.async_wait(std::bind_front(&ConnectionInfo::onTimerDone, this,
                                         shared_from_this()));
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
        shutdownConn(true);
    }

    void restartConnection()
    {
        BMCWEB_LOG_DEBUG("{}, id: {}  restartConnection", host,
                         std::to_string(connId));
        initializeConnection(host.scheme() == "https");
        doResolve();
    }

    void shutdownConn(bool retry)
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

        if (retry)
        {
            // Now let's try to resend the data
            state = ConnState::retry;
            restartConnection();
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
            BMCWEB_LOG_ERROR("{}, id: {} shutdown failed: {}", host, connId,
                             ec.message());
        }
        else
        {
            BMCWEB_LOG_DEBUG("{}, id: {} closed gracefully", host, connId);
        }
        shutdownConn(retry);
    }

    void setCipherSuiteTLSext()
    {
        if (!sslConn)
        {
            return;
        }

        std::string hostname(host.encoded_host_address());

        // SNI is only defined for DNS names (RFC 6066)
        if (host.host_type() == boost::urls::host_type::name)
        {
            if (SSL_set_tlsext_host_name(sslConn->native_handle(),
                                         hostname.data()) == 0)
            {
                boost::beast::error_code ec{
                    static_cast<int>(::ERR_get_error()),
                    boost::asio::error::get_ssl_category()};

                BMCWEB_LOG_ERROR(
                    "SSL_set_tlsext_host_name {}, id: {} failed: {}", host,
                    connId, ec.message());
                state = ConnState::sslInitFailed;
                waitAndRetry();
                return;
            }
        }

        // Verify the peer certificate matches the destination hostname.
        // Without this, verify_peer only checks the chain up to a
        // trusted CA but not whether the cert was issued for this host,
        // allowing MITM with any CA-signed certificate.
        if (verifyCert != ensuressl::VerifyCertificate::NoVerify)
        {
            boost::system::error_code ec;
            sslConn->set_verify_callback(
                boost::asio::ssl::host_name_verification(hostname), ec);
            if (ec)
            {
                BMCWEB_LOG_ERROR("set_verify_callback {}, id: {} failed: {}",
                                 host, connId, ec.message());
                state = ConnState::sslInitFailed;
                waitAndRetry();
                return;
            }
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
                state = ConnState::sslInitFailed;
                waitAndRetry();
                return;
            }
            sslConn.emplace(conn, *sslCtx);
            setCipherSuiteTLSext();
        }
    }

  public:
    explicit ConnectionInfo(
        boost::asio::io_context& iocIn, const std::string& idIn,
        const std::shared_ptr<ConnectionPolicy>& connPolicyIn,
        const boost::urls::url_view_base& hostIn,
        ensuressl::VerifyCertificate verifyCertIn, unsigned int connIdIn) :
        subId(idIn), connPolicy(connPolicyIn), host(hostIn),
        verifyCert(verifyCertIn), connId(connIdIn), ioc(iocIn), resolver(iocIn),
        conn(iocIn), timer(iocIn)
    {
        initializeConnection(host.scheme() == "https");
    }
};

class ConnectionPool : public std::enable_shared_from_this<ConnectionPool>
{
  private:
    boost::asio::io_context& ioc;
    std::string id;
    std::shared_ptr<ConnectionPolicy> connPolicy;
    boost::urls::url destIP;
    std::vector<std::shared_ptr<ConnectionInfo>> connections;
    boost::container::devector<PendingRequest> requestQueue;
    ensuressl::VerifyCertificate verifyCert;

    friend class HttpClient;

    // Configure a connections's request, callback, and retry info in
    // preparation to begin sending the request
    void setConnProps(ConnectionInfo& conn)
    {
        if (requestQueue.empty())
        {
            BMCWEB_LOG_ERROR(
                "setConnProps() should not have been called when requestQueue is empty");
            return;
        }

        PendingRequest& nextReq = requestQueue.front();
        conn.req = std::move(nextReq.req);
        conn.callback = std::move(nextReq.callback);

        BMCWEB_LOG_DEBUG("Setting properties for connection {}, id: {}",
                         conn.host, conn.connId);

        // We can remove the request from the queue at this point
        requestQueue.pop_front();
    }

    // Gets called as part of callback after request is sent
    // Reuses the connection if there are any requests waiting to be sent
    // Otherwise closes the connection if it is not a keep-alive
    void sendNext(bool keepAlive, uint32_t connId)
    {
        if (connId >= connections.size())
        {
            BMCWEB_LOG_ERROR("Invalid connId: {} (size: {})", connId,
                             connections.size());
            return;
        }

        auto conn = connections[connId];
        if (!conn)
        {
            BMCWEB_LOG_ERROR("Connection at index {} is null", connId);
            return;
        }

        // Allow the connection's handler to be deleted
        // This is needed because of Redfish Aggregation passing an
        // AsyncResponse shared_ptr to this callback
        conn->callback = nullptr;

        // Body still streaming; sendNext deferred to onRelayDone.
        if (conn->isBusyStreaming())
        {
            BMCWEB_LOG_DEBUG(
                "sendNext() deferring idle for conn {} - streaming active",
                connId);
            return;
        }

        // Reuse the connection to send the next request in the queue
        if (!requestQueue.empty())
        {
            BMCWEB_LOG_DEBUG(
                "{} requests remaining in queue for {}, reusing connection {}",
                requestQueue.size(), destIP, connId);

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
                conn->restartConnection();
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
                std::string commonMsg = std::format("{} from pool {}", i, id);

                if (conn->state == ConnState::idle)
                {
                    BMCWEB_LOG_DEBUG("Grabbing idle connection {}", commonMsg);
                    conn->sendMessage();
                }
                else
                {
                    BMCWEB_LOG_DEBUG("Reusing existing connection {}",
                                     commonMsg);
                    conn->restartConnection();
                }
                return;
            }
        }

        // All connections in use so create a new connection or add request
        // to the queue
        if (connections.size() < connPolicy->maxConnections)
        {
            BMCWEB_LOG_DEBUG("Adding new connection to pool {}", id);
            auto conn = addConnection();
            conn->req = std::move(thisReq);
            conn->callback = std::move(cb);
            conn->doResolve();
        }
        else if (requestQueue.size() < maxRequestQueueSize)
        {
            BMCWEB_LOG_DEBUG("Max pool size reached. Adding data to queue {}",
                             id);
            requestQueue.emplace_back(std::move(thisReq), std::move(cb));
        }
        else
        {
            // If we can't buffer the request then we should let the
            // callback handle a 429 Too Many Requests dummy response
            BMCWEB_LOG_ERROR("{} request queue full.  Dropping request.", id);
            Response dummyRes;
            dummyRes.result(boost::beast::http::status::too_many_requests);
            resHandler(dummyRes);
        }
    }

    // Callback to be called once the request has been sent
    static void afterSendData(const std::weak_ptr<ConnectionPool>& weakSelf,
                              const std::function<void(Response&)>& resHandler,
                              bool keepAlive, uint32_t connId, Response& res)
    {
        // If requests remain in the queue then we want to reuse this
        // connection to send the next request
        std::shared_ptr<ConnectionPool> self = weakSelf.lock();
        if (!self)
        {
            BMCWEB_LOG_CRITICAL("{} Failed to capture connection",
                                logPtr(self.get()));
            return;
        }

        // Allow provided callback to perform additional processing of the
        // request
        resHandler(res);

        self->sendNext(keepAlive, connId);
    }

    std::shared_ptr<ConnectionInfo>& addConnection()
    {
        unsigned int newId = static_cast<unsigned int>(connections.size());

        auto& ret = connections.emplace_back(std::make_shared<ConnectionInfo>(
            ioc, id, connPolicy, destIP, verifyCert, newId));

        ret->onRelayDone = [weakPool = weak_from_this(), newId]() {
            if (auto pool = weakPool.lock())
            {
                pool->sendNext(false, newId);
            }
        };

        BMCWEB_LOG_DEBUG("Added connection {} to pool {}",
                         connections.size() - 1, id);

        return ret;
    }

  public:
    explicit ConnectionPool(
        boost::asio::io_context& iocIn, const std::string& idIn,
        const std::shared_ptr<ConnectionPolicy>& connPolicyIn,
        const boost::urls::url_view_base& destIPIn,
        ensuressl::VerifyCertificate verifyCertIn) :
        ioc(iocIn), id(idIn), connPolicy(connPolicyIn), destIP(destIPIn),
        verifyCert(verifyCertIn)
    {
        BMCWEB_LOG_DEBUG("Initializing connection pool for {}", id);

        // Initialize the pool with a single connection
        addConnection();
    }

    // Check whether all connections are terminated
    bool areAllConnectionsTerminated()
    {
        if (connections.empty())
        {
            BMCWEB_LOG_DEBUG("There are no connections for pool id:{}", id);
            return false;
        }
        for (const auto& conn : connections)
        {
            if (conn != nullptr && conn->state != ConnState::terminated)
            {
                BMCWEB_LOG_DEBUG(
                    "Not all connections of pool id:{} are terminated", id);
                return false;
            }
        }
        BMCWEB_LOG_INFO("All connections of pool id:{} are terminated", id);
        return true;
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
            verify = "ssl no verify";
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
