#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http/utility.hpp"
#include "human_sort.hpp"

#include <boost/url/url.hpp>
#include <nlohmann/json.hpp>

#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace redfish
{
namespace fabric_util
{

/**
 * @brief Workaround to handle duplicate Fabric device list
 *
 * retrieve Fabric device endpoint information and if path is
 * system/chassisN/logical_slotN/io_moduleN then, replace redfish
 * Fabric device as "system-chassisN-logical_slotN-io_moduleN" (MEX)
 *
 * chassisN/boardN/logical_slotN/io_moduleN would be
 * chassisN-boardN-logical_slotN-io_moduleN (Splitter)
 *
 * Because Splitter added an extra segment, had to go 4 deep.
 *
 * @param[i]   fullPath  object path of Fabric device
 *
 * @return string: unique Fabric device name
 */
inline std::string buildFabricUniquePath(const std::string& fullPath)
{
    sdbusplus::message::object_path path(fullPath);
    sdbusplus::message::object_path parentPath = path.parent_path();
    sdbusplus::message::object_path grandparentPath = parentPath.parent_path();

    std::string devName;

    if (!grandparentPath.parent_path().filename().empty())
    {
        devName = grandparentPath.parent_path().filename() + "-";
    }
    if (!grandparentPath.filename().empty())
    {
        devName += grandparentPath.filename() + "-";
    }
    if (!parentPath.filename().empty())
    {
        devName += parentPath.filename() + "-";
    }
    devName += path.filename();
    return devName;
}

/**
 * @brief get FabricAdapters to resp
 *
 * @param[in,out]   asyncResp   Async HTTP response.
 * @param[in]      jsonKeyName
 */
inline void
    getFabricAdapterList(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const nlohmann::json::json_pointer& jsonKeyName)
{
    static constexpr std::array<std::string_view, 1> fabricInterfaces = {
        "xyz.openbmc_project.Inventory.Item.FabricAdapter"};

    dbus::utility::getSubTreePaths(
        "/xyz/openbmc_project/inventory", 0, fabricInterfaces,
        [asyncResp, jsonKeyName](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreePathsResponse& paths) {
        nlohmann::json::json_pointer jsonCountKeyName = jsonKeyName;
        std::string back = jsonCountKeyName.back();
        jsonCountKeyName.pop_back();
        jsonCountKeyName /= back + "@odata.count";

        nlohmann::json& members = asyncResp->res.jsonValue[jsonKeyName];
        members = nlohmann::json::array();

        if (ec)
        {
            // Not an error, system just doesn't have FabricAdapter
            BMCWEB_LOG_DEBUG("no FabricAdapter paths found ec: {}", ec.value());
            asyncResp->res.jsonValue[jsonCountKeyName] = members.size();
            return;
        }

        for (const auto& pcieDevicePath : paths)
        {
            std::string adapterUniq = buildFabricUniquePath(pcieDevicePath);
            if (adapterUniq.empty())
            {
                BMCWEB_LOG_DEBUG("Invalid Name");
                continue;
            }
            nlohmann::json::object_t device;
            device["@odata.id"] = boost::urls::format(
                "/redfish/v1/Systems/system/FabricAdapters/{}", adapterUniq);
            members.emplace_back(std::move(device));
        }
        asyncResp->res.jsonValue[jsonCountKeyName] = members.size();
    });
}

} // namespace fabric_util
} // namespace redfish
