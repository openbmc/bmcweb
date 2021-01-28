#pragma once
#include "config.h"

#include "authorization.hpp"
#include "http_response.hpp"
#include "http_utility.hpp"
#include "logging.hpp"
#include "timer_queue.hpp"
#include "utility.hpp"

#include <nghttp2/nghttp2.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/container/flat_set.hpp>
#include <json_html_serializer.hpp>
#include <security_headers.hpp>
#include <ssl_key_handler.hpp>

#include <atomic>
#include <chrono>
#include <vector>

namespace crow
{

struct http2_stream_data
{
    http2_stream_data() : req(reqBase)
    {}
    boost::beast::http::request<boost::beast::http::string_body> reqBase;
    crow::Request req;
    std::shared_ptr<crow::Response> res;
};

struct nghttp2_session_callbacks_ptr
{
    nghttp2_session_callbacks_ptr()
    {
        nghttp2_session_callbacks_new(&ptr);
    }

    ~nghttp2_session_callbacks_ptr()
    {
        nghttp2_session_callbacks_del(ptr);
    }

    nghttp2_session_callbacks* get()
    {
        return ptr;
    }

    nghttp2_session_callbacks* ptr = nullptr;
};

struct nghttp2_session_ptr
{
    nghttp2_session_ptr(nghttp2_session_callbacks_ptr& callbacks)
    {
        if (nghttp2_session_server_new(&ptr, callbacks.get(), nullptr) != 0)
        {
            BMCWEB_LOG_ERROR << "nghttp2_session_server_new failed";
            return;
        }
    }

    ~nghttp2_session_ptr()
    {
        nghttp2_session_del(ptr);
    }

    nghttp2_session* get()
    {
        return ptr;
    }

  private:
    nghttp2_session* ptr = nullptr;
};

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

