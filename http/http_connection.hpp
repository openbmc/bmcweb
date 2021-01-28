#pragma once
#include "bmcweb_config.h"

#include "authentication.hpp"
#include "http_response.hpp"
#include "http_utility.hpp"
#include "logging.hpp"
#include "utility.hpp"

#include <nghttp2/nghttp2.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core/flat_static_buffer.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/serializer.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/url/url_view.hpp>
#include <http2.hpp>
#include <json_html_serializer.hpp>
#include <mutual_tls.hpp>
#include <security_headers.hpp>
#include <ssl_key_handler.hpp>

#include <atomic>
#include <chrono>
#include <vector>

namespace crow
{

struct Http2StreamData
{
    crow::Request req{};
    crow::Response res{};
    size_t sentSofar = 0;
};

inline void prettyPrintJson(crow::Response& res)
{
    json_html_util::dumpHtml(res.body(), res.jsonValue);

    res.addHeader(boost::beast::http::field::content_type,
                  "text/html;charset=UTF-8");
}

static int connectionCount = 0;

// request body limit size set by the bmcwebHttpReqBodyLimitMb option
constexpr uint64_t httpReqBodyLimit =
    1024UL * 1024UL * bmcwebHttpReqBodyLimitMb;

constexpr uint64_t loggedOutPostBodyLimit = 4096;

constexpr uint32_t httpHeaderLimit = 8192;

template <typename Adaptor, typename Handler>
class Connection :
    public std::enable_shared_from_this<Connection<Adaptor, Handler>>
{
    using self_type = Connection<Adaptor, Handler>;

  public:
    Connection(Handler* handlerIn, boost::asio::steady_timer&& timerIn,
               std::function<std::string()>& getCachedDateStrF,
               Adaptor adaptorIn) :
        adaptor(std::move(adaptorIn)),
        handler(handlerIn), timer(std::move(timerIn)),
        getCachedDateStr(getCachedDateStrF)
    {
        parser.emplace(std::piecewise_construct, std::make_tuple());
        parser->body_limit(httpReqBodyLimit);
        parser->header_limit(httpHeaderLimit);

#ifdef BMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION
        prepareMutualTls();
#endif // BMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION

        connectionCount++;

        BMCWEB_LOG_DEBUG << this << " Connection open, total "
                         << connectionCount;
    }

    ~Connection()
    {
        connectionCount--;
        BMCWEB_LOG_DEBUG << this << " Connection closed, total "
                         << connectionCount;
    }

    Connection(const Connection&) = delete;
    Connection(Connection&&) = delete;
    Connection& operator=(const Connection&) = delete;
    Connection& operator=(Connection&&) = delete;

    bool tlsVerifyCallback(bool preverified,
                           boost::asio::ssl::verify_context& ctx)
    {
        // We always return true to allow full auth flow for resources that
        // don't require auth
        if (preverified)
        {
            userSession = verifyMtlsUser(req->ipAddress, ctx);
            if (userSession)
            {
                sessionIsFromTransport = true;
            }
        }
        return true;
    }

    void prepareMutualTls()
    {
        std::error_code error;
        std::filesystem::path caPath(ensuressl::trustStorePath);
        auto caAvailable = !std::filesystem::is_empty(caPath, error);
        caAvailable = caAvailable && !error;
        if (caAvailable && persistent_data::SessionStore::getInstance()
                               .getAuthMethodsConfig()
                               .tls)
        {
            adaptor.set_verify_mode(boost::asio::ssl::verify_peer);
            constexpr std::string_view id = "bmcweb";

            const auto* idC = std::bit_cast<const unsigned char*>(id.data());
            int ret = SSL_set_session_id_context(
                adaptor.native_handle(), idC,
                static_cast<unsigned int>(id.length()));
            if (ret == 0)
            {
                BMCWEB_LOG_ERROR << this << " failed to set SSL id";
            }
        }

        adaptor.set_verify_callback(
            std::bind_front(&self_type::tlsVerifyCallback, this));
    }

    Adaptor& socket()
    {
        return adaptor;
    }

    void start()
    {
        if (connectionCount >= 100)
        {
            BMCWEB_LOG_CRITICAL << this << "Max connection count exceeded.";
            return;
        }

        startDeadline();

        // TODO(ed) Abstract this to a more clever class with the idea of an
        // asynchronous "start"
        if constexpr (std::is_same_v<Adaptor,
                                     boost::beast::ssl_stream<
                                         boost::asio::ip::tcp::socket>>)
        {
            adaptor.async_handshake(boost::asio::ssl::stream_base::server,
                                    [this, self(shared_from_this())](
                                        const boost::system::error_code& ec) {
                if (ec)
                {
                    return;
                }

                afterSslHandshake();
            });
        }
        else
        {
            doReadHeaders();
        }
    }

    void afterSslHandshake()
    {
        const unsigned char* alpn = nullptr;
        unsigned int alpnlen = 0;
        SSL_get0_alpn_selected(adaptor.native_handle(), &alpn, &alpnlen);
        if (alpn != nullptr)
        {
            std::string_view selectedProtocol(std::bit_cast<const char*>(alpn),
                                              alpnlen);
            BMCWEB_LOG_DEBUG << "ALPN selected protocol \"" << selectedProtocol
                             << "\" len: " << alpnlen;
            if (selectedProtocol == "h2")
            {
                cancelDeadlineTimer();

                // Create the control stream
                streams.emplace(0, std::make_unique<Http2StreamData>());

                ngSession = initializeNghttp2Session();

                if (sendServerConnectionHeader() != 0)
                {
                    BMCWEB_LOG_ERROR << "send_server_connection_header failed";
                    return;
                }
                readBuffer();
                return;
            }
        }

        doReadHeaders();
    }

    void handle()
    {
        std::error_code reqEc;
        crow::Request& thisReq = req.emplace(parser->release(), reqEc);
        if (reqEc)
        {
            BMCWEB_LOG_DEBUG << "Request failed to construct" << reqEc;
            return;
        }
        thisReq.session = userSession;

        // Fetch the client IP address
        readClientIp();

        Response res;

        // Check for HTTP version 1.1.
        if (thisReq.version() == 11)
        {
            if (thisReq.getHeaderValue(boost::beast::http::field::host).empty())
            {
                res.result(boost::beast::http::status::bad_request);
                completeRequest(res);
                return;
            }
        }

        BMCWEB_LOG_INFO << "Request: "
                        << " " << this << " HTTP/" << thisReq.version() / 10
                        << "." << thisReq.version() % 10 << ' '
                        << thisReq.methodString() << " " << thisReq.target()
                        << " " << thisReq.ipAddress.to_string();

        res.isAliveHelper = [this]() -> bool { return isAlive(); };

        thisReq.ioService = static_cast<decltype(thisReq.ioService)>(
            &adaptor.get_executor().context());

        if (res.completed)
        {
            completeRequest(res);
            return;
        }
#ifndef BMCWEB_INSECURE_DISABLE_AUTHX
        if (!crow::authentication::isOnAllowlist(req->url, req->method()) &&
            thisReq.session == nullptr)
        {
            BMCWEB_LOG_WARNING << "Authentication failed";
            forward_unauthorized::sendUnauthorized(
                req->url, req->getHeaderValue("X-Requested-With"),
                req->getHeaderValue("Accept"), res);
            completeRequest(res);
            return;
        }
#endif // BMCWEB_INSECURE_DISABLE_AUTHX
        auto asyncResp = std::make_shared<bmcweb::AsyncResp>(std::move(res));
        BMCWEB_LOG_DEBUG << "Setting completion handler";
        asyncResp->res.setCompleteRequestHandler(
            [self(shared_from_this())](crow::Response& thisRes) {
            self->completeRequest(thisRes);
        });

        if (thisReq.isUpgrade() &&
            boost::iequals(
                thisReq.getHeaderValue(boost::beast::http::field::upgrade),
                "websocket"))
        {
            handler->handleUpgrade(thisReq, res, std::move(adaptor));
            // delete lambda with self shared_ptr
            // to enable connection destruction
            asyncResp->res.setCompleteRequestHandler(nullptr);
            return;
        }
        handler->handle(thisReq, asyncResp);
    }

    bool isAlive()
    {
        if constexpr (std::is_same_v<Adaptor,
                                     boost::beast::ssl_stream<
                                         boost::asio::ip::tcp::socket>>)
        {
            return adaptor.next_layer().is_open();
        }
        else
        {
            return adaptor.is_open();
        }
    }
    void close()
    {
        if constexpr (std::is_same_v<Adaptor,
                                     boost::beast::ssl_stream<
                                         boost::asio::ip::tcp::socket>>)
        {
            adaptor.next_layer().close();
            if (sessionIsFromTransport && userSession != nullptr)
            {
                BMCWEB_LOG_DEBUG
                    << this
                    << " Removing TLS session: " << userSession->uniqueId;
                persistent_data::SessionStore::getInstance().removeSession(
                    userSession);
            }
        }
        else
        {
            adaptor.close();
        }
    }

    void completeResponseFields(const crow::Request& initiatingRequest,
                                crow::Response& resToComplete)
    {
        BMCWEB_LOG_INFO << "Response: " << this << ' ' << initiatingRequest.url
                        << ' ' << resToComplete.resultInt()
                        << " keepalive=" << initiatingRequest.keepAlive();

        addSecurityHeaders(initiatingRequest, resToComplete);

        crow::authentication::cleanupTempSession(initiatingRequest);

        if (!isAlive())
        {
            // BMCWEB_LOG_DEBUG << this << " delete (socket is closed) " <<
            // isReading
            // << ' ' << isWriting;
            // delete this;

            // delete lambda with self shared_ptr
            // to enable connection destruction
            resToComplete.setCompleteRequestHandler(nullptr);
            return;
        }
        if (resToComplete.body().empty() && !resToComplete.jsonValue.empty())
        {
            using http_helpers::ContentType;
            std::array<ContentType, 3> allowed{
                ContentType::CBOR, ContentType::JSON, ContentType::HTML};
            ContentType prefered = getPreferedContentType(
                initiatingRequest.getHeaderValue("Accept"), allowed);

            if (prefered == ContentType::HTML)
            {
                prettyPrintJson(resToComplete);
            }
            else
            {
                // Technically prefered could also be NoMatch here, but we'd
                // like to default to something rather than return 400 for
                // backward compatibility.
                resToComplete.addHeader(boost::beast::http::field::content_type,
                                        "application/json");
                resToComplete.body() = resToComplete.jsonValue.dump(
                    2, ' ', true, nlohmann::json::error_handler_t::replace);
            }
        }

        if (resToComplete.resultInt() >= 400 && resToComplete.body().empty())
        {
            resToComplete.body() = std::string(resToComplete.reason());
        }

        if (resToComplete.result() == boost::beast::http::status::no_content)
        {
            // Boost beast throws if content is provided on a no-content
            // response.  Ideally, this would never happen, but in the case that
            // it does, we don't want to throw.
            BMCWEB_LOG_CRITICAL
                << " Response content provided but code was no-content";
            resToComplete.body().clear();
        }

        resToComplete.addHeader(boost::beast::http::field::date,
                                getCachedDateStr());

        resToComplete.keepAlive(initiatingRequest.keepAlive());
        resToComplete.stringResponse->prepare_payload();
    }

    void completeRequest(Response& completeRes)
    {
        crow::authentication::cleanupTempSession(*req);

        if (!isAlive())
        {
            // BMCWEB_LOG_DEBUG << this << " delete (socket is closed) " <<
            // isReading
            // << ' ' << isWriting;
            // delete this;

            // delete lambda with self shared_ptr
            // to enable connection destruction
            completeRes.setCompleteRequestHandler(nullptr);
            return;
        }

        completeResponseFields(*req, completeRes);
        doWrite(completeRes);

        // delete lambda with self shared_ptr
        // to enable connection destruction
        completeRes.setCompleteRequestHandler(nullptr);
    }

    void readClientIp()
    {
        boost::asio::ip::address ip;
        boost::system::error_code ec = getClientIp(ip);
        if (ec)
        {
            return;
        }
        req->ipAddress = ip;
    }

    boost::system::error_code getClientIp(boost::asio::ip::address& ip)
    {
        boost::system::error_code ec;
        boost::asio::ip::tcp::endpoint endpoint =
            boost::beast::get_lowest_layer(adaptor).remote_endpoint(ec);

        if (ec)
        {
            // If remote endpoint fails keep going. "ClientOriginIPAddress"
            // will be empty.
            BMCWEB_LOG_ERROR << "Failed to get the client's IP Address. ec : "
                             << ec;
            return ec;
        }
        ip = endpoint.address();
        return ec;
    }

    int sendServerConnectionHeader()
    {
        BMCWEB_LOG_DEBUG << "send_server_connection_header()";

        uint32_t maxStreams = 4;
        std::array<nghttp2_settings_entry, 2> iv = {
            {{NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, maxStreams},
             {NGHTTP2_SETTINGS_ENABLE_PUSH, 0}}};
        int rv = ngSession.submitSettings(iv);
        if (rv != 0)
        {
            BMCWEB_LOG_ERROR << "Fatal error: " << nghttp2_strerror(rv);
            return -1;
        }
        return 0;
    }

  private:
    void doReadHeaders()
    {
        BMCWEB_LOG_DEBUG << this << " doReadHeaders";
        // Clean up any previous Connection.
        boost::beast::http::async_read_header(
            adaptor, inBuffer, *parser,
            [this,
             self(shared_from_this())](const boost::system::error_code& ec,
                                       std::size_t bytesTransferred) {
            BMCWEB_LOG_DEBUG << this << " async_read_header "
                             << bytesTransferred << " Bytes";
            bool errorWhileReading = false;
            if (ec)
            {
                errorWhileReading = true;
                if (ec == boost::asio::error::eof)
                {
                    BMCWEB_LOG_WARNING
                        << this << " Error while reading: " << ec.message();
                }
                else
                {
                    BMCWEB_LOG_ERROR
                        << this << " Error while reading: " << ec.message();
                }
            }
            else
            {
                // if the adaptor isn't open anymore, and wasn't handed to a
                // websocket, treat as an error
                if (!isAlive() &&
                    !boost::beast::websocket::is_upgrade(parser->get()))
                {
                    errorWhileReading = true;
                }
            }

            cancelDeadlineTimer();

            if (errorWhileReading)
            {
                close();
                BMCWEB_LOG_DEBUG << this << " from read(1)";
                return;
            }

            readClientIp();

            boost::asio::ip::address ip;
            if (getClientIp(ip))
            {
                BMCWEB_LOG_DEBUG << "Unable to get client IP";
            }
            sessionIsFromTransport = false;
#ifndef BMCWEB_INSECURE_DISABLE_AUTHX
            boost::beast::http::verb method = parser->get().method();
            userSession = crow::authentication::authenticate(
                ip, method, parser->get().base(), userSession);

            bool loggedIn = userSession != nullptr;
            if (!loggedIn)
            {
                const boost::optional<uint64_t> contentLength =
                    parser->content_length();
                if (contentLength && *contentLength > loggedOutPostBodyLimit)
                {
                    BMCWEB_LOG_DEBUG << "Content length greater than limit "
                                     << *contentLength;
                    close();
                    return;
                }

                BMCWEB_LOG_DEBUG << "Starting quick deadline";
            }
#endif // BMCWEB_INSECURE_DISABLE_AUTHX

            doRead();
            });
    }

    void doRead()
    {
        BMCWEB_LOG_DEBUG << this << " doRead";
        startDeadline();
        boost::beast::http::async_read(adaptor, inBuffer, *parser,
                                       [this, self(shared_from_this())](
                                           const boost::system::error_code& ec,
                                           std::size_t bytesTransferred) {
            BMCWEB_LOG_DEBUG << this << " async_read " << bytesTransferred
                             << " Bytes";
            cancelDeadlineTimer();
            if (ec)
            {
                BMCWEB_LOG_ERROR << this
                                 << " Error while reading: " << ec.message();
                close();
                BMCWEB_LOG_DEBUG << this << " from read(1)";
                return;
            }
            handle();
        });
    }

    void doWrite(crow::Response& thisRes)
    {
        BMCWEB_LOG_DEBUG << this << " doWrite";
        thisRes.preparePayload();
        serializer.emplace(*thisRes.stringResponse);
        startDeadline();
        bool keepalive = thisRes.keepAlive();
        boost::beast::http::async_write(
            adaptor, *serializer,
            [this, self(shared_from_this()),
             keepalive](const boost::system::error_code& ec,
                        std::size_t bytesTransferred) {
            BMCWEB_LOG_DEBUG << this << " async_write " << bytesTransferred
                             << " bytes";

            cancelDeadlineTimer();

            if (ec)
            {
                BMCWEB_LOG_DEBUG << this << " from write(2)";
                return;
            }
            if (!keepalive)
            {
                close();
                BMCWEB_LOG_DEBUG << this << " from write(1)";
                return;
            }

            serializer.reset();
            BMCWEB_LOG_DEBUG << this << " Clearing response";
            parser.emplace(std::piecewise_construct, std::make_tuple());
            parser->body_limit(httpReqBodyLimit); // reset body limit for
                                                  // newly created parser
            inBuffer.consume(inBuffer.size());

            // If the session was built from the transport, we don't need to
            // clear it.  All other sessions are generated per request.
            if (!sessionIsFromTransport)
            {
                userSession = nullptr;
            }

            // Destroy the Request via the std::optional
            req.reset();
            doReadHeaders();
            });
    }

    void cancelDeadlineTimer()
    {
        timer.cancel();
    }

    void startDeadline()
    {
        cancelDeadlineTimer();

        std::chrono::seconds timeout(15);
        // allow slow uploads for logged in users
        bool loggedIn = userSession != nullptr;
        if (loggedIn)
        {
            timeout = std::chrono::seconds(60);
            return;
        }

        std::weak_ptr<Connection<Adaptor, Handler>> weakSelf = weak_from_this();
        timer.expires_after(timeout);
        timer.async_wait([weakSelf](const boost::system::error_code ec) {
            // Note, we are ignoring other types of errors here;  If the timer
            // failed for any reason, we should still close the connection
            if (ec == boost::asio::error::operation_aborted)
            {
                // Canceled wait means the path succeeeded.
                return;
            }
            std::shared_ptr<Connection<Adaptor, Handler>> self =
                weakSelf.lock();
            if (!self)
            {
                BMCWEB_LOG_CRITICAL << self << " Failed to capture connection";
                return;
            }
            if (ec)
            {
                BMCWEB_LOG_CRITICAL << self << " timer failed " << ec;
            }

            BMCWEB_LOG_WARNING << self << "Connection timed out, closing";

            self->close();
        });

        BMCWEB_LOG_DEBUG << this << " timer started";
    }

    static ssize_t fileReadCallback(nghttp2_session* /* session */,
                                    int32_t /* stream_id */, uint8_t* buf,
                                    size_t length, uint32_t* dataFlags,
                                    nghttp2_data_source* source,
                                    void* /*unused*/)
    {
        if (source == nullptr || source->ptr == nullptr)
        {
            BMCWEB_LOG_DEBUG << "Source was null???";
            return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
        }

        BMCWEB_LOG_DEBUG << "File read callback length: " << length;
        Http2StreamData* str = reinterpret_cast<Http2StreamData*>(source->ptr);
        crow::Response& res = str->res;

        BMCWEB_LOG_DEBUG << "total: " << res.body().size()
                         << " send_sofar: " << str->sentSofar;

        size_t toSend = std::min(res.body().size() - str->sentSofar, length);
        BMCWEB_LOG_DEBUG << "Copying " << toSend << " bytes to buf";

        memcpy(buf, res.body().data() + str->sentSofar, toSend);
        str->sentSofar += toSend;

        if (str->sentSofar >= res.body().size())
        {
            BMCWEB_LOG_DEBUG << "Setting OEF flag";
            *dataFlags |= NGHTTP2_DATA_FLAG_EOF;
            //*dataFlags |= NGHTTP2_DATA_FLAG_NO_COPY;
        }
        return static_cast<ssize_t>(toSend);
    }

    nghttp2_nv headerFromStringViews(std::string_view name,
                                     std::string_view value)
    {
        uint8_t* nameData = std::bit_cast<uint8_t*>(name.data());
        uint8_t* valueData = const_cast<uint8_t*>(
            reinterpret_cast<const uint8_t*>(value.data()));
        return {nameData, valueData, name.size(), value.size(),
                NGHTTP2_NV_FLAG_NONE};
    }

    int sendResponse(Response& completedRes, int32_t streamId)
    {
        BMCWEB_LOG_DEBUG << "send_response stream_id:" << streamId;

        auto it = streams.find(streamId);
        if (it == streams.end())
        {
            close();
            return -1;
        }
        Response& thisRes = it->second->res;
        thisRes = std::move(completedRes);
        crow::Request& thisReq = it->second->req;
        std::vector<nghttp2_nv> hdr;

        completeResponseFields(thisReq, thisRes);

        boost::beast::http::fields& fields = thisRes.stringResponse->base();
        std::string code = std::to_string(thisRes.stringResponse->result_int());
        hdr.emplace_back(headerFromStringViews(":status", code));
        for (const boost::beast::http::fields::value_type& header : fields)
        {
            hdr.emplace_back(
                headerFromStringViews(header.name_string(), header.value()));
        }
        Http2StreamData* streamPtr = it->second.get();
        streamPtr->sentSofar = 0;

        nghttp2_data_provider dataPrd{
            .source{
                .ptr = streamPtr,
            },
            .read_callback = fileReadCallback,
        };

        int rv = ngSession.submitResponse(streamId, hdr, &dataPrd);
        if (rv != 0)
        {
            BMCWEB_LOG_ERROR << "Fatal error: " << nghttp2_strerror(rv);
            close();
            return -1;
        }
        sendNgBuffer();

        return 0;
    }

    void sendNgBuffer()
    {
        const uint8_t* data = nullptr;
        ssize_t ret = ngSession.send(&data);
        if (ret < 0)
        {
            BMCWEB_LOG_ERROR << "ngSession.send() failed: " << ret;
            close();
            return;
        }
        if (socket_sending)
        {
            BMCWEB_LOG_DEBUG << "socket already sending";
            return;
        }
        size_t size = static_cast<size_t>(ret);
        BMCWEB_LOG_DEBUG << "Copying in " << size;
        std::array<boost::asio::const_buffer, 1> copyBuffer = {
            boost::asio::buffer(data, size)};
        size_t copied =
            boost::asio::buffer_copy(sendBuffer.prepare(size), copyBuffer);
        BMCWEB_LOG_DEBUG << "copied " << copied;
        sendBuffer.commit(copied);
        BMCWEB_LOG_DEBUG << "size is " << sendBuffer.size();

        if (sendBuffer.size() == 0)
        {
            BMCWEB_LOG_DEBUG << "No data to send";
            return;
        }

        socket_sending = true;
        BMCWEB_LOG_DEBUG << "async_write_some sending " << sendBuffer.size()
                         << " bytes";
        adaptor.async_write_some(
            sendBuffer.data(), [this, self(shared_from_this())](
                                   boost::system::error_code ec, size_t bytes) {
                BMCWEB_LOG_DEBUG << "Sent " << bytes << " bytes";
                sendBuffer.consume(bytes);
                socket_sending = false;
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "async_write_some failed: " << ec;
                    close();
                    return;
                }
                sendNgBuffer();
            });
    }

