#pragma once
// Just the class definition of a Server, no functions
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include "timer_queue.hpp"
#include <boost/asio/signal_set.hpp>

namespace crow {
    // Repeat the class definition here to make the compiler happy
    template <typename Handler, typename Adaptor>
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

        void updateDateStr();

        void run();

        void loadCertificate();

        void startAsyncWaitForSignal();

        void stop();

        void doAccept();

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
}