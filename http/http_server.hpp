#pragma once

#include "http_connection.hpp"
#include "logging.hpp"
#include "timer_queue.hpp"

#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <ssl_key_handler.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <future>
#include <memory>
#include <utility>
#include <vector>

namespace crow
{

template <typename Handler, typename Adaptor = boost::asio::ip::tcp::socket>
class Server
{
  public:
    Server(Handler* handlerIn,
           std::unique_ptr<boost::asio::ip::tcp::acceptor>&& acceptorIn,
           std::shared_ptr<boost::asio::ssl::context> adaptorCtx,
           std::shared_ptr<boost::asio::io_context> io =
               std::make_shared<boost::asio::io_context>()) :
        ioService(std::move(io)),
        acceptor(std::move(acceptorIn)),
        signals(*ioService, SIGINT, SIGTERM, SIGHUP), timer(*ioService),
        handler(handlerIn), adaptorCtx(std::move(adaptorCtx))
    {}

    Server(Handler* handlerIn, const std::string& bindaddr, uint16_t port,
           const std::shared_ptr<boost::asio::ssl::context>& adaptorCtx,
           const std::shared_ptr<boost::asio::io_context>& io =
               std::make_shared<boost::asio::io_context>()) :
        Server(handlerIn,
               std::make_unique<boost::asio::ip::tcp::acceptor>(
                   *io, boost::asio::ip::tcp::endpoint(
                            boost::asio::ip::make_address(bindaddr), port)),
               adaptorCtx, io)
    {}

    Server(Handler* handlerIn, int existingSocket,
           const std::shared_ptr<boost::asio::ssl::context>& adaptorCtx,
           const std::shared_ptr<boost::asio::io_context>& io =
               std::make_shared<boost::asio::io_context>()) :
        Server(handlerIn,
               std::make_unique<boost::asio::ip::tcp::acceptor>(
                   *io, boost::asio::ip::tcp::v6(), existingSocket),
               adaptorCtx, io)
    {}

    void updateDateStr()
    {
        time_t lastTimeT = time(nullptr);
        tm myTm{};

        gmtime_r(&lastTimeT, &myTm);

        dateStr.resize(100);
        size_t dateStrSz =
            strftime(&dateStr[0], 99, "%a, %d %b %Y %H:%M:%S GMT", &myTm);
        dateStr.resize(dateStrSz);
    }

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

        BMCWEB_LOG_INFO << "bmcweb server is running, local endpoint "
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
                    boost::system::error_code ec2;
                    acceptor->cancel(ec2);
                    if (ec2)
                    {
                        BMCWEB_LOG_ERROR
                            << "Error while canceling async operations:"
                            << ec2.message();
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
            auto p = std::make_shared<Connection<Adaptor, Handler>>(
                handler, getCachedDateStr, timerQueue,
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
            auto p = std::make_shared<Connection<Adaptor, Handler>>(
                handler, getCachedDateStr, timerQueue,
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
    std::shared_ptr<boost::asio::io_context> ioService;
    detail::TimerQueue timerQueue;
    std::function<std::string()> getCachedDateStr;
    std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor;
    boost::asio::signal_set signals;
    boost::asio::steady_timer timer;

    std::string dateStr;

    Handler* handler;

    std::function<void(const boost::system::error_code& ec)> timerHandler;

#ifdef BMCWEB_ENABLE_SSL
    bool useSsl{false};
#endif
    std::shared_ptr<boost::asio::ssl::context> adaptorCtx;
}; // namespace crow
} // namespace crow
