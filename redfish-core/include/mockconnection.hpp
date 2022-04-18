#include <boost/asio/io_context.hpp>
#include <sdbusplus/asio/connection.hpp>

#include <iostream>
#include <string>

#define MOCK_CONNECTION
class mockConnection : public sdbusplus::asio::connection
{
  public:
    mockConnection(boost::asio::io_context& io) :
        sdbusplus::asio::connection(io)
    {}

    template <typename MessageHandler, typename... InputArgs>
    void async_method_call(MessageHandler&&,   // handler,
                           const std::string&, /// service,
                           const std::string&, // objpath,
                           const std::string&, // interf,
                           const std::string&, // method,
                           const InputArgs&...)
    {

        std::cerr << "in mock connnection FINDTEXT" << std::endl;
    }
};
