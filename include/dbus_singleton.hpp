#pragma once
#include <boost/asio/io_context.hpp>
#include <sdbusplus/asio/connection.hpp>

#include <memory>

namespace crow
{
namespace connections
{

// Initialze before using!
// Please see webserver_main for the example how this variable is initialzed,
extern sdbusplus::asio::connection* systemBus;

class DBusConnection
{
  public:
    explicit DBusConnection(
        const std::shared_ptr<boost::asio::io_context>& ioIn);
    sdbusplus::asio::connection* getSystemBus();

  private:
    // Note, |io| shall be destructed after |systemBus|
    std::shared_ptr<boost::asio::io_context> io;
    sdbusplus::asio::connection systemBus;
};

} // namespace connections
} // namespace crow
