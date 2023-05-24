#include "dbus_singleton.hpp"

namespace crow
{
namespace connections
{
boost::asio::io_context& getIoContext()
{
    int threadCount = 4;
    static boost::asio::io_context io(threadCount);
    return io;
}

sdbusplus::asio::connection& systemBus()
{
    thread_local static sdbusplus::asio::connection systemBus(getIoContext());
    return systemBus;
}

} // namespace connections
} // namespace crow
