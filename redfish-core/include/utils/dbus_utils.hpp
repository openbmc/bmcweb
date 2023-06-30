#pragma once

#include "logging.hpp"

#include <sdbusplus/unpack_properties.hpp>

namespace redfish::dbus_utils
{

struct UnpackErrorPrinter
{
    void operator()(const sdbusplus::UnpackErrorReason reason,
                    const std::string& property) const noexcept
    {
        BMCWEB_LOG_ERROR
            << "DBUS property error in property: " << property << ", reason: "
            << static_cast<
                   std::underlying_type_t<sdbusplus::UnpackErrorReason>>(
                   reason);
    }
};

} // namespace redfish::dbus_utils