        streams.emplace(0, std::make_unique<http2_stream_data>());

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
            adaptor.async_handshake(
                boost::asio::ssl::stream_base::server,
                [this, self(shared_from_this())](
                    const boost::system::error_code& ec) {
                    if (ec)
                    {
                        return;
                    }

                    const unsigned char* alpn = nullptr;
                    unsigned int alpnlen = 0;
                    SSL_get0_alpn_selected(adaptor.native_handle(), &alpn,
                                           &alpnlen);
                    if (alpn != nullptr)
                    {
                        std::string_view selected_protocol(
                            reinterpret_cast<const char*>(alpn), alpnlen);
                        BMCWEB_LOG_DEBUG << "ALPN selected protocol \""
                                         << selected_protocol
                                         << "\" len: " << alpnlen;
                        if (selected_protocol == "h2")
                        {
                            cancelDeadlineTimer();
                            initialize_nghttp2_session();
                            if (send_server_connection_header() != 0)
                            {
                                BMCWEB_LOG_ERROR
                                    << "send_server_connection_header failed";
                                return;
                            }
                            send_buffer();
                            read_buffer();
                            return;
                        }
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

        if (!isInvalidRequest)
        {
            res.completeRequestHandler = [] {};
            res.isAliveHelper = [this]() -> bool { return isAlive(); };

            req->ioService = static_cast<decltype(req->ioService)>(
                &adaptor.get_executor().context());

            if (!res.completed)
            {
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

    void completeResponseFields(crow::Response& res)
    {
        BMCWEB_LOG_INFO << "Response: " << this << ' ' << req->url << ' '
                        << res.resultInt() << " keepalive=" << req->keepAlive();

        addSecurityHeaders(res);

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
    }

    void completeRequest()
    {
        crow::authorization::cleanupTempSession(*req);

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

        completeResponseFields(res);
        doWrite();

        // delete lambda with self shared_ptr
        // to enable connection destruction
        res.completeRequestHandler = nullptr;
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
            BMCWEB_LOG_ERROR << "Failed to get the client's IP Address. ec : "
                             << ec;
        }
        else
        {
            req->ipAddress = endpoint.address();
            BMCWEB_LOG_DEBUG << "Client IP address " << req->ipAddress;
        }
    }

    int send_server_connection_header()
    {
        BMCWEB_LOG_DEBUG << "send_server_connection_header()";

        std::array<nghttp2_settings_entry, 1> iv = {
            {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 10}};
        int rv = nghttp2_submit_settings(ngSession->get(), NGHTTP2_FLAG_NONE,
                                         iv.data(), iv.size());
        if (rv != 0)
        {
            BMCWEB_LOG_ERROR << "Fatal error: " << nghttp2_strerror(rv);
            return -1;
        }
        return 0;
    }

    int session_send()
    {
        BMCWEB_LOG_DEBUG << "session_send()";
        int rv = nghttp2_session_send(ngSession->get());
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
            adaptor, inBuffer, *parser,
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
                inBuffer.consume(inBuffer.size());

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

    struct SendPayload
    {
        std::weak_ptr<crow::Response> res;
        size_t sent_sofar;
    };

    static ssize_t file_read_callback(nghttp2_session* /* session */,
                                      int32_t /* stream_id */, uint8_t* buf,
                                      size_t length, uint32_t* data_flags,
                                      nghttp2_data_source* source, void*)
    {
        BMCWEB_LOG_DEBUG << "File read callback length: " << length;
        SendPayload* str = reinterpret_cast<SendPayload*>(source->ptr);

        std::shared_ptr<crow::Response> res = str->res.lock();
        if (res == nullptr)
        {
            *data_flags |= NGHTTP2_DATA_FLAG_EOF;
            return 0;
        }

        size_t to_send = std::min(res->body().size() - str->sent_sofar, length);
        BMCWEB_LOG_DEBUG << "Sending " << to_send << " bytes";

        memcpy(buf, res->body().data(), to_send);
        str->sent_sofar += to_send;

        if (str->sent_sofar >= res->body().size())
        {
            *data_flags |= NGHTTP2_DATA_FLAG_EOF;
        }
        return static_cast<ssize_t>(to_send);
    }

    int send_response(nghttp2_session* session, int32_t stream_id)
    {
        BMCWEB_LOG_DEBUG << "send_response stream_id:" << stream_id;

        auto it = streams.find(stream_id);
        if (it == streams.end())
        {
            close();
            return -1;
        }

        crow::Response& res = *it->second->res;
        std::vector<nghttp2_nv> hdr;

        completeResponseFields(res);

        // TODO(ed), this would need to be copied somewhere safe if we ever set
        // NGHTTP2_NV_FLAG_NO_COPY_VALUE
        boost::beast::http::fields& fields = res.stringResponse->base();
        static std::string_view status = ":status";
        std::string code = std::to_string(res.stringResponse->result_int());
        hdr.emplace_back(nghttp2_nv{
            const_cast<uint8_t*>(
                reinterpret_cast<const uint8_t*>(status.data())),
            const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(code.data())),
            status.size(), code.size(), 0});

        for (const boost::beast::http::fields::value_type& header : fields)
        {
            hdr.emplace_back(nghttp2_nv{
                const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(
                    header.name_string().data())),
                const_cast<uint8_t*>(
                    reinterpret_cast<const uint8_t*>(header.value().data())),
                header.name_string().size(), header.value().size(), 0});
        }

        SendPayload sendPayload = {it->second->res, 0};

        nghttp2_data_provider data_prd;

        data_prd.source.ptr = &sendPayload;
        data_prd.read_callback = file_read_callback;

        int rv = nghttp2_submit_response(session, stream_id, hdr.data(),
                                         hdr.size(), &data_prd);
        if (rv != 0)
        {
            BMCWEB_LOG_ERROR << "Fatal error: " << nghttp2_strerror(rv);
            close();
            return -1;
        }
        send_buffer();

        return 0;
    }

    void create_http2_stream_data(int32_t stream_id)
    {
        BMCWEB_LOG_DEBUG << "create_http2_stream_data for stream id "
                         << stream_id;

        std::unique_ptr<http2_stream_data> stream_data =
            std::make_unique<http2_stream_data>();
        streams.emplace(stream_id, std::move(stream_data));
    }

    void send_buffer()
    {
        BMCWEB_LOG_DEBUG << "send_buffer " << inBuffer.size()
                         << " bytes to send";
        if (socket_sending)
        {
            BMCWEB_LOG_DEBUG << "socket already sending";
            return;
        }

        int r = session_send();
        if (r != 0)
        {
            BMCWEB_LOG_ERROR << "session_send failed: " << r;
            close();
            return;
        }

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
                ng_consume_input();
                send_buffer();
            });
    }

    static ssize_t send_callback(nghttp2_session*, const uint8_t* data,
                                 size_t length, int, void* user_data)
    {
        BMCWEB_LOG_DEBUG << "send_callback";
        if (user_data == nullptr)
        {
            BMCWEB_LOG_CRITICAL << "user data was null?";
            return NGHTTP2_ERR_CALLBACK_FAILURE;
        }
        Connection<Adaptor, Handler>* self =
            reinterpret_cast<Connection<Adaptor, Handler>*>(user_data);
        boost::system::error_code ec;
        BMCWEB_LOG_DEBUG << "Writing " << length << " Bytes";

        if (self->sendBuffer.capacity() - self->sendBuffer.size() < length)
        {
            BMCWEB_LOG_WARNING << "Returning would block capacity: "
                               << self->sendBuffer.max_size() << " size "
                               << self->sendBuffer.size() << " len:" << length;
            return NGHTTP2_ERR_WOULDBLOCK;
        }
        std::array<boost::asio::const_buffer, 1> copyBuffer = {
            boost::asio::buffer(data, length)};

        size_t copied = boost::asio::buffer_copy(
            self->sendBuffer.prepare(length), copyBuffer);
        BMCWEB_LOG_DEBUG << "copied " << copied;
        self->sendBuffer.commit(copied);

        if (ec)
        {
            BMCWEB_LOG_ERROR << "write_some failed with ec:" << ec;
            self->close();
        }
        return static_cast<ssize_t>(copied);
    }

    void ng_consume_input()
    {
        size_t readSofar = 0;
        for (boost::asio::const_buffer buf : inBuffer.data())
        {
            size_t bytesToRead = boost::asio::buffer_size(buf);
            BMCWEB_LOG_DEBUG << "Buffer size " << bytesToRead;
            if (bytesToRead <= 0)
            {
                BMCWEB_LOG_DEBUG << "No more data?";
                break;
            }
            BMCWEB_LOG_DEBUG << "nghttp2_session_mem_recv calling with "
                             << bytesToRead << " bytes";

            ssize_t readLen = nghttp2_session_mem_recv(
                ngSession->get(), reinterpret_cast<const uint8_t*>(buf.data()),
                bytesToRead);

            if (readLen <= 0)
            {
                BMCWEB_LOG_ERROR << "nghttp2_session_mem_recv returned "
                                 << readLen;
                return;
            }
            readSofar += static_cast<size_t>(readLen);
        }
        inBuffer.consume(readSofar);
    }

    void read_buffer()
    {
        BMCWEB_LOG_DEBUG << "read-buffer";
        if (socket_reading)
        {
            BMCWEB_LOG_DEBUG << "socket already reading";
            close();
            return;
        }

        socket_reading = true;
        adaptor.async_read_some(
            inBuffer.prepare(8096),
            [this, self(shared_from_this())](boost::system::error_code ec,
                                             size_t bytes) {
                socket_reading = false;
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "async_read_some got error " << ec;
                    close();
                    return;
                }
                BMCWEB_LOG_DEBUG << "async_read_some Got " << bytes << " bytes";

                inBuffer.commit(bytes);
                if (!socket_sending)
                {
                    ng_consume_input();
                }

                read_buffer();
            });
    }

