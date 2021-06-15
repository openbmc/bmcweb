
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
inline bool getLocationType(std::shared_ptr<bmcweb::AsyncResp> asyncResp,
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
    getLocationCode(std::shared_ptr<bmcweb::AsyncResp> asyncResp,
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

/**
 * @brief Populate the collection "Location" and "LocationType" from
 * "LocationCode" and the Connector type.
 *
 * @param[i,o] asyncResp         Async response object
 * @param[i]   resourceId        Resouce id to for the current resource
 * @param[i]   resourceInteraces List of interfaces to constrain the GetSubTree
 * search
 * @param[i,o] location          Location json to update
 * @param[i]   locationName      Field to update for the location resouce
 *
 * @return void
 */
inline void getLocation(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& resourceId,
                        const std::vector<const char*>& interfaces,
                        const nlohmann::json_pointer<nlohmann::json>& location =
                            "/Location"_json_pointer)
{
    BMCWEB_LOG_DEBUG << "Get Location for: " << resourceId;

    crow::connections::systemBus->async_method_call(
        [asyncResp, location,
         resourceId](const boost::system::error_code ec,
                     const crow::openbmc_mapper::GetSubTreeType& subtree) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
                return;
            }

            for (const std::pair<std::string,
                                 std::vector<std::pair<
                                     std::string, std::vector<std::string>>>>&
                     object : subtree)
            {
                sdbusplus::message::object_path path(object.first);
                const std::vector<
                    std::pair<std::string, std::vector<std::string>>>&
                    connections = object.second;

                if (path.filename() != resourceId)
                {
                    continue;
                }
                if (connections.empty())
                {
                    BMCWEB_LOG_ERROR << "Got 0 connections";
                    continue;
                }

                const std::string& connection = connections[0].first;
                const std::vector<std::string>& interfaces =
                    connections[0].second;

                for (const auto& interface : interfaces)
                {
                    if (boost::starts_with(
                            interface,
                            "xyz.openbmc_project.Inventory.Connector") &&
                        getLocationType(asyncResp, interface, location))
                    {
                        break;
                    }
                }

                if (std::find(interfaces.begin(), interfaces.end(),
                              "xyz.openbmc_project.Inventory.Decorator."
                              "LocationCode") == interfaces.end())
                {
                    continue;
                }

                getLocationCode(asyncResp, connection, path, location);

                return;
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", 0, interfaces);
}

} // namespace location_util
} // namespace redfish
