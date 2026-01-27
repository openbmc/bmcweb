// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "async_resp.hpp"
#include "boost_formatters.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "logging.hpp"

#include <sdbusplus/message/native_types.hpp>

#include <algorithm>
#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>

namespace redfish
{

namespace processor_utils
{

static constexpr std::array<std::string_view, 3> processorInterfaces = {
    "xyz.openbmc_project.Inventory.Item.Cpu",
    "xyz.openbmc_project.Inventory.Item.Accelerator",
    "xyz.openbmc_project.Inventory.Item.Board"};

/**
 * @brief Retrieves valid processor path
 * @param asyncResp   Pointer to object holding response data
 * @param processorId The processor ID to validate
 * @param callback    Callback for next step to get valid processor path
 */
template <typename Callback>
void getValidProcessorPath(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& processorId, Callback&& callback)
{
    BMCWEB_LOG_DEBUG("getValidProcessorPath enter");

    constexpr std::array<std::string_view, 10> interfaces = {
        "xyz.openbmc_project.Common.UUID",
        "xyz.openbmc_project.Inventory.Decorator.Asset",
        "xyz.openbmc_project.Inventory.Decorator.Revision",
        "xyz.openbmc_project.Inventory.Item.Cpu",
        "xyz.openbmc_project.Inventory.Decorator.LocationCode",
        "xyz.openbmc_project.Inventory.Item.Accelerator",
        "xyz.openbmc_project.Control.Processor.CurrentOperatingConfig",
        "xyz.openbmc_project.Inventory.Decorator.UniqueIdentifier",
        "xyz.openbmc_project.Control.Power.Cap",
        "xyz.openbmc_project.Control.Power.Throttle"};

    dbus::utility::getSubTree(
        "/xyz/openbmc_project", 0, interfaces,
        [callback = std::forward<Callback>(callback), asyncResp, processorId](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreeResponse& subtree) mutable {
            BMCWEB_LOG_DEBUG("getValidProcessorPath respHandler enter");
            if (ec)
            {
                BMCWEB_LOG_ERROR(
                    "getValidProcessorPath respHandler DBUS error: {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }

            constexpr std::array<std::string_view, 2> processorPaths = {
                "/xyz/openbmc_project/inventory",
                "/xyz/openbmc_project/control"};

            std::string inventoryObjectPath;
            dbus::utility::MapperServiceMap mergedServiceMap;

            for (const auto& [objectPath, serviceMap] : subtree)
            {
                bool validPath = false;
                for (const auto& procPath : processorPaths)
                {
                    if (objectPath.starts_with(procPath))
                    {
                        validPath = true;
                        break;
                    }
                }
                if (!validPath)
                {
                    continue;
                }

                sdbusplus::message::object_path path(objectPath);
                if (path.filename() == processorId)
                {
                    for (const auto& [serviceName, interfaceList] : serviceMap)
                    {
                        auto it = std::find_if(
                            mergedServiceMap.begin(), mergedServiceMap.end(),
                            [&serviceName](const auto& pair) {
                                return pair.first == serviceName;
                            });

                        if (it != mergedServiceMap.end())
                        {
                            it->second.insert(it->second.end(),
                                              interfaceList.begin(),
                                              interfaceList.end());
                        }
                        else
                        {
                            mergedServiceMap.emplace_back(serviceName,
                                                          interfaceList);
                        }
                    }

                    if (objectPath.starts_with(
                            "/xyz/openbmc_project/inventory"))
                    {
                        inventoryObjectPath = objectPath;
                    }
                }
            }

            if (inventoryObjectPath.empty() || mergedServiceMap.empty())
            {
                messages::resourceNotFound(asyncResp->res, "Processor",
                                           processorId);
                return;
            }

            // Filter out objects that don't have the CPU-specific
            // interfaces to make sure we can return 404 on non-CPUs
            // (e.g. /redfish/../Processors/dimm0)
            for (const auto& [serviceName, interfaceList] : mergedServiceMap)
            {
                if (std::ranges::find_first_of(interfaceList,
                                               processorInterfaces) !=
                    interfaceList.end())
                {
                    // Process the merged object which matches processor name
                    // and required interfaces.
                    callback(inventoryObjectPath, mergedServiceMap);
                    return;
                }
            }

            messages::resourceNotFound(asyncResp->res, "Processor",
                                       processorId);
        });
    BMCWEB_LOG_DEBUG("getValidProcessorPath exit");
}

} // namespace processor_utils
} // namespace redfish
