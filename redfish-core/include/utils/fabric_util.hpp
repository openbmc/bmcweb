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

/*
 * @brief Verify whether a given adapter is the same to a given adapterId.
 *
 * @param[i]   adapterId that represents a fabric object
 * @param[i]   adapter that is to check whetehr it is the same adaptor
 *
 * @return void
 */
inline bool checkFabricAdapterId(const std::string& adapterId,
                                 const std::string& adapter)
{
    return !(adapter.empty() || adapterId != adapter);
}

/**
 * @brief Workaround to handle duplicate Fabric device list
 *
 * retrieve Fabric device endpoint information and if path is
 * ~/chassisN/logical_slotN/io_moduleN then, replace redfish
 * Fabric device as "chassisN-logical_slotN-io_moduleN"
 *
 * @param[i]   fullPath  object path of Fabric device
 *
 * @return string: unique Fabric device name
 */
inline std::string buildFabricUniquePath(const std::string& fullPath)
{
    sdbusplus::message::object_path path(fullPath);
    sdbusplus::message::object_path parentPath = path.parent_path();

    std::string devName;

    if (!parentPath.parent_path().filename().empty())
    {
        devName = parentPath.parent_path().filename() + "-";
    }
    if (!parentPath.filename().empty())
    {
        devName += parentPath.filename() + "-";
    }
    devName += path.filename();
    return devName;
}

} // namespace fabric_util
} // namespace redfish
