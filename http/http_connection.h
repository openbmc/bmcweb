#pragma once
#include "config.h"

#include "http_utility.hpp"

#include <atomic>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core/flat_static_buffer.hpp>
#if BOOST_VERSION >= 107000
#include <boost/beast/ssl/ssl_stream.hpp>
#else
#include <boost/beast/experimental/core/ssl_stream.hpp>
#endif
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <chrono>
#include <vector>

#include "http_response.h"
#include "logging.h"
#include "middleware_context.h"
#include "timer_queue.h"
#include "utility.h"

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

using namespace boost;
using tcp = asio::ip::tcp;

namespace detail
{
template <typename MW> struct CheckBeforeHandleArity3Const
{
    template <typename T,
              void (T::*)(Request&, Response&, typename MW::Context&) const =
                  &T::beforeHandle>
    struct Get
    {
    };
};

template <typename MW> struct CheckBeforeHandleArity3
{
    template <typename T, void (T::*)(Request&, Response&,
                                      typename MW::Context&) = &T::beforeHandle>
    struct Get
    {
    };
};

template <typename MW> struct CheckAfterHandleArity3Const
{
    template <typename T,
              void (T::*)(Request&, Response&, typename MW::Context&) const =
                  &T::afterHandle>
    struct Get
    {
    };
};

template <typename MW> struct CheckAfterHandleArity3
{
    template <typename T, void (T::*)(Request&, Response&,
                                      typename MW::Context&) = &T::afterHandle>
    struct Get
    {
    };
};

template <typename T> struct IsBeforeHandleArity3Impl
{
    template <typename C>
    static std::true_type
        f(typename CheckBeforeHandleArity3Const<T>::template Get<C>*);

    template <typename C>
    static std::true_type
        f(typename CheckBeforeHandleArity3<T>::template Get<C>*);

    template <typename C> static std::false_type f(...);

  public:
    static constexpr bool value = decltype(f<T>(nullptr))::value;
};

template <typename T> struct IsAfterHandleArity3Impl
{
    template <typename C>
    static std::true_type
        f(typename CheckAfterHandleArity3Const<T>::template Get<C>*);

    template <typename C>
    static std::true_type
        f(typename CheckAfterHandleArity3<T>::template Get<C>*);

    template <typename C> static std::false_type f(...);

  public:
    static constexpr bool value = decltype(f<T>(nullptr))::value;
};

template <typename MW, typename Context, typename ParentContext>
typename std::enable_if<!IsBeforeHandleArity3Impl<MW>::value>::type
    beforeHandlerCall(MW& mw, Request& req, Response& res, Context& ctx,
                      ParentContext& /*parent_ctx*/)
{
    mw.beforeHandle(req, res, ctx.template get<MW>(), ctx);
}

template <typename MW, typename Context, typename ParentContext>
typename std::enable_if<IsBeforeHandleArity3Impl<MW>::value>::type
    beforeHandlerCall(MW& mw, Request& req, Response& res, Context& ctx,
                      ParentContext& /*parent_ctx*/)
{
    mw.beforeHandle(req, res, ctx.template get<MW>());
}

template <typename MW, typename Context, typename ParentContext>
typename std::enable_if<!IsAfterHandleArity3Impl<MW>::value>::type
    afterHandlerCall(MW& mw, Request& req, Response& res, Context& ctx,
                     ParentContext& /*parent_ctx*/)
{
    mw.afterHandle(req, res, ctx.template get<MW>(), ctx);
}

template <typename MW, typename Context, typename ParentContext>
typename std::enable_if<IsAfterHandleArity3Impl<MW>::value>::type
    afterHandlerCall(MW& mw, Request& req, Response& res, Context& ctx,
                     ParentContext& /*parent_ctx*/)
{
    mw.afterHandle(req, res, ctx.template get<MW>());
}

template <size_t N, typename Context, typename Container, typename CurrentMW,
          typename... Middlewares>
bool middlewareCallHelper(Container& middlewares, Request& req, Response& res,
                          Context& ctx)
{
    using parent_context_t = typename Context::template partial<N - 1>;
    beforeHandlerCall<CurrentMW, Context, parent_context_t>(
        std::get<N>(middlewares), req, res, ctx,
        static_cast<parent_context_t&>(ctx));

    if (res.isCompleted())
    {
        afterHandlerCall<CurrentMW, Context, parent_context_t>(
            std::get<N>(middlewares), req, res, ctx,
            static_cast<parent_context_t&>(ctx));
        return true;
    }

    if (middlewareCallHelper<N + 1, Context, Container, Middlewares...>(
            middlewares, req, res, ctx))
    {
        afterHandlerCall<CurrentMW, Context, parent_context_t>(
            std::get<N>(middlewares), req, res, ctx,
            static_cast<parent_context_t&>(ctx));
        return true;
    }

    return false;
}

template <size_t N, typename Context, typename Container>
bool middlewareCallHelper(Container& /*middlewares*/, Request& /*req*/,
                          Response& /*res*/, Context& /*ctx*/)
{
    return false;
}

template <size_t N, typename Context, typename Container>
typename std::enable_if<(N < 0)>::type
    afterHandlersCallHelper(Container& /*middlewares*/, Context& /*Context*/,
                            Request& /*req*/, Response& /*res*/)
{
}

template <size_t N, typename Context, typename Container>
typename std::enable_if<(N == 0)>::type
    afterHandlersCallHelper(Container& middlewares, Context& ctx, Request& req,
                            Response& res)
{
    using parent_context_t = typename Context::template partial<N - 1>;
    using CurrentMW = typename std::tuple_element<
        N, typename std::remove_reference<Container>::type>::type;
    afterHandlerCall<CurrentMW, Context, parent_context_t>(
        std::get<N>(middlewares), req, res, ctx,
        static_cast<parent_context_t&>(ctx));
}

template <size_t N, typename Context, typename Container>
typename std::enable_if<(N > 0)>::type
    afterHandlersCallHelper(Container& middlewares, Context& ctx, Request& req,
                            Response& res)
{
    using parent_context_t = typename Context::template partial<N - 1>;
    using CurrentMW = typename std::tuple_element<
        N, typename std::remove_reference<Container>::type>::type;
    afterHandlerCall<CurrentMW, Context, parent_context_t>(
        std::get<N>(middlewares), req, res, ctx,
        static_cast<parent_context_t&>(ctx));
    afterHandlersCallHelper<N - 1, Context, Container>(middlewares, ctx, req,
                                                       res);
}
} // namespace detail

