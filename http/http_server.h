#pragma once

#include <atomic>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/steady_timer.hpp>
#if BOOST_VERSION >= 107000
#include <boost/beast/ssl/ssl_stream.hpp>
#else
#include <boost/beast/experimental/core/ssl_stream.hpp>
#endif

#include <boost/date_time/posix_time/posix_time.hpp>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <future>
#include <memory>
#include <ssl_key_handler.hpp>
#include <utility>
#include <vector>

#include "http_connection.h"
#include "logging.h"
#include "timer_queue.h"

namespace crow
{
using namespace boost;
using tcp = asio::ip::tcp;

template <typename Handler, typename Adaptor = boost::asio::ip::tcp::socket,
          typename... Middlewares>
class Server
{
  public:
    Server(Handler* handler, std::unique_ptr<tcp::acceptor>&& acceptor,
           std::shared_ptr<boost::asio::ssl::context> adaptor_ctx,
           std::tuple<Middlewares...>* middlewares = nullptr,
           std::shared_ptr<boost::asio::io_context> io =
               std::make_shared<boost::asio::io_context>()) :
        ioService(std::move(io)),
        acceptor(std::move(acceptor)),
        signals(*ioService, SIGINT, SIGTERM, SIGHUP), tickTimer(*ioService),
        timer(*ioService), handler(handler), middlewares(middlewares),
        adaptorCtx(adaptor_ctx)
    {
    }

    Server(Handler* handler, const std::string& bindaddr, uint16_t port,
           std::shared_ptr<boost::asio::ssl::context> adaptor_ctx,
           std::tuple<Middlewares...>* middlewares = nullptr,
           std::shared_ptr<boost::asio::io_context> io =
               std::make_shared<boost::asio::io_context>()) :
        Server(handler,
               std::make_unique<tcp::acceptor>(
                   *io, tcp::endpoint(boost::asio::ip::make_address(bindaddr),
                                      port)),
               adaptor_ctx, middlewares, io)
    {
    }

    Server(Handler* handler, int existing_socket,
           std::shared_ptr<boost::asio::ssl::context> adaptor_ctx,
           std::tuple<Middlewares...>* middlewares = nullptr,
           std::shared_ptr<boost::asio::io_context> io =
               std::make_shared<boost::asio::io_context>()) :
        Server(handler,
               std::make_unique<tcp::acceptor>(*io, boost::asio::ip::tcp::v6(),
                                               existing_socket),
               adaptor_ctx, middlewares, io)
    {
    }

    void setTickFunction(std::chrono::milliseconds d, std::function<void()> f)
    {
        tickInterval = d;
        tickFunction = f;
    }

    void onTick()
    {
        tickFunction();
        tickTimer.expires_after(
            std::chrono::milliseconds(tickInterval.count()));
        tickTimer.async_wait([this](const boost::system::error_code& ec) {
            if (ec)
            {
                return;
            }
            onTick();
        });
    }

    void updateDateStr()
    {
        time_t lastTimeT = time(nullptr);
        tm myTm{};

        gmtime_r(&lastTimeT, &myTm);

        dateStr.resize(100);
        size_t dateStrSz =
            strftime(&dateStr[0], 99, "%a, %d %b %Y %H:%M:%S GMT", &myTm);
        dateStr.resize(dateStrSz);
    };

    void run()
    {
        loadCertificate();
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

        timer.expires_after(std::chrono::seconds(1));

        timerHandler = [this](const boost::system::error_code& ec) {
            if (ec)
            {
                return;
            }
            timerQueue.process();
            timer.expires_after(std::chrono::seconds(1));
            timer.async_wait(timerHandler);
        };
        timer.async_wait(timerHandler);

        if (tickFunction && tickInterval.count() > 0)
        {
            tickTimer.expires_after(
                std::chrono::milliseconds(tickInterval.count()));
            tickTimer.async_wait([this](const boost::system::error_code& ec) {
                if (ec)
                {
                    return;
                }
                onTick();
            });
        }

        BMCWEB_LOG_INFO << serverName << " server is running, local endpoint "
                        << acceptor->local_endpoint();
        startAsyncWaitForSignal();
        doAccept();
    }

