#include "dbus_singleton.hpp"

#include <boost/asio/io_context.hpp>

namespace crow
{
namespace connections
{

sdbusplus::asio::connection* systemBus = nullptr;

DBusConnection::DBusConnection(boost::asio::io_context& io) : systemBus(io)
{}

sdbusplus::asio::connection* DBusConnection::getSystemBus()
{
    return &systemBus;
}

} // namespace connections
} // namespace crow
