
#pragma once
#include "bmcweb_config.h"
#include "async_resp.hpp"
#include "authentication.hpp"
#include "complete_response_fields.hpp"
#include "http2_connection.hpp"
#include "http_response.hpp"
#include "http_utility.hpp"
#include "logging.hpp"
#include "mutual_tls.hpp"
#include "ssl_key_handler.hpp"
#include "utility.hpp"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core/buffers_generator.hpp>
#include <boost/beast/core/flat_static_buffer.hpp>
#include <boost/beast/http/error.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/beast/websocket.hpp>
#include <atomic>
#include <chrono>
#include <vector>
namespace crow
{
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static int connectionCount = 0;
// request body limit size set by the bmcwebHttpReqBodyLimitMb option
constexpr uint64_t httpReqBodyLimit = 1024UL * 1024UL *
                                      bmcwebHttpReqBodyLimitMb;
constexpr uint64_t loggedOutPostBodyLimit = 4096;
constexpr uint32_t httpHeaderLimit = 8192;
template <typename Adaptor, typename Handler>
class Connection :
    public std::enable_shared_from_this<Connection<Adaptor, Handler>>
{
    using self_type = Connection<Adaptor, Handler>;
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
            if (!parser->is_done())
            {
                doRead();
                return;
            }
            cancelDeadlineTimer();
            handle();
            });
    }
    void afterDoWrite(const std::shared_ptr<self_type>& /*self*/,
                      const boost::system::error_code& ec,
                      std::size_t bytesTransferred)
    {
        BMCWEB_LOG_DEBUG("{} async_write {} bytes", logPtr(this),
                         bytesTransferred);
        cancelDeadlineTimer();
        if (ec)
        {
            BMCWEB_LOG_DEBUG("{} from write(2)", logPtr(this));
            return;
        }
        if (!keepAlive)
        {
            close();
            BMCWEB_LOG_DEBUG("{} from write(1)", logPtr(this));
            return;
        }
        BMCWEB_LOG_DEBUG("{} Clearing response", logPtr(this));
        res.clear();
        parser.emplace(std::piecewise_construct, std::make_tuple());
        parser->body_limit(httpReqBodyLimit); // reset body limit for
                                              // newly created parser
        buffer.consume(buffer.size());
        userSession = nullptr;
        // Destroy the Request via the std::optional
        req.reset();
        doReadHeaders();
    }
    void doWrite(crow::Response& thisRes)
    {
        BMCWEB_LOG_DEBUG("{} doWrite", logPtr(this));
        thisRes.preparePayload();
        startDeadline();
        boost::beast::async_write(adaptor, thisRes.generator(),
                                  std::bind_front(&self_type::afterDoWrite,
                                                  this, shared_from_this()));
    }






    void cancelDeadlineTimer()
    {
        timer.cancel();
    }
    void startDeadline()
    {
        // Timer is already started so no further action is required.
        if (timerStarted)
        {
            return;
        }
        std::chrono::seconds timeout(15);
        std::weak_ptr<Connection<Adaptor, Handler>> weakSelf = weak_from_this();
        timer.expires_after(timeout);
        timer.async_wait([weakSelf](const boost::system::error_code& ec) {
            // Note, we are ignoring other types of errors here;  If the timer
            // failed for any reason, we should still close the connection
            std::shared_ptr<Connection<Adaptor, Handler>> self =
                weakSelf.lock();
            if (!self)
            {
                BMCWEB_LOG_CRITICAL("{} Failed to capture connection",
                                    logPtr(self.get()));
                return;
            }
            self->timerStarted = false;
            if (ec == boost::asio::error::operation_aborted)
            {
                // Canceled wait means the path succeeeded.
                return;
            }
            if (ec)
            {
                BMCWEB_LOG_CRITICAL("{} timer failed {}", logPtr(self.get()),
                                    ec);
            }
            BMCWEB_LOG_WARNING("{}Connection timed out, closing",
                               logPtr(self.get()));
            self->close();
        });
        timerStarted = true;
        BMCWEB_LOG_DEBUG("{} timer started", logPtr(this));
    }
    Adaptor adaptor;
    Handler* handler;
    // Making this a std::optional allows it to be efficiently destroyed and
    // re-created on Connection reset
    std::optional<
        boost::beast::http::request_parser<boost::beast::http::string_body>>
        parser;
    boost::beast::flat_static_buffer<8192> buffer;
    std::optional<crow::Request> req;
    crow::Response res;
    std::shared_ptr<persistent_data::UserSession> userSession;
    std::shared_ptr<persistent_data::UserSession> mtlsSession;
    boost::asio::steady_timer timer;
    bool keepAlive = true;
    bool timerStarted = false;
    std::function<std::string()>& getCachedDateStr;
    using std::enable_shared_from_this<
        Connection<Adaptor, Handler>>::shared_from_this;
    using std::enable_shared_from_this<
        Connection<Adaptor, Handler>>::weak_from_this;
};
} // namespace crow
