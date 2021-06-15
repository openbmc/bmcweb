
#pragma once

#include <async_resp.hpp>
#include <boost/container/flat_map.hpp>

#include <algorithm>
#include <string>
#include <variant>
#include <vector>

namespace redfish
{
namespace location_util
{

/**
 * @brief Fill out location info of a resource by requesting data from the
 * given D-Bus object.
 *
 * @param[in,out]   asyncResp   Async HTTP response.
 * @param[in]       interface   Location type interface.
 * @param[in]       location    Location resource to update.
 */
inline bool getLocationType(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& interface,
                            const nlohmann::json_pointer<nlohmann::json>&
                                location = "/Location"_json_pointer)
{
    std::optional<std::string> locationType = std::nullopt;
    if (interface == "xyz.openbmc_project.Inventory.Connector.Slot")
    {
        locationType = "Slot";
    }
    if (interface == "xyz.openbmc_project.Inventory.Connector.Embedded")
    {
        locationType = "Embedded";
    }

    if (!locationType.has_value())
    {
        return false;
    }
    asyncResp->res.jsonValue[location]["PartLocation"]["LocationType"] =
        locationType.value();
    return true;
}

/**
 * @brief Fill out location info of a resource by requesting data from the
 * given D-Bus object.
 *
 * @param[in,out]   asyncResp   Async HTTP response.
 * @param[in]       service     D-Bus service to query.
 * @param[in]       objPath     D-Bus object to query.
 * @param[in]       location    Location resource to update.
 */
inline void
    getLocationCode(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                    const std::string& service, const std::string& objPath,
                    const nlohmann::json_pointer<nlohmann::json>& location =
                        "/Location"_json_pointer)
{
    BMCWEB_LOG_DEBUG << "Get Location Data";

    crow::connections::systemBus->async_method_call(
        [asyncResp, location](const boost::system::error_code ec,
                              const std::variant<std::string>& property) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error for Location";
                return;
            }

            const std::string* value = std::get_if<std::string>(&property);
            if (value == nullptr)
            {
                BMCWEB_LOG_DEBUG << "Null value returned for locaton code";
                return;
            }
            asyncResp->res.jsonValue[location]["PartLocation"]["ServiceLabel"] =
                *value;
        },
        service, objPath, "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Inventory.Decorator.LocationCode", "LocationCode");
}

inline bool isConnector(std::string_view interface)
{
    return interface == "xyz.openbmc_project.Inventory.Connector.Embedded" ||
           interface == "xyz.openbmc_project.Inventory.Connector.Slot";
}

/**
 * @brief Populate the collection "Location" and "LocationType" from
 * "LocationCode" and the Connector type.
 *
 * @param[in,out] asyncResp    Async response object
 * @param[in]     path         D-bus object path to for the current resource
 * @param[in]     service      D-bus service managing the resource
 * @param[in]     interfaces   List of interfaces the resource supports
 * @param[in,out] location     Json pointer to the location json
 *
 * @return void
 */
inline void getLocation(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& path, const std::string& service,
                        const std::vector<std::string>& interfaces,
                        const nlohmann::json_pointer<nlohmann::json>& location =
                            "/Location"_json_pointer)
{
    BMCWEB_LOG_DEBUG << "Get Location for: " << path;

    for (const auto& interface : interfaces)
    {
        if (interface == "xyz.openbmc_project.Inventory.Decorator.LocationCode")
        {
            getLocationCode(asyncResp, service, path, location);
        }
        if (isConnector(interface))
        {
            getLocationType(asyncResp, interface, location);
        }
    }
}

} // namespace location_util
} // namespace redfish