    int on_request_recv(nghttp2_session* session, int32_t stream_id)
    {
        BMCWEB_LOG_DEBUG << "on_request_recv";

        auto it = streams.find(stream_id);
        if (it == streams.end())
        {
            close();
            return -1;
        }

        crow::Request& req = it->second->req;
        BMCWEB_LOG_DEBUG << "Handling " << &req << " \"" << req.url << "\"";

        it->second->res = std::make_shared<crow::Response>();

        it->second->res->completeRequestHandler =
            [this, session, stream_id, shared_res{it->second->res}]() {
                // This request is complete, so no need to keep a reference to
                // ourselves
                shared_res->completeRequestHandler = nullptr;
                BMCWEB_LOG_DEBUG << "res.completeRequestHandler called";
                /*
                if (!isAlive())
                {
                    BMCWEB_LOG_DEBUG << "Connection was closed";
                    return;
                }
                */
                if (send_response(session, stream_id) != 0)
                {
                    close();
                    return;
                }
                send_buffer();
            };

        handler->handle(req, *it->second->res);

        return 0;
    }

    static int on_frame_recv_callback(nghttp2_session* /* session */,
                                      const nghttp2_frame* frame,
                                      void* user_data)
    {
        BMCWEB_LOG_DEBUG << "on_frame_recv_callback";
        if (user_data == nullptr)
        {
            BMCWEB_LOG_CRITICAL << "user data was null?";
            return NGHTTP2_ERR_CALLBACK_FAILURE;
        }
        Connection<Adaptor, Handler>* self =
            reinterpret_cast<Connection<Adaptor, Handler>*>(user_data);
        BMCWEB_LOG_DEBUG << "frame type " << static_cast<int>(frame->hd.type);
        switch (frame->hd.type)
        {
            case NGHTTP2_DATA:
            case NGHTTP2_HEADERS:
                // Check that the client request has finished
                if (frame->hd.flags & NGHTTP2_FLAG_END_STREAM)
                {
                    return self->on_request_recv(self->ngSession->get(),
                                                 frame->hd.stream_id);
                }
                break;
            default:
                break;
        }
        return 0;
    }

