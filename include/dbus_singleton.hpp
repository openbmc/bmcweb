#pragma once
#include <sdbusplus/asio/connection.hpp>

namespace crow
{
namespace connections
{

// TODO, this needs to be in its own file/class
boost::asio::io_context& getIoContext();

sdbusplus::asio::connection& systemBus();

} // namespace connections
} // namespace crow
