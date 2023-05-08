#pragma once

#include "chassis_utils.hpp"
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"

#include <async_resp.hpp>
#include <sdbusplus/asio/property.hpp>

#include <string>
#include <unordered_set>

namespace redfish
{
namespace location_util
{

/**
 * @brief Fill out location info of a resource by requesting data from the
 * given D-Bus object.
 *
 * @param[in]       interface   Location type interface.
 *
 * @return location if interface is supported Connector, otherwise, return
 * std::nullopt.
 */
inline std::optional<std::string> getLocationType(const std::string& interface)
{
    if (interface == "xyz.openbmc_project.Inventory.Connector.Embedded")
    {
        return "Embedded";
    }
    if (interface == "xyz.openbmc_project.Inventory.Connector.Slot")
    {
        return "Slot";
    }
    return std::nullopt;
}

/**
 * @brief Fill out location code of a resource by requesting data from the
 * given D-Bus object.
 *
 * @param[in,out]   asyncResp   Async HTTP response.
 * @param[in]       service     D-Bus service to query.
 * @param[in]       objPath     D-Bus object to query.
 * @param[in]       location    Json path of where to find the location
 *                                property.
 */
inline void getLocationCode(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& service,
                            const std::string& objPath,
                            const nlohmann::json::json_pointer& location)
{
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, service, objPath,
        "xyz.openbmc_project.Inventory.Decorator.LocationCode", "LocationCode",
        [asyncResp, location](const boost::system::error_code ec,
                              const std::string& property) {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR << "DBUS response error for Location";
            messages::internalError(asyncResp->res);
            return;
        }

        asyncResp->res.jsonValue[location]["PartLocation"]["ServiceLabel"] =
            property;
        });
}

/**
 * @brief Fill out PartLocationContext of a redfish resource.
 *
 * @param[in,out]   asyncResp  Async HTTP response
 * @param[in]       jsonPtr    Json path of where to put the location context
 * @param[in]       label      Service label to update the location context with
 */
inline void
    updateLocationContext(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const nlohmann::json::json_pointer& jsonPtr,
                          const std::string& label)
{
    BMCWEB_LOG_DEBUG << "Add service label " << label
                     << " to PartLocationContext";
    nlohmann::json& propertyJson =
        asyncResp->res.jsonValue[jsonPtr]["PartLocationContext"];
    const std::string* val = propertyJson.get_ptr<const std::string*>();
    if (val != nullptr && !val->empty())
    {
        propertyJson = label + "/" + *val;
    }
    else
    {
        propertyJson = label;
    }
}

/**
 * @brief Parses given chassis subtree for object path and connection to retrive
 * chassis location code used to fill PartLocationContext.
 *
 * @param[in]       upstreamChassisPaths   Set of chassis object paths queried.
 * @param[in]       jsonPtr     Json location to fill PartLocationContext
 * @param[in,out]   asyncResp   Async HTTP response
 * @param[in]       subtree     Subtree associated with chassis endpoint
 */
inline void getAssociatedChassisSubtreeCallback(
    std::unordered_set<std::string> upstreamChassisPaths,
    const nlohmann::json::json_pointer& jsonPtr,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    // Base case as we recurse to get upstream chassis association.
    if (subtree.empty())
    {
        BMCWEB_LOG_DEBUG << "Chassis association not found."
                            "Must be root chassis or link is broken";
        return;
    }

    if (subtree.size() > 1)
    {
        BMCWEB_LOG_ERROR << "Found multiple upstream chassis.";
        messages::internalError(asyncResp->res);
        return;
    }

    auto chassisPathToConnMap = subtree.begin();

    // Upstream chassis.
    const std::string& chassisPath = chassisPathToConnMap->first;
    if (!upstreamChassisPaths.emplace(chassisPath).second)
    {
        BMCWEB_LOG_ERROR << "Loop detected in upstream chassis associations.";
        messages::internalError(asyncResp->res);
        return;
    }

    const dbus::utility::MapperServiceMap& serviceMap =
        chassisPathToConnMap->second;
    if (serviceMap.empty())
    {
        BMCWEB_LOG_ERROR << "Associated chassis obj path " << chassisPath
                         << " has no service mapping.";
        messages::internalError(asyncResp->res);
        return;
    }

    if (serviceMap.size() > 1)
    {
        BMCWEB_LOG_ERROR << "Associated chassis path " << chassisPath
                         << " found in multiple connections";
        messages::internalError(asyncResp->res);
        return;
    }

    // Get service name to retrieve upstream chassis location code property.
    const std::string& service = serviceMap.begin()->first;

    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, service, chassisPath,
        "xyz.openbmc_project.Inventory.Decorator.LocationCode", "LocationCode",
        [asyncResp, jsonPtr, chassisPath,
         upstreamChassisPaths](const boost::system::error_code ec2,
                               const std::string& locationCode) {
        if (ec2)
        {
            BMCWEB_LOG_DEBUG << "DBUS response error for Location, ec: "
                             << ec2.message();
            return;
        }

        updateLocationContext(asyncResp, jsonPtr, locationCode);

        chassis_utils::getAssociatedChassisSubtree(
            asyncResp, chassisPath + "/containedby",
            std::bind_front(getAssociatedChassisSubtreeCallback,
                            upstreamChassisPaths, jsonPtr));
    });
}

/**
 * @brief Recursively gets LocationCode for upstream chassis to populate
 * PartLocationContext.
 *
 * @param[in,out]   asyncResp       Async HTTP response
 * @param[in]       jsonPtr         Json location to fill PartLocationContext
 * @param[in]       associationPath Path used to associate with upstream chassis
 */
inline void
    getPartLocationContext(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const nlohmann::json::json_pointer& jsonPtr,
                           const std::string& associationPath)
{
    BMCWEB_LOG_DEBUG << "Get chassis endpoints associated with "
                     << associationPath;
    // Set of chassis object paths used to detect cycle as we resolve usptream
    // chassis associations to get to root chassis to create PartLocationContext
    std::unordered_set<std::string> upstreamChassisPaths;
    chassis_utils::getAssociatedChassisSubtree(
        asyncResp, associationPath,
        std::bind_front(getAssociatedChassisSubtreeCallback,
                        std::move(upstreamChassisPaths), jsonPtr));
}

} // namespace location_util
} // namespace redfish