#ifdef BMCWEB_ENABLE_DEBUG
static std::atomic<int> connectionCount;
#endif

// request body limit size set by the BMCWEB_HTTP_REQ_BODY_LIMIT_MB option
constexpr unsigned int httpReqBodyLimit =
    1024 * 1024 * BMCWEB_HTTP_REQ_BODY_LIMIT_MB;

template <typename Adaptor, typename Handler, typename... Middlewares>
class Connection : public std::enable_shared_from_this<
                       Connection<Adaptor, Handler, Middlewares...>>
{
  public:
    Connection(boost::asio::io_context& ioService, Handler* handlerIn,
               const std::string& ServerNameIn,
               std::tuple<Middlewares...>* middlewaresIn,
               std::function<std::string()>& get_cached_date_str_f,
               detail::TimerQueue& timerQueueIn, Adaptor adaptorIn) :
        adaptor(std::move(adaptorIn)),
        handler(handlerIn), serverName(ServerNameIn),
        middlewares(middlewaresIn), getCachedDateStr(get_cached_date_str_f),
        timerQueue(timerQueueIn)
    {
        parser.emplace(std::piecewise_construct, std::make_tuple());
        // Temporarily set by the BMCWEB_HTTP_REQ_BODY_LIMIT_MB variable; Need
        // to modify uploading/authentication mechanism to a better method that
        // disallows a DOS attack based on a large file size.
        parser->body_limit(httpReqBodyLimit);
        req.emplace(parser->get());

#ifdef BMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION
        auto ca_available = !std::filesystem::is_empty(
            std::filesystem::path(ensuressl::trustStorePath));
        if (ca_available && crow::persistent_data::SessionStore::getInstance()
                                .getAuthMethodsConfig()
                                .tls)
        {
            adaptor.set_verify_mode(boost::asio::ssl::verify_peer);
            SSL_set_session_id_context(
                adaptor.native_handle(),
                reinterpret_cast<const unsigned char*>(serverName.c_str()),
                static_cast<unsigned int>(serverName.length()));
            BMCWEB_LOG_DEBUG << this << " TLS is enabled on this connection.";
        }

        adaptor.set_verify_callback([this](
                                        bool preverified,
                                        boost::asio::ssl::verify_context& ctx) {
            // do nothing if TLS is disabled
            if (!crow::persistent_data::SessionStore::getInstance()
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
                X509_get_ext_d2i(peerCert, NID_key_usage, NULL, NULL));

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

            // Determine that ExtendedKeyUsage includes Client Auth

            stack_st_ASN1_OBJECT* extUsage = static_cast<stack_st_ASN1_OBJECT*>(
                X509_get_ext_d2i(peerCert, NID_ext_key_usage, NULL, NULL));

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

            session = persistent_data::SessionStore::getInstance()
                          .generateUserSession(
                              sslUser,
                              crow::persistent_data::PersistenceType::TIMEOUT);
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
                        << " " << req->target();

        needToCallAfterHandlers = false;

        if (!isInvalidRequest)
        {
            res.completeRequestHandler = [] {};
            res.isAliveHelper = [this]() -> bool { return isAlive(); };

            ctx = detail::Context<Middlewares...>();
            req->middlewareContext = static_cast<void*>(&ctx);
            req->ioService = static_cast<decltype(req->ioService)>(
                &adaptor.get_executor().context());

#ifdef BMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION
            if (auto sp = session.lock())
            {
                // set cookie only if this is req from the browser.
                if (req->getHeaderValue("User-Agent").empty())
                {
                    BMCWEB_LOG_DEBUG << this << " TLS session: " << sp->uniqueId
                                     << " will be used for this request.";
                    req->session = sp;
                }
                else
                {
                    std::string_view cookieValue =
                        req->getHeaderValue("Cookie");
                    if (cookieValue.empty() ||
                        cookieValue.find("SESSION=") == std::string::npos)
                    {
                        res.addHeader("Set-Cookie",
                                      "XSRF-TOKEN=" + sp->csrfToken +
                                          "; Secure\r\nSet-Cookie: SESSION=" +
                                          sp->sessionToken +
                                          "; Secure; HttpOnly\r\nSet-Cookie: "
                                          "IsAuthenticated=true; Secure");
                        BMCWEB_LOG_DEBUG
                            << this << " TLS session: " << sp->uniqueId
                            << " with cookie will be used for this request.";
                        req->session = sp;
                    }
                }
            }
#endif // BMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION

            detail::middlewareCallHelper<
                0U, decltype(ctx), decltype(*middlewares), Middlewares...>(
                *middlewares, *req, res, ctx);

            if (!res.completed)
            {
                needToCallAfterHandlers = true;
                res.completeRequestHandler = [self(shared_from_this())] {
                    self->completeRequest();
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

        if (needToCallAfterHandlers)
        {
            needToCallAfterHandlers = false;

            // call all afterHandler of middlewares
            detail::afterHandlersCallHelper<sizeof...(Middlewares) - 1,
                                            decltype(ctx),
                                            decltype(*middlewares)>(
                *middlewares, ctx, *req, res);
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

        res.addHeader(boost::beast::http::field::server, serverName);
        res.addHeader(boost::beast::http::field::date, getCachedDateStr());

        res.keepAlive(req->keepAlive());

        doWrite();

        // delete lambda with self shared_ptr
        // to enable connection destruction
        res.completeRequestHandler = nullptr;
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
                                       std::size_t bytes_transferred) {
                BMCWEB_LOG_ERROR << this << " async_read_header "
                                 << bytes_transferred << " Bytes";
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

                if (errorWhileReading)
                {
                    cancelDeadlineTimer();
                    close();
                    BMCWEB_LOG_DEBUG << this << " from read(1)";
                    return;
                }

                // Compute the url parameters for the request
                req->url = req->target();
                std::size_t index = req->url.find("?");
                if (index != std::string_view::npos)
                {
                    req->url = req->url.substr(0, index);
                }
                req->urlParams = QueryString(std::string(req->target()));
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
                                       std::size_t bytes_transferred) {
                BMCWEB_LOG_DEBUG << this << " async_read " << bytes_transferred
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
                    if (!isAlive())
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
        BMCWEB_LOG_DEBUG << this << " doWrite";
        res.preparePayload();
        serializer.emplace(*res.stringResponse);
        boost::beast::http::async_write(
            adaptor, *serializer,
            [this,
             self(shared_from_this())](const boost::system::error_code& ec,
                                       std::size_t bytes_transferred) {
                BMCWEB_LOG_DEBUG << this << " async_write " << bytes_transferred
                                 << " bytes";

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

    void startDeadline()
    {
        cancelDeadlineTimer();

        timerCancelKey =
            timerQueue.add([this, self(shared_from_this()),
                            readCount{parser->get().body().size()}] {
                // Mark timer as not active to avoid canceling it during
                // Connection destructor which leads to double free issue
                timerCancelKey.reset();
                if (!isAlive())
                {
                    return;
                }

                // Restart timer if read is in progress.
                // With threshold can be used to drop slow connections
                // to protect against slow-rate DoS attack
                if (parser->get().body().size() > readCount)
                {
                    BMCWEB_LOG_DEBUG << this
                                     << " restart timer - read in progress";
                    startDeadline();
                    return;
                }

                close();
            });
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
#ifdef BMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION
    std::weak_ptr<crow::persistent_data::UserSession> session;
#endif // BMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION

    const std::string& serverName;

    std::optional<size_t> timerCancelKey;

    bool needToCallAfterHandlers{};
    bool needToStartReadAfterComplete{};

    std::tuple<Middlewares...>* middlewares;
    detail::Context<Middlewares...> ctx;

    std::function<std::string()>& getCachedDateStr;
    detail::TimerQueue& timerQueue;

    using std::enable_shared_from_this<
        Connection<Adaptor, Handler, Middlewares...>>::shared_from_this;
};
} // namespace crow
