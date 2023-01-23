#pragma once
#include "bmcweb_config.h"

#include "async_resp.hpp"
#include "authentication.hpp"
#include "http_response.hpp"
#include "http_utility.hpp"
#include "json_html_serializer.hpp"
#include "logging.hpp"
#include "mutual_tls.hpp"
#include "security_headers.hpp"
#include "ssl_key_handler.hpp"
#include "utility.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core/flat_static_buffer.hpp>
#include <boost/beast/http/error.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/serializer.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/url/url_view.hpp>

#include <atomic>
#include <chrono>
#include <vector>

namespace crow
{

inline void prettyPrintJson(crow::Response& res)
{
    json_html_util::dumpHtml(res.body(), res.jsonValue);

    res.addHeader(boost::beast::http::field::content_type,
                  "text/html;charset=UTF-8");
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
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
        res.setCompleteRequestHandler(nullptr);
        cancelDeadlineTimer();

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
            mtlsSession = verifyMtlsUser(req->ipAddress, ctx);
            if (mtlsSession)
            {
                BMCWEB_LOG_DEBUG
                    << this
                    << " Generating TLS session: " << mtlsSession->uniqueId;
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
            std::string id = "bmcweb";

            const char* cStr = id.c_str();
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            const auto* idC = reinterpret_cast<const unsigned char*>(cStr);
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
                doReadHeaders();
            });
        }
        else
        {
            doReadHeaders();
        }
    }

    void handle()
    {
        std::error_code reqEc;
        crow::Request& thisReq = req.emplace(parser->release(), reqEc);
        if (reqEc)
        {
            BMCWEB_LOG_DEBUG << "Request failed to construct" << reqEc;
            res.result(boost::beast::http::status::bad_request);
            completeRequest(res);
            return;
        }
        thisReq.session = userSession;

        // Fetch the client IP address
        readClientIp();

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
        keepAlive = thisReq.keepAlive();
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
        auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
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
        std::string_view expected =
            req->getHeaderValue(boost::beast::http::field::if_none_match);
        if (!expected.empty())
        {
            res.setExpectedHash(expected);
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
            if (mtlsSession != nullptr)
            {
                BMCWEB_LOG_DEBUG
                    << this
                    << " Removing TLS session: " << mtlsSession->uniqueId;
                persistent_data::SessionStore::getInstance().removeSession(
                    mtlsSession);
            }
        }
        else
        {
            adaptor.close();
        }
    }

    void completeRequest(crow::Response& thisRes)
    {
        if (!req)
        {
            return;
        }
        res = std::move(thisRes);
        res.keepAlive(keepAlive);

        BMCWEB_LOG_INFO << "Response: " << this << ' ' << req->url << ' '
                        << res.resultInt() << " keepalive=" << keepAlive;

        addSecurityHeaders(*req, res);

        crow::authentication::cleanupTempSession(*req);

        if (!isAlive())
        {
            // BMCWEB_LOG_DEBUG << this << " delete (socket is closed) " <<
            // isReading
            // << ' ' << isWriting;
            // delete this;

            // delete lambda with self shared_ptr
            // to enable connection destruction
            res.setCompleteRequestHandler(nullptr);
            return;
        }

        res.setHashAndHandleNotModified();

        if (res.body().empty() && !res.jsonValue.empty())
        {
            using http_helpers::ContentType;
            std::array<ContentType, 3> allowed{
                ContentType::CBOR, ContentType::JSON, ContentType::HTML};
            ContentType prefered =
                getPreferedContentType(req->getHeaderValue("Accept"), allowed);

            if (prefered == ContentType::HTML)
            {
                prettyPrintJson(res);
            }
            else if (prefered == ContentType::CBOR)
            {
                res.addHeader(boost::beast::http::field::content_type,
                              "application/cbor");
                nlohmann::json::to_cbor(res.jsonValue, res.body());
            }
            else
            {
                // Technically prefered could also be NoMatch here, but we'd
                // like to default to something rather than return 400 for
                // backward compatibility.
                res.addHeader(boost::beast::http::field::content_type,
                              "application/json");
                res.body() = res.jsonValue.dump(
                    2, ' ', true, nlohmann::json::error_handler_t::replace);
            }
        }

        if (res.resultInt() >= 400 && res.body().empty())
        {
            res.body() = std::string(res.reason());
        }

        if (res.result() == boost::beast::http::status::no_content)
        {
            // Boost beast throws if content is provided on a no-content
            // response.  Ideally, this would never happen, but in the case that
            // it does, we don't want to throw.
            BMCWEB_LOG_CRITICAL
                << this << " Response content provided but code was no-content";
            res.body().clear();
        }

        res.addHeader(boost::beast::http::field::date, getCachedDateStr());

        doWrite(res);

        // delete lambda with self shared_ptr
        // to enable connection destruction
        res.setCompleteRequestHandler(nullptr);
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
        BMCWEB_LOG_DEBUG << "Fetch the client IP address";
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

  private:
    void doReadHeaders()
    {
        BMCWEB_LOG_DEBUG << this << " doReadHeaders";

        // Clean up any previous Connection.
        boost::beast::http::async_read_header(
            adaptor, buffer, *parser,
            [this,
             self(shared_from_this())](const boost::system::error_code& ec,
                                       std::size_t bytesTransferred) {
            BMCWEB_LOG_DEBUG << this << " async_read_header "
                             << bytesTransferred << " Bytes";
            bool errorWhileReading = false;
            if (ec)
            {
                errorWhileReading = true;
                if (ec == boost::beast::http::error::end_of_stream)
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
#ifndef BMCWEB_INSECURE_DISABLE_AUTHX
            boost::beast::http::verb method = parser->get().method();
            userSession = crow::authentication::authenticate(
                ip, res, method, parser->get().base(), mtlsSession);

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
        BMCWEB_LOG_DEBUG << this << " doRead";
        startDeadline();
        boost::beast::http::async_read_some(
            adaptor, buffer, *parser,
            [this,
             self(shared_from_this())](const boost::system::error_code& ec,
                                       std::size_t bytesTransferred) {
            BMCWEB_LOG_DEBUG << this << " async_read_some " << bytesTransferred
                             << " Bytes";

            if (ec)
            {
                BMCWEB_LOG_ERROR << this
                                 << " Error while reading: " << ec.message();
                close();
                BMCWEB_LOG_DEBUG << this << " from read(1)";
                return;
            }

            // If the user is logged in, allow them to send files incrementally
            // one piece at a time. If authentication is disabled then there is
            // no user session hence always allow to send one piece at a time.
            if (userSession != nullptr
#ifdef BMCWEB_INSECURE_DISABLE_AUTHX
                || true
#endif // BMCWEB_INSECURE_DISABLE_AUTHX
            )
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

    void doWrite(crow::Response& thisRes)
    {
        BMCWEB_LOG_DEBUG << this << " doWrite";
        thisRes.preparePayload();
        serializer.emplace(*thisRes.stringResponse);
        startDeadline();
        boost::beast::http::async_write(adaptor, *serializer,
                                        [this, self(shared_from_this())](
                                            const boost::system::error_code& ec,
                                            std::size_t bytesTransferred) {
            BMCWEB_LOG_DEBUG << this << " async_write " << bytesTransferred
                             << " bytes";

            cancelDeadlineTimer();

            if (ec)
            {
                BMCWEB_LOG_DEBUG << this << " from write(2)";
                return;
            }
            if (!keepAlive)
            {
                close();
                BMCWEB_LOG_DEBUG << this << " from write(1)";
                return;
            }

            serializer.reset();
            BMCWEB_LOG_DEBUG << this << " Clearing response";
            res.clear();
            parser.emplace(std::piecewise_construct, std::make_tuple());
            parser->body_limit(httpReqBodyLimit); // reset body limit for
                                                  // newly created parser
            buffer.consume(buffer.size());

            userSession = nullptr;

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
        // Timer is already started so no further action is required.
        if (timerStarted)
        {
            return;
        }

        std::chrono::seconds timeout(15);

        std::weak_ptr<Connection<Adaptor, Handler>> weakSelf = weak_from_this();
        timer.expires_after(timeout);
        timer.async_wait([weakSelf](const boost::system::error_code ec) {
            // Note, we are ignoring other types of errors here;  If the timer
            // failed for any reason, we should still close the connection
            std::shared_ptr<Connection<Adaptor, Handler>> self =
                weakSelf.lock();
            if (!self)
            {
                BMCWEB_LOG_CRITICAL << self << " Failed to capture connection";
                return;
            }

            self->timerStarted = false;

            if (ec == boost::asio::error::operation_aborted)
            {
                // Canceled wait means the path succeeeded.
                return;
            }
            if (ec)
            {
                BMCWEB_LOG_CRITICAL << self << " timer failed " << ec;
            }

            BMCWEB_LOG_WARNING << self << "Connection timed out, closing";

            self->close();
        });

        timerStarted = true;
        BMCWEB_LOG_DEBUG << this << " timer started";
    }

    Adaptor adaptor;
    Handler* handler;
    // Making this a std::optional allows it to be efficiently destroyed and
    // re-created on Connection reset
    std::optional<
        boost::beast::http::request_parser<boost::beast::http::string_body>>
        parser;

    boost::beast::flat_static_buffer<8192> buffer;

    std::optional<boost::beast::http::response_serializer<
        boost::beast::http::string_body>>
        serializer;

    std::optional<crow::Request> req;
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