    nghttp2_session_ptr initializeNghttp2Session()
    {
        Nghttp2SessionCallbacksPtr callbacks;
        callbacks.setOnFrameRecvCallback(onFrameRecvCallbackStatic);
        callbacks.setOnStreamCloseCallback(onStreamCloseCallbackStatic);
        callbacks.setOnHeaderCallback(onHeaderCallbackStatic);
        callbacks.setOnBeginHeadersCallback(onBeginHeadersCallbackStatic);
        callbacks.setOnBeginHeadersCallback(onBeginHeadersCallbackStatic);

        nghttp2_session_ptr session(callbacks);
        session.setUserData(this);

        return session;
    }

    void readBuffer()
    {
        BMCWEB_LOG_DEBUG << "read-buffer";

        adaptor.async_read_some(
            inBuffer.prepare(8192),
            [this, self(shared_from_this())](boost::system::error_code ec,
                                             size_t bytes) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "async_read_some got error " << ec;
                close();
                return;
            }
            inBuffer.commit(bytes);
            BMCWEB_LOG_DEBUG << "async_read_some Got " << bytes << " bytes";
            size_t consumed = 0;
            for (const auto bufferIt : inBuffer.data())
            {
                std::span<const uint8_t> bufferSpan{
                    std::bit_cast<const uint8_t*>(bufferIt.data()),
                    bufferIt.size()};
                ssize_t readLen = ngSession.memRecv(bufferSpan);
                if (readLen <= 0)
                {
                    BMCWEB_LOG_ERROR << "nghttp2_session_mem_recv returned "
                                     << readLen;
                    close();
                    return;
                }
                consumed += static_cast<size_t>(readLen);
            }
            inBuffer.consume(consumed);

            readBuffer();
            });
    }

    int onRequestRecv(int32_t streamId)
    {
        BMCWEB_LOG_DEBUG << "on_request_recv";

        auto it = streams.find(streamId);
        if (it == streams.end())
        {
            close();
            return -1;
        }

        crow::Request& thisReq = it->second->req;
        BMCWEB_LOG_DEBUG << "Handling " << &thisReq << " \"" << thisReq.url
                         << "\"";

        crow::Response& thisRes = it->second->res;

        thisRes.completeRequestHandler =
            [this, streamId](Response& completeRes) {
            BMCWEB_LOG_DEBUG << "res.completeRequestHandler called";
            if (sendResponse(completeRes, streamId) != 0)
            {
                close();
                return;
            }
        };
        auto asyncResp =
            std::make_shared<bmcweb::AsyncResp>(std::move(it->second->res));
        handler->handle(thisReq, asyncResp);

        return 0;
    }

    int onFrameRecvCallback(const nghttp2_frame& frame)
    {
        BMCWEB_LOG_DEBUG << "frame type " << static_cast<int>(frame.hd.type);
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
        BMCWEB_LOG_DEBUG << "on_frame_recv_callback";
        if (userData == nullptr)
        {
            BMCWEB_LOG_CRITICAL << "user data was null?";
            return NGHTTP2_ERR_CALLBACK_FAILURE;
        }
        if (frame == nullptr)
        {
            BMCWEB_LOG_CRITICAL << "frame was null?";
            return NGHTTP2_ERR_CALLBACK_FAILURE;
        }
        return userPtrToSelf(userData).onFrameRecvCallback(*frame);
    }

    static Connection<Adaptor, Handler>& userPtrToSelf(void* userData)
    {
        // This method exists to keep the unsafe reinterpret cast in one
        // place.
        return *reinterpret_cast<Connection<Adaptor, Handler>*>(userData);
    }

    static int onStreamCloseCallbackStatic(nghttp2_session* /* session */,
                                           int32_t streamId,
                                           uint32_t /*unused*/, void* userData)
    {
        BMCWEB_LOG_DEBUG << "on_stream_close_callback stream " << streamId;
        if (userData == nullptr)
        {
            BMCWEB_LOG_CRITICAL << "user data was null?";
            return NGHTTP2_ERR_CALLBACK_FAILURE;
        }
        auto stream = userPtrToSelf(userData).streams.find(streamId);
        if (stream == userPtrToSelf(userData).streams.end())
        {
            return -1;
        }

        userPtrToSelf(userData).streams.erase(streamId);
        return 0;
    }

    int onHeaderCallback(const nghttp2_frame& frame,
                         std::span<const uint8_t> name,
                         std::span<const uint8_t> value)
    {
        std::string_view nameSv(reinterpret_cast<const char*>(name.data()),
                                name.size());
        std::string_view valueSv(reinterpret_cast<const char*>(value.data()),
                                 value.size());

        BMCWEB_LOG_DEBUG << "on_header_callback name: " << nameSv << " value "
                         << valueSv;

        switch (frame.hd.type)
        {
            case NGHTTP2_HEADERS:
                if (frame.headers.cat != NGHTTP2_HCAT_REQUEST)
                {
                    break;
                }
                auto thisStream = streams.find(frame.hd.stream_id);
                if (thisStream == streams.end())
                {
                    BMCWEB_LOG_ERROR << "Unknown stream" << frame.hd.stream_id;
                    close();
                    return -1;
                }

                crow::Request& thisReq = thisStream->second->req;

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
                        BMCWEB_LOG_ERROR << "Unknown http verb " << valueSv;
                        close();
                        return -1;
                    }
                    thisReq.req.method(verb);
                }
                else if (nameSv == ":scheme")
                {
                    // Nothing to check on scheme
                }
                else
                {
                    thisReq.fields.set(nameSv, valueSv);
                }
                break;
        }
        return 0;
    }

    static int onHeaderCallbackStatic(nghttp2_session* /* session */,
                                      const nghttp2_frame* frame,
                                      const uint8_t* name, size_t namelen,
                                      const uint8_t* value, size_t vallen,
                                      uint8_t /* flags */, void* userData)
    {
        if (userData == nullptr)
        {
            BMCWEB_LOG_CRITICAL << "user data was null?";
            return NGHTTP2_ERR_CALLBACK_FAILURE;
        }
        if (frame == nullptr)
        {
            BMCWEB_LOG_CRITICAL << "frame was null?";
            return NGHTTP2_ERR_CALLBACK_FAILURE;
        }
        if (name == nullptr)
        {
            BMCWEB_LOG_CRITICAL << "name was null?";
            return NGHTTP2_ERR_CALLBACK_FAILURE;
        }
        if (value == nullptr)
        {
            BMCWEB_LOG_CRITICAL << "value was null?";
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

            BMCWEB_LOG_DEBUG << "create stream for id " << frame.hd.stream_id;

            std::pair<boost::container::flat_map<
                          int32_t, std::unique_ptr<Http2StreamData>>::iterator,
                      bool>
                stream = streams.emplace(frame.hd.stream_id,
                                         std::make_unique<Http2StreamData>());
            // http2 is by definition always tls
            stream.first->second->req.isSecure = true;
        }
        return 0;
    }

    static int onBeginHeadersCallbackStatic(nghttp2_session* /* session */,
                                            const nghttp2_frame* frame,
                                            void* userData)
    {
        BMCWEB_LOG_DEBUG << "on_begin_headers_callback";
        if (userData == nullptr)
        {
            BMCWEB_LOG_CRITICAL << "user data was null?";
            return NGHTTP2_ERR_CALLBACK_FAILURE;
        }
        if (frame == nullptr)
        {
            BMCWEB_LOG_CRITICAL << "frame was null?";
            return NGHTTP2_ERR_CALLBACK_FAILURE;
        }
        return userPtrToSelf(userData).onBeginHeadersCallback(*frame);
    }

  private:
    nghttp2_session_ptr ngSession;

    boost::container::flat_map<int32_t, std::unique_ptr<Http2StreamData>>
        streams;

    bool socket_sending = false;

    Adaptor adaptor;
    Handler* handler;
    // Making this a std::optional allows it to be efficiently destroyed and
    // re-created on Connection reset
    std::optional<
        boost::beast::http::request_parser<boost::beast::http::string_body>>
        parser;
    boost::beast::multi_buffer inBuffer;

    boost::beast::multi_buffer sendBuffer;

    std::optional<boost::beast::http::response_serializer<
        boost::beast::http::string_body>>
        serializer;

    std::optional<crow::Request> req;

    bool sessionIsFromTransport = false;
    std::shared_ptr<persistent_data::UserSession> userSession;

    boost::asio::steady_timer timer;

    std::function<std::string()>& getCachedDateStr;

    using std::enable_shared_from_this<
        Connection<Adaptor, Handler>>::shared_from_this;

    using std::enable_shared_from_this<
        Connection<Adaptor, Handler>>::weak_from_this;
};
} // namespace crow
