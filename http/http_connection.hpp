#pragma once
#include "bmcweb_config.h"

#include "authorization.hpp"
#include "http_response_class_decl.hpp"
#include "http_utility.hpp"
#include "logging.hpp"
#include "timer_queue.hpp"
#include "utility.hpp"

#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core/flat_static_buffer.hpp>
#include <boost/url/url_view.hpp>
#include <json_html_serializer.hpp>
#include <security_headers.hpp>
#include <ssl_key_handler.hpp>
#include <boost/asio/ip/tcp.hpp>

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
               detail::TimerQueue& timerQueueIn, Adaptor adaptorIn);

    ~Connection();

    void prepareMutualTls();

    Adaptor& socket();

    void start();

    void handle();
    
    bool isAlive();
    void close();
    void completeRequest();
    void readClientIp();
    boost::system::error_code getClientIp(boost::asio::ip::address& ip);

  private:
    void doReadHeaders();

    void doRead();
    void doWrite();

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

    bool sessionIsFromTransport = false;
    std::shared_ptr<persistent_data::UserSession> userSession;

    std::optional<size_t> timerCancelKey;

    std::function<std::string()>& getCachedDateStr;
    detail::TimerQueue& timerQueue;

    using std::enable_shared_from_this<
        Connection<Adaptor, Handler>>::shared_from_this;

    void init();
};
} // namespace crow
