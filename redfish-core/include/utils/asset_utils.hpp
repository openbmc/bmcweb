#pragma once

#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"

#include <async_resp.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/asio/property.hpp>

#include <memory>
#include <string>
namespace redfish
{

namespace asset_utils
{
/**
 * @brief Parse a DBus property from
 * xyz.openbmc_project.Inventory.Decorator.Asset and convert it to a json
 * object.
 *
 * @param[in] name   Property name
 * @param[in] value  Property value
 *
 * @return a pair whose first value indicates if there was an error, and whose
 * second value contains the JSON representation of the property if one exists.
 */
std::pair<bool, std::optional<nlohmann::json>>
    parseProperty(const std::string& name,
                  const dbus::utility::DbusVariantType& value)
{
    const auto* valueStr = std::get_if<std::string>(&value);
    if (valueStr == nullptr)
    {
        BMCWEB_LOG_ERROR << "Null value returned for " << name;
        return {true, std::nullopt};
    }
    // SparePartNumber is optional on D-Bus
    // so skip if it is empty
    if (name == "SparePartNumber")
    {
        if (valueStr->empty())
        {
            return {false, std::nullopt};
        }
    }
    return {false, nlohmann::json(*valueStr)};
}

/**
 * @brief Inserts properties from a DBus object with interface
 * xyz.openbmc_project.Inventory.Decorator.Asset into the specified location in
 * the response. object.
 *
 * @param[in] asyncResp    Async response pointer
 * @param[in] serviceName  Service name that exposes the properties
 * @param[in] obj          DBus object path
 * @param[in] location     Location to insert the JSON response
 *
 */
void addAssetProperties(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& serviceName, const std::string& obj,
                        const nlohmann::json_pointer<nlohmann::json>& location,
                        std::function<bool(const std::string&)>&& isExpected)
{
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, serviceName, obj,
        "xyz.openbmc_project.Inventory.Decorator.Asset",
        [location, asyncResp, isExpected = std::move(isExpected)](
            const boost::system::error_code ec,
            const dbus::utility::DBusPropertiesMap& properties) mutable {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error for asset properties";
                messages::internalError(asyncResp->res);
                return;
            }
            for (const auto& [name, value] : properties)
            {
                if (isExpected(name))
                {
                    auto [error, jsonValue] = parseProperty(name, value);
                    if (error)
                    {
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    if (jsonValue)
                    {
                        asyncResp->res.jsonValue[location][name] = *jsonValue;
                    }
                }
            }
        });
}

} // namespace asset_utils
} // namespace redfish
