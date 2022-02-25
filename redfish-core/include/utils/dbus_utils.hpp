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

struct UnpackErrorHandler
{
    UnpackErrorHandler(crow::Response& res) : res(res)
    {}

    void operator()(const sdbusplus::UnpackErrorReason reason,
                    const std::string& property) const noexcept
    {
        UnpackErrorPrinter()(reason, property);

        switch (reason)
        {
            case sdbusplus::UnpackErrorReason::missingProperty:
                messages::propertyMissing(res, property);
                break;
            case sdbusplus::UnpackErrorReason::wrongType:
                messages::propertyValueTypeError(res, "", property);
                break;
        }
    }

  private:
    crow::Response& res;
};

} // namespace dbus_utils
} // namespace redfish
