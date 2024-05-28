#pragma once
#include "bmcweb_config.h"

#include "async_resp.hpp"
#ifdef BMCWEB_ENABLE_LINUX_AUDIT_EVENTS
#include "audit_events.hpp"
#endif
#include "authentication.hpp"
#include "complete_response_fields.hpp"
#include "dump_utils.hpp"
#include "http2_connection.hpp"
#include "http_body.hpp"
#include "http_response.hpp"
#include "http_utility.hpp"
#include "logging.hpp"
#include "mutual_tls.hpp"
#include "ssl_key_handler.hpp"
#include "str_utility.hpp"
#include "utility.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/_experimental/test/stream.hpp>
#include <boost/beast/core/buffers_generator.hpp>
#include <boost/beast/core/flat_static_buffer.hpp>
#include <boost/beast/http/error.hpp>
#include <boost/beast/http/message_generator.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/websocket.hpp>

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace crow
{

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static int connectionCount = 0;

// request body limit size set by the bmcwebHttpReqBodyLimitMb option
constexpr uint64_t httpReqBodyLimit = 1024UL * 1024UL *
                                      bmcwebHttpReqBodyLimitMb;

constexpr uint64_t loggedOutPostBodyLimit = 4096U;

constexpr uint32_t httpHeaderLimit = 8192U;

template <typename>
struct IsTls : std::false_type
{};

template <typename T>
struct IsTls<boost::asio::ssl::stream<T>> : std::true_type
{};

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
        initParser();

#ifdef BMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION
        prepareMutualTls();
#endif // BMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION

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
        // We always return true to allow full auth flow for resources that
        // don't require auth
        if (preverified)
        {
            mtlsSession = verifyMtlsUser(ip, ctx);
            if (mtlsSession)
            {
                BMCWEB_LOG_DEBUG("{} Generating TLS session: {}", logPtr(this),
                                 mtlsSession->uniqueId);
            }
        }
        return true;
    }

    void prepareMutualTls()
    {
        if constexpr (IsTls<Adaptor>::value)
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
                std::string id = "bmcweb";

                const char* cStr = id.c_str();
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                const auto* idC = reinterpret_cast<const unsigned char*>(cStr);
                int ret = SSL_set_session_id_context(
                    adaptor.native_handle(), idC,
                    static_cast<unsigned int>(id.length()));
                if (ret == 0)
                {
                    BMCWEB_LOG_ERROR("{} failed to set SSL id", logPtr(this));
                }
            }

            adaptor.set_verify_callback(
                std::bind_front(&self_type::tlsVerifyCallback, this));
        }
    }

    Adaptor& socket()
    {
        return adaptor;
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

        startDeadline();

        readClientIp();

        // TODO(ed) Abstract this to a more clever class with the idea of an
        // asynchronous "start"
        if constexpr (IsTls<Adaptor>::value)
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
        // If http2 is enabled, negotiate the protocol
        if constexpr (bmcwebEnableHTTP2)
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
        if constexpr (!std::is_same_v<Adaptor, boost::beast::test::stream>)
        {
#ifndef BMCWEB_INSECURE_DISABLE_AUTHX
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
#endif // BMCWEB_INSECURE_DISABLE_AUTHX
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
            handler->handleUpgrade(req, asyncResp, std::move(adaptor));
            return;
        }

        std::string url(req->target());
        if (boost::algorithm::contains(url,
                                       "/system/LogServices/Dump/Entries/") &&
            boost::algorithm::ends_with(url, "/attachment"))
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

            redfish::dump_utils::getValidDumpEntryForAttachment(
                asyncResp, req->url(),
                [asyncResp, this, self(shared_from_this())](
                    [[maybe_unused]] const std::string& objectPath,
                    [[maybe_unused]] const std::string& entryID,
                    [[maybe_unused]] const std::string& dumpType) {
                BMCWEB_LOG_DEBUG("upgrade stream connection");
                handler->handleUpgrade(req, asyncResp, std::move(adaptor));

                // delete lambda with self shared_ptr
                // to enable connection destruction
                res.completeRequestHandler = nullptr;
            });
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
        boost::beast::get_lowest_layer(adaptor).close();
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
        if (mtlsSession != nullptr)
        {
            BMCWEB_LOG_DEBUG("{} Removing TLS session: {}", logPtr(this),
                             mtlsSession->uniqueId);
            persistent_data::SessionStore::getInstance().removeSession(
                mtlsSession);
        }
        if constexpr (IsTls<Adaptor>::value)
        {
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

#ifdef BMCWEB_ENABLE_LINUX_AUDIT_EVENTS
        if (audit::wantAudit(*req))
        {
            if (userSession != nullptr)
            {
                bool requestSuccess = false;
                // Look for good return codes and if so we know the operation
                // passed
                if ((res.resultInt() >= 200) && (res.resultInt() < 300))
                {
                    requestSuccess = true;
                }

                audit::auditEvent(*req, userSession->username, requestSuccess);
            }
            else
            {
                BMCWEB_LOG_DEBUG(
                    "UserSession is null, not able to log audit event!");
            }
        }
#endif // BMCWEB_ENABLE_LINUX_AUDIT_EVENTS

        completeResponseFields(*req, res);
        res.addHeader(boost::beast::http::field::date, getCachedDateStr());

        doWrite();

        // delete lambda with self shared_ptr
        // to enable connection destruction
        res.setCompleteRequestHandler(nullptr);
    }

    void readClientIp()
    {
        boost::system::error_code ec;

        if constexpr (!std::is_same_v<Adaptor, boost::beast::test::stream>)
        {
            boost::asio::ip::tcp::endpoint endpoint =
                boost::beast::get_lowest_layer(adaptor).remote_endpoint(ec);

            if (ec)
            {
                // If remote endpoint fails keep going. "ClientOriginIPAddress"
                // will be empty.
                BMCWEB_LOG_ERROR(
                    "Failed to get the client's IP Address. ec : {}", ec);
                return;
            }
            ip = endpoint.address();
        }
    }

  private:
    uint64_t getContentLengthLimit()
    {
#ifndef BMCWEB_INSECURE_DISABLE_AUTHX
        if (userSession == nullptr)
        {
            return loggedOutPostBodyLimit;
        }
#endif

        return httpReqBodyLimit;
    }

    // Returns true if content length was within limits
    // Returns false if content length error has been returned
    bool handleContentLengthError()
    {
        if (!parser)
        {
            BMCWEB_LOG_CRITICAL("Paser was null");
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

    void doReadHeaders()
    {
        BMCWEB_LOG_DEBUG("{} doReadHeaders", logPtr(this));
        if (!parser)
        {
            BMCWEB_LOG_CRITICAL("Parser was not initialized.");
            return;
        }
        // Clean up any previous Connection.
        boost::beast::http::async_read_header(
            adaptor, buffer, *parser,
            [this,
             self(shared_from_this())](const boost::system::error_code& ec,
                                       std::size_t bytesTransferred) {
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
                    BMCWEB_LOG_WARNING("{} End of stream, closing {}",
                                       logPtr(this), ec);
                    hardClose();
                    return;
                }

                BMCWEB_LOG_DEBUG("{} Closing socket due to read error {}",
                                 logPtr(this), ec.message());
                gracefulClose();

                return;
            }

            if constexpr (!std::is_same_v<Adaptor, boost::beast::test::stream>)
            {
#ifndef BMCWEB_INSECURE_DISABLE_AUTHX
                boost::beast::http::verb method = parser->get().method();
                userSession = crow::authentication::authenticate(
                    ip, res, method, parser->get().base(), mtlsSession);
#endif // BMCWEB_INSECURE_DISABLE_AUTHX
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
        });
    }

    void doRead()
    {
        BMCWEB_LOG_DEBUG("{} doRead", logPtr(this));
        if (!parser)
        {
            return;
        }
        startDeadline();
        boost::beast::http::async_read_some(
            adaptor, buffer, *parser,
            [this,
             self(shared_from_this())](const boost::system::error_code& ec,
                                       std::size_t bytesTransferred) {
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

            // If the user is logged in, allow them to send files incrementally
            // one piece at a time. If authentication is disabled then there is
            // no user session hence always allow to send one piece at a time.
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
        });
    }

    void afterDoWrite(const std::shared_ptr<self_type>& /*self*/,
                      const boost::system::error_code& ec,
                      std::size_t bytesTransferred)
    {
        BMCWEB_LOG_DEBUG("{} async_write wrote {} bytes, ec=", logPtr(this),
                         bytesTransferred, ec);

        cancelDeadlineTimer();

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
        boost::beast::async_write(
            adaptor,
            boost::beast::http::message_generator(std::move(res.response)),
            std::bind_front(&self_type::afterDoWrite, this,
                            shared_from_this()));
    }

    void cancelDeadlineTimer()
    {
        timer.cancel();
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
        timer.async_wait([weakSelf](const boost::system::error_code& ec) {
            // Note, we are ignoring other types of errors here;  If the timer
            // failed for any reason, we should still close the connection
            std::shared_ptr<Connection<Adaptor, Handler>> self =
                weakSelf.lock();
            if (!self)
            {
                if (ec == boost::asio::error::operation_aborted)
                {
                    BMCWEB_LOG_DEBUG(
                        "{} Timer canceled on connection being destroyed",
                        logPtr(self.get()));
                    return;
                }
                BMCWEB_LOG_CRITICAL("{} Failed to capture connection",
                                    logPtr(self.get()));
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
                BMCWEB_LOG_CRITICAL("{} Timer failed {}", logPtr(self.get()),
                                    ec);
            }

            BMCWEB_LOG_WARNING("{} Connection timed out, hard closing",
                               logPtr(self.get()));

            self->hardClose();
        });

        timerStarted = true;
        BMCWEB_LOG_DEBUG("{} timer started", logPtr(this));
    }

    Adaptor adaptor;
    Handler* handler;

    boost::asio::ip::address ip;

    // Making this a std::optional allows it to be efficiently destroyed and
    // re-created on Connection reset
    std::optional<boost::beast::http::request_parser<bmcweb::HttpBody>> parser;

    boost::beast::flat_static_buffer<8192> buffer;

    std::shared_ptr<crow::Request> req;
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
