#pragma once
#include <boost/asio/io_context.hpp>
#include <sdbusplus/asio/connection.hpp>

namespace crow
{
namespace connections
{

// Kept for backward compatibility. Initialze before using!
// Please see webserver_main for the example how this variable is initialzed,
extern sdbusplus::asio::connection* systemBus;

// A wrapper around sdbusplus::asio::connection.
// Typical usage:
//  DBusSingleton::initialize(io);
//  DBusSingleton::systemBus()->async_method_call(...);
class DBusSingleton
{
  public:
    static void initialize(boost::asio::io_context& io);
    static sdbusplus::asio::connection& systemBus();

  private:
    static sdbusplus::asio::connection& systemBus(boost::asio::io_context* io);
};

} // namespace connections
} // namespace crow
