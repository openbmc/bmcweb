#include "dbus_singleton.hpp"

#include <boost/asio/io_context.hpp>

namespace crow
{
namespace connections
{

void DBusSingleton::initialize(boost::asio::io_context& io)
{
    systemBus(&io);
}

sdbusplus::asio::connection&
    DBusSingleton::systemBus(boost::asio::io_context* io)
{
    // Callers shall call initialize first, where this static local variable is
    // initialized. Afterwards |io| won't be referenced anymore.
    // NOLINTNEXTLINE(clang-analyzer-core.NonNullParamChecker)
    static sdbusplus::asio::connection bus(*io);
    return bus;
}

sdbusplus::asio::connection& DBusSingleton::systemBus()
{
    return systemBus(nullptr);
}

} // namespace connections
} // namespace crow