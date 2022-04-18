#include <boost/asio/io_context.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/message.hpp>

#include <iostream>
#include <string>

#define MOCK_CONNECTION
class MockConnection : public sdbusplus::asio::connection
{
  public:
    //    MockConnection(boost::asio::io_context& io) :
    //        sdbusplus::asio::connection(io)
    //    {}

    template <typename MessageHandler, typename... InputArgs>
    void async_method_call(MessageHandler&& handler,
                           const std::string& /*unused*/, // service,
                           const std::string& /*unused*/, // objpath,
                           const std::string& /*unused*/, // interf,
                           const std::string& /*unused*/, // method,
                           const InputArgs&... /*unused*/)
    {

        // std::tuple responseData;

        // std::apply(handler);
    }
};
