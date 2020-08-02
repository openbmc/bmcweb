#pragma once

#include "http_connect_types.hpp"
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

struct Acceptor
{
    boost::asio::ip::tcp::acceptor acceptor;
    HttpType httpType;
};

template <typename Handler, typename Adaptor = boost::asio::ip::tcp::socket>
class Server
{
  public:
    Server(Handler* handlerIn, std::vector<Acceptor>&& acceptorsIn,
           std::shared_ptr<boost::asio::ssl::context> adaptorCtx,
           std::shared_ptr<boost::asio::io_context> io =
               std::make_shared<boost::asio::io_context>()) :
        ioService(std::move(io)),
        acceptors(std::move(acceptorsIn)),
        signals(*ioService, SIGINT, SIGTERM, SIGHUP), timer(*ioService),
        handler(handlerIn), adaptorCtx(std::move(adaptorCtx))
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

        for (Acceptor& accept : acceptors)
        {
            BMCWEB_LOG_INFO << "bmcweb server is running, local endpoint "
                            << accept.acceptor.local_endpoint();
        }
        startAsyncWaitForSignal();

        for (Acceptor& acceptor : acceptors)
        {
            doAccept(acceptor);
        }
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

                    for (Acceptor& acc : acceptors)
                    {
                        acc.acceptor.cancel(ec2);
                        if (ec2)
                        {
                            BMCWEB_LOG_ERROR
                                << "Error while canceling async operations:"
                                << ec2.message();
                        }
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

    void doAccept(Acceptor& acc)
    {
        auto p = std::make_shared<Connection<Adaptor, Handler>>(
            handler, getCachedDateStr, timerQueue, adaptorCtx, *ioService,
            acc.httpType);

        acc.acceptor.async_accept(
            p->socket(), [this, p, &acc](boost::system::error_code ec) {
                if (!ec)
                {
                    boost::asio::post(*this->ioService, [p] { p->start(); });
                }
                doAccept(acc);
            });
    }

  private:
    std::shared_ptr<boost::asio::io_context> ioService;
    detail::TimerQueue timerQueue;
    std::function<std::string()> getCachedDateStr;

    std::vector<Acceptor> acceptors;
    boost::asio::signal_set signals;
    boost::asio::steady_timer timer;

    std::string dateStr;

    Handler* handler;

    std::function<void(const boost::system::error_code& ec)> timerHandler;

    std::shared_ptr<boost::asio::ssl::context> adaptorCtx;
};
} // namespace crow
