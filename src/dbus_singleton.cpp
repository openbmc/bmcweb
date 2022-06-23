#include "dbus_singleton.hpp"

#include <boost/asio/io_context.hpp>

#include <memory>

namespace crow
{
namespace connections
{

sdbusplus::asio::connection* systemBus = nullptr;

DBusConnection::DBusConnection(
    const std::shared_ptr<boost::asio::io_context>& ioIn) :
    io(ioIn),
    systemBus(*io)
{}

sdbusplus::asio::connection* DBusConnection::getSystemBus()
{
    return &systemBus;
}

} // namespace connections
} // namespace crow
