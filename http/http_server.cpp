#include <boost/beast/ssl/ssl_stream.hpp>

#include <unistd.h>
#include <optional>
#include <boost/asio/signal_set.hpp>
#include "timer_queue.hpp"
#include "http_server_class_decl.hpp"
#include "app_class_decl.hpp"
#include "http_connection_class_decl.hpp"
#include "../include/ssl_key_handler.hpp"

namespace crow
{

using Handler = crow::App;
#ifdef BMCWEB_ENABLE_SSL
using Adaptor = boost::beast::ssl_stream<boost::asio::ip::tcp::socket>;
#else
using Adaptor = boost::asio::ip::tcp::socket
#endif

template <>
void Server<Handler, Adaptor>::updateDateStr()
{
    time_t lastTimeT = time(nullptr);
    tm myTm{};

    gmtime_r(&lastTimeT, &myTm);

    dateStr.resize(100);
    size_t dateStrSz =
        strftime(&dateStr[0], 99, "%a, %d %b %Y %H:%M:%S GMT", &myTm);
    dateStr.resize(dateStrSz);
}

template<>
void Server<Handler, Adaptor>::startAsyncWaitForSignal()
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

template<>
void Server<Handler, Adaptor>::doAccept()
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
        #ifndef BMCWEB_ENABLE_SSL
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
        #endif
    }
}

// specialization after instantiation?
template<typename Handler, typename Adaptor>
void Server<Handler, Adaptor>::loadCertificate()
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

template<>
void Server<Handler, Adaptor>::run()
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

// Specialization after initialization
template<typename Handler, typename Adaptor>
void Server<Handler, Adaptor>::stop()
{
    ioService->stop();
}

}