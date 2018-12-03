#pragma once

#include <atomic>
<<<<<<< HEAD
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
=======
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
>>>>>>> e700991... Various cleanups
#include <chrono>
#include <cstdint>
#include <future>
#include <memory>
#include <utility>
#include <vector>

#include "crow/http_connection.h"
#include "crow/logging.h"
#include "crow/timer_queue.h"
#ifdef BMCWEB_ENABLE_SSL
#include <boost/asio/ssl/context.hpp>
#include <boost/beast/experimental/core/ssl_stream.hpp>
#endif

namespace crow
{

template <typename Handler, typename Adaptor> class Server
{

  public:
    Server(Handler* handler, const std::string& bindaddr, uint16_t port,
           boost::asio::ssl::context* adaptor_ctx,
           boost::asio::io_service& io) :
        acceptor(io,
                 boost::asio::ip::tcp::endpoint(
                     boost::asio::ip::address::from_string(bindaddr), port)),
        signals(io, SIGINT, SIGTERM), tickTimer(io), handler(handler),
        adaptorCtx(adaptor_ctx)
    {
    }

    Server(Handler* handler, int existing_socket,
           boost::asio::ssl::context* adaptor_ctx,
           boost::asio::io_service& io) :
        acceptor(io, boost::asio::ip::tcp::v6(), existing_socket),
        signals(io, SIGINT, SIGTERM), tickTimer(io), handler(handler),
        adaptorCtx(adaptor_ctx)
    {
    }

  public:
    void onTick(boost::system::error_code ec)
    {
        if (ec)
        {
            return;
        }
        timerQueue.process();
        tickTimer.expires_after(std::chrono::seconds(1));
        tickTimer.async_wait(
            [this](const boost::system::error_code& ec) { onTick(ec); });
    }

    void updateDateStr()
    {
        auto lastTimeT = time(0);
        tm myTm{};

#ifdef _MSC_VER
        gmtime_s(&my_tm, &last_time_t);
#else
        gmtime_r(&lastTimeT, &myTm);
#endif
        dateStr.resize(100);
        size_t dateStrSz =
            strftime(&dateStr[0], 99, "%a, %d %b %Y %H:%M:%S GMT", &myTm);
        dateStr.resize(dateStrSz);
    };

    void run()
    {
        updateDateStr();

        getCachedDateStr = [this]() -> std::string {
            static std::chrono::time_point<std::chrono::steady_clock>
                lastDateUpdate = std::chrono::steady_clock::now();
            if (std::chrono::steady_clock::now() - lastDateUpdate >=
                std::chrono::seconds(10))
            {
                lastDateUpdate = std::chrono::steady_clock::now();
                updateDateStr();
            }
            return this->dateStr;
        };

        tickTimer.expires_after(std::chrono::seconds(1));
        tickTimer.async_wait(
            [this](const boost::system::error_code& ec) { onTick(ec); });

        BMCWEB_LOG_INFO << "server is running, local endpoint "
                        << acceptor.local_endpoint();

        signals.async_wait(
            [this](const boost::system::error_code& ec, int /*sig_num*/) {
                if (ec)
                {
                    return;
                }
                stop();
            });

        boost::asio::spawn(
            acceptor.get_executor().context(),
            [this](boost::asio::yield_context yield) { doAccept(yield); });
    }

    void stop()
    {
        acceptor.get_executor().context().stop();
    }

    void doAccept(boost::asio::yield_context yield)
    {
        boost::system::error_code ec;

        for (;;)
        {
            std::shared_ptr<Connection<Adaptor, Handler>> p;
            if constexpr (std::is_same<
                              Adaptor,
                              boost::beast::ssl_stream<
                                  boost::asio::ip::tcp::socket>>::value)
            {
                p = std::make_shared<Connection<Adaptor, Handler>>(
                    acceptor.get_executor().context(), handler,
                    getCachedDateStr, timerQueue,
                    Adaptor(acceptor.get_executor().context(), *adaptorCtx));
            }
            else
            {
                p = std::make_shared<Connection<Adaptor, Handler>>(
                    acceptor.get_executor().context(), handler,
                    getCachedDateStr, timerQueue,
                    Adaptor(acceptor.get_executor().context()));
            }

            acceptor.async_accept(p->socket().lowest_layer(), yield[ec]);
            if (ec)
            {
                continue;
            }

            boost::asio::spawn(
                acceptor.get_executor().context(),
                [p = std::move(p)](boost::asio::yield_context yield) {
                    p->start(yield);
                });
        }
    }

  private:
    detail::TimerQueue timerQueue;
    std::function<std::string()> getCachedDateStr;
    boost::asio::ip::tcp::acceptor acceptor;
    boost::asio::signal_set signals;
    boost::asio::steady_timer tickTimer;

    std::string dateStr;

    Handler* handler;

    boost::asio::ssl::context* adaptorCtx;
};
} // namespace crow