    static int on_stream_close_callback(nghttp2_session* /* session */,
                                        int32_t stream_id, uint32_t,
                                        void* user_data)
    {
        BMCWEB_LOG_DEBUG << "on_stream_close_callback";
        if (user_data == nullptr)
        {
            BMCWEB_LOG_CRITICAL << "user data was null?";
            return NGHTTP2_ERR_CALLBACK_FAILURE;
        }
        Connection<Adaptor, Handler>* self =
            reinterpret_cast<Connection<Adaptor, Handler>*>(user_data);

        self->streams.erase(stream_id);
        return 0;
    }

    // nghttp2_on_header_callback: Called when nghttp2 library emits
    //    single header name/value pair.

    static int on_header_callback(nghttp2_session* /* session */,
                                  const nghttp2_frame* frame,
                                  const uint8_t* name, size_t namelen,
                                  const uint8_t* value, size_t vallen, uint8_t,
                                  void* user_data)
    {

        if (user_data == nullptr)
        {
            BMCWEB_LOG_CRITICAL << "user data was null?";
            return NGHTTP2_ERR_CALLBACK_FAILURE;
        }
        Connection<Adaptor, Handler>* self =
            reinterpret_cast<Connection<Adaptor, Handler>*>(user_data);
        std::string_view nameSv(reinterpret_cast<const char*>(name), namelen);
        std::string_view valueSv(reinterpret_cast<const char*>(value), vallen);

        BMCWEB_LOG_DEBUG << "on_header_callback name: " << nameSv << " value "
                         << valueSv;

        switch (frame->hd.type)
        {
            case NGHTTP2_HEADERS:
                if (frame->headers.cat != NGHTTP2_HCAT_REQUEST)
                {
                    break;
                }
                auto thisStream = self->streams.find(frame->hd.stream_id);
                if (thisStream == self->streams.end())
                {
                    BMCWEB_LOG_ERROR << "Unknown stream" << frame->hd.stream_id;
                    self->close();
                    return -1;
                }

                crow::Request& req = thisStream->second->req;

                if (nameSv == ":path")
                {
                    try
                    {
                        // TODO Should use per stream structures
                        req.req.target(valueSv);
                        req.urlView = boost::urls::url_view(req.req.target());
                        req.url = req.urlView.encoded_path();
                        req.urlParams = req.urlView.params();
                    }
                    catch (std::exception& p)
                    {
                        BMCWEB_LOG_ERROR << p.what();
                    }
                }
                else if (nameSv == ":method")
                {
                    req.req.method(boost::beast::http::string_to_verb(valueSv));
                }
                else if (nameSv == ":scheme")
                {
                    req.isSecure = valueSv == "https";
                }
                else
                {
                    req.fields.set(nameSv, valueSv);
                }
                break;
        }
        return 0;
    }

