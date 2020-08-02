#pragma once
#include "bmcweb_config.h"

#include "async_resp.hpp"
#include "authentication.hpp"
#include "complete_response_fields.hpp"
#include "http2_connection.hpp"
#include "http_body.hpp"
#include "http_connect_types.hpp"
#include "http_response.hpp"
#include "http_utility.hpp"
#include "logging.hpp"
#include "mutual_tls.hpp"
#include "ssl_key_handler.hpp"
#include "str_utility.hpp"
#include "utility.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/_experimental/test/stream.hpp>
#include <boost/beast/core/buffers_generator.hpp>
#include <boost/beast/core/detect_ssl.hpp>
#include <boost/beast/core/flat_static_buffer.hpp>
#include <boost/beast/http/error.hpp>
#include <boost/beast/http/message_generator.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/websocket.hpp>

#include <atomic>
#include <chrono>
#include <memory>
#include <vector>

namespace crow
{

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static int connectionCount = 0;

// request body limit size set by the BMCWEB_HTTP_BODY_LIMIT option
constexpr uint64_t httpReqBodyLimit = 1024UL * 1024UL * BMCWEB_HTTP_BODY_LIMIT;

constexpr uint64_t loggedOutPostBodyLimit = 4096U;

constexpr uint32_t httpHeaderLimit = 8192U;

template <typename Adaptor, typename Handler>
class Connection :
    public std::enable_shared_from_this<Connection<Adaptor, Handler>>
{
    using self_type = Connection<Adaptor, Handler>;

  public:
    Connection(Handler* handlerIn, HttpType httpTypeIn,
               boost::asio::steady_timer&& timerIn,

               std::function<std::string()>& getCachedDateStrF,
               boost::asio::ssl::stream<Adaptor>&& adaptorIn) :
        httpType(httpTypeIn), adaptor(std::move(adaptorIn)), handler(handlerIn),
        timer(std::move(timerIn)), getCachedDateStr(getCachedDateStrF)
    {
        initParser();

        connectionCount++;

        BMCWEB_LOG_DEBUG("{} Connection created, total {}", logPtr(this),
                         connectionCount);
    }

    ~Connection()
    {
        res.releaseCompleteRequestHandler();
        cancelDeadlineTimer();

        connectionCount--;
        BMCWEB_LOG_DEBUG("{} Connection closed, total {}", logPtr(this),
                         connectionCount);
    }

    Connection(const Connection&) = delete;
    Connection(Connection&&) = delete;
    Connection& operator=(const Connection&) = delete;
    Connection& operator=(Connection&&) = delete;

    bool tlsVerifyCallback(bool preverified,
                           boost::asio::ssl::verify_context& ctx)
    {
        BMCWEB_LOG_DEBUG("{} tlsVerifyCallback called with preverified {}",
                         logPtr(this), preverified);
        if (preverified)
        {
            mtlsSession = verifyMtlsUser(ip, ctx);
            if (mtlsSession)
            {
                BMCWEB_LOG_DEBUG("{} Generated TLS session: {}", logPtr(this),
                                 mtlsSession->uniqueId);
            }
        }
        const persistent_data::AuthConfigMethods& c =
            persistent_data::SessionStore::getInstance().getAuthMethodsConfig();
        if (c.tlsStrict)
        {
            return preverified;
        }
        // If tls strict mode is disabled
        // We always return true to allow full auth flow for resources that
        // don't require auth
        return true;
    }

    bool prepareMutualTls()
    {
        BMCWEB_LOG_DEBUG("prepareMutualTls");

        constexpr std::string_view id = "bmcweb";

        const char* idPtr = id.data();
        const auto* idCPtr = std::bit_cast<const unsigned char*>(idPtr);
        auto idLen = static_cast<unsigned int>(id.length());
        int ret =
            SSL_set_session_id_context(adaptor.native_handle(), idCPtr, idLen);
        if (ret == 0)
        {
            BMCWEB_LOG_ERROR("{} failed to set SSL id", logPtr(this));
            return false;
        }

        BMCWEB_LOG_DEBUG("set_verify_callback");

        boost::system::error_code ec;
        adaptor.set_verify_callback(
            std::bind_front(&self_type::tlsVerifyCallback, this), ec);
        if (ec)
        {
            BMCWEB_LOG_ERROR("Failed to set verify callback {}", ec);
            return false;
        }

        return true;
    }

    void afterDetectSsl(const std::shared_ptr<self_type> /*self*/,
                        boost::beast::error_code ec, bool isTls)
    {
        if (ec)
        {
            BMCWEB_LOG_ERROR("Couldn't detect ssl ", ec);
            return;
        }

        if (isTls)
        {
            if (httpType != HttpType::HTTPS && httpType != HttpType::BOTH)
            {
                BMCWEB_LOG_WARNING(
                    "{} Connection closed due to incompatible type",
                    logPtr(this));
                return;
            }
            httpType = HttpType::HTTPS;
            adaptor.async_handshake(
                boost::asio::ssl::stream_base::server,
                std::bind_front(&self_type::afterSslHandshake, this,
                                shared_from_this()));
        }
        else
        {
            if (httpType != HttpType::HTTP && httpType != HttpType::BOTH)
            {
                BMCWEB_LOG_WARNING(
                    "{} Connection closed due to incompatible type",
                    logPtr(this));
                return;
            }

            httpType = HttpType::HTTP;
            BMCWEB_LOG_INFO("Starting non-SSL session");
            doReadHeaders();
        }
    }

    void start()
    {
        BMCWEB_LOG_DEBUG("{} Connection started, total {}", logPtr(this),
                         connectionCount);
        if (connectionCount >= 200)
        {
            BMCWEB_LOG_CRITICAL("{} Max connection count exceeded.",
                                logPtr(this));
            return;
        }

        if constexpr (BMCWEB_MUTUAL_TLS_AUTH)
        {
            if (!prepareMutualTls())
            {
                BMCWEB_LOG_ERROR("{} Failed to prepare mTLS", logPtr(this));
                return;
            }
        }

        startDeadline();

        readClientIp();
        boost::beast::async_detect_ssl(
            adaptor.next_layer(), buffer,
            std::bind_front(&self_type::afterDetectSsl, this,
                            shared_from_this()));
    }

    void afterSslHandshake(const std::shared_ptr<self_type>& /*self*/,
                           const boost::system::error_code& ec)
    {
        if (ec)
        {
            BMCWEB_LOG_ERROR("{} SSL handshake failed", logPtr(this));
            return;
        }
        // If http2 is enabled, negotiate the protocol
        if constexpr (BMCWEB_EXPERIMENTAL_HTTP2)
        {
            const unsigned char* alpn = nullptr;
            unsigned int alpnlen = 0;
            SSL_get0_alpn_selected(adaptor.native_handle(), &alpn, &alpnlen);
            if (alpn != nullptr)
            {
                std::string_view selectedProtocol(
                    std::bit_cast<const char*>(alpn), alpnlen);
                BMCWEB_LOG_DEBUG("ALPN selected protocol \"{}\" len: {}",
                                 selectedProtocol, alpnlen);
                if (selectedProtocol == "h2")
                {
                    auto http2 =
                        std::make_shared<HTTP2Connection<Adaptor, Handler>>(
                            std::move(adaptor), handler, getCachedDateStr);
                    http2->start();
                    return;
                }
            }
        }

        doReadHeaders();
    }

    void initParser()
    {
        boost::beast::http::request_parser<bmcweb::HttpBody>& instance =
            parser.emplace(std::piecewise_construct, std::make_tuple());

        // reset header limit for newly created parser
        instance.header_limit(httpHeaderLimit);

        // Initially set no body limit. We don't yet know if the user is
        // authenticated.
        instance.body_limit(boost::none);
    }

    void handle()
    {
        std::error_code reqEc;
        if (!parser)
        {
            return;
        }
        req = std::make_shared<crow::Request>(parser->release(), reqEc);
        if (reqEc)
        {
            BMCWEB_LOG_DEBUG("Request failed to construct{}", reqEc.message());
            res.result(boost::beast::http::status::bad_request);
            completeRequest(res);
            return;
        }
        req->session = userSession;
        accept = req->getHeaderValue("Accept");
        // Fetch the client IP address
        req->ipAddress = ip;

        // Check for HTTP version 1.1.
        if (req->version() == 11)
        {
            if (req->getHeaderValue(boost::beast::http::field::host).empty())
            {
                res.result(boost::beast::http::status::bad_request);
                completeRequest(res);
                return;
            }
        }

        BMCWEB_LOG_INFO("Request:  {} HTTP/{}.{} {} {} {}", logPtr(this),
                        req->version() / 10, req->version() % 10,
                        req->methodString(), req->target(),
                        req->ipAddress.to_string());

        req->ioService = static_cast<decltype(req->ioService)>(
            &adaptor.get_executor().context());

        if (res.completed)
        {
            completeRequest(res);
            return;
        }
        keepAlive = req->keepAlive();

        if (authenticationEnabled)
        {
            if (!crow::authentication::isOnAllowlist(req->url().path(),
                                                     req->method()) &&
                req->session == nullptr)
            {
                BMCWEB_LOG_WARNING("Authentication failed");
                forward_unauthorized::sendUnauthorized(
                    req->url().encoded_path(),
                    req->getHeaderValue("X-Requested-With"),
                    req->getHeaderValue("Accept"), res);
                completeRequest(res);
                return;
            }
        }

        auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
        BMCWEB_LOG_DEBUG("Setting completion handler");
        asyncResp->res.setCompleteRequestHandler(
            [self(shared_from_this())](crow::Response& thisRes) {
                self->completeRequest(thisRes);
            });
        bool isSse =
            isContentTypeAllowed(req->getHeaderValue("Accept"),
                                 http_helpers::ContentType::EventStream, false);
        std::string_view upgradeType(
            req->getHeaderValue(boost::beast::http::field::upgrade));
        if ((req->isUpgrade() &&
             bmcweb::asciiIEquals(upgradeType, "websocket")) ||
            isSse)
        {
            asyncResp->res.setCompleteRequestHandler(
                [self(shared_from_this())](crow::Response& thisRes) {
                    if (thisRes.result() != boost::beast::http::status::ok)
                    {
                        // When any error occurs before handle upgradation,
                        // the result in response will be set to respective
                        // error. By default the Result will be OK (200),
                        // which implies successful handle upgrade. Response
                        // needs to be sent over this connection only on
                        // failure.
                        self->completeRequest(thisRes);
                        return;
                    }
                });

            if (httpType == HttpType::HTTP)
            {
                handler->handleUpgrade(req, asyncResp,
                                       std::move(adaptor.next_layer()));
            }
            else
            {
                handler->handleUpgrade(req, asyncResp, std::move(adaptor));
            }
            return;
        }
        std::string_view expected =
            req->getHeaderValue(boost::beast::http::field::if_none_match);
        if (!expected.empty())
        {
            res.setExpectedHash(expected);
        }
        handler->handle(req, asyncResp);
    }

    void hardClose()
    {
        BMCWEB_LOG_DEBUG("{} Closing socket", logPtr(this));
        adaptor.next_layer().close();
    }

    void tlsShutdownComplete(const std::shared_ptr<self_type>& self,
                             const boost::system::error_code& ec)
    {
        if (ec)
        {
            BMCWEB_LOG_WARNING("{} Failed to shut down TLS cleanly {}",
                               logPtr(self.get()), ec);
        }
        self->hardClose();
    }

    void gracefulClose()
    {
        BMCWEB_LOG_DEBUG("{} Socket close requested", logPtr(this));

        if (httpType == HttpType::HTTPS)
        {
            if (mtlsSession != nullptr)
            {
                BMCWEB_LOG_DEBUG("{} Removing TLS session: {}", logPtr(this),
                                 mtlsSession->uniqueId);
                persistent_data::SessionStore::getInstance().removeSession(
                    mtlsSession);
            }

            adaptor.async_shutdown(std::bind_front(
                &self_type::tlsShutdownComplete, this, shared_from_this()));
        }
        else
        {
            hardClose();
        }
    }

    void completeRequest(crow::Response& thisRes)
    {
        res = std::move(thisRes);
        res.keepAlive(keepAlive);

        completeResponseFields(accept, res);
        res.addHeader(boost::beast::http::field::date, getCachedDateStr());

        doWrite();

        // delete lambda with self shared_ptr
        // to enable connection destruction
        res.setCompleteRequestHandler(nullptr);
    }

    void readClientIp()
    {
        boost::system::error_code ec;

        boost::asio::ip::tcp::endpoint endpoint =
            boost::beast::get_lowest_layer(adaptor).remote_endpoint(ec);

        if (ec)
        {
            // If remote endpoint fails keep going. "ClientOriginIPAddress"
            // will be empty.
            BMCWEB_LOG_ERROR("Failed to get the client's IP Address. ec : {}",
                             ec);
            return;
        }
        ip = endpoint.address();
    }

    void disableAuth()
    {
        authenticationEnabled = false;
    }

  private:
    uint64_t getContentLengthLimit()
    {
        if constexpr (!BMCWEB_INSECURE_DISABLE_AUTH)
        {
            if (userSession == nullptr)
            {
                return loggedOutPostBodyLimit;
            }
        }

        return httpReqBodyLimit;
    }

    // Returns true if content length was within limits
    // Returns false if content length error has been returned
    bool handleContentLengthError()
    {
        if (!parser)
        {
            BMCWEB_LOG_CRITICAL("Parser was null");
            return false;
        }
        const boost::optional<uint64_t> contentLength =
            parser->content_length();
        if (!contentLength)
        {
            BMCWEB_LOG_DEBUG("{} No content length available", logPtr(this));
            return true;
        }

        uint64_t maxAllowedContentLength = getContentLengthLimit();

        if (*contentLength > maxAllowedContentLength)
        {
            // If the users content limit is between the logged in
            // and logged out limits They probably just didn't log
            // in
            if (*contentLength > loggedOutPostBodyLimit &&
                *contentLength < httpReqBodyLimit)
            {
                BMCWEB_LOG_DEBUG(
                    "{} Content length {} valid, but greater than logged out"
                    " limit of {}. Setting unauthorized",
                    logPtr(this), *contentLength, loggedOutPostBodyLimit);
                res.result(boost::beast::http::status::unauthorized);
            }
            else
            {
                // Otherwise they're over both limits, so inform
                // them
                BMCWEB_LOG_DEBUG(
                    "{} Content length {} was greater than global limit {}."
                    " Setting payload too large",
                    logPtr(this), *contentLength, httpReqBodyLimit);
                res.result(boost::beast::http::status::payload_too_large);
            }

            keepAlive = false;
            doWrite();
            return false;
        }

        return true;
    }

    void afterReadHeaders(const std::shared_ptr<self_type>& /*self*/,
                          const boost::system::error_code& ec,
                          std::size_t bytesTransferred)
    {
        BMCWEB_LOG_DEBUG("{} async_read_header {} Bytes", logPtr(this),
                         bytesTransferred);

        if (ec)
        {
            cancelDeadlineTimer();

            if (ec == boost::beast::http::error::header_limit)
            {
                BMCWEB_LOG_ERROR("{} Header field too large, closing",
                                 logPtr(this), ec.message());

                res.result(boost::beast::http::status::
                               request_header_fields_too_large);
                keepAlive = false;
                doWrite();
                return;
            }
            if (ec == boost::beast::http::error::end_of_stream)
            {
                BMCWEB_LOG_WARNING("{} End of stream, closing {}", logPtr(this),
                                   ec);
                hardClose();
                return;
            }

            BMCWEB_LOG_DEBUG("{} Closing socket due to read error {}",
                             logPtr(this), ec.message());
            gracefulClose();

            return;
        }

        if (!parser)
        {
            BMCWEB_LOG_ERROR("Parser was unexpectedly null");
            return;
        }

        if (authenticationEnabled)
        {
            boost::beast::http::verb method = parser->get().method();
            userSession = authentication::authenticate(
                ip, res, method, parser->get().base(), mtlsSession);
        }

        std::string_view expect =
            parser->get()[boost::beast::http::field::expect];
        if (bmcweb::asciiIEquals(expect, "100-continue"))
        {
            res.result(boost::beast::http::status::continue_);
            doWrite();
            return;
        }

        if (!handleContentLengthError())
        {
            return;
        }

        parser->body_limit(getContentLengthLimit());

        if (parser->is_done())
        {
            handle();
            return;
        }

        doRead();
    }

    void doReadHeaders()
    {
        BMCWEB_LOG_DEBUG("{} doReadHeaders", logPtr(this));
        if (!parser)
        {
            BMCWEB_LOG_CRITICAL("Parser was not initialized.");
            return;
        }

        if (httpType == HttpType::HTTP)
        {
            boost::beast::http::async_read_header(
                adaptor.next_layer(), buffer, *parser,
                std::bind_front(&self_type::afterReadHeaders, this,
                                shared_from_this()));
        }
        else
        {
            boost::beast::http::async_read_header(
                adaptor, buffer, *parser,
                std::bind_front(&self_type::afterReadHeaders, this,
                                shared_from_this()));
        }
    }

    void afterRead(const std::shared_ptr<self_type>& /*self*/,
                   const boost::system::error_code& ec,
                   std::size_t bytesTransferred)
    {
        BMCWEB_LOG_DEBUG("{} async_read_some {} Bytes", logPtr(this),
                         bytesTransferred);

        if (ec)
        {
            BMCWEB_LOG_ERROR("{} Error while reading: {}", logPtr(this),
                             ec.message());
            if (ec == boost::beast::http::error::body_limit)
            {
                if (handleContentLengthError())
                {
                    BMCWEB_LOG_CRITICAL("Body length limit reached, "
                                        "but no content-length "
                                        "available?  Should never happen");
                    res.result(
                        boost::beast::http::status::internal_server_error);
                    keepAlive = false;
                    doWrite();
                }
                return;
            }

            gracefulClose();
            return;
        }

        // If the user is logged in, allow them to send files
        // incrementally one piece at a time. If authentication is
        // disabled then there is no user session hence always allow to
        // send one piece at a time.
        if (userSession != nullptr)
        {
            cancelDeadlineTimer();
        }
        if (!parser->is_done())
        {
            doRead();
            return;
        }

        cancelDeadlineTimer();
        handle();
    }

    void doRead()
    {
        BMCWEB_LOG_DEBUG("{} doRead", logPtr(this));
        if (!parser)
        {
            return;
        }
        startDeadline();
        if (httpType == HttpType::HTTP)
        {
            boost::beast::http::async_read_some(
                adaptor.next_layer(), buffer, *parser,
                std::bind_front(&self_type::afterRead, this,
                                shared_from_this()));
        }
        else
        {
            boost::beast::http::async_read_some(
                adaptor, buffer, *parser,
                std::bind_front(&self_type::afterRead, this,
                                shared_from_this()));
        }
    }

    void afterDoWrite(const std::shared_ptr<self_type>& /*self*/,
                      const boost::system::error_code& ec,
                      std::size_t bytesTransferred)
    {
        BMCWEB_LOG_DEBUG("{} async_write wrote {} bytes, ec={}", logPtr(this),
                         bytesTransferred, ec);

        cancelDeadlineTimer();

        if (ec == boost::system::errc::operation_would_block ||
            ec == boost::system::errc::resource_unavailable_try_again)
        {
            doWrite();
            return;
        }
        if (ec)
        {
            BMCWEB_LOG_DEBUG("{} from write(2)", logPtr(this));
            return;
        }

        if (res.result() == boost::beast::http::status::continue_)
        {
            // Reset the result to ok
            res.result(boost::beast::http::status::ok);
            doRead();
            return;
        }

        if (!keepAlive)
        {
            BMCWEB_LOG_DEBUG("{} keepalive not set.  Closing socket",
                             logPtr(this));

            gracefulClose();
            return;
        }

        BMCWEB_LOG_DEBUG("{} Clearing response", logPtr(this));
        res.clear();
        initParser();

        userSession = nullptr;

        req->clear();
        doReadHeaders();
    }

    void doWrite()
    {
        BMCWEB_LOG_DEBUG("{} doWrite", logPtr(this));
        res.preparePayload();

        startDeadline();
        if (httpType == HttpType::HTTP)
        {
            boost::beast::async_write(
                adaptor.next_layer(),
                boost::beast::http::message_generator(std::move(res.response)),
                std::bind_front(&self_type::afterDoWrite, this,
                                shared_from_this()));
        }
        else
        {
            boost::beast::async_write(
                adaptor,
                boost::beast::http::message_generator(std::move(res.response)),
                std::bind_front(&self_type::afterDoWrite, this,
                                shared_from_this()));
        }
    }

    void cancelDeadlineTimer()
    {
        timer.cancel();
    }

    void afterTimerWait(const std::weak_ptr<self_type>& weakSelf,
                        const boost::system::error_code& ec)
    {
        // Note, we are ignoring other types of errors here;  If the timer
        // failed for any reason, we should still close the connection
        std::shared_ptr<Connection<Adaptor, Handler>> self = weakSelf.lock();
        if (!self)
        {
            if (ec == boost::asio::error::operation_aborted)
            {
                BMCWEB_LOG_DEBUG(
                    "{} Timer canceled on connection being destroyed",
                    logPtr(self.get()));
            }
            else
            {
                BMCWEB_LOG_CRITICAL("{} Failed to capture connection",
                                    logPtr(self.get()));
            }
            return;
        }

        self->timerStarted = false;

        if (ec)
        {
            if (ec == boost::asio::error::operation_aborted)
            {
                BMCWEB_LOG_DEBUG("{} Timer canceled", logPtr(self.get()));
                return;
            }
            BMCWEB_LOG_CRITICAL("{} Timer failed {}", logPtr(self.get()), ec);
        }

        BMCWEB_LOG_WARNING("{} Connection timed out, hard closing",
                           logPtr(self.get()));

        self->hardClose();
    }

    void startDeadline()
    {
        // Timer is already started so no further action is required.
        if (timerStarted)
        {
            return;
        }

        std::chrono::seconds timeout(15);

        std::weak_ptr<Connection<Adaptor, Handler>> weakSelf = weak_from_this();
        timer.expires_after(timeout);
        timer.async_wait(std::bind_front(&self_type::afterTimerWait, this,
                                         weak_from_this()));

        timerStarted = true;
        BMCWEB_LOG_DEBUG("{} timer started", logPtr(this));
    }

    bool authenticationEnabled = !BMCWEB_INSECURE_DISABLE_AUTH;
    HttpType httpType = HttpType::BOTH;

    boost::asio::ssl::stream<Adaptor> adaptor;
    Handler* handler;

    boost::asio::ip::address ip;

    // Making this a std::optional allows it to be efficiently destroyed and
    // re-created on Connection reset
    std::optional<boost::beast::http::request_parser<bmcweb::HttpBody>> parser;

    boost::beast::flat_static_buffer<8192> buffer;

    std::shared_ptr<crow::Request> req;
    std::string accept;

    crow::Response res;

    std::shared_ptr<persistent_data::UserSession> userSession;
    std::shared_ptr<persistent_data::UserSession> mtlsSession;

    boost::asio::steady_timer timer;

    bool keepAlive = true;

    bool timerStarted = false;

    std::function<std::string()>& getCachedDateStr;

    using std::enable_shared_from_this<
        Connection<Adaptor, Handler>>::shared_from_this;

    using std::enable_shared_from_this<
        Connection<Adaptor, Handler>>::weak_from_this;
};
} // namespace crow
