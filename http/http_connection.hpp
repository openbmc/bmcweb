#pragma once
#include "config.h"

#include "authorization.hpp"
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
#include <boost/beast/core/flat_static_buffer.hpp>
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
    Connection(Handler* handlerIn,
               std::function<std::string()>& getCachedDateStrF,
               detail::TimerQueue& timerQueueIn, Adaptor adaptorIn) :
        adaptor(std::move(adaptorIn)),
        handler(handlerIn), getCachedDateStr(getCachedDateStrF),
        timerQueue(timerQueueIn)
    {
        parser.emplace(std::piecewise_construct, std::make_tuple());
        parser->body_limit(httpReqBodyLimit);
        parser->header_limit(httpHeaderLimit);
        req.emplace(parser->get());

#ifdef BMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION
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
            int ret = SSL_set_session_id_context(
                adaptor.native_handle(),
                reinterpret_cast<const unsigned char*>(id.c_str()),
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
                BMCWEB_LOG_DEBUG << this
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

            session =
                persistent_data::SessionStore::getInstance()
                    .generateUserSession(
                        sslUser, persistent_data::PersistenceType::TIMEOUT,
                        false, req->ipAddress.to_string());
            if (auto sp = session.lock())
            {
                BMCWEB_LOG_DEBUG << this
                                 << " Generating TLS session: " << sp->uniqueId;
            }
            return true;
        });
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

    Adaptor& socket()
    {
        return adaptor;
    }

    void start()
    {

        startDeadline(0);

        // Fetch the client IP address
        readClientIp();

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
        cancelDeadlineTimer();

        bool isInvalidRequest = false;

        // Check for HTTP version 1.1.
        if (req->version() == 11)
        {
            if (req->getHeaderValue(boost::beast::http::field::host).empty())
            {
                isInvalidRequest = true;
                res.result(boost::beast::http::status::bad_request);
            }
        }

        BMCWEB_LOG_INFO << "Request: "
                        << " " << this << " HTTP/" << req->version() / 10 << "."
                        << req->version() % 10 << ' ' << req->methodString()
                        << " " << req->target() << " " << req->ipAddress;

        needToCallAfterHandlers = false;

        if (!isInvalidRequest)
        {
            res.completeRequestHandler = [] {};
            res.isAliveHelper = [this]() -> bool { return isAlive(); };

            req->ioService = static_cast<decltype(req->ioService)>(
                &adaptor.get_executor().context());

            if (!res.completed)
            {
                needToCallAfterHandlers = true;
                res.completeRequestHandler = [self(shared_from_this())] {
                    boost::asio::post(self->adaptor.get_executor(),
                                      [self] { self->completeRequest(); });
                };
                if (req->isUpgrade() &&
                    boost::iequals(
                        req->getHeaderValue(boost::beast::http::field::upgrade),
                        "websocket"))
                {
                    handler->handleUpgrade(*req, res, std::move(adaptor));
                    // delete lambda with self shared_ptr
                    // to enable connection destruction
                    res.completeRequestHandler = nullptr;
                    return;
                }
                handler->handle(*req, res);
            }
            else
            {
                completeRequest();
            }
        }
        else
        {
            completeRequest();
        }
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
#ifdef BMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION
            if (auto sp = session.lock())
            {
                BMCWEB_LOG_DEBUG << this
                                 << " Removing TLS session: " << sp->uniqueId;
                persistent_data::SessionStore::getInstance().removeSession(sp);
            }
#endif // BMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION
        }
        else
        {
            adaptor.close();
        }
    }

    void completeRequest()
    {
        BMCWEB_LOG_INFO << "Response: " << this << ' ' << req->url << ' '
                        << res.resultInt() << " keepalive=" << req->keepAlive();

        addSecurityHeaders(res);

        if (needToCallAfterHandlers)
        {
            crow::authorization::cleanupTempSession(*req);
        }

        if (!isAlive())
        {
            // BMCWEB_LOG_DEBUG << this << " delete (socket is closed) " <<
            // isReading
            // << ' ' << isWriting;
            // delete this;

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

        res.addHeader(boost::beast::http::field::date, getCachedDateStr());

        res.keepAlive(req->keepAlive());

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
        }
        else
        {
            req->ipAddress = endpoint.address();
        }
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
                BMCWEB_LOG_ERROR << this << " async_read_header "
                                 << bytesTransferred << " Bytes";
                bool errorWhileReading = false;
                if (ec)
                {
                    errorWhileReading = true;
                    BMCWEB_LOG_ERROR
                        << this << " Error while reading: " << ec.message();
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

                // Note, despite the bmcweb coding policy on use of exceptions
                // for error handling, this one particular use of exceptions is
                // deemed acceptible, as it solved a significant error handling
                // problem that resulted in seg faults, the exact thing that the
                // exceptions rule is trying to avoid. If at some point,
                // boost::urls makes the parser object public (or we port it
                // into bmcweb locally) this will be replaced with
                // parser::parse, which returns a status code

                try
                {
                    req->urlView = boost::urls::url_view(req->target());
                    req->url = req->urlView.encoded_path();
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
                    if (contentLength &&
                        *contentLength > loggedOutPostBodyLimit)
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
            });
    }

    void doRead()
    {
        BMCWEB_LOG_DEBUG << this << " doRead";

        boost::beast::http::async_read(
            adaptor, buffer, *parser,
            [this,
             self(shared_from_this())](const boost::system::error_code& ec,
                                       std::size_t bytesTransferred) {
                BMCWEB_LOG_DEBUG << this << " async_read " << bytesTransferred
                                 << " Bytes";

                bool errorWhileReading = false;
                if (ec)
                {
                    BMCWEB_LOG_ERROR
                        << this << " Error while reading: " << ec.message();
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
            });
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

                req.emplace(parser->get());
                doReadHeaders();
            });
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

    bool needToCallAfterHandlers{};
    bool needToStartReadAfterComplete{};

    std::function<std::string()>& getCachedDateStr;
    detail::TimerQueue& timerQueue;

    using std::enable_shared_from_this<
        Connection<Adaptor, Handler>>::shared_from_this;
};
} // namespace crow