    static int on_begin_headers_callback(nghttp2_session* /* session */,
                                         const nghttp2_frame* frame,
                                         void* user_data)
    {
        BMCWEB_LOG_DEBUG << "on_begin_headers_callback";
        if (user_data == nullptr)
        {
            BMCWEB_LOG_CRITICAL << "user data was null?";
            return NGHTTP2_ERR_CALLBACK_FAILURE;
        }

        Connection<Adaptor, Handler>* self =
            reinterpret_cast<Connection<Adaptor, Handler>*>(user_data);

        if (frame->hd.type != NGHTTP2_HEADERS ||
            frame->headers.cat != NGHTTP2_HCAT_REQUEST)
        {
            return 0;
        }
        self->create_http2_stream_data(frame->hd.stream_id);
        return 0;
    }

    void initialize_nghttp2_session()
    {
        nghttp2_session_callbacks_ptr callbacks;

        nghttp2_session_callbacks_set_send_callback(callbacks.get(),
                                                    send_callback);

        nghttp2_session_callbacks_set_on_frame_recv_callback(
            callbacks.get(), on_frame_recv_callback);

        nghttp2_session_callbacks_set_on_stream_close_callback(
            callbacks.get(), on_stream_close_callback);

        nghttp2_session_callbacks_set_on_header_callback(callbacks.get(),
                                                         on_header_callback);

        nghttp2_session_callbacks_set_on_begin_headers_callback(
            callbacks.get(), on_begin_headers_callback);

        ngSession.emplace(callbacks);

        nghttp2_session_set_user_data(ngSession->get(), this);
    }

  private:
    boost::container::flat_map<int32_t, std::unique_ptr<http2_stream_data>>
        streams;

    std::optional<nghttp2_session_ptr> ngSession;
    bool socket_sending = false;
    bool socket_reading = false;

    Adaptor adaptor;
    Handler* handler;

    // Making this a std::optional allows it to be efficiently destroyed and
    // re-created on Connection reset
    std::optional<
        boost::beast::http::request_parser<boost::beast::http::string_body>>
        parser;

    boost::beast::flat_static_buffer<8096> sendBuffer;

    boost::beast::multi_buffer inBuffer;

    std::optional<boost::beast::http::response_serializer<
        boost::beast::http::string_body>>
        serializer;

    std::optional<crow::Request> req;
    crow::Response res;

    std::weak_ptr<persistent_data::UserSession> session;

    std::optional<size_t> timerCancelKey;

    std::function<std::string()>& getCachedDateStr;
    detail::TimerQueue& timerQueue;

    using std::enable_shared_from_this<
        Connection<Adaptor, Handler>>::shared_from_this;
};
} // namespace crow
