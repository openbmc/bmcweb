#include "dbus_singleton.hpp"

#include <boost/asio/io_context.hpp>

namespace crow
{
namespace connections
{

sdbusplus::asio::connection* systemBus = nullptr;

} // namespace connections
} // namespace crow
