#pragma once
#include "http_utility.hpp"

#include <array>
#include <atomic>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <chrono>
#include <regex>
#include <vector>

#include "crow/http_response.h"
#include "crow/logging.h"
#include "crow/middleware_context.h"
#include "crow/socket_adaptors.h"
#include "crow/timer_queue.h"

#ifdef BMCWEB_ENABLE_SSL
#include <boost/asio/ssl.hpp>
#endif

namespace crow
{

inline void escapeHtml(std::string& data)
{
    std::string buffer;
    // less than 5% of characters should be larger, so reserve a buffer of the
    // right size
    buffer.reserve(data.size() * 1.05);
    for (size_t pos = 0; pos != data.size(); ++pos)
    {
        switch (data[pos])
        {
            case '&':
                buffer.append("&amp;");
                break;
            case '\"':
                buffer.append("&quot;");
                break;
            case '\'':
                buffer.append("&apos;");
                break;
            case '<':
                buffer.append("&lt;");
                break;
            case '>':
                buffer.append("&gt;");
                break;
            default:
                buffer.append(&data[pos], 1);
                break;
        }
    }
    data.swap(buffer);
}

inline void convertToLinks(std::string& s)
{
    const static std::regex r{"(&quot;@odata\\.((id)|(Context))&quot;[ \\n]*:[ "
                              "\\n]*)(&quot;((?!&quot;).*)&quot;)"};
    s = std::regex_replace(s, r, "$1<a href=\"$6\">$5</a>");
}

inline void prettyPrintJson(crow::Response& res)
{
    std::string value = res.jsonValue.dump(4);
    escapeHtml(value);
    convertToLinks(value);
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
    static const bool value = decltype(f<T>(nullptr))::value;
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
    static const bool value = decltype(f<T>(nullptr))::value;
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

template <int N, typename Context, typename Container, typename CurrentMW,
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

template <int N, typename Context, typename Container>
bool middlewareCallHelper(Container& /*middlewares*/, Request& /*req*/,
                          Response& /*res*/, Context& /*ctx*/)
{
    return false;
}

template <int N, typename Context, typename Container>
typename std::enable_if<(N < 0)>::type
    afterHandlersCallHelper(Container& /*middlewares*/, Context& /*Context*/,
                            Request& /*req*/, Response& /*res*/)
{
}

template <int N, typename Context, typename Container>
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

template <int N, typename Context, typename Container>
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

// request body limit size: 30M
constexpr unsigned int httpReqBodyLimit = 1024 * 1024 * 30;

template <typename Adaptor, typename Handler, typename... Middlewares>
class Connection
{
  public:
    Connection(boost::asio::io_service& ioService, Handler* handler,
               const std::string& server_name,
               std::tuple<Middlewares...>* middlewares,
               std::function<std::string()>& get_cached_date_str_f,
               detail::TimerQueue& timerQueue,
               typename Adaptor::context* adaptorCtx) :
        adaptor(ioService, adaptorCtx),
        handler(handler), serverName(server_name), middlewares(middlewares),
        getCachedDateStr(get_cached_date_str_f), timerQueue(timerQueue)
    {
        parser.emplace(std::piecewise_construct, std::make_tuple());
        // Temporarily changed to 30MB; Need to modify uploading/authentication
        // mechanism
        parser->body_limit(httpReqBodyLimit);
        req.emplace(parser->get());
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

    decltype(std::declval<Adaptor>().rawSocket())& socket()
    {
        return adaptor.rawSocket();
    }

    void start()
    {
        adaptor.start([this](const boost::system::error_code& ec) {
            if (!ec)
            {
                startDeadline();

                doReadHeaders();
            }
            else
            {
                checkDestroy();
            }
        });
    }

    void handle()
    {
        cancelDeadlineTimer();
        bool isInvalidRequest = false;
        const boost::string_view connection =
            req->getHeaderValue(boost::beast::http::field::connection);

        // Check for HTTP version 1.1.
        if (req->version() == 11)
        {
            if (req->getHeaderValue(boost::beast::http::field::host).empty())
            {
                isInvalidRequest = true;
                res = Response(boost::beast::http::status::bad_request);
            }
        }

        BMCWEB_LOG_INFO << "Request: " << adaptor.remoteEndpoint() << " "
                        << this << " HTTP/" << req->version() / 10 << "."
                        << req->version() % 10 << ' ' << req->methodString()
                        << " " << req->target();

        needToCallAfterHandlers = false;

        if (!isInvalidRequest)
        {
            res.completeRequestHandler = [] {};
            res.isAliveHelper = [this]() -> bool { return adaptor.isOpen(); };

            ctx = detail::Context<Middlewares...>();
            req->middlewareContext = (void*)&ctx;
            req->ioService = &adaptor.getIoService();
            detail::middlewareCallHelper<
                0, decltype(ctx), decltype(*middlewares), Middlewares...>(
                *middlewares, *req, res, ctx);

            if (!res.completed)
            {
                if (req->isUpgrade() &&
                    boost::iequals(
                        req->getHeaderValue(boost::beast::http::field::upgrade),
                        "websocket"))
                {
                    handler->handleUpgrade(*req, res, std::move(adaptor));
                    return;
                }
                res.completeRequestHandler = [this] {
                    this->completeRequest();
                };
                needToCallAfterHandlers = true;
                handler->handle(*req, res);
                if (req->keepAlive())
                {
                    res.addHeader("connection", "Keep-Alive");
                }
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

    void completeRequest()
    {
        BMCWEB_LOG_INFO << "Response: " << this << ' ' << req->url << ' '
                        << res.resultInt() << " keepalive=" << req->keepAlive();

        if (needToCallAfterHandlers)
        {
            needToCallAfterHandlers = false;

            // call all afterHandler of middlewares
            detail::afterHandlersCallHelper<((int)sizeof...(Middlewares) - 1),
                                            decltype(ctx),
                                            decltype(*middlewares)>(
                *middlewares, ctx, *req, res);
        }

        // auto self = this->shared_from_this();
        res.completeRequestHandler = nullptr;

        if (!adaptor.isOpen())
        {
            // BMCWEB_LOG_DEBUG << this << " delete (socket is closed) " <<
            // isReading
            // << ' ' << isWriting;
            // delete this;
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
                res.body() = res.jsonValue.dump(2);
            }
        }

        if (res.resultInt() >= 400 && res.body().empty())
        {
            res.body() = std::string(res.reason());
        }
        res.addHeader(boost::beast::http::field::server, serverName);
        res.addHeader(boost::beast::http::field::date, getCachedDateStr());

        res.keepAlive(req->keepAlive());

        doWrite();
    }

  private:
    void doReadHeaders()
    {
        // auto self = this->shared_from_this();
        isReading = true;
        BMCWEB_LOG_DEBUG << this << " doReadHeaders";

        // Clean up any previous Connection.
        boost::beast::http::async_read_header(
            adaptor.socket(), buffer, *parser,
            [this](const boost::system::error_code& ec,
                   std::size_t bytes_transferred) {
                isReading = false;
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
                    if (!adaptor.isOpen() && !req->isUpgrade())
                    {
                        errorWhileReading = true;
                    }
                }

                if (errorWhileReading)
                {
                    cancelDeadlineTimer();
                    adaptor.close();
                    BMCWEB_LOG_DEBUG << this << " from read(1)";
                    checkDestroy();
                    return;
                }

                // Compute the url parameters for the request
                req->url = req->target();
                std::size_t index = req->url.find("?");
                if (index != boost::string_view::npos)
                {
                    req->url = req->url.substr(0, index - 1);
                }
                req->urlParams = QueryString(std::string(req->target()));
                doRead();
            });
    }

    void doRead()
    {
        // auto self = this->shared_from_this();
        isReading = true;
        BMCWEB_LOG_DEBUG << this << " doRead";

        boost::beast::http::async_read(
            adaptor.socket(), buffer, *parser,
            [this](const boost::system::error_code& ec,
                   std::size_t bytes_transferred) {
                BMCWEB_LOG_ERROR << this << " async_read " << bytes_transferred
                                 << " Bytes";
                isReading = false;

                bool errorWhileReading = false;
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Error while reading: " << ec.message();
                    errorWhileReading = true;
                }
                else
                {
                    if (!adaptor.isOpen())
                    {
                        errorWhileReading = true;
                    }
                }
                if (errorWhileReading)
                {
                    cancelDeadlineTimer();
                    adaptor.close();
                    BMCWEB_LOG_DEBUG << this << " from read(1)";
                    checkDestroy();
                    return;
                }
                handle();
            });
    }

    void doWrite()
    {
        // auto self = this->shared_from_this();
        isWriting = true;
        BMCWEB_LOG_DEBUG << "Doing Write";
        res.preparePayload();
        serializer.emplace(*res.stringResponse);
        boost::beast::http::async_write(
            adaptor.socket(), *serializer,
            [&](const boost::system::error_code& ec,
                std::size_t bytes_transferred) {
                isWriting = false;
                BMCWEB_LOG_DEBUG << this << " Wrote " << bytes_transferred
                                 << " bytes";

                if (ec)
                {
                    BMCWEB_LOG_DEBUG << this << " from write(2)";
                    checkDestroy();
                    return;
                }
                if (!req->keepAlive())
                {
                    adaptor.close();
                    BMCWEB_LOG_DEBUG << this << " from write(1)";
                    checkDestroy();
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

    void checkDestroy()
    {
        BMCWEB_LOG_DEBUG << this << " isReading " << isReading << " isWriting "
                         << isWriting;
        if (!isReading && !isWriting)
        {
            BMCWEB_LOG_DEBUG << this << " delete (idle) ";
            delete this;
        }
    }

    void cancelDeadlineTimer()
    {
        BMCWEB_LOG_DEBUG << this << " timer cancelled: " << &timerQueue << ' '
                         << timerCancelKey;
        timerQueue.cancel(timerCancelKey);
    }

    void startDeadline()
    {
        cancelDeadlineTimer();

        timerCancelKey = timerQueue.add([this] {
            if (!adaptor.isOpen())
            {
                return;
            }
            adaptor.close();
        });
        BMCWEB_LOG_DEBUG << this << " timer added: " << &timerQueue << ' '
                         << timerCancelKey;
    }

  private:
    Adaptor adaptor;
    Handler* handler;

    // Making this a boost::optional allows it to be efficiently destroyed and
    // re-created on Connection reset
    boost::optional<
        boost::beast::http::request_parser<boost::beast::http::string_body>>
        parser;

    boost::beast::flat_buffer buffer{8192};

    boost::optional<boost::beast::http::response_serializer<
        boost::beast::http::string_body>>
        serializer;

    boost::optional<crow::Request> req;
    crow::Response res;

    const std::string& serverName;

    int timerCancelKey{-1};

    bool isReading{};
    bool isWriting{};
    bool needToCallAfterHandlers{};
    bool needToStartReadAfterComplete{};
    bool addKeepAlive{};

    std::tuple<Middlewares...>* middlewares;
    detail::Context<Middlewares...> ctx;

    std::function<std::string()>& getCachedDateStr;
    detail::TimerQueue& timerQueue;
}; // namespace crow
} // namespace crow
