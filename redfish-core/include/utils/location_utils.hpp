
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
 * @brief Populate the collection "Location" and "Locationtype" from
 * "LocationCode" and the Connector tyep.
 *
 * @param[i,o] asyncResp         Async response object
 * @param[i]   resourceId        Resouce id to for the current resource
 * @param[i]   resourceInteraces List of interfaces to constrain the GetSubTree
 * search
 * @param[i]   location          Location json to update
 * @param[i]   locationName      Field to update for the location resouce
 *
 * @return void
 */
inline void getLocation(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& resourceId,
                        const std::vector<const char*>& interfaces,
                        nlohmann::json* location,
                        const std::string& locationName)
{
    BMCWEB_LOG_DEBUG << "Get Location for: " << resourceId;

    crow::connections::systemBus->async_method_call(
        [asyncResp, location, locationName,
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

                const boost::container::flat_map<std::string, std::string>
                    supportedLocationTypes = {
                        {"xyz.openbmc_project.Inventory.Connector.Slot",
                         "Slot"},
                        {"xyz.openbmc_project.Inventory.Connector.Embedded",
                         "Embedded"},
                    };

                for (const auto& [typeInterface, type] : supportedLocationTypes)
                {
                    if (std::find(interfaces.begin(), interfaces.end(),
                                  typeInterface) != interfaces.end())
                    {
                        (*location)[locationName]["PartLocation"]
                                   ["LocationType"] = type;
                        break;
                    }
                }

                const std::string locationInterface =
                    "xyz.openbmc_project.Inventory.Decorator."
                    "LocationCode";
                if (std::find(interfaces.begin(), interfaces.end(),
                              locationInterface) == interfaces.end())
                {
                    continue;
                }

                crow::connections::systemBus->async_method_call(
                    [asyncResp, location,
                     locationName](const boost::system::error_code ec,
                                   const std::variant<std::string>& property) {
                        if (ec)
                        {
                            BMCWEB_LOG_DEBUG
                                << "DBUS response error for Location";
                            return;
                        }

                        const std::string* value =
                            std::get_if<std::string>(&property);
                        if (value == nullptr)
                        {
                            BMCWEB_LOG_DEBUG
                                << "Null value returned for locaton code";
                            return;
                        }
                        (*location)[locationName]["PartLocation"]
                                   ["ServiceLabel"] = *value;
                    },
                    connection, path, "org.freedesktop.DBus.Properties", "Get",
                    locationInterface, "LocationCode");
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", 0, interfaces);
}

} // namespace location_util
} // namespace redfish
