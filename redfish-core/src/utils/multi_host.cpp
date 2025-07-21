// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "utils/multi_host.hpp"

#include "bmcweb_config.h"

#include "async_resp.hpp"
#include "error_messages.hpp"
#include "logging.hpp"

#include <memory>
#include <string>

namespace redfish
{

void getManagedHostProperty(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& systemPath,
    const std::string& serviceName,
    std::function<void(const uint64_t computerSystemIndex)> callback)
{
    dbus::utility::getProperty<uint64_t>(
        *crow::connections::systemBus, serviceName, systemPath,
        "xyz.openbmc_project.Inventory.Decorator.ManagedHost", "HostIndex",
        [asyncResp, systemName, systemPath, serviceName,
         callback = std::move(callback)](const boost::system::error_code& ec,
                                         const uint64_t hostIndex) {
            if (ec)
            {
                BMCWEB_LOG_WARNING("DBUS response error {}", ec);
                messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                           systemName);
                return;
            }
            BMCWEB_LOG_DEBUG("Got index {} for path {}", hostIndex, systemPath);
            callback(hostIndex);
        });
}

void afterGetComputerSystemSubTree(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName,
    std::function<void(const uint64_t computerSystemIndex)>& callback,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    sdbusplus::message::object_path systemPath;
    std::string serviceName;
    if (ec)
    {
        if (ec.value() == boost::system::errc::io_error)
        {
            BMCWEB_LOG_WARNING("EIO - System not found");
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }

        BMCWEB_LOG_ERROR("DBus method call failed with error {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }

    const auto& found = std::ranges::find_if(
        subtree,
        [systemName](const std::pair<sdbusplus::message::object_path,
                                     dbus::utility::MapperServiceMap>& elem) {
            return elem.first.filename() == systemName;
        });

    if (found == subtree.end())
    {
        BMCWEB_LOG_ERROR("Failed to match systemName: {}", systemName);
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    systemPath = found->first;
    serviceName = found->second.begin()->first;

    getManagedHostProperty(asyncResp, systemName, systemPath, serviceName,
                           std::move(callback));
}

/**
 * @brief Retrieve the index associated with the requested system based on the
 * ManagedHost interface
 *
 * @param[i] asyncResp  Async response object
 * @param[i] systemName The requested system
 * @param[i] callback   Callback to call once the index has been found
 *
 * @return None
 */
void getComputerSystemIndex(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName,
    std::function<void(const uint64_t computerSystemIndex)>&& callback)
{
    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        constexpr std::array<std::string_view, 1> interfaces{
            "xyz.openbmc_project.Inventory.Decorator.ManagedHost"};
        dbus::utility::getSubTree(
            "/xyz/openbmc_project/inventory", 0, interfaces,
            std::bind_front(afterGetComputerSystemSubTree, asyncResp,
                            systemName, std::move(callback)));
    }
    else
    {
        // on single-host, fallback to index 0
        BMCWEB_LOG_DEBUG(
            "Single-host detected, fallback to computerSystemIndex 0");
        callback(0);
    }
}

sdbusplus::message::object_path getHostStateObjectPath(
    const uint64_t computerSystemIndex)
{
    const sdbusplus::message::object_path hostStatePath(
        "/xyz/openbmc_project/state/host" +
        std::to_string(computerSystemIndex));

    return hostStatePath;
}

std::string getHostStateServiceName(const uint64_t computerSystemIndex)
{
    std::string hostStateService = "xyz.openbmc_project.State.Host";
    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        hostStateService += std::to_string(computerSystemIndex);
    }

    return hostStateService;
}

sdbusplus::message::object_path getChassisStateObjectPath(
    const uint64_t computerSystemIndex)
{
    const sdbusplus::message::object_path chassisStatePath(
        "/xyz/openbmc_project/state/chassis" +
        std::to_string(computerSystemIndex));

    return chassisStatePath;
}

std::string getChassisStateServiceName(const uint64_t computerSystemIndex)
{
    std::string chassisStateService = "xyz.openbmc_project.State.Chassis";
    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        chassisStateService += std::to_string(computerSystemIndex);
    }

    return chassisStateService;
}
} // namespace redfish
