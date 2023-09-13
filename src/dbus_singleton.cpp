#include "dbus_singleton.hpp"

namespace crow
{
namespace connections
{
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
sdbusplus::asio::connection* systemBus = nullptr;

boost::asio::io_context& getNextIoContext()
{
    int threadCount = 4;
    static boost::asio::io_context io(threadCount);
    return io;
}

} // namespace connections
} // namespace crow
