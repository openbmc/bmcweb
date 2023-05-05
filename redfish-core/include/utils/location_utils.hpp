#pragma once

#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"

#include <async_resp.hpp>
#include <sdbusplus/asio/property.hpp>

#include <string>

namespace redfish
{
namespace location_utils
{

/**
 * @brief Recursively gets LocationCode for upstream chassis to populate
 * PartLocationContext.
 *
 * @param[in,out]   asyncResp       Async HTTP response
 * @param[in]       jsonPtr         Json location to fill PartLocationContext
 * @param[in]       associationPath Path used to associate with upstream chassis
 */
inline void getPartLocationContext(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const nlohmann::json::json_pointer& jsonPtr,
    const std::string& associationPath)
{
    BMCWEB_LOG_DEBUG << "Get chassis endpoints associated with "
                     << associationPath;

    constexpr std::array<std::string_view, 2> chassisInterfaces = {
        "xyz.openbmc_project.Inventory.Item.Board",
        "xyz.openbmc_project.Inventory.Item.Chassis"};

    sdbusplus::message::object_path association (associationPath);
    sdbusplus::message::object_path root ("/xyz/openbmc_project/inventory");
    dbus::utility::getAssociatedSubTree(
        association, root, 0, chassisInterfaces,
        [asyncResp, jsonPtr, associationPath](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreeResponse& subtree){
        if (ec)
        {
            BMCWEB_LOG_ERROR << "Failed to get chassis associations for "
                             << associationPath << " ec: " << ec.message();
            return;
        }

        if (subtree.empty())
        {
            BMCWEB_LOG_DEBUG << associationPath
                             << " is not associated with any chassis endpoint";
            return;
        }

        if (subtree.size() > 1)
        {
            BMCWEB_LOG_ERROR << associationPath
                             << " is contained by mutliple chassis.";
            return;
        }

        auto chassisPathToConnMap = subtree.begin();
        // Upstream chassis.
        const std::string& chassisPath = chassisPathToConnMap->first;
        const dbus::utility::MapperServiceMap& serviceMap =
            chassisPathToConnMap->second;
        if (serviceMap.empty())
        {
            BMCWEB_LOG_ERROR << "Associated chassis obj path " << chassisPath
                             << " has no service mapping.";
            return;
        }

        if (serviceMap.size() > 1)
        {
            BMCWEB_LOG_ERROR << "Associated chassis " << chassisPath
                             << " found in multiple connections";
            return;
        }

        // Get service name to get upstream chassis location code property.
        const std::string& service = serviceMap.begin()->first;

        sdbusplus::asio::getProperty<std::string>(
            *crow::connections::systemBus, service, chassisPath,
            "xyz.openbmc_project.Inventory.Decorator.LocationCode",
            "LocationCode", [asyncResp, jsonPtr, chassisPath](
                const boost::system::error_code ec2,
                const std::string& locationCode) {
            if (ec2)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error for Location, ec: "
                                 << ec2.message();
                return;
            }

            // Fill PartLocationContext using the location code retrieved for
            // upstream chassis.
            BMCWEB_LOG_DEBUG << "Add location code " << locationCode
                             << " to PartLocationContext";
            nlohmann::json& propertyJson =
                asyncResp->res.jsonValue[jsonPtr]["PartLocationContext"];
            const std::string* val = propertyJson.get_ptr<const std::string*>();
            // If PartLocationContext is already populated, prepend with the
            // location code obtained.
            if (val && !val->empty())
            {
                propertyJson = locationCode + "/" + *val;
            }
            else
            {
                propertyJson = locationCode;
            }

            // Recurse to get location code for upstream chassis to update
            // PartLocationContext with.
            getPartLocationContext(
                asyncResp, jsonPtr, chassisPath + "/containedby");
        });
    });
}

} // namespace location_util
} // namespace redfish
