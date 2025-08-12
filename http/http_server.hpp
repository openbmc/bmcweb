// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "bmcweb_config.h"

#include "http_auth_modes.hpp"
#include "http_connect_types.hpp"
#include "http_connection.hpp"
#include "io_context_singleton.hpp"
#include "logging.hpp"
#include "ssl_key_handler.hpp"

#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/steady_timer.hpp>

#include <chrono>
#include <csignal>
#include <cstddef>
#include <ctime>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace crow
{

struct Acceptor
{
    boost::asio::ip::tcp::acceptor acceptor;
    HttpType httpType;
    AuthMode httpAuthMode;
};

template <typename Handler, typename Adaptor = boost::asio::ip::tcp::socket>
class Server
{
    using self_t = Server<Handler, Adaptor>;

  public:
    Server(Handler* handlerIn, std::vector<Acceptor>&& acceptorsIn) :
        acceptors(std::move(acceptorsIn)),

        // NOLINTNEXTLINE(misc-include-cleaner)
        signals(getIoContext(), SIGINT, SIGTERM, SIGHUP), handler(handlerIn)
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

        for (const Acceptor& accept : acceptors)
        {
            BMCWEB_LOG_INFO(
                "bmcweb server is running, local endpoint {}",
                accept.acceptor.local_endpoint().address().to_string());
        }
        startAsyncWaitForSignal();
        doAccept();
    }

    void loadCertificate()
    {
        if constexpr (BMCWEB_INSECURE_DISABLE_SSL)
        {
            return;
        }

        adaptorCtx = ensuressl::getSslServerContext();
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
                        getIoContext().stop();
                    }
                }
            });
    }

    using SocketPtr = std::unique_ptr<Adaptor>;

    void afterAccept(SocketPtr socket, HttpType httpType, AuthMode httpAuthMode,
                     const boost::system::error_code& ec)
    {
        if (ec)
        {
            BMCWEB_LOG_ERROR("Failed to accept socket {}", ec);
            return;
        }

        boost::asio::steady_timer timer(getIoContext());
        if (adaptorCtx == nullptr)
        {
            adaptorCtx = std::make_shared<boost::asio::ssl::context>(
                boost::asio::ssl::context::tls_server);
        }

        boost::asio::ssl::stream<Adaptor> stream(std::move(*socket),
                                                 *adaptorCtx);
        using ConnectionType = Connection<Adaptor, Handler>;
        auto connection = std::make_shared<ConnectionType>(
            handler, httpType, std::move(timer), getCachedDateStr,
            std::move(stream), httpAuthMode);

        boost::asio::post(getIoContext(),
                          [connection] { connection->start(); });

        doAccept();
    }

    void doAccept()
    {
        for (Acceptor& accept : acceptors)
        {
            SocketPtr socket = std::make_unique<Adaptor>(getIoContext());
            // Keep a raw pointer so when the socket is moved, the pointer is
            // still valid
            Adaptor* socketPtr = socket.get();
            accept.acceptor.async_accept(
                *socketPtr,
                std::bind_front(&self_t::afterAccept, this, std::move(socket),
                                accept.httpType, accept.httpAuthMode));
        }
    }

  private:
    std::function<std::string()> getCachedDateStr;
    std::vector<Acceptor> acceptors;
    boost::asio::signal_set signals;

    std::string dateStr;

    Handler* handler;

    std::shared_ptr<boost::asio::ssl::context> adaptorCtx;
};
} // namespace crow
