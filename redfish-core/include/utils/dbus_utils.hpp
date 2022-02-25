#pragma once

#include <sdbusplus/unpack_properties.hpp>

namespace redfish
{
namespace dbus_utils
{

struct UnpackErrorPrinter
{
    void operator()(const sdbusplus::UnpackErrorReason reason,
                    const std::string& property) const noexcept
    {
        BMCWEB_LOG_DEBUG << "DBUS property error in property: " << property
                         << ", reason: " << reason;
    }
};

} // namespace dbus_utils
} // namespace redfish
