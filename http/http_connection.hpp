#pragma once
#include "bmcweb_config.h"

#include "authorization.hpp"
#include "http_connect_types.hpp"
#include "http_response.hpp"
#include "http_utility.hpp"
#include "logging.hpp"
#include "timer_queue.hpp"
#include "utility.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core/detect_ssl.hpp>
#include <boost/beast/core/flat_static_buffer.hpp>
#include <boost/beast/core/stream_traits.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/beast/websocket.hpp>
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

#ifdef BMCWEB_ENABLE_DEBUG
static std::atomic<int> connectionCount;
#endif

// request body limit size set by the bmcwebHttpReqBodyLimitMb option
constexpr unsigned int httpReqBodyLimit =
    1024 * 1024 * bmcwebHttpReqBodyLimitMb;

constexpr uint64_t loggedOutPostBodyLimit = 4096;

constexpr uint32_t httpHeaderLimit = 8192;

// drop all connections after 1 minute, this time limit was chosen
// arbitrarily and can be adjusted later if needed
static constexpr const size_t loggedInAttempts =
    (60 / timerQueueTimeoutSeconds);

static constexpr const size_t loggedOutAttempts =
    (15 / timerQueueTimeoutSeconds);

template <typename Adaptor, typename Handler>
class Connection :
    public std::enable_shared_from_this<Connection<Adaptor, Handler>>
{
  public:
    Connection(Handler* handlerIn,
               std::function<std::string()>& getCachedDateStrF,
               detail::TimerQueue& timerQueueIn,
               const std::shared_ptr<boost::asio::ssl::context>& sslContextIn,
               boost::asio::io_context& ioc, HttpType httpTypeIn) :
        adaptor(std::in_place_type<Adaptor>, ioc),
        sslContext(sslContextIn), handler(handlerIn),
        getCachedDateStr(getCachedDateStrF), timerQueue(timerQueueIn),
        httpType(httpTypeIn)
    {
        parser.emplace(std::piecewise_construct, std::make_tuple());
        parser->body_limit(httpReqBodyLimit);
        parser->header_limit(httpHeaderLimit);
        req.emplace(parser->get());

#ifdef BMCWEB_ENABLE_DEBUG
        connectionCount++;
        BMCWEB_LOG_DEBUG << this << " Connection open, total "
                         << connectionCount;
#endif
    }

    void prepareMutualTls()
    {
#ifdef BMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION
        boost::beast::ssl_stream<Adaptor>* sslAdaptor =
            std::get_if<boost::beast::ssl_stream<Adaptor>>(&adaptor);
        if (sslAdaptor)
        {
            auto caAvailable = !std::filesystem::is_empty(
                std::filesystem::path(ensuressl::trustStorePath));
            if (caAvailable && persistent_data::SessionStore::getInstance()
                                   .getAuthMethodsConfig()
                                   .tls)
            {
                sslAdaptor->set_verify_mode(boost::asio::ssl::verify_peer);
                std::string id = "bmcweb";
                int ret = SSL_set_session_id_context(
                    sslAdaptor->native_handle(),
                    reinterpret_cast<const unsigned char*>(id.c_str()),
                    static_cast<unsigned int>(id.length()));
                if (ret == 0)
                {
                    BMCWEB_LOG_ERROR << this << " failed to set SSL id";
                }
            }

            sslAdaptor->set_verify_callback(
                [this](bool preverified,
                       boost::asio::ssl::verify_context& ctx) {
                    // do nothing if TLS is disabled
                    if (!persistent_data::SessionStore::getInstance()
                             .getAuthMethodsConfig()
                             .tls)
                    {
                        BMCWEB_LOG_DEBUG << this
                                         << " TLS auth_config is disabled";
                        return true;
                    }

                    // We always return true to allow full auth flow
                    if (!preverified)
                    {
                        BMCWEB_LOG_DEBUG << this
                                         << " TLS preverification failed.";
                        return true;
                    }

                    X509_STORE_CTX* cts = ctx.native_handle();
                    if (cts == nullptr)
                    {
                        BMCWEB_LOG_DEBUG << this
                                         << " Cannot get native TLS handle.";
                        return true;
                    }

                    // Get certificate
                    X509* peerCert =
                        X509_STORE_CTX_get_current_cert(ctx.native_handle());
                    if (peerCert == nullptr)
                    {
                        BMCWEB_LOG_DEBUG
                            << this << " Cannot get current TLS certificate.";
                        return true;
                    }

                    // Check if certificate is OK
                    int error = X509_STORE_CTX_get_error(cts);
                    if (error != X509_V_OK)
                    {
                        BMCWEB_LOG_INFO << this
                                        << " Last TLS error is: " << error;
                        return true;
                    }
                    // Check that we have reached final certificate in chain
                    int32_t depth = X509_STORE_CTX_get_error_depth(cts);
                    if (depth != 0)

                    {
                        BMCWEB_LOG_DEBUG
                            << this
                            << " Certificate verification in progress (depth "
                            << depth << "), waiting to reach final depth";
                        return true;
                    }

                    BMCWEB_LOG_DEBUG
                        << this << " Certificate verification of final depth";

                    // Verify KeyUsage
                    bool isKeyUsageDigitalSignature = false;
                    bool isKeyUsageKeyAgreement = false;

                    ASN1_BIT_STRING* usage =
                        static_cast<ASN1_BIT_STRING*>(X509_get_ext_d2i(
                            peerCert, NID_key_usage, nullptr, nullptr));

                    if (usage == nullptr)
                    {
                        BMCWEB_LOG_DEBUG << this << " TLS usage is null";
                        return true;
                    }

                    for (int i = 0; i < usage->length; i++)
                    {
                        if (KU_DIGITAL_SIGNATURE & usage->data[i])
                        {
                            isKeyUsageDigitalSignature = true;
                        }
                        if (KU_KEY_AGREEMENT & usage->data[i])
                        {
                            isKeyUsageKeyAgreement = true;
                        }
                    }

                    if (!isKeyUsageDigitalSignature || !isKeyUsageKeyAgreement)
                    {
                        BMCWEB_LOG_DEBUG
                            << this
                            << " Certificate ExtendedKeyUsage does "
                               "not allow provided certificate to "
                               "be used for user authentication";
                        return true;
                    }
                    ASN1_BIT_STRING_free(usage);

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
                        if (NID_client_auth ==
                            OBJ_obj2nid(sk_ASN1_OBJECT_value(extUsage, i)))
                        {
                            isExKeyUsageClientAuth = true;
                            break;
                        }
                    }
                    sk_ASN1_OBJECT_free(extUsage);

                    // Certificate has to have proper key usages set
                    if (!isExKeyUsageClientAuth)
                    {
                        BMCWEB_LOG_DEBUG
                            << this
                            << " Certificate ExtendedKeyUsage does "
                               "not allow provided certificate to "
                               "be used for user authentication";
                        return true;
                    }
                    std::string sslUser;
                    // Extract username contained in CommonName
                    sslUser.resize(256, '\0');

                    int status = X509_NAME_get_text_by_NID(
                        X509_get_subject_name(peerCert), NID_commonName,
                        sslUser.data(), static_cast<int>(sslUser.size()));

                    if (status == -1)
                    {
                        BMCWEB_LOG_DEBUG
                            << this
                            << " TLS cannot get username to create session";
                        return true;
                    }

                    size_t lastChar = sslUser.find('\0');
                    if (lastChar == std::string::npos || lastChar == 0)
                    {
                        BMCWEB_LOG_DEBUG << this << " Invalid TLS user name";
                        return true;
                    }
                    sslUser.resize(lastChar);
                    std::string unsupportedClientId = "";
            session = persistent_data::SessionStore::getInstance()
                          .generateUserSession(
                              sslUser, req->ipAddress.to_string(),
                              unsupportedClientId,
                              persistent_data::PersistenceType::TIMEOUT);
                    if (auto sp = session.lock())
                    {
                        BMCWEB_LOG_DEBUG
                            << this
                            << " Generating TLS session: " << sp->uniqueId;
                    }
                    return true;
                });
        }
#endif // BMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION
    }

    ~Connection()
    {
        res.completeRequestHandler = nullptr;
        cancelDeadlineTimer();
#ifdef BMCWEB_ENABLE_DEBUG
        connectionCount--;
        BMCWEB_LOG_DEBUG << this << " Connection closed, total "
                         << connectionCount;
#endif
    }

    Adaptor& socket()

    {
        return std::visit(
            [](auto& thisAdaptor) -> Adaptor& {
                return boost::beast::get_lowest_layer(thisAdaptor);
            },
            adaptor);
    }

    void afterDetectSsl(bool isSsl)
    {
        req->isSecure = isSsl;
        if (isSsl)
        {
            doHandshake();
        }
        else
        {
            BMCWEB_LOG_INFO << "Starting non-SSL session";
            doReadHeaders();
        }
    }

    void doHandshake()
    {
        if (sslContext == nullptr)
        {
            BMCWEB_LOG_ERROR
                << "Attempted to start SSL session without SSL context";
            return;
        }
        BMCWEB_LOG_INFO << "Starting SSL session";

        Adaptor* thisAdaptor = std::get_if<Adaptor>(&adaptor);
        if (thisAdaptor == nullptr)
        {
            BMCWEB_LOG_CRITICAL << "Adaptor was ssl before handshake?";
            return;
        }
        Adaptor tempAdaptor = std::move(*thisAdaptor);
        using SslType = boost::beast::ssl_stream<Adaptor>;
        SslType& sslAdaptor = adaptor.template emplace<SslType>(
            std::move(tempAdaptor), *sslContext);

        prepareMutualTls();

        sslAdaptor.async_handshake(
            boost::asio::ssl::stream_base::server, buffer.data(),
            [self(shared_from_this())](boost::system::error_code ec,
                                       size_t bytesParsed) {
                self->buffer.consume(bytesParsed);
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Failed to start handshake " << ec;
                    return;
                }
                self->doReadHeaders();
            });
    }

    void start()
    {
        startDeadline(0);
        BMCWEB_LOG_DEBUG << "Starting ssl detect";

        Adaptor* thisAdaptor = std::get_if<Adaptor>(&adaptor);
        if (thisAdaptor == nullptr)
        {
            BMCWEB_LOG_CRITICAL << "Adapter wasn't base type at start?????";
            return;
        }
        boost::beast::async_detect_ssl(
            *thisAdaptor, buffer,
            [self(shared_from_this())](boost::beast::error_code ec,
                                       bool isSsl) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Couldn't detect ssl " << ec;
                    return;
                }

                self->afterDetectSsl(isSsl);
            });
        return;
    }

    void handle()
    {
        cancelDeadlineTimer();

        // Check for HTTP version 1.1.
        if (req->version() == 11)
        {
            if (req->getHeaderValue(boost::beast::http::field::host).empty())
            {
                res.result(boost::beast::http::status::bad_request);
                completeRequest();
                return;
            }
        }

        BMCWEB_LOG_INFO << "Request: "
                        << " " << this << " HTTP/" << req->version() / 10 << "."
                        << req->version() % 10 << ' ' << req->methodString()
                        << " " << req->target() << " " << req->ipAddress;

        // If this isn't an SSL connection, and this isn't explicitly and HTTP
        // connection redirect to HTTPS
        if (req->isSecure)
        {
            if (httpType == HttpType::HTTP)
            {
                // If we got an SSL connection on a non SSL port, reject it.
                return;
            }
        }
        else
        {
            if (httpType == HttpType::HTTPS)
            {
                // If this is HTTPS and we got an HTTP connection, close the
                // connection immediately.
                return;
            }
            // If this is anything other than a raw HTTP connection,
            // Redirect to HTTPS
            if (httpType != HttpType::HTTP)
            {
                res.completeRequestHandler = [] {};
                res.isAliveHelper = [this]() -> bool { return isAlive(); };

                // We don't redirect anything except a GET request to /.  This
                // is for security,
                std::string_view host =
                    req->getHeaderValue(boost::beast::http::field::host);
                if ((req->method() != boost::beast::http::verb::get &&
                     req->method() != boost::beast::http::verb::head) ||
                    host.empty())
                {
                    res.result(boost::beast::http::status::not_found);
                    completeRequest();
                    return;
                }
                // if this is not an ssl stream, we need to redirect
                res.result(boost::beast::http::status::moved_permanently);
                res.addHeader(boost::beast::http::field::location,
                              "https://" + std::string(host));
                completeRequest();
                return;
            }
        }

        BMCWEB_LOG_INFO << "Request: "
                        << " " << this << " HTTP/" << req->version() / 10 << "."
                        << req->version() % 10 << ' ' << req->methodString()
                        << " " << req->target();

        res.isAliveHelper = [this]() -> bool { return isAlive(); };

        req->ioService = static_cast<boost::asio::io_context*>(
            &socket().get_executor().context());

        res.completeRequestHandler = [self{shared_from_this()}] {
            self->completeRequest();
        };

        if (req->isUpgrade() &&
            boost::iequals(
                req->getHeaderValue(boost::beast::http::field::upgrade),
                "websocket"))
        {
            // only allow upgrade if request is SSL
            boost::beast::ssl_stream<Adaptor>* sslStream =
                std::get_if<boost::beast::ssl_stream<Adaptor>>(&adaptor);
            if (sslStream != nullptr)
            {
                handler
                    ->template handleUpgrade<boost::beast::ssl_stream<Adaptor>>(
                        *req, res, std::move(*sslStream));
            }
            // delete lambda with self shared_ptr
            // to enable connection destruction
            res.completeRequestHandler = nullptr;
            return;
        }
        handler->handle(*req, res);
    }

    bool isAlive()
    {
        return socket().is_open();
    }

    void close()
    {
        if (std::holds_alternative<boost::beast::ssl_stream<Adaptor>>(adaptor))
        {
#ifdef BMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION
            if (auto sp = session.lock())
            {
                BMCWEB_LOG_DEBUG << this
                                 << " Removing TLS session: " << sp->uniqueId;
                persistent_data::SessionStore::getInstance().removeSession(sp);
            }
#endif // BMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION
        }
        socket().close();
    }
    void completeRequest()
    {
        BMCWEB_LOG_INFO << "Response: " << this << ' ' << req->url << ' '
                        << res.resultInt() << " keepalive=" << req->keepAlive();

        addSecurityHeaders(*req, res);

        crow::authorization::cleanupTempSession(*req);

        if (!isAlive())
        {
            // delete lambda with self shared_ptr
            // to enable connection destruction
            res.completeRequestHandler = nullptr;
            return;
        }
        if (res.body().empty() && !res.jsonValue.empty())
        {
            if (http_helpers::requestPrefersHtml(*req))
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

        // Allow keepalive for secure connections only
        if (std::holds_alternative<Adaptor>(adaptor))
        {
            res.keepAlive(false);
        }
        else
        {
            res.keepAlive(req->keepAlive());
        }
        doWrite();

        // delete lambda with self shared_ptr
        // to enable connection destruction
        res.completeRequestHandler = nullptr;
    }

    void readClientIp()
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
            return;
        }

        req->ipAddress = endpoint.address();
    }

  private:
    void doReadHeaders()
    {
        BMCWEB_LOG_DEBUG << this << " doReadHeaders";

        auto callback = [this, self(shared_from_this())](
                            const boost::system::error_code ec,
                            std::size_t bytesTransferred) {
            BMCWEB_LOG_ERROR << this << " async_read_header "
                             << bytesTransferred << " Bytes";
            if (ec)
            {
                BMCWEB_LOG_ERROR << this
                                 << " Error while reading: " << ec.message();
                close();
                return;
            }
            cancelDeadlineTimer();

            // if the adaptor isn't open anymore, and wasn't handed to a
            // websocket, treat as an error
            if (!isAlive() && !req->isUpgrade())
            {
                close();
                BMCWEB_LOG_DEBUG << this << " from read(1)";
                return;
            }

            if (!req)
            {
                close();
                return;
            }

            // Note, despite the bmcweb coding policy on use of exceptions
            // for error handling, this one particular use of exceptions is
            // deemed acceptable, as it solved a significant error handling
            // problem that resulted in seg faults, the exact thing that the
            // exceptions rule is trying to avoid. If at some point,
            // boost::urls makes the parser object public (or we port it
            // into bmcweb locally) this will be replaced with
            // parser::parse, which returns a status code

            try
            {
                req->urlView = boost::urls::url_view(req->target());
                req->url = req->urlView.encoded_path();
                req->urlParams = req->urlView.params();
            }
            catch (std::exception& p)
            {
                BMCWEB_LOG_ERROR << p.what();
            }

            crow::authorization::authenticate(*req, res, session);

            bool loggedIn = req && req->session;
            if (loggedIn)
            {
                startDeadline(loggedInAttempts);
                BMCWEB_LOG_DEBUG << "Starting slow deadline";
            }
            else
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

                startDeadline(loggedOutAttempts);
                BMCWEB_LOG_DEBUG << "Starting quick deadline";
            }
            doRead();
        };

        std::visit(
            [this, callback(std::move(callback))](auto& thisAdaptor) {
                boost::beast::http::async_read_header(
                    thisAdaptor, buffer, *parser, std::move(callback));
            },
            adaptor);
    }

    void doRead()
    {
        BMCWEB_LOG_DEBUG << this << " doRead";

        auto callback =
            [this, self(shared_from_this())](const boost::system::error_code ec,
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

                if (!isAlive())
                {
                    close();
                    BMCWEB_LOG_DEBUG << this << " from read(1)";
                    return;
                }

                bool loggedIn = req && req->session;
                if (loggedIn)
                {
                    startDeadline(loggedInAttempts);
                }
                else
                {
                    startDeadline(loggedOutAttempts);
                }

                handle();
            };
        std::visit(
            [this, callback(std::move(callback))](auto& thisAdaptor) {
                boost::beast::http::async_read(thisAdaptor, buffer, *parser,
                                               std::move(callback));
            },
            adaptor);
    }

    void doWrite()
    {
        bool loggedIn = req && req->session;
        if (loggedIn)
        {
            startDeadline(loggedInAttempts);
        }
        else
        {
            startDeadline(loggedOutAttempts);
        }
        BMCWEB_LOG_DEBUG << this << " doWrite";
        res.preparePayload();
        serializer.emplace(*res.stringResponse);
        auto callback =
            [this, self(shared_from_this())](const boost::system::error_code ec,
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

                req.emplace(parser->get());
                req->isSecure =
                    std::holds_alternative<boost::beast::ssl_stream<Adaptor>>(
                        adaptor);
                doReadHeaders();
            };
        std::visit(
            [this, callback(std::move(callback))](auto& thisAdaptor) {
                boost::beast::http::async_write(thisAdaptor, *serializer,
                                                std::move(callback));
            },
            adaptor);
    }

    void cancelDeadlineTimer()
    {
        if (timerCancelKey)
        {
            BMCWEB_LOG_DEBUG << this << " timer cancelled: " << &timerQueue
                             << ' ' << *timerCancelKey;
            timerQueue.cancel(*timerCancelKey);
            timerCancelKey.reset();
        }
    }

    void startDeadline(size_t timerIterations)
    {
        cancelDeadlineTimer();

        if (timerIterations)
        {
            timerIterations--;
        }

        timerCancelKey =
            timerQueue.add([self(shared_from_this()), timerIterations,
                            readCount{parser->get().body().size()}] {
                // Mark timer as not active to avoid canceling it during
                // Connection destructor which leads to double free issue
                self->timerCancelKey.reset();
                if (!self->isAlive())
                {
                    return;
                }

                bool loggedIn = self->req && self->req->session;
                // allow slow uploads for logged in users
                if (loggedIn && self->parser->get().body().size() > readCount)
                {
                    BMCWEB_LOG_DEBUG << self.get()
                                     << " restart timer - read in progress";
                    self->startDeadline(timerIterations);
                    return;
                }

                // Threshold can be used to drop slow connections
                // to protect against slow-rate DoS attack
                if (timerIterations)
                {
                    BMCWEB_LOG_DEBUG << self.get() << " restart timer";
                    self->startDeadline(timerIterations);
                    return;
                }

                self->close();
            });

        if (!timerCancelKey)
        {
            close();
            return;
        }
        BMCWEB_LOG_DEBUG << this << " timer added: " << &timerQueue << ' '
                         << *timerCancelKey;
    }

  private:
    std::variant<Adaptor, boost::beast::ssl_stream<Adaptor>> adaptor;
    std::shared_ptr<boost::asio::ssl::context> sslContext;

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

    std::weak_ptr<persistent_data::UserSession> session;

    std::optional<size_t> timerCancelKey;

    std::function<std::string()>& getCachedDateStr;
    detail::TimerQueue& timerQueue;

    HttpType httpType;

    using std::enable_shared_from_this<
        Connection<Adaptor, Handler>>::shared_from_this;
};
} // namespace crow
