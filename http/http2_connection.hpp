// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include "bmcweb_config.h"

#include "async_resp.hpp"
#include "authentication.hpp"
#include "complete_response_fields.hpp"
#include "forward_unauthorized.hpp"
#include "http_body.hpp"
#include "http_connect_types.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "logging.hpp"

// NOLINTNEXTLINE(misc-include-cleaner)
#include "nghttp2_adapters.hpp"
#include "sessions.hpp"

#include <nghttp2/nghttp2.h>
#include <unistd.h>

#include <boost/asio/buffer.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/optional/optional.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/url_view.hpp>

#include <array>
#include <bit>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace crow
{

// request body limit size set by the BMCWEB_HTTP_BODY_LIMIT option
constexpr uint64_t httpReqBodyLimit = 1024UL * 1024UL * BMCWEB_HTTP_BODY_LIMIT;

constexpr uint64_t loggedOutPostBodyLimit = 4096U;

// Total number of simultaneous connections, shared between HTTP/1.1
// Connection (http_connection.hpp) and HTTP2Connection. A function-local
// static (rather than a namespace-scope variable) guarantees a single
// instance even though this header is included by multiple translation
// units.
inline int& getConnectionCount()
{
    static int count = 0;
    return count;
}

enum class DeadlineTimerType
{
    Default,
    Keepalive,
};

struct Http2StreamData
{
    std::shared_ptr<Request> req = std::make_shared<Request>();
    std::optional<bmcweb::HttpBody::reader> reqReader;
    // Set when the body exceeds bodyLimit or the reader rejects it.
    bool bodyRejected = false;
    // Running total of DATA bytes received on this stream, checked against
    // bodyLimit since Content-Length can be missing or under-declared.
    uint64_t bodyBytesReceived = 0;
    uint64_t bodyLimit = httpReqBodyLimit;
    std::string accept;
    std::string acceptEnc;
    boost::optional<uint64_t> contentLength;
    Response res;
    std::optional<bmcweb::HttpBody::writer> writer;
};

template <typename Adaptor, typename Handler>
class HTTP2Connection :
    public std::enable_shared_from_this<HTTP2Connection<Adaptor, Handler>>
{
    using self_type = HTTP2Connection<Adaptor, Handler>;

  public:
    HTTP2Connection(
        boost::asio::ssl::stream<Adaptor>&& adaptorIn, Handler* handlerIn,
        std::function<std::string()>& getCachedDateStrF, HttpType httpTypeIn,
        const std::shared_ptr<persistent_data::UserSession>& mtlsSessionIn,
        boost::asio::ip::address ipIn) :
        httpType(httpTypeIn), adaptor(std::move(adaptorIn)),
        ngSession(initializeNghttp2Session()), handler(handlerIn),
        getCachedDateStr(getCachedDateStrF), mtlsSession(mtlsSessionIn),
        ip(std::move(ipIn)), timer(adaptor.get_executor())
    {
        getConnectionCount()++;
    }

    ~HTTP2Connection()
    {
        cancelDeadlineTimer();
        getConnectionCount()--;
    }

    HTTP2Connection(const HTTP2Connection&) = delete;
    HTTP2Connection(HTTP2Connection&&) = delete;
    HTTP2Connection& operator=(const HTTP2Connection&) = delete;
    HTTP2Connection& operator=(HTTP2Connection&&) = delete;

    void start()
    {
        // Create the control stream
        streams[0];

        if (sendServerConnectionHeader() != 0)
        {
            BMCWEB_LOG_ERROR("send_server_connection_header failed");
            return;
        }
        startDeadline(DeadlineTimerType::Keepalive);
        doRead();
    }

    void startFromSettings(std::string_view http2UpgradeSettings)
    {
        int ret = ngSession.sessionUpgrade2(http2UpgradeSettings,
                                            false /*head_request*/);
        if (ret != 0)
        {
            BMCWEB_LOG_ERROR("Failed to load upgrade header");
            return;
        }
        // Create the control stream
        streams[0];

        if (sendServerConnectionHeader() != 0)
        {
            BMCWEB_LOG_ERROR("send_server_connection_header failed");
            return;
        }
        startDeadline(DeadlineTimerType::Keepalive);
        doRead();
    }

    int sendServerConnectionHeader()
    {
        BMCWEB_LOG_DEBUG("send_server_connection_header()");

        uint32_t maxStreams = 4;

        // Both of these settings were found experimentally to allow a single
        // fast stream to upload at a rate equivalent to http1.1  They will
        // likely be tuned in the future.
        uint32_t maxFrameSize = 1 << 14;
        uint32_t windowSize = 1 << 20;
        std::array<nghttp2_settings_entry, 4> iv = {{
            {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, maxStreams},
            {NGHTTP2_SETTINGS_ENABLE_PUSH, 0},
            // Set an approximately 1MB window size
            {NGHTTP2_SETTINGS_INITIAL_WINDOW_SIZE, windowSize},
            {NGHTTP2_SETTINGS_MAX_FRAME_SIZE, maxFrameSize},
        }};
        if (ngSession.setLocalWindowSize(NGHTTP2_FLAG_NONE, 0, 1 << 20) != 0)
        {
            BMCWEB_LOG_ERROR("Failed to set local window size");
        }
        int rv = ngSession.submitSettings(iv);
        if (rv != 0)
        {
            BMCWEB_LOG_ERROR("Fatal error: {}", nghttp2_strerror(rv));
            return -1;
        }
        writeBuffer();
        return 0;
    }

    static ssize_t fileReadCallback(
        nghttp2_session* /* session */, int32_t streamId, uint8_t* buf,
        size_t length, uint32_t* dataFlags, nghttp2_data_source* /*source*/,
        void* userPtr)
    {
        self_type& self = userPtrToSelf(userPtr);

        auto streamIt = self.streams.find(streamId);
        if (streamIt == self.streams.end())
        {
            return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
        }
        Http2StreamData& stream = streamIt->second;
        BMCWEB_LOG_DEBUG("File read callback length: {}", length);
        if (!stream.writer)
        {
            return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
        }
        boost::beast::error_code ec;
        boost::optional<std::pair<boost::asio::const_buffer, bool>> out =
            stream.writer->getWithMaxSize(ec, length);
        if (ec)
        {
            BMCWEB_LOG_CRITICAL("Failed to get buffer");
            return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
        }
        if (!out)
        {
            BMCWEB_LOG_ERROR("Empty file, setting EOF");
            *dataFlags |= NGHTTP2_DATA_FLAG_EOF;
            return 0;
        }

        BMCWEB_LOG_DEBUG("Send chunk of size: {}", out->first.size());
        if (length < out->first.size())
        {
            BMCWEB_LOG_CRITICAL(
                "Buffer overflow that should never happen happened");
            // Should never happen because of length limit on get() above
            return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
        }
        boost::asio::mutable_buffer writeableBuf(buf, length);
        BMCWEB_LOG_DEBUG("Copying {} bytes to buf", out->first.size());
        size_t copied = boost::asio::buffer_copy(writeableBuf, out->first);
        if (copied != out->first.size())
        {
            BMCWEB_LOG_ERROR(
                "Couldn't copy all {} bytes into buffer, only copied {}",
                out->first.size(), copied);
            return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
        }

        if (!out->second)
        {
            BMCWEB_LOG_DEBUG("Setting EOF flag");
            *dataFlags |= NGHTTP2_DATA_FLAG_EOF;
        }
        return static_cast<ssize_t>(copied);
    }

    nghttp2_nv headerFromStringViews(std::string_view name,
                                     std::string_view value, uint8_t flags)
    {
        uint8_t* nameData = std::bit_cast<uint8_t*>(name.data());
        uint8_t* valueData = std::bit_cast<uint8_t*>(value.data());
        return {nameData, valueData, name.size(), value.size(), flags};
    }

    int sendResponse(Response& completedRes, int32_t streamId)
    {
        BMCWEB_LOG_DEBUG("send_response stream_id:{}", streamId);

        auto it = streams.find(streamId);
        if (it == streams.end())
        {
            close();
            return -1;
        }
        Http2StreamData& stream = it->second;
        Response& res = stream.res;
        res = std::move(completedRes);

        completeResponseFields(stream.accept, stream.acceptEnc, res);
        res.addHeader(boost::beast::http::field::date, getCachedDateStr());
        boost::urls::url_view urlView;
        if (stream.req != nullptr)
        {
            urlView = stream.req->url();
        }
        res.preparePayload(urlView);

        boost::beast::http::fields& fields = res.fields();
        std::string code = std::to_string(res.resultInt());
        std::vector<nghttp2_nv> hdr;
        hdr.emplace_back(
            headerFromStringViews(":status", code, NGHTTP2_NV_FLAG_NONE));
        for (const boost::beast::http::fields::value_type& header : fields)
        {
            hdr.emplace_back(headerFromStringViews(
                header.name_string(), header.value(), NGHTTP2_NV_FLAG_NONE));
        }
        http::response<bmcweb::HttpBody>& fbody = res.response;
        stream.writer.emplace(fbody.base(), fbody.body());

        nghttp2_data_provider dataPrd{
            .source = {.fd = 0},
            .read_callback = fileReadCallback,
        };

        int rv = ngSession.submitResponse(streamId, hdr, &dataPrd);
        if (rv != 0)
        {
            BMCWEB_LOG_ERROR("Fatal error: {}", nghttp2_strerror(rv));
            close();
            return -1;
        }
        writeBuffer();

        return 0;
    }

    nghttp2_session initializeNghttp2Session()
    {
        nghttp2_session_callbacks callbacks;
        callbacks.setOnFrameRecvCallback(onFrameRecvCallbackStatic);
        callbacks.setOnStreamCloseCallback(onStreamCloseCallbackStatic);
        callbacks.setOnHeaderCallback(onHeaderCallbackStatic);
        callbacks.setOnBeginHeadersCallback(onBeginHeadersCallbackStatic);
        callbacks.setOnDataChunkRecvCallback(onDataChunkRecvStatic);

        nghttp2_session session(callbacks);
        session.setUserData(this);

        return session;
    }

    static void afterCompleteRequest(const std::weak_ptr<self_type>& weakSelf,
                                     int32_t streamId, Response& completeRes)
    {
        BMCWEB_LOG_DEBUG("res.completeRequestHandler called");
        std::shared_ptr<self_type> self = weakSelf.lock();
        if (self == nullptr)
        {
            return;
        }
        if (self->sendResponse(completeRes, streamId) != 0)
        {
            self->close();
        }
    }

    int onRequestRecv(int32_t streamId)
    {
        BMCWEB_LOG_DEBUG("onRequestRecv streamId:{}", streamId);

        auto it = streams.find(streamId);
        if (it == streams.end())
        {
            close();
            return -1;
        }
        auto& reqReader = it->second.reqReader;
        if (reqReader)
        {
            boost::beast::error_code ec;
            reqReader->finish(ec);
            if (ec)
            {
                BMCWEB_LOG_CRITICAL("Failed to finalize payload");
                close();
                return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
            }
        }
        Request& thisReq = *it->second.req;
        using boost::beast::http::field;
        it->second.accept = thisReq.getHeaderValue(field::accept);
        it->second.acceptEnc = thisReq.getHeaderValue(field::accept_encoding);

        BMCWEB_LOG_DEBUG("Handling {} \"{}\"", logPtr(&thisReq),
                         thisReq.url().encoded_path());

        Response& thisRes = it->second.res;

        thisRes.setCompleteRequestHandler(std::bind_front(
            &self_type::afterCompleteRequest, weak_from_this(), streamId));

        auto asyncResp =
            std::make_shared<bmcweb::AsyncResp>(std::move(it->second.res));
        if constexpr (!BMCWEB_INSECURE_DISABLE_AUTH)
        {
            thisReq.session = authentication::authenticate(
                ip, asyncResp->res, thisReq.method(), thisReq.req, mtlsSession);
            if (!authentication::isOnAllowlist(thisReq.url().path(),
                                               thisReq.method()) &&
                thisReq.session == nullptr)
            {
                BMCWEB_LOG_WARNING("Authentication failed");
                forward_unauthorized::sendUnauthorized(
                    thisReq.url().encoded_path(),
                    thisReq.getHeaderValue("X-Requested-With"),
                    thisReq.getHeaderValue("Accept"), asyncResp->res);
                return 0;
            }
        }
        std::string_view expectedEtag =
            thisReq.getHeaderValue(boost::beast::http::field::if_none_match);
        BMCWEB_LOG_DEBUG("Setting expected etag {}", expectedEtag);
        if (!expectedEtag.empty())
        {
            asyncResp->res.setExpectedEtag(expectedEtag);
        }
        it->second.req->ipAddress = ip;
        handler->handle(it->second.req, asyncResp);
        return 0;
    }

    // The session is only resolved once the whole request is received, so
    // decide the limit from the credentials presented in the headers.
    uint64_t getBodyLimit(const Request& req) const
    {
        if constexpr (!BMCWEB_INSECURE_DISABLE_AUTH)
        {
            using boost::beast::http::field;
            if (mtlsSession == nullptr &&
                req.getHeaderValue(field::authorization).empty() &&
                req.getHeaderValue("X-Auth-Token").empty() &&
                req.getHeaderValue(field::cookie).empty())
            {
                return loggedOutPostBodyLimit;
            }
        }
        return httpReqBodyLimit;
    }

    void rejectStreamBody(int32_t streamId, Http2StreamData& stream)
    {
        stream.bodyRejected = true;
        // Callback return code alone doesn't reset the stream.
        ngSession.submitRstStream(streamId, NGHTTP2_CANCEL);
    }

    int onDataChunkRecvCallback(uint8_t /*flags*/, int32_t streamId,
                                const uint8_t* data, size_t len)
    {
        auto thisStream = streams.find(streamId);
        if (thisStream == streams.end())
        {
            BMCWEB_LOG_ERROR("Unknown stream{}", streamId);
            close();
            return -1;
        }

        Http2StreamData& streamData = thisStream->second;
        if (streamData.bodyRejected)
        {
            return 0;
        }
        if (streamData.bodyBytesReceived == 0)
        {
            // All headers have been received by the first DATA frame
            streamData.bodyLimit = getBodyLimit(*streamData.req);
        }
        // Same limits as HTTP/1.1, including the logged out limit.
        if (len > streamData.bodyLimit ||
            streamData.bodyBytesReceived > streamData.bodyLimit - len)
        {
            BMCWEB_LOG_WARNING(
                "Stream {} body exceeds limit of {} bytes, resetting", streamId,
                streamData.bodyLimit);
            rejectStreamBody(streamId, streamData);
            return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
        }
        streamData.bodyBytesReceived += len;

        std::optional<bmcweb::HttpBody::reader>& reqReader =
            thisStream->second.reqReader;
        if (!reqReader)
        {
            Request::Body& req = thisStream->second.req->req;
            reqReader.emplace(req.base(), req.body());
            boost::beast::error_code initEc;
            reqReader->init(thisStream->second.contentLength, initEc);
            if (initEc)
            {
                BMCWEB_LOG_CRITICAL("Failed to initialize payload");
                rejectStreamBody(streamId, streamData);
                return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
            }
        }
        boost::beast::error_code ec;
        reqReader->put(boost::asio::const_buffer(data, len), ec);
        if (ec)
        {
            BMCWEB_LOG_CRITICAL("Failed to write payload");
            rejectStreamBody(streamId, streamData);
            return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
        }
        return 0;
    }

    static int onDataChunkRecvStatic(
        nghttp2_session* /* session */, uint8_t flags, int32_t streamId,
        const uint8_t* data, size_t len, void* userData)
    {
        BMCWEB_LOG_DEBUG("onDataChunkRecvStatic");
        if (userData == nullptr)
        {
            BMCWEB_LOG_CRITICAL("user data was null?");
            return NGHTTP2_ERR_CALLBACK_FAILURE;
        }
        return userPtrToSelf(userData).onDataChunkRecvCallback(
            flags, streamId, data, len);
    }

    int onFrameRecvCallback(const nghttp2_frame& frame)
    {
        BMCWEB_LOG_DEBUG("frame type {}", static_cast<int>(frame.hd.type));
        switch (frame.hd.type)
        {
            case NGHTTP2_DATA:
            case NGHTTP2_HEADERS:
                // Check that the client request has finished
                if ((frame.hd.flags & NGHTTP2_FLAG_END_STREAM) != 0)
                {
                    return onRequestRecv(frame.hd.stream_id);
                }
                break;
            default:
                break;
        }
        return 0;
    }

    static int onFrameRecvCallbackStatic(nghttp2_session* /* session */,
                                         const nghttp2_frame* frame,
                                         void* userData)
    {
        BMCWEB_LOG_DEBUG("on_frame_recv_callback.  Frame type {}",
                         static_cast<int>(frame->hd.type));
        if (userData == nullptr)
        {
            BMCWEB_LOG_CRITICAL("user data was null?");
            return NGHTTP2_ERR_CALLBACK_FAILURE;
        }
        if (frame == nullptr)
        {
            BMCWEB_LOG_CRITICAL("frame was null?");
            return NGHTTP2_ERR_CALLBACK_FAILURE;
        }
        return userPtrToSelf(userData).onFrameRecvCallback(*frame);
    }

    static self_type& userPtrToSelf(void* userData)
    {
        // This method exists to keep the unsafe reinterpret cast in one
        // place.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<self_type*>(userData);
    }

    static int onStreamCloseCallbackStatic(nghttp2_session* /* session */,
                                           int32_t streamId,
                                           uint32_t /*unused*/, void* userData)
    {
        BMCWEB_LOG_DEBUG("on_stream_close_callback stream {}", streamId);
        if (userData == nullptr)
        {
            BMCWEB_LOG_CRITICAL("user data was null?");
            return NGHTTP2_ERR_CALLBACK_FAILURE;
        }
        self_type& self = userPtrToSelf(userData);
        if (self.streams.erase(streamId) <= 0)
        {
            return -1;
        }
        // streams map always has stream 0 (control); if only that remains,
        // the connection is idle -- switch to the longer keepalive timeout
        self.refreshDeadline();
        return 0;
    }

    int onHeaderCallback(const nghttp2_frame& frame,
                         std::span<const uint8_t> name,
                         std::span<const uint8_t> value)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        std::string_view nameSv(reinterpret_cast<const char*>(name.data()),
                                name.size());
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        std::string_view valueSv(reinterpret_cast<const char*>(value.data()),
                                 value.size());

        BMCWEB_LOG_DEBUG("on_header_callback name: {} value {}", nameSv,
                         valueSv);
        if (frame.hd.type != NGHTTP2_HEADERS)
        {
            return 0;
        }
        if (frame.headers.cat != NGHTTP2_HCAT_REQUEST)
        {
            return 0;
        }
        auto thisStream = streams.find(frame.hd.stream_id);
        if (thisStream == streams.end())
        {
            BMCWEB_LOG_ERROR("Unknown stream{}", frame.hd.stream_id);
            close();
            return -1;
        }

        Request& thisReq = *thisStream->second.req;

        if (nameSv == ":path")
        {
            thisReq.target(valueSv);
        }
        else if (nameSv == ":method")
        {
            boost::beast::http::verb verb =
                boost::beast::http::string_to_verb(valueSv);
            if (verb == boost::beast::http::verb::unknown)
            {
                BMCWEB_LOG_ERROR("Unknown http verb {}", valueSv);
                verb = boost::beast::http::verb::trace;
            }
            thisReq.method(verb);
        }
        else if (nameSv.starts_with(":"))
        {
            // Ignore all other http2 headers
            // :scheme and :authority are other valid http2 fields that might
            // show up here.
        }
        else
        {
            thisReq.addHeader(nameSv, valueSv);
            if (nameSv == "content-length")
            {
                uint64_t contentLength = 0;
                auto [ptr, err] = std::from_chars(valueSv.begin(),
                                                  valueSv.end(), contentLength);
                if (err != std::errc() || ptr != valueSv.end())
                {
                    BMCWEB_LOG_ERROR("Invalid content length {}", valueSv);
                    return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
                }
                thisStream->second.contentLength = contentLength;
            }
        }
        return 0;
    }

    static int onHeaderCallbackStatic(
        nghttp2_session* /* session */, const nghttp2_frame* frame,
        const uint8_t* name, size_t namelen, const uint8_t* value,
        size_t vallen, uint8_t /* flags */, void* userData)
    {
        if (userData == nullptr)
        {
            BMCWEB_LOG_CRITICAL("user data was null?");
            return NGHTTP2_ERR_CALLBACK_FAILURE;
        }
        if (frame == nullptr)
        {
            BMCWEB_LOG_CRITICAL("frame was null?");
            return NGHTTP2_ERR_CALLBACK_FAILURE;
        }
        if (name == nullptr)
        {
            BMCWEB_LOG_CRITICAL("name was null?");
            return NGHTTP2_ERR_CALLBACK_FAILURE;
        }
        if (value == nullptr)
        {
            BMCWEB_LOG_CRITICAL("value was null?");
            return NGHTTP2_ERR_CALLBACK_FAILURE;
        }
        return userPtrToSelf(userData).onHeaderCallback(*frame, {name, namelen},
                                                        {value, vallen});
    }

    int onBeginHeadersCallback(const nghttp2_frame& frame)
    {
        if (frame.hd.type == NGHTTP2_HEADERS &&
            frame.headers.cat == NGHTTP2_HCAT_REQUEST)
        {
            BMCWEB_LOG_DEBUG("create stream for id {}", frame.hd.stream_id);

            streams[frame.hd.stream_id];
            if (ngSession.setLocalWindowSize(
                    NGHTTP2_FLAG_NONE, frame.hd.stream_id, 16384 * 32) != 0)
            {
                BMCWEB_LOG_ERROR("Failed to set local window size");
            }
            // A new stream means the connection is no longer idle
            refreshDeadline();
        }
        return 0;
    }

    static int onBeginHeadersCallbackStatic(nghttp2_session* /* session */,
                                            const nghttp2_frame* frame,
                                            void* userData)
    {
        BMCWEB_LOG_DEBUG("on_begin_headers_callback");
        if (userData == nullptr)
        {
            BMCWEB_LOG_CRITICAL("user data was null?");
            return NGHTTP2_ERR_CALLBACK_FAILURE;
        }
        if (frame == nullptr)
        {
            BMCWEB_LOG_CRITICAL("frame was null?");
            return NGHTTP2_ERR_CALLBACK_FAILURE;
        }
        return userPtrToSelf(userData).onBeginHeadersCallback(*frame);
    }

    static void afterWriteBuffer(const std::shared_ptr<self_type>& self,
                                 const boost::system::error_code& ec,
                                 size_t sendLength)
    {
        self->isWriting = false;
        BMCWEB_LOG_DEBUG("Sent {}", sendLength);
        if (ec)
        {
            self->close();
            return;
        }
        self->refreshDeadline();
        self->writeBuffer();
    }

    void writeBuffer()
    {
        if (isWriting)
        {
            return;
        }
        std::span<const uint8_t> data = ngSession.memSend();
        if (data.empty())
        {
            return;
        }
        isWriting = true;
        if (httpType == HttpType::HTTPS)
        {
            boost::asio::async_write(
                adaptor, boost::asio::const_buffer(data.data(), data.size()),
                std::bind_front(afterWriteBuffer, shared_from_this()));
        }
        else if (httpType == HttpType::HTTP)
        {
            boost::asio::async_write(
                adaptor.next_layer(),
                boost::asio::const_buffer(data.data(), data.size()),
                std::bind_front(afterWriteBuffer, shared_from_this()));
        }
    }

    void close()
    {
        cancelDeadlineTimer();
        adaptor.next_layer().close();
    }

    void afterDoRead(const std::shared_ptr<self_type>& /*self*/,
                     const boost::system::error_code& ec,
                     size_t bytesTransferred)
    {
        BMCWEB_LOG_DEBUG("{} async_read_some {} Bytes", logPtr(this),
                         bytesTransferred);

        if (ec)
        {
            // EOF is normal when client closes HTTP/2 connection
            // Only log non-EOF errors
            if (ec != boost::asio::error::eof &&
                ec != boost::asio::ssl::error::stream_truncated)
            {
                BMCWEB_LOG_ERROR("{} Error while reading: {}", logPtr(this),
                                 ec.message());
            }
            close();
            BMCWEB_LOG_DEBUG("{} from read(1)", logPtr(this));
            return;
        }
        std::span<uint8_t> bufferSpan{inBuffer.data(), bytesTransferred};

        ssize_t readLen = ngSession.memRecv(bufferSpan);
        if (readLen < 0)
        {
            BMCWEB_LOG_ERROR("nghttp2_session_mem_recv returned {}", readLen);
            close();
            return;
        }
        refreshDeadline();
        writeBuffer();

        doRead();
    }

    void cancelDeadlineTimer()
    {
        timer.cancel();
        timerStarted = false;
        // Bump the generation so a completion for the just-canceled wait
        // (delivered asynchronously, possibly after a new timer has already
        // been armed) is recognized as stale in afterTimerWait().
        ++timerGeneration;
    }

    // Push the deadline back on read/write progress, so a long-running
    // transfer over an open stream isn't killed by the Default timeout
    // just because no new stream has opened or closed recently.
    void refreshDeadline()
    {
        cancelDeadlineTimer();
        // streams map always has stream 0 (control); if only that remains,
        // the connection is idle -- use the longer keepalive timeout
        if (streams.size() <= 1)
        {
            startDeadline(DeadlineTimerType::Keepalive);
            return;
        }
        startDeadline(DeadlineTimerType::Default);
    }

    void afterTimerWait(const boost::system::error_code& ec,
                        uint64_t generation)
    {
        if (generation != timerGeneration)
        {
            // Stale callback for a timer arm that has since been canceled
            // and replaced; ignore it so it can't clobber the active timer.
            BMCWEB_LOG_DEBUG("{} HTTP2 stale timer callback ignored",
                             logPtr(this));
            return;
        }

        timerStarted = false;

        if (ec)
        {
            if (ec == boost::asio::error::operation_aborted)
            {
                BMCWEB_LOG_DEBUG("{} HTTP2 timer canceled", logPtr(this));
                return;
            }
            BMCWEB_LOG_CRITICAL("{} HTTP2 timer failed {}", logPtr(this), ec);
        }

        BMCWEB_LOG_WARNING("{} HTTP2 connection timed out, closing",
                           logPtr(this));
        close();
    }

    void startDeadline(DeadlineTimerType timerType)
    {
        if (timerStarted)
        {
            return;
        }

        int timeoutDurationSeconds = 15;
        if (timerType == DeadlineTimerType::Keepalive)
        {
            // idle with no open streams, allow up to 15 minutes of delay
            timeoutDurationSeconds = 15 * 60;
        }

        std::chrono::seconds timeout(timeoutDurationSeconds);

        uint64_t generation = ++timerGeneration;
        timer.expires_after(timeout);
        timer.async_wait([weakSelf = weak_from_this(),
                          generation](const boost::system::error_code& ec) {
            std::shared_ptr<self_type> self = weakSelf.lock();
            if (!self)
            {
                return;
            }
            self->afterTimerWait(ec, generation);
        });
        timerStarted = true;
        BMCWEB_LOG_DEBUG("{} HTTP2 timer started ({} seconds)", logPtr(this),
                         timeoutDurationSeconds);
    }

    void doRead()
    {
        BMCWEB_LOG_DEBUG("{} doRead", logPtr(this));
        if (httpType == HttpType::HTTPS)
        {
            adaptor.async_read_some(boost::asio::buffer(inBuffer),
                                    std::bind_front(&self_type::afterDoRead,
                                                    this, shared_from_this()));
        }
        else if (httpType == HttpType::HTTP)
        {
            adaptor.next_layer().async_read_some(
                boost::asio::buffer(inBuffer),
                std::bind_front(&self_type::afterDoRead, this,
                                shared_from_this()));
        }
    }

    // A mapping from http2 stream ID to Stream Data
    std::map<int32_t, Http2StreamData> streams;

    std::array<uint8_t, 8192> inBuffer{};

    HttpType httpType = HttpType::BOTH;
    boost::asio::ssl::stream<Adaptor> adaptor;
    bool isWriting = false;

    nghttp2_session ngSession;

    Handler* handler;
    std::function<std::string()>& getCachedDateStr;

    std::shared_ptr<persistent_data::UserSession> mtlsSession;
    boost::asio::ip::address ip;

    boost::asio::steady_timer timer;
    bool timerStarted = false;
    uint64_t timerGeneration = 0;

    using std::enable_shared_from_this<
        HTTP2Connection<Adaptor, Handler>>::shared_from_this;

    using std::enable_shared_from_this<
        HTTP2Connection<Adaptor, Handler>>::weak_from_this;
};
} // namespace crow
