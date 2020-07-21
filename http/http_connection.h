#pragma once
#include "config.h"

#include "http_response.h"
#include "logging.h"
#include "multipart_parser.h"
#include "timer_queue.h"
#include "utility.h"

#include "authorization.hpp"
#include "http_utility.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core/detect_ssl.hpp>
#include <boost/beast/core/flat_static_buffer.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/beast/websocket.hpp>
#include <security_headers.hpp>
#include <ssl_key_handler.hpp>

#include <atomic>
#include <chrono>
#include <vector>

namespace crow
{

inline void prettyPrintJson(crow::Response& res)
{
    std::string value = res.jsonValue.dump(4, ' ', true);
    utility::escapeHtml(value);
    utility::convertToLinks(value);
    res.body() = "<html>\n"
                 "<head>\n"
                 "<title>Redfish API</title>\n"
                 "<link rel=\"stylesheet\" type=\"text/css\" "
                 "href=\"/styles/default.css\">\n"
                 "<script src=\"/highlight.pack.js\"></script>"
                 "<script>hljs.initHighlightingOnLoad();</script>"
                 "</head>\n"
                 "<body>\n"
                 "<div style=\"max-width: 576px;margin:0 auto;\">\n"
                 "<img src=\"/DMTF_Redfish_logo_2017.svg\" alt=\"redfish\" "
                 "height=\"406px\" "
                 "width=\"576px\">\n"
                 "<br>\n"
                 "<pre>\n"
                 "<code class=\"json\">" +
                 value +
                 "</code>\n"
                 "</pre>\n"
                 "</div>\n"
                 "</body>\n"
                 "</html>\n";
    res.addHeader("Content-Type", "text/html;charset=UTF-8");
}

#ifdef BMCWEB_ENABLE_DEBUG
static std::atomic<int> connectionCount;
#endif

// request body limit size set by the BMCWEB_HTTP_REQ_BODY_LIMIT_MB option
constexpr unsigned int httpReqBodyLimit =
    1024 * 1024 * BMCWEB_HTTP_REQ_BODY_LIMIT_MB;

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
    Connection(Handler* handlerIn, const std::string& ServerNameIn,
               std::function<std::string()>& get_cached_date_str_f,
               detail::TimerQueue& timerQueueIn,
               std::shared_ptr<boost::asio::ssl::context> sslContextIn,
               boost::asio::io_context& ioc) :
        adaptor(ioc),
        sslContext(sslContextIn), handler(handlerIn), serverName(ServerNameIn),
        getCachedDateStr(get_cached_date_str_f), timerQueue(timerQueueIn)
    {
        parser.emplace(std::piecewise_construct, std::make_tuple());
        parser->body_limit(httpReqBodyLimit);
        parser->header_limit(httpHeaderLimit);
        req.emplace(parser->get());

#ifdef BMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION
        if (sslStream)
        {
            auto ca_available = !std::filesystem::is_empty(
                std::filesystem::path(ensuressl::trustStorePath));
            if (ca_available && persistent_data::SessionStore::getInstance()
                                    .getAuthMethodsConfig()
                                    .tls)
            {
                sslStream->set_verify_mode(boost::asio::ssl::verify_peer);
                SSL_set_session_id_context(
                    sslStream->native_handle(),
                    reinterpret_cast<const unsigned char*>(serverName.c_str()),
                    static_cast<unsigned int>(serverName.length()));
                BMCWEB_LOG_DEBUG << this
                                 << " TLS is enabled on this connection.";
            }

            sslStream->set_verify_callback(
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

                    session =
                        persistent_data::SessionStore::getInstance()
                            .generateUserSession(
                                sslUser,
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

#ifdef BMCWEB_ENABLE_DEBUG
        connectionCount++;
        BMCWEB_LOG_DEBUG << this << " Connection open, total "
                         << connectionCount;
#endif
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

    Adaptor& base_socket()
    {
        return adaptor;
    }

    void after_detect_ssl(bool isSsl)
    {
        req->isSecure = isSsl;
        if (isSsl)
        {
            if (sslContext == nullptr)
            {
                BMCWEB_LOG_ERROR
                    << "Attempted to start SSL session without SSL context";
                return;
            }
            BMCWEB_LOG_INFO << "Starting SSL session";
            sslStream.emplace(adaptor, *sslContext);
            sslStream->async_handshake(
                boost::asio::ssl::stream_base::server, buffer.data(),
                [self(shared_from_this())](boost::system::error_code ec,
                                           size_t bytes_parsed) {
                    self->buffer.consume(bytes_parsed);
                    if (ec)
                    {
                        BMCWEB_LOG_ERROR << "Failed to start handshake " << ec;
                        return;
                    }
                    self->doReadHeaders();
                });
        }
        else
        {
            BMCWEB_LOG_INFO << "Starting non-SSL session";
            doReadHeaders();
        }
    }

    void start()
    {
        startDeadline(0);
        BMCWEB_LOG_DEBUG << "Starting ssl detect";
        boost::beast::async_detect_ssl(
            adaptor, buffer,
            [self(shared_from_this())](boost::beast::error_code ec,
                                       bool isSsl) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Couldn't detect ssl " << ec;
                    return;
                }

                self->after_detect_ssl(isSsl);
            });
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

        if (!sslStream)
        {

            // We don't redirect anything except a GET request to /.  This is
            // for security,

            std::string_view host =
                req->getHeaderValue(boost::beast::http::field::host);
            if (req->method() != boost::beast::http::verb::get ||
                req->target() != "/" || host.empty())
            {
                res.result(boost::beast::http::status::not_found);
            }
            else
            {
                // if this is not an ssl stream, we need to redirect
                res.result(boost::beast::http::status::moved_permanently);
                std::string_view host =
                    req->getHeaderValue(boost::beast::http::field::host);
                res.addHeader(boost::beast::http::field::location,
                              "https://" + std::string(host));
            }
            completeRequest();
            return;
        }

        BMCWEB_LOG_INFO << "Request: "
                        << " " << this << " HTTP/" << req->version() / 10 << "."
                        << req->version() % 10 << ' ' << req->methodString()
                        << " " << req->target();

        res.completeRequestHandler = [] {};
        res.isAliveHelper = [this]() -> bool { return isAlive(); };

        req->ioService = static_cast<decltype(req->ioService)>(
            &adaptor.get_executor().context());

        res.completeRequestHandler = [self(shared_from_this())] {
            self->completeRequest();
        };
        if (req->isUpgrade() &&
            boost::iequals(
                req->getHeaderValue(boost::beast::http::field::upgrade),
                "websocket"))
        {
            handler->template handleUpgrade<Adaptor>(
                *req, res, std::move(adaptor), std::move(sslStream));
            // delete lambda with self shared_ptr
            // to enable connection destruction
            res.completeRequestHandler = nullptr;
            return;
        }
        std::string_view ct =
            req->getHeaderValue(boost::beast::http::field::content_type);
        if (boost::starts_with(ct, "multipart/form-data;"))
        {
            BMCWEB_LOG_INFO << "Parsing multipart form data\n";

            BMCWEB_LOG_INFO << req->body;

            MultipartParser parser(*req);
            parser.parse();

            if (!parser.succeeded())
            {
                // handle error
                BMCWEB_LOG_ERROR << "mime parse failed "
                                 << parser.getErrorMessage();
                res.result(boost::beast::http::status::bad_request);
                completeRequest();
            }
            BMCWEB_LOG_INFO << "Found " << req->mime_fields.size()
                            << " mime fields";
        }

        handler->handle(*req, res);
    }

    bool isAlive()
    {
        return adaptor.is_open();
    }

    void close()
    {
        if (sslStream)
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
        adaptor.close();
    }

    void completeRequest()
    {
        BMCWEB_LOG_INFO << "Response: " << this << ' ' << req->url << ' '
                        << res.resultInt() << " keepalive=" << req->keepAlive();

        addSecurityHeaders(res);

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
                res.body() = res.jsonValue.dump(2, ' ', true);
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

        res.addHeader(boost::beast::http::field::server, serverName);
        res.addHeader(boost::beast::http::field::date, getCachedDateStr());

        // Allow keepalive for secure connections only
        if (sslStream)
        {
            res.keepAlive(req->keepAlive());
        }
        else
        {
            res.keepAlive(false);
        }
        doWrite();

        // delete lambda with self shared_ptr
        // to enable connection destruction
        res.completeRequestHandler = nullptr;
    }

  private:
    void doReadHeaders()
    {
        BMCWEB_LOG_DEBUG << this << " doReadHeaders";

        auto callback = [this, self(shared_from_this())](
                            const boost::system::error_code& ec,
                            std::size_t bytes_transferred) {
            BMCWEB_LOG_ERROR << this << " async_read_header "
                             << bytes_transferred << " Bytes";
            bool errorWhileReading = false;
            if (ec)
            {
                errorWhileReading = true;
                BMCWEB_LOG_ERROR << this
                                 << " Error while reading: " << ec.message();
            }
            else
            {
                // if the adaptor isn't open anymore, and wasn't handed to a
                // websocket, treat as an error
                if (!isAlive() && !req->isUpgrade())
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

            if (!req)
            {
                close();
                return;
            }

            req->urlView = boost::urls::url_view(req->target());
            req->url = req->urlView.encoded_path();

            crow::authorization::authenticate(*req, res, session);

            bool loggedIn = req && req->session;
            if (loggedIn)
            {
                startDeadline(loggedInAttempts);
                BMCWEB_LOG_DEBUG << "Starting slow deadline";

                req->urlParams = req->urlView.params();

#ifdef BMCWEB_ENABLE_DEBUG
                std::string paramList = "";
                for (const auto param : req->urlParams)
                {
                    paramList += param->key() + " " + param->value() + " ";
                }
                BMCWEB_LOG_DEBUG << "QueryParams: " << paramList;
#endif
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
        if (sslStream)
        {
            boost::beast::http::async_read(*sslStream, buffer, *parser,
                                           std::move(callback));
        }
        else
        {
            boost::beast::http::async_read(adaptor, buffer, *parser,
                                           std::move(callback));
        }
    }

    void doRead()
    {
        BMCWEB_LOG_DEBUG << this << " doRead";

        auto callback = [this, self(shared_from_this())](
                            const boost::system::error_code& ec,
                            std::size_t bytes_transferred) {
            BMCWEB_LOG_DEBUG << this << " async_read " << bytes_transferred
                             << " Bytes";

            bool errorWhileReading = false;
            if (ec)
            {
                BMCWEB_LOG_ERROR << this
                                 << " Error while reading: " << ec.message();
                errorWhileReading = true;
            }
            else
            {
                if (isAlive())
                {
                    cancelDeadlineTimer();
                    bool loggedIn = req && req->session;
                    if (loggedIn)
                    {
                        startDeadline(loggedInAttempts);
                    }
                    else
                    {
                        startDeadline(loggedOutAttempts);
                    }
                }
                else
                {
                    errorWhileReading = true;
                }
            }
            if (errorWhileReading)
            {
                cancelDeadlineTimer();
                close();
                BMCWEB_LOG_DEBUG << this << " from read(1)";
                return;
            }
            handle();
        };
        if (sslStream)
        {
            boost::beast::http::async_read(*sslStream, buffer, *parser,
                                           std::move(callback));
        }
        else
        {
            boost::beast::http::async_read(adaptor, buffer, *parser,
                                           std::move(callback));
        }
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
                                             std::size_t bytes_transferred) {
                BMCWEB_LOG_DEBUG << this << " async_write " << bytes_transferred
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
                req->isSecure = sslStream.has_value();
                doReadHeaders();
            };

        if (sslStream)
        {
            boost::beast::http::async_write(*sslStream, *serializer,
                                            std::move(callback));
        }
        else
        {
            boost::beast::http::async_write(adaptor, *serializer,
                                            std::move(callback));
        }
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
    Adaptor adaptor;
    std::shared_ptr<boost::asio::ssl::context> sslContext;
    std::optional<boost::beast::ssl_stream<Adaptor&>> sslStream;

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

    const std::string& serverName;

    std::optional<size_t> timerCancelKey;

    std::function<std::string()>& getCachedDateStr;
    detail::TimerQueue& timerQueue;

    using std::enable_shared_from_this<
        Connection<Adaptor, Handler>>::shared_from_this;
};
} // namespace crow
