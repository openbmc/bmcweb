// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "http_connection.hpp"
#include "logging.hpp"
#include "ssl_key_handler.hpp"

#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core/stream_traits.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <future>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace crow
{

template <typename Handler, typename Adaptor = boost::asio::ip::tcp::socket>
class Server
{
    using self_t = Server<Handler, Adaptor>;

  public:
    Server(Handler* handlerIn, boost::asio::ip::tcp::acceptor&& acceptorIn,
           std::shared_ptr<boost::asio::ssl::context> adaptorCtxIn,
           std::shared_ptr<boost::asio::io_context> io) :
        ioService(std::move(io)), acceptor(std::move(acceptorIn)),
        signals(*ioService, SIGINT, SIGTERM, SIGHUP), handler(handlerIn),
        adaptorCtx(std::move(adaptorCtxIn))
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
                        acceptor.local_endpoint().address().to_string());
        startAsyncWaitForSignal();
        doAccept();
    }

    void loadCertificate()
    {
        if constexpr (BMCWEB_INSECURE_DISABLE_SSL)
        {
            return;
        }

        auto sslContext = ensuressl::getSslServerContext();

        adaptorCtx = sslContext;
        handler->ssl(std::move(sslContext));
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
                        startAsyncWaitForSignal();
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
    using Socket = boost::beast::lowest_layer_type<Adaptor>;
    using SocketPtr = std::unique_ptr<Socket>;

    void afterAccept(SocketPtr socket, const boost::system::error_code& ec)
    {
        if (ec)
        {
            BMCWEB_LOG_ERROR("Failed to accept socket {}", ec);
            return;
        }

        boost::asio::steady_timer timer(*ioService);
        std::shared_ptr<Connection<Adaptor, Handler>> connection;

        if constexpr (std::is_same<Adaptor,
                                   boost::asio::ssl::stream<
                                       boost::asio::ip::tcp::socket>>::value)
        {
            if (adaptorCtx == nullptr)
            {
                BMCWEB_LOG_CRITICAL(
                    "Asked to launch TLS socket but no context available");
                return;
            }
            connection = std::make_shared<Connection<Adaptor, Handler>>(
                handler, std::move(timer), getCachedDateStr,
                Adaptor(std::move(*socket), *adaptorCtx));
        }
        else
        {
            connection = std::make_shared<Connection<Adaptor, Handler>>(
                handler, std::move(timer), getCachedDateStr,
                Adaptor(std::move(*socket)));
        }

        boost::asio::post(*ioService, [connection] { connection->start(); });

        doAccept();
    }

    void doAccept()
    {
        if (ioService == nullptr)
        {
            BMCWEB_LOG_CRITICAL("IoService was null");
            return;
        }

        SocketPtr socket = std::make_unique<Socket>(*ioService);
        // Keep a raw pointer so when the socket is moved, the pointer is still
        // valid
        Socket* socketPtr = socket.get();

        acceptor.async_accept(
            *socketPtr,
            std::bind_front(&self_t::afterAccept, this, std::move(socket)));
    }

  private:
    std::shared_ptr<boost::asio::io_context> ioService;
    std::function<std::string()> getCachedDateStr;
    boost::asio::ip::tcp::acceptor acceptor;
    boost::asio::signal_set signals;

    std::string dateStr;

    Handler* handler;

    std::shared_ptr<boost::asio::ssl::context> adaptorCtx;
};
} // namespace crow
