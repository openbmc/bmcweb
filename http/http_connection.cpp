#include <string>
#include "http_connection.hpp"
#include "timer_queue.hpp"
#include <boost/beast/ssl/ssl_stream.hpp>

// Build CMD:
// cd /opt/openbmc-build/tmp/work/arm1176jzs-openbmc-linux-gnueabi/bmcweb/1.0+git999-r0/bmcweb-1.0+git999
// arm-openbmc-linux-gnueabi-g++ -marm -mcpu=arm1176jz-s -fstack-protector-strong -Os -D_FORTIFY_SOURCE=2 -Wformat -Wformat-security -Werror=format-security --sysroot=/opt/openbmc-build/tmp/work/arm1176jzs-openbmc-linux-gnueabi/bmcweb/1.0+git999-r0/recipe-sysroot -Ibmcweb.p -I. -I../../../../../../../../usr/local/google/home/suichen/openbmc/bmcweb -I../../../../../../../../usr/local/google/home/suichen/openbmc/bmcweb/include -I../../../../../../../../usr/local/google/home/suichen/openbmc/bmcweb/redfish-core/include -I../../../../../../../../usr/local/google/home/suichen/openbmc/bmcweb/redfish-core/lib -I../../../../../../../../usr/local/google/home/suichen/openbmc/bmcweb/http -flto=auto -fdiagnostics-color=always -D_FILE_OFFSET_BITS=64 -Wall -Winvalid-pch -Wnon-virtual-dtor -Wextra -Wpedantic -Werror -std=c++17 -fno-rtti -Os -g -fdata-sections -ffunction-sections -DNDEBUG -DBMCWEB_ENABLE_BASIC_AUTHENTICATION -DBMCWEB_ENABLE_COOKIE_AUTHENTICATION -DBMCWEB_ENABLE_HOST_SERIAL_WEBSOCKET -DBMCWEB_ENABLE_SSL -DBMCWEB_ENABLE_KVM -DBMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION -DWEBSERVER_ENABLE_PAM -DBMCWEB_ENABLE_REDFISH -DBMCWEB_ENABLE_DBUS_REST -DBMCWEB_ENABLE_SESSION_AUTHENTICATION -DBMCWEB_ENABLE_STATIC_HOSTING -DBMCWEB_ENABLE_VM_WEBSOCKET -DBMCWEB_ENABLE_XTOKEN_AUTHENTICATION -Wold-style-cast -Wcast-align -Wunused -Woverloaded-virtual -Wconversion -Wsign-conversion -Wno-attributes -Wno-stringop-overflow -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wunused-parameter -Wnull-dereference -Wdouble-promotion -Wformat=2 -fno-fat-lto-objects -fvisibility=hidden -fvisibility-inlines-hidden -DBMCWEB_ENABLE_LOGGING -DBMCWEB_ENABLE_DEBUG -DBMCWEB_ALLOW_DEPRECATED_POWER_THERMAL -fstack-protector-strong -fPIE -fPIC -D_FORTIFY_SOURCE=2 -Wformat -Wformat-security -DBOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT -DBOOST_BEAST_USE_STD_STRING_VIEW -DBOOST_BEAST_SEPARATE_COMPILATION -DBOOST_ASIO_SEPARATE_COMPILATION -DBOOST_ASIO_NO_DEPRECATED -DBOOST_NO_RTTI -DBOOST_NO_TYPEID -DBOOST_URL_STANDALONE -DBOOST_URL_HEADER_ONLY -DBOOST_ALLOW_DEPRECATED_HEADERS -DJSON_NOEXCEPTION -Os -fvisibility-inlines-hidden -DBOOST_ASIO_DISABLE_THREADS -DBOOST_ALL_NO_LIB -DBOOST_SYSTEM_NO_DEPRECATED -DBOOST_ERROR_CODE_HEADER_ONLY -DBOOST_COROUTINES_NO_DEPRECATION_WARNING -MD -MQ bmcweb.p/http_http_connection.cpp.o -MF bmcweb.p/http_http_connection.cpp.o.d -o bmcweb.p/http_http_connection.cpp.o -c ../../../../../../../../usr/local/google/home/suichen/openbmc/bmcweb/http/http_connection.cpp

namespace crow {

// Manually specify template specialization params
// Those type names are now known from the build commands
using Adaptor = boost::beast::ssl_stream<boost::asio::ip::tcp::socket>;
using Handler = crow::App;

template <>
void Connection<Adaptor, Handler>::init()
{
    parser.emplace(std::piecewise_construct, std::make_tuple());
    parser->body_limit(httpReqBodyLimit);
    parser->header_limit(httpHeaderLimit);

#ifdef BMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION
    prepareMutualTls();
#endif // BMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION

#ifdef BMCWEB_ENABLE_DEBUG
    connectionCount++;
    BMCWEB_LOG_DEBUG << this << " Connection open, total "
                        << connectionCount;
#endif
}

// Do not specialize, so as to avoid the "Explicit specialization after
// instantiation" error
template<typename Adaptor, typename Handler>
void Connection<Adaptor, Handler>::prepareMutualTls()
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
        std::string unsupportedClientId = "";
        sessionIsFromTransport = true;
        userSession = persistent_data::SessionStore::getInstance()
                            .generateUserSession(
                                sslUser, req->ipAddress.to_string(),
                                unsupportedClientId,
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

template <typename Adaptor, typename Handler>
void Connection<Adaptor, Handler>::handle()
{
    cancelDeadlineTimer();
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
            completeRequest();
            return;
        }
    }

    BMCWEB_LOG_INFO << "Request: "
                    << " " << this << " HTTP/" << thisReq.version() / 10
                    << "." << thisReq.version() % 10 << ' '
                    << thisReq.methodString() << " " << thisReq.target()
                    << " " << thisReq.ipAddress;

    res.setCompleteRequestHandler(nullptr);
    res.isAliveHelper = [this]() -> bool { return isAlive(); };

    thisReq.ioService = static_cast<decltype(thisReq.ioService)>(
        &adaptor.get_executor().context());

    if (res.completed)
    {
        completeRequest();
        return;
    }

    if (!crow::authorization::isOnAllowlist(req->url, req->method()) &&
        thisReq.session == nullptr)
    {
        BMCWEB_LOG_WARNING << "[AuthMiddleware] authorization failed";
        forward_unauthorized::sendUnauthorized(
            req->url, req->getHeaderValue("User-Agent"),
            req->getHeaderValue("Accept"), res);
        completeRequest();
        return;
    }

    res.setCompleteRequestHandler([self(shared_from_this())] {
        boost::asio::post(self->adaptor.get_executor(),
                            [self] { self->completeRequest(); });
    });

    if (thisReq.isUpgrade() &&
        boost::iequals(
            thisReq.getHeaderValue(boost::beast::http::field::upgrade),
            "websocket"))
    {
        handler->handleUpgrade(thisReq, res, std::move(adaptor));
        // delete lambda with self shared_ptr
        // to enable connection destruction
        res.setCompleteRequestHandler(nullptr);
        return;
    }
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>(res);
    handler->handle(thisReq, asyncResp);
}

template<>
void Connection<Adaptor, Handler>::completeRequest()
{
    if (!req)
    {
        return;
    }
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

    doWrite();

    // delete lambda with self shared_ptr
    // to enable connection destruction
    res.setCompleteRequestHandler(nullptr);
}

template<>
void Connection<Adaptor, Handler>::doReadHeaders()
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

            boost::beast::http::verb method = parser->get().method();
            readClientIp();

            boost::asio::ip::address ip;
            if (getClientIp(ip))
            {
                BMCWEB_LOG_DEBUG << "Unable to get client IP";
            }
            sessionIsFromTransport = false;
            userSession = crow::authorization::authenticate(
                ip, res, method, parser->get().base(), userSession);
            bool loggedIn = userSession != nullptr;
            if (loggedIn)
            {
                startDeadline(loggedInAttempts);
                BMCWEB_LOG_DEBUG << "Starting slow deadline";

                req->urlParams = req->urlView.query_params();

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

template<>
void Connection<Adaptor, Handler>::start()
{
    startDeadline(0);

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

template<>
Connection<Adaptor, Handler>::~Connection()
{
    res.setCompleteRequestHandler(nullptr);
    cancelDeadlineTimer();
#ifdef BMCWEB_ENABLE_DEBUG
    connectionCount--;
    BMCWEB_LOG_DEBUG << this << " Connection closed, total "
                        << connectionCount;
#endif
}

template<>
Connection<Adaptor, Handler>::Connection(Handler* handlerIn,
               std::function<std::string()>& getCachedDateStrF,
               detail::TimerQueue& timerQueueIn, Adaptor adaptorIn) :
        adaptor(std::move(adaptorIn)),
        handler(handlerIn), getCachedDateStr(getCachedDateStrF),
        timerQueue(timerQueueIn)
{
    init();
}

template<>
Adaptor& Connection<Adaptor, Handler>::socket()
{
    return adaptor;
}

}