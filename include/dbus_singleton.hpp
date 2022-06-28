#pragma once
#include <sdbusplus/asio/connection.hpp>

namespace crow
{
namespace connections
{
#ifdef BMCWEB_ENABLE_REDFISH_DBUS
static std::shared_ptr<sdbusplus::asio::connection> systemBus;
#endif
} // namespace connections
} // namespace crow
