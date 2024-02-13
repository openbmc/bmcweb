#pragma once

#include "async_resp.hpp"
#include "dbus_singleton.hpp"
#include "error_messages.hpp"
#include "logging.hpp"

#include <nlohmann/json.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <memory>
#include <string_view>

namespace redfish
{
namespace dbus_utils
{

struct UnpackErrorPrinter
{
    void operator()(const sdbusplus::UnpackErrorReason reason,
                    const std::string& property) const noexcept
    {
        BMCWEB_LOG_ERROR(
            "DBUS property error in property: {}, reason: {}", property,
            static_cast<std::underlying_type_t<sdbusplus::UnpackErrorReason>>(
                reason));
    }
};

} // namespace dbus_utils

namespace details
{
void afterSetProperty(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& redfishPropertyName,
                      const nlohmann::json& propertyValue,
                      const boost::system::error_code& ec,
                      const sdbusplus::message_t& msg);
}

template <typename PropertyType>
void setDbusProperty(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     std::string_view processName,
                     const sdbusplus::message::object_path& path,
                     std::string_view interface, std::string_view dbusProperty,
                     std::string_view redfishPropertyName,
                     const PropertyType& prop)
{
    std::string processNameStr(processName);
    std::string interfaceStr(interface);
    std::string dbusPropertyStr(dbusProperty);

    sdbusplus::asio::setProperty(
        *crow::connections::systemBus, processNameStr, path.str, interfaceStr,
        dbusPropertyStr, prop,
        [asyncResp, redfishPropertyNameStr = std::string{redfishPropertyName},
         jsonProp = nlohmann::json(prop)](const boost::system::error_code& ec,
                                          const sdbusplus::message_t& msg) {
        details::afterSetProperty(asyncResp, redfishPropertyNameStr, jsonProp,
                                  ec, msg);
    });
}

} // namespace redfish
