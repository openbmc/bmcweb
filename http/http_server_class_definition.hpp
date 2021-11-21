// Just the class definition of a Server, no functions

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
                std::make_shared<boost::asio::io_context>());

        Server(Handler* handlerIn, const std::string& bindaddr, uint16_t port,
            const std::shared_ptr<boost::asio::ssl::context>& adaptorCtx,
            const std::shared_ptr<boost::asio::io_context>& io =
                std::make_shared<boost::asio::io_context>());

        Server(Handler* handlerIn, int existingSocket,
            const std::shared_ptr<boost::asio::ssl::context>& adaptorCtx,
            const std::shared_ptr<boost::asio::io_context>& io =
                std::make_shared<boost::asio::io_context>());

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