#pragma once
#include <crow/http_request.h>
#include <crow/http_response.h>
#include <crow/logging.h>
#include <crow/timer_queue.h>

#include <atomic>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/flat_static_buffer.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/lexical_cast.hpp>
#include <chrono>
#include <http_utility.hpp>
#include <optional>
#include <regex>
#include <security_headers.hpp>
#include <token_authorization_middleware.hpp>
#include <vector>
#ifdef BMCWEB_ENABLE_SSL
#include <boost/asio/ssl.hpp>
#include <boost/beast/experimental/core/ssl_stream.hpp>
#endif

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
}

#ifdef BMCWEB_ENABLE_DEBUG
static std::atomic<int> connectionCount;
#endif

template <typename Adaptor, typename Handler>
class Connection : std::enable_shared_from_this<Connection<Adaptor, Handler>>
{
  public:
    Connection(boost::asio::io_service& ioService, Handler* handler,
               std::function<std::string()>& get_cached_date_str_f,
               detail::TimerQueue& timerQueue, Adaptor adaptorIn) :
        adaptor(std::move(adaptorIn)),
        parser(std::in_place), req(parser->get()), handler(handler),
        getCachedDateStr(get_cached_date_str_f), timerQueue(timerQueue)
    {
        parser->header_limit(4096);
#ifdef BMCWEB_ENABLE_DEBUG
        connectionCount++;
        BMCWEB_LOG_DEBUG << this << " Connection open, total "
                         << connectionCount;
#endif
    }

    ~Connection()
    {
        crow::token_authorization::cleanupTemporarySession(req);
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

    void start(boost::asio::yield_context yield)
    {
        std::shared_ptr<Connection> self = this->shared_from_this();
        boost::system::error_code ec;
        while (true)
        {
            startDeadline();
            if constexpr (std::is_same_v<Adaptor,
                                         boost::beast::ssl_stream<
                                             boost::asio::ip::tcp::socket>>)
            {
                adaptor.async_handshake(boost::asio::ssl::stream_base::server,
                                        yield[ec]);
                if (ec)
                {
                    BMCWEB_LOG_DEBUG << this << " Handshake failed";
                    break;
                }
            }
            BMCWEB_LOG_DEBUG << this << " async_read_header";

            boost::beast::http::async_read_header(adaptor, buffer, *parser,
                                                  yield[ec]);
            if (ec)
            {
                BMCWEB_LOG_ERROR << this
                                 << " Error while reading: " << ec.message();
                cancelDeadlineTimer();
                adaptor.lowest_layer().close();
                BMCWEB_LOG_DEBUG << this << " from read(1)";
                break;
            }

            // After we have the headers, there should be enough information
            // to authenticate the request.  We do this before the payload
            // so we can modify the payload sizes.  Authenticated users can
            // be trusted with larger payloads than unauthenticated users.
            crow::token_authorization::authorizeRequest(req, res);
            if (req.session != nullptr)
            {
                parser->body_limit(30 * 1024 * 1024);
            }
            else
            {
                // TODO(ed) validate that 16KB is a reasonable limit for
                // unauthenticated users.
                parser->body_limit(16 * 1024);
            }
            // Compute the url parameters for the request
            req.url = req.target();
            std::size_t index = req.url.find("?");
            if (index != boost::string_view::npos)
            {
                req.url = req.url.substr(0, index);
            }
            req.urlParams = QueryString(std::string(req.target()));
            BMCWEB_LOG_DEBUG << this << " async_read";
            boost::beast::http::async_read(adaptor, buffer, *parser, yield[ec]);
            if (ec)
            {
                BMCWEB_LOG_ERROR << "Error while reading: " << ec;
                cancelDeadlineTimer();
                adaptor.lowest_layer().close();
                BMCWEB_LOG_DEBUG << this << " from read(1)";
                break;
            }
            handle(yield);
            completeRequest();
            res.preparePayload();
            serializer.emplace(*res.stringResponse);
            BMCWEB_LOG_DEBUG << this << " async_write";
            boost::beast::http::async_write(adaptor, *serializer, yield[ec]);
            if (ec)
            {
                BMCWEB_LOG_DEBUG << this
                                 << " closing after async_write ec=" << ec;
                break;
            }
            if (!req.keepAlive())
            {
                adaptor.lowest_layer().close();
                BMCWEB_LOG_DEBUG << this
                                 << " closing after keepAlive() == false";
                break;
            }
            if (req.isUpgrade())
            {
                break;
            }

            serializer.reset();
            BMCWEB_LOG_DEBUG << this << " Clearing response";
            res.clear();
            parser.emplace(std::piecewise_construct, std::make_tuple());
            buffer.consume(buffer.size());

            parser->header_limit(4096);
            req.req = parser->get();
        }
    }

    void handle(boost::asio::yield_context yield)
    {
        cancelDeadlineTimer();
        const boost::string_view connection =
            req.getHeaderValue(boost::beast::http::field::connection);

        // Check for HTTP version 1.1.
        if (req.version() == 11)
        {
            if (req.getHeaderValue(boost::beast::http::field::host).empty())
            {
                res.result(boost::beast::http::status::bad_request);
                completeRequest();
                return;
            }
        }

        boost::system::error_code ec;
        boost::asio::ip::tcp::endpoint ep =
            adaptor.lowest_layer().remote_endpoint(ec);
        if (ec)
        {
            return;
        }

        std::string ep_name = boost::lexical_cast<std::string>(ep);
        BMCWEB_LOG_INFO << "Request: " << ep_name << " " << this << " HTTP/"
                        << req.version() / 10 << "." << req.version() % 10
                        << ' ' << req.methodString() << " " << req.target();

        needToCallAfterHandlers = false;

        res.completeRequestHandler = [] {};

        req.ioService = &adaptor.get_executor().context();
        bmcweb::addSecurityHeaders(res);
        if (res.completed)
        {
            completeRequest();
            return;
        }
        if (req.isUpgrade())
        {
            handler->handleUpgrade(req, res, std::move(adaptor));
            return;
        }
        res.completeRequestHandler = [this] { this->completeRequest(); };
        needToCallAfterHandlers = true;
        handler->handle(req, res);
        if (req.keepAlive())
        {
            res.addHeader(boost::beast::http::field::connection, "Keep-Alive");
        }
        completeRequest();
    }

    void completeRequest()
    {
        BMCWEB_LOG_INFO << "Response: " << this << ' ' << req.url << ' '
                        << res.resultInt() << " keepalive=" << req.keepAlive();

        if (needToCallAfterHandlers)
        {
            needToCallAfterHandlers = false;
        }
        res.completeRequestHandler = nullptr;

        if (res.body().empty() && !res.jsonValue.empty())
        {
            if (http_helpers::requestPrefersHtml(req))
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
        res.addHeader(boost::beast::http::field::server, "iBMC");
        res.addHeader(boost::beast::http::field::date, getCachedDateStr());

        res.keepAlive(req.keepAlive());
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

        timerCancelKey = timerQueue.add([self = this->shared_from_this()] {
            if (!self->adaptor.lowest_layer().is_open())
            {
                return;
            }
            self->adaptor.lowest_layer().close();
        });
        BMCWEB_LOG_DEBUG << this << " timer added: " << &timerQueue << ' '
                         << timerCancelKey;
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

    crow::Request req;
    crow::Response res;

    int timerCancelKey{-1};

    bool needToCallAfterHandlers{};

    std::function<std::string()>& getCachedDateStr;
    detail::TimerQueue& timerQueue;
};
} // namespace crow
