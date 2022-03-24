#pragma once
#include "bmcweb_config.h"

#include "authorization.hpp"
#include "http_response.hpp"
#include "http_utility.hpp"
#include "logging.hpp"
#include "utility.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core/flat_static_buffer.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/url/url_view.hpp>
#include <json_html_serializer.hpp>
#include <security_headers.hpp>
#include <ssl_key_handler.hpp>

#include <atomic>
#include <chrono>
#include <vector>

namespace crow
{

inline void prettyPrintJson(crow::Response& res)
{
    json_html_util::dumpHtml(res.body(), res.jsonValue);

    res.addHeader("Content-Type", "text/html;charset=UTF-8");
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

        adaptor.set_verify_callback([this](
                                        bool preverified,
                                        boost::asio::ssl::verify_context& ctx) {
            // do nothing if TLS is disabled
            if (!persistent_data::SessionStore::getInstance()
                     .getAuthMethodsConfig()
                     .tls)
            {
                BMCWEB_LOG_DEBUG << this << " TLS auth_config is disabled";
                return true;
            }

            // We always return true to allow full auth flow
            if (!preverified)
            {
                BMCWEB_LOG_DEBUG << this << " TLS preverification failed.";
                return true;
            }

            X509_STORE_CTX* cts = ctx.native_handle();
            if (cts == nullptr)
            {
                BMCWEB_LOG_DEBUG << this << " Cannot get native TLS handle.";
                return true;
            }

            // Get certificate
            X509* peerCert =
                X509_STORE_CTX_get_current_cert(ctx.native_handle());
            if (peerCert == nullptr)
            {
                BMCWEB_LOG_DEBUG << this
                                 << " Cannot get current TLS certificate.";
                return true;
            }

            // Check if certificate is OK
            int error = X509_STORE_CTX_get_error(cts);
            if (error != X509_V_OK)
            {
                BMCWEB_LOG_INFO << this << " Last TLS error is: " << error;
                return true;
            }
            // Check that we have reached final certificate in chain
            int32_t depth = X509_STORE_CTX_get_error_depth(cts);
            if (depth != 0)

            {
                BMCWEB_LOG_DEBUG
                    << this << " Certificate verification in progress (depth "
                    << depth << "), waiting to reach final depth";
                return true;
            }

            BMCWEB_LOG_DEBUG << this
                             << " Certificate verification of final depth";

            // Verify KeyUsage
            bool isKeyUsageDigitalSignature = false;
            bool isKeyUsageKeyAgreement = false;

            ASN1_BIT_STRING* usage = static_cast<ASN1_BIT_STRING*>(
                X509_get_ext_d2i(peerCert, NID_key_usage, nullptr, nullptr));

            if (usage == nullptr)
            {
                BMCWEB_LOG_DEBUG << this << " TLS usage is null";
                return true;
            }

            for (int i = 0; i < usage->length; i++)
            {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                unsigned char usageChar = usage->data[i];
                if (KU_DIGITAL_SIGNATURE & usageChar)
                {
                    isKeyUsageDigitalSignature = true;
                }
                if (KU_KEY_AGREEMENT & usageChar)
                {
                    isKeyUsageKeyAgreement = true;
                }
            }
            ASN1_BIT_STRING_free(usage);

            if (!isKeyUsageDigitalSignature || !isKeyUsageKeyAgreement)
            {
                BMCWEB_LOG_DEBUG << this
                                 << " Certificate ExtendedKeyUsage does "
                                    "not allow provided certificate to "
                                    "be used for user authentication";
                return true;
            }

            // Determine that ExtendedKeyUsage includes Client Auth

            stack_st_ASN1_OBJECT* extUsage =
                static_cast<stack_st_ASN1_OBJECT*>(X509_get_ext_d2i(
                    peerCert, NID_ext_key_usage, nullptr, nullptr));

            if (extUsage == nullptr)
            {
                BMCWEB_LOG_DEBUG << this << " TLS extUsage is null";
                return true;
            }

            bool isExKeyUsageClientAuth = false;
            for (int i = 0; i < sk_ASN1_OBJECT_num(extUsage); i++)
            {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
                int nid = OBJ_obj2nid(sk_ASN1_OBJECT_value(extUsage, i));
                if (NID_client_auth == nid)
                {
                    isExKeyUsageClientAuth = true;
                    break;
                }
            }
            sk_ASN1_OBJECT_free(extUsage);

            // Certificate has to have proper key usages set
            if (!isExKeyUsageClientAuth)
            {
                BMCWEB_LOG_DEBUG << this
                                 << " Certificate ExtendedKeyUsage does "
                                    "not allow provided certificate to "
                                    "be used for user authentication";
                return true;
            }
            std::string sslUser;
            // Extract username contained in CommonName
            sslUser.resize(256, '\0');

            int status = X509_NAME_get_text_by_NID(
                X509_get_subject_name(peerCert), NID_commonName, sslUser.data(),
                static_cast<int>(sslUser.size()));

            if (status == -1)
            {
                BMCWEB_LOG_DEBUG
                    << this << " TLS cannot get username to create session";
                return true;
            }

            size_t lastChar = sslUser.find('\0');
            if (lastChar == std::string::npos || lastChar == 0)
            {
                BMCWEB_LOG_DEBUG << this << " Invalid TLS user name";
                return true;
            }
            sslUser.resize(lastChar);
            std::string unsupportedClientId;
            sessionIsFromTransport = true;
            userSession = persistent_data::SessionStore::getInstance()
                              .generateUserSession(
                                  sslUser, req->ipAddress, unsupportedClientId,
                                  persistent_data::PersistenceType::TIMEOUT);
            if (userSession != nullptr)
            {
                BMCWEB_LOG_DEBUG
                    << this
                    << " Generating TLS session: " << userSession->uniqueId;
            }
            return true;
        });
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
#ifndef BMCWEB_INSECURE_DISABLE_AUTHENTICATION
        if (!crow::authorization::isOnAllowlist(req->url, req->method()) &&
            thisReq.session == nullptr)
        {
            BMCWEB_LOG_WARNING << "Authentication failed";
            forward_unauthorized::sendUnauthorized(
                req->url, req->getHeaderValue("User-Agent"),
                req->getHeaderValue("Accept"), res);
            completeRequest(res);
            return;
        }
#endif // BMCWEB_INSECURE_DISABLE_AUTHENTICATION
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

    void completeRequest(crow::Response& thisRes)
    {
        if (!req)
        {
            return;
        }
        res = std::move(thisRes);
        BMCWEB_LOG_INFO << "Response: " << this << ' ' << req->url << ' '
                        << res.resultInt() << " keepalive=" << req->keepAlive();

        addSecurityHeaders(*req, res);

        crow::authorization::cleanupTempSession(*req);

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
        if (res.body().empty() && !res.jsonValue.empty())
        {
            if (http_helpers::requestPrefersHtml(req->getHeaderValue("Accept")))
            {
                prettyPrintJson(res);
            }
            else
            {
                res.jsonMode();
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

        res.keepAlive(req->keepAlive());

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
#ifndef BMCWEB_INSECURE_DISABLE_AUTHENTICATION
                boost::beast::http::verb method = parser->get().method();
                userSession = crow::authorization::authenticate(
                    ip, res, method, parser->get().base(), userSession);

                bool loggedIn = userSession != nullptr;
                if (!loggedIn)
                {
                    const boost::optional<uint64_t> contentLength =
                        parser->content_length();
                    if (contentLength &&
                        *contentLength > loggedOutPostBodyLimit)
                    {
                        BMCWEB_LOG_DEBUG << "Content length greater than limit "
                                         << *contentLength;
                        close();
                        return;
                    }

                    BMCWEB_LOG_DEBUG << "Starting quick deadline";
                }
#endif // BMCWEB_INSECURE_DISABLE_AUTHENTICATION

                doRead();
            });
    }

    void doRead()
    {
        BMCWEB_LOG_DEBUG << this << " doRead";
        startDeadline();
        boost::beast::http::async_read(
            adaptor, buffer, *parser,
            [this,
             self(shared_from_this())](const boost::system::error_code& ec,
                                       std::size_t bytesTransferred) {
                BMCWEB_LOG_DEBUG << this << " async_read " << bytesTransferred
                                 << " Bytes";
                cancelDeadlineTimer();
                if (ec)
                {
                    BMCWEB_LOG_ERROR
                        << this << " Error while reading: " << ec.message();
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
        boost::beast::http::async_write(
            adaptor, *serializer,
            [this,
             self(shared_from_this())](const boost::system::error_code& ec,
                                       std::size_t bytesTransferred) {
                BMCWEB_LOG_DEBUG << this << " async_write " << bytesTransferred
                                 << " bytes";

                cancelDeadlineTimer();

                if (ec)
                {
                    BMCWEB_LOG_DEBUG << this << " from write(2)";
                    return;
                }
                if (!res.keepAlive())
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

            std::shared_ptr<Connection<Adaptor, Handler>> self =
                weakSelf.lock();
            if (!self)
            {
                BMCWEB_LOG_CRITICAL << self << " Failed to capture connection";
                return;
            }
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