    void loadCertificate()
    {
#ifdef BMCWEB_ENABLE_SSL
        namespace fs = std::filesystem;
        // Cleanup older certificate file existing in the system
        fs::path oldCert = "/home/root/server.pem";
        if (fs::exists(oldCert))
        {
            fs::remove("/home/root/server.pem");
        }
        fs::path certPath = "/etc/ssl/certs/https/";
        // if path does not exist create the path so that
        // self signed certificate can be created in the
        // path
        if (!fs::exists(certPath))
        {
            fs::create_directories(certPath);
        }
        fs::path certFile = certPath / "server.pem";
        BMCWEB_LOG_INFO << "Building SSL Context file=" << certFile;
        std::string sslPemFile(certFile);
        ensuressl::ensureOpensslKeyPresentAndValid(sslPemFile);
        std::shared_ptr<boost::asio::ssl::context> sslContext =
            ensuressl::getSslContext(sslPemFile);
        adaptorCtx = sslContext;
        handler->ssl(std::move(sslContext));
#endif
    }

    void startAsyncWaitForSignal()
    {
        signals.async_wait([this](const boost::system::error_code& ec,
                                  int signalNo) {
            if (ec)
            {
                BMCWEB_LOG_INFO << "Error in signal handler" << ec.message();
            }
            else
            {
                if (signalNo == SIGHUP)
                {
                    BMCWEB_LOG_INFO << "Receivied reload signal";
                    loadCertificate();
                    boost::system::error_code ec;
                    acceptor->cancel(ec);
                    if (ec)
                    {
                        BMCWEB_LOG_ERROR
                            << "Error while canceling async operations:"
                            << ec.message();
                    }
                    this->startAsyncWaitForSignal();
                }
                else
                {
                    stop();
                }
            }
        });
    }

    void stop()
    {
        ioService->stop();
    }

    void doAccept()
    {
        std::optional<Adaptor> adaptorTemp;
        if constexpr (std::is_same<Adaptor,
                                   boost::beast::ssl_stream<
                                       boost::asio::ip::tcp::socket>>::value)
        {
            adaptorTemp = Adaptor(*ioService, *adaptorCtx);
            auto p =
                std::make_shared<Connection<Adaptor, Handler, Middlewares...>>(
                    *ioService, handler, serverName, middlewares,
                    getCachedDateStr, timerQueue,
                    std::move(adaptorTemp.value()));

            acceptor->async_accept(p->socket().next_layer(),
                                   [this, p](boost::system::error_code ec) {
                                       if (!ec)
                                       {
                                           boost::asio::post(
                                               *this->ioService,
                                               [p] { p->start(); });
                                       }
                                       doAccept();
                                   });
        }
        else
        {
            adaptorTemp = Adaptor(*ioService);
            auto p =
                std::make_shared<Connection<Adaptor, Handler, Middlewares...>>(
                    *ioService, handler, serverName, middlewares,
                    getCachedDateStr, timerQueue,
                    std::move(adaptorTemp.value()));

            acceptor->async_accept(
                p->socket(), [this, p](boost::system::error_code ec) {
                    if (!ec)
                    {
                        boost::asio::post(*this->ioService,
                                          [p] { p->start(); });
                    }
                    doAccept();
                });
        }
    }

  private:
    std::shared_ptr<asio::io_context> ioService;
    detail::TimerQueue timerQueue;
    std::function<std::string()> getCachedDateStr;
    std::unique_ptr<tcp::acceptor> acceptor;
    boost::asio::signal_set signals;
    boost::asio::steady_timer tickTimer;
    boost::asio::steady_timer timer;

    std::string dateStr;

    Handler* handler;
    std::string serverName = "iBMC";

    std::chrono::milliseconds tickInterval{};
    std::function<void()> tickFunction;
    std::function<void(const boost::system::error_code& ec)> timerHandler;

    std::tuple<Middlewares...>* middlewares;

#ifdef BMCWEB_ENABLE_SSL
    bool useSsl{false};
#endif
    std::shared_ptr<boost::asio::ssl::context> adaptorCtx;
}; // namespace crow
} // namespace crow
