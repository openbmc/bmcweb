#ifdef BMCWEB_ENABLE_REDFISH_DBUS
#include "dbus_singleton.hpp"

namespace crow
{
namespace connections
{
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
sdbusplus::asio::connection* systemBus = nullptr;

} // namespace connections
} // namespace crow
#endif
