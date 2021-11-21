#include "websocket_class_decl.hpp"

namespace crow
{

namespace websocket
{

Connection::Connection(const crow::Request& reqIn) :
  req(reqIn.req), userdataPtr(nullptr)
{}

Connection::Connection(const crow::Request& reqIn, std::string user) :
    req(reqIn.req), userName{std::move(user)}, userdataPtr(nullptr)
{}

void Connection::userdata(void* u)
{
    userdataPtr = u;
}
void* Connection::userdata()
{
    return userdataPtr;
}

const std::string& Connection::getUserName() const
{
    return userName;
}



void HandleUpgrade(const crow::Request& req,
                   boost::asio::ip::tcp::socket&& adaptor,
                   std::function<void(crow::websocket::Connection&,
                       std::shared_ptr<bmcweb::AsyncResp>)> openHandler,
                   std::function<void(crow::websocket::Connection&, const std::string&, bool)>
                       messageHandler,
                   std::function<void(crow::websocket::Connection&, const std::string&)>
                        closeHandler,
                   std::function<void(crow::websocket::Connection&)> errorHandler)
{
    std::shared_ptr<
        crow::websocket::ConnectionImpl<boost::asio::ip::tcp::socket>>
        myConnection = std::make_shared<
            crow::websocket::ConnectionImpl<boost::asio::ip::tcp::socket>>(
            req, std::move(adaptor), openHandler, messageHandler,
            closeHandler, errorHandler);
    myConnection->start();
}


#ifdef BMCWEB_ENABLE_SSL
void HandleUpgradeSSL(const crow::Request& req,
                      boost::beast::ssl_stream<boost::asio::ip::tcp::socket>&& adaptor,
                      std::function<void(crow::websocket::Connection&,
                          std::shared_ptr<bmcweb::AsyncResp>)> openHandler,
                      std::function<void(crow::websocket::Connection&, const std::string&, bool)>
                          messageHandler,
                      std::function<void(crow::websocket::Connection&, const std::string&)>
                           closeHandler,
                      std::function<void(crow::websocket::Connection&)> errorHandler)
{
    std::shared_ptr<crow::websocket::ConnectionImpl<
        boost::beast::ssl_stream<boost::asio::ip::tcp::socket>>>
        myConnection = std::make_shared<crow::websocket::ConnectionImpl<
            boost::beast::ssl_stream<boost::asio::ip::tcp::socket>>>(
            req, std::move(adaptor), openHandler, messageHandler,
            closeHandler, errorHandler);
    myConnection->start();
}

} // namespace websocket
} // namespace crow


#endif