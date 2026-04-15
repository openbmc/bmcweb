// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "bmcweb_config.h"

#include "async_resp.hpp"
#include "boost_formatters.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "logging.hpp"

#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace redfish
{

static constexpr std::array<std::string_view, 2> chassisInterfaces = {
    "xyz.openbmc_project.Inventory.Item.Board",
    "xyz.openbmc_project.Inventory.Item.Chassis"};

namespace chassis_utils
{
/**
 * @brief Retrieves valid chassis path
 * @param asyncResp   Pointer to object holding response data
 * @param callback  Callback for next step to get valid chassis path
 */
template <typename Callback>
void getValidChassisPath(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& chassisId, Callback&& callback)
{
    BMCWEB_LOG_DEBUG("checkChassisId enter");

    // Get the Chassis Collection
    dbus::utility::getSubTreePaths(
        "/xyz/openbmc_project/inventory", 0, chassisInterfaces,
        [callback = std::forward<Callback>(callback), asyncResp,
         chassisId](const boost::system::error_code& ec,
                    const dbus::utility::MapperGetSubTreePathsResponse&
                        chassisPaths) mutable {
            BMCWEB_LOG_DEBUG("getValidChassisPath respHandler enter");
            if (ec)
            {
                BMCWEB_LOG_ERROR(
                    "getValidChassisPath respHandler DBUS error: {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }

            std::optional<std::string> chassisPath;
            for (const std::string& chassis : chassisPaths)
            {
                sdbusplus::object_path path(chassis);
                std::string chassisName = path.filename();
                if (chassisName.empty())
                {
                    BMCWEB_LOG_ERROR("Failed to find '/' in {}", chassis);
                    continue;
                }
                if (chassisName == chassisId)
                {
                    chassisPath = chassis;
                    break;
                }
            }
            callback(chassisPath);
        });
    BMCWEB_LOG_DEBUG("checkChassisId exit");
}

/**
 * @brief Fill out Processors links by traversing the "containing"
 * association from the chassis object to find all associated
 * processors.
 *
 * @param[in,out]   aResp       Async HTTP response.
 * @param[in]       chassisPath D-Bus object path of the chassis.
 */
inline void getChassisProcessorLinks(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const std::string& chassisPath)
{
    static constexpr std::array<std::string_view, 2> procInterfaces = {
        "xyz.openbmc_project.Inventory.Item.Accelerator",
        "xyz.openbmc_project.Inventory.Item.Cpu"};

    BMCWEB_LOG_DEBUG("Get chassis processor links for {}", chassisPath);

    dbus::utility::getAssociatedSubTreePaths(
        chassisPath + "/containing",
        sdbusplus::object_path("/xyz/openbmc_project/inventory"), 0,
        procInterfaces,
        [aResp](const boost::system::error_code& ec,
                const dbus::utility::MapperGetSubTreePathsResponse& procs) {
            if (ec)
            {
                return;
            }
            nlohmann::json::array_t processors;
            for (const auto& procPath : procs)
            {
                sdbusplus::object_path objectPath(procPath);
                std::string name = objectPath.filename();
                if (name.empty())
                {
                    continue;
                }
                nlohmann::json::object_t proc;
                proc["@odata.id"] =
                    boost::urls::format("/redfish/v1/Systems/{}/Processors/{}",
                                        BMCWEB_REDFISH_SYSTEM_URI_NAME, name);
                processors.emplace_back(std::move(proc));
            }
            aResp->res.jsonValue["Links"]["Processors"] = std::move(processors);
        });
}

} // namespace chassis_utils
} // namespace redfish
