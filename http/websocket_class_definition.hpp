#pragma once
#include "http_request_class_definition.hpp"

namespace crow
{
namespace websocket
{

struct Connection : std::enable_shared_from_this<Connection>
{
  public:
    explicit Connection(const crow::Request& reqIn);

    explicit Connection(const crow::Request& reqIn, std::string user);

    virtual void sendBinary(const std::string_view msg) = 0;
    virtual void sendBinary(std::string&& msg) = 0;
    virtual void sendText(const std::string_view msg) = 0;
    virtual void sendText(std::string&& msg) = 0;
    virtual void close(const std::string_view msg = "quit") = 0;
    virtual boost::asio::io_context& getIoContext() = 0;
    virtual ~Connection() = default;

    void userdata(void* u);
    void* userdata();

    const std::string& getUserName() const;

    boost::beast::http::request<boost::beast::http::string_body> req;
    crow::Response res;

  private:
    std::string userName{};
    void* userdataPtr;
};

template <typename Adaptor>
class ConnectionImpl : public Connection
{
  public:
    ConnectionImpl(
        const crow::Request& reqIn, Adaptor adaptorIn,
        std::function<void(Connection&, std::shared_ptr<bmcweb::AsyncResp>)>
            openHandler,
        std::function<void(Connection&, const std::string&, bool)>
            messageHandler,
        std::function<void(Connection&, const std::string&)> closeHandler,
        std::function<void(Connection&)> errorHandler);

    boost::asio::io_context& getIoContext() override;

    void start();

    void sendBinary(const std::string_view msg) override;

    void sendBinary(std::string&& msg) override;

    void sendText(const std::string_view msg) override;

    void sendText(std::string&& msg) override;

    void close(const std::string_view msg) override;

    void acceptDone();

    void doRead();

    void doWrite();

  private:
    boost::beast::websocket::stream<Adaptor, false> ws;

    std::string inString;
    boost::asio::dynamic_string_buffer<std::string::value_type,
                                       std::string::traits_type,
                                       std::string::allocator_type>
        inBuffer;
    std::vector<std::string> outBuffer;
    bool doingWrite = false;

    std::function<void(Connection&, std::shared_ptr<bmcweb::AsyncResp>)>
        openHandler;
    std::function<void(Connection&, const std::string&, bool)> messageHandler;
    std::function<void(Connection&, const std::string&)> closeHandler;
    std::function<void(Connection&)> errorHandler;
    std::shared_ptr<persistent_data::UserSession> session;
};

}
}