#pragma once

#include <sdbusplus/unpack_properties.hpp>

namespace redfish
{

namespace log_utils
{

constexpr const char* dbusResponseError = "DBUS response error";

inline void logPropertyError(const sdbusplus::UnpackError& error)
{
    BMCWEB_LOG_DEBUG << "DBUS property error - reason: " << error.reason
                     << ", index: " << error.index;
}

} // namespace log_utils

} // namespace redfish
