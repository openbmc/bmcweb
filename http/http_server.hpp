#pragma once

#include "http_connection.hpp"
#include "logging.hpp"
#include "ssl_key_handler.hpp"

#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <future>
#include <memory>
#include <utility>
#include <vector>
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
#include <persistent_data.hpp>
#endif
namespace crow
{

template <typename Handler, typename Adaptor = boost::asio::ip::tcp::socket>
class Server
{
  public:
    Server(Handler* handlerIn,
           std::unique_ptr<boost::asio::ip::tcp::acceptor>&& acceptorIn,
           std::shared_ptr<boost::asio::ssl::context> adaptorCtxIn,
           std::shared_ptr<boost::asio::io_context> io =
               std::make_shared<boost::asio::io_context>()) :
        ioService(std::move(io)),
        acceptor(std::move(acceptorIn)),
        signals(*ioService, SIGINT, SIGTERM, SIGHUP), handler(handlerIn),
        adaptorCtx(std::move(adaptorCtxIn))
    {
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
        signals.add(SIGUSR1);
#endif
    }

    Server(Handler* handlerIn, uint16_t port,
           const std::shared_ptr<boost::asio::ssl::context>& adaptorCtxIn,
           const std::shared_ptr<boost::asio::io_context>& io =
               std::make_shared<boost::asio::io_context>()) :
        Server(handlerIn,
               std::make_unique<boost::asio::ip::tcp::acceptor>(
                   *io, boost::asio::ip::tcp::endpoint(
                            boost::asio::ip::make_address("0.0.0.0"), port)),
               adaptorCtxIn, io)
    {}

    Server(Handler* handlerIn, int existingSocket,
           const std::shared_ptr<boost::asio::ssl::context>& adaptorCtxIn,
           const std::shared_ptr<boost::asio::io_context>& io =
               std::make_shared<boost::asio::io_context>()) :
        Server(handlerIn,
               std::make_unique<boost::asio::ip::tcp::acceptor>(
                   *io, boost::asio::ip::tcp::v6(), existingSocket),
               adaptorCtxIn, io)
    {}

    void updateDateStr()
    {
        time_t lastTimeT = time(nullptr);
        tm myTm{};

        gmtime_r(&lastTimeT, &myTm);

        dateStr.resize(100);
        size_t dateStrSz = strftime(dateStr.data(), dateStr.size() - 1,
                                    "%a, %d %b %Y %H:%M:%S GMT", &myTm);
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
            return dateStr;
        };

        BMCWEB_LOG_INFO("bmcweb server is running, local endpoint {}",
                        acceptor->local_endpoint().address().to_string());
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
        BMCWEB_LOG_INFO("Building SSL Context file={}", certFile.string());
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
        signals.async_wait(
            [this](const boost::system::error_code& ec, int signalNo) {
            if (ec)
            {
                BMCWEB_LOG_INFO("Error in signal handler{}", ec.message());
            }
            else
            {
                if (signalNo == SIGHUP)
                {
                    BMCWEB_LOG_INFO("Receivied reload signal");
                    loadCertificate();
                    boost::system::error_code ec2;
                    acceptor->cancel(ec2);
                    if (ec2)
                    {
                        BMCWEB_LOG_ERROR(
                            "Error while canceling async operations:{}",
                            ec2.message());
                    }
                    startAsyncWaitForSignal();
                }
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
                if (signalNo == SIGUSR1)
                {
                    BMCWEB_LOG_CRITICAL(
                        "INFO: Receivied USR1 signal to dump latest session  data for bmc dump");
                    persistent_data::getConfig().writeCurrentSessionData();
                    this->startAsyncWaitForSignal();
                }
#endif
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
        boost::asio::steady_timer timer(*ioService);
        std::shared_ptr<Connection<Adaptor, Handler>> connection;
        if constexpr (std::is_same<Adaptor,
                                   boost::beast::ssl_stream<
                                       boost::asio::ip::tcp::socket>>::value)
        {
            connection = std::make_shared<Connection<Adaptor, Handler>>(
                handler, std::move(timer), getCachedDateStr,
                Adaptor(*ioService, *adaptorCtx));
        }
        else
        {
            connection = std::make_shared<Connection<Adaptor, Handler>>(
                handler, std::move(timer), getCachedDateStr,
                Adaptor(*ioService));
        }
        acceptor->async_accept(
            boost::beast::get_lowest_layer(connection->socket()),
            [this, connection](const boost::system::error_code& ec) {
            if (!ec)
            {
                boost::asio::post(*ioService,
                                  [connection] { connection->start(); });
            }
            doAccept();
        });
    }

  private:
    std::shared_ptr<boost::asio::io_context> ioService;
    std::function<std::string()> getCachedDateStr;
    std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor;
    boost::asio::signal_set signals;

    std::string dateStr;

    Handler* handler;

    std::shared_ptr<boost::asio::ssl::context> adaptorCtx;
};
} // namespace crow
