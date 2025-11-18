// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "async_resp.hpp"
#include "boost_formatters.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "human_sort.hpp"
#include "logging.hpp"
#include "utils/chassis_utils.hpp"

#include <asm-generic/errno.h>

#include <boost/system/error_code.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <algorithm>
#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace redfish
{

static constexpr std::array<std::string_view, 1> assemblyInterfaces = {
    "xyz.openbmc_project.Inventory.Item.Panel"};

namespace assembly_utils
{

inline void afterGetChassisAssembly(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    std::function<void(const boost::system::error_code&,
                       const std::vector<std::string>& sortedAssemblyList)>&
        callback,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& subtreePaths)
{
    if (ec)
    {
        if (ec.value() == boost::system::errc::io_error || ec.value() == EBADR)
        {
            // Not found
            callback(ec, std::vector<std::string>());
            return;
        }

        BMCWEB_LOG_ERROR("DBUS response error {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }

    std::vector<std::string> sortedAssemblyList = subtreePaths;
    std::ranges::sort(sortedAssemblyList, AlphanumLess<std::string>());

    callback(ec, sortedAssemblyList);
}

/**
 * @brief Get chassis path with given chassis ID
 * @param[in] asyncResp - Shared pointer for asynchronous calls.
 * @param[in] chassisId - Chassis to which the assemblies are
 * associated.
 * @param[in] callback
 *
 * @return None.
 */
inline void getChassisAssembly(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId,
    std::function<void(const boost::system::error_code& ec,
                       const std::vector<std::string>& sortedAssemblyList)>&&
        callback)
{
    BMCWEB_LOG_DEBUG("Get ChassisAssembly");

    dbus::utility::getAssociatedSubTreePathsById(
        chassisId, dbus_utils::inventoryPath, chassisInterfaces, "containing",
        assemblyInterfaces,
        std::bind_front(afterGetChassisAssembly, asyncResp,
                        std::move(callback)));
}

} // namespace assembly_utils
} // namespace redfish
