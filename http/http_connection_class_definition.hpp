#pragma once

namespace crow
{

template <typename Adaptor, typename Handler>
class Connection :
    public std::enable_shared_from_this<Connection<Adaptor, Handler>>
{
  public:
    Connection(Handler* handlerIn,
               std::function<std::string()>& getCachedDateStrF,
               detail::TimerQueue& timerQueueIn, Adaptor adaptorIn);

    ~Connection();

    void prepareMutualTls();

    Adaptor& socket();

    void start();

    void handle();
    
    bool isAlive();

    void close();
    
    void completeRequest();

    void readClientIp();

    boost::system::error_code getClientIp(boost::asio::ip::address& ip);

  private:
    void doReadHeaders();

    void doRead();

    void doWrite();

    void cancelDeadlineTimer();

    void startDeadline(size_t timerIterations);

  private:
    Adaptor adaptor;
    Handler* handler;
    // Making this a std::optional allows it to be efficiently destroyed and
    // re-created on Connection reset
    std::optional<
        boost::beast::http::request_parser<boost::beast::http::string_body>>
        parser;

    boost::beast::flat_static_buffer<8192> buffer;

    std::optional<boost::beast::http::response_serializer<
        boost::beast::http::string_body>>
        serializer;

    std::optional<crow::Request> req;
    crow::Response res;

    bool sessionIsFromTransport = false;
    std::shared_ptr<persistent_data::UserSession> userSession;

    std::optional<size_t> timerCancelKey;

    std::function<std::string()>& getCachedDateStr;
    detail::TimerQueue& timerQueue;

    using std::enable_shared_from_this<
        Connection<Adaptor, Handler>>::shared_from_this;

    void init();
};
}