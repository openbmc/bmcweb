// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "async_resp.hpp"
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/redundancy.hpp"
#include "generated/enums/resource.hpp"
#include "logging.hpp"
#include "utils/dbus_utils.hpp"

#include <asm-generic/errno.h>

#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <cstddef>
#include <format>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>

namespace redfish
{

inline redundancy::RedundancyMode dBusRedundancyEnabledToRedfish(
    const bool redundancyEnabled)
{
    if (redundancyEnabled)
    {
        return redundancy::RedundancyMode::Failover;
    }

    return redundancy::RedundancyMode::NotRedundant;
}

inline resource::State getRedundantState(bool failoversAllowed,
                                         bool redundancyEnabled)
{
    if (!redundancyEnabled || !failoversAllowed)
    {
        return resource::State::Disabled;
    }

    return resource::State::Enabled;
}

inline void getRedundantData(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& managerId, nlohmann::json::object_t& redundantObject,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& propertiesList)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("D-Bus response error {}", ec);
            messages::internalError(asyncResp->res);
            return;
        }
        BMCWEB_LOG_DEBUG("Can't get Redundancy properties");
        return;
    }

    const bool* failoversAllowed = nullptr;
    const bool* redundancyEnabled = nullptr;
    const size_t* redundancyMinimum = nullptr;
    const size_t* redundancyMaximum = nullptr;
    const size_t* functionalMinimum = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), propertiesList, "FailoversAllowed",
        failoversAllowed, "RedundancyEnabled", redundancyEnabled,
        "RedundancyMinimum", redundancyMinimum, "RedundancyMaximum",
        redundancyMaximum, "FunctionalMinimum", functionalMinimum);

    if (!success)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    auto redundancyIt = asyncResp->res.jsonValue.find("Redundancy");

    if (redundancyIt == asyncResp->res.jsonValue.end())
    {
        // Initialize the Redundancy array
        BMCWEB_LOG_DEBUG("Initializing Redundancy array");
        asyncResp->res.jsonValue["Redundancy"] = nlohmann::json::array_t();
        asyncResp->res.jsonValue["Redundancy@odata.count"] = 0;
        redundancyIt = asyncResp->res.jsonValue.find("Redundancy");
    }

    auto* redundancy = redundancyIt->get_ptr<nlohmann::json::array_t*>();
    if (redundancy == nullptr)
    {
        messages::internalError(asyncResp->res);
        return;
    }
    size_t objIdx = redundancy->size();

    redundantObject["@odata.id"] = boost::urls::format(
        "/redfish/v1/Managers/{}#/Redundancy/{}", managerId, objIdx);
    redundantObject["MemberId"] = std::to_string(objIdx);
    redundantObject["Name"] = "Manager Redundancy";

    if (redundancyEnabled != nullptr)
    {
        redundantObject["Mode"] =
            dBusRedundancyEnabledToRedfish(*redundancyEnabled);

        if (failoversAllowed != nullptr)
        {
            redundantObject["Status"]["State"] =
                getRedundantState(*failoversAllowed, *redundancyEnabled);
        }
    }

    // TODO: Handle badpath for health
    redundantObject["Status"]["Health"] = resource::Health::OK;

    if (redundancyMinimum != nullptr)
    {
        redundantObject["MinNumNeededForFaultTolerance"] = *redundancyMinimum;
    }

    if (redundancyMaximum != nullptr &&
        *redundancyMaximum != std::numeric_limits<size_t>::max())
    {
        redundantObject["MaxNumSupported"] = *redundancyMaximum;
    }

    if (functionalMinimum != nullptr)
    {
        redundantObject["MinNumNeeded"] = *functionalMinimum;
    }

    redundancy->emplace_back(std::move(redundantObject));
    asyncResp->res.jsonValue["Redundancy@odata.count"] = redundancy->size();
}

inline void handleRedundancySubTree(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& managerId, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        if (ec == boost::system::errc::io_error)
        {
            BMCWEB_LOG_DEBUG("No redundant bmc objects");
            return;
        }

        BMCWEB_LOG_ERROR("D-Bus response error on GetSubTree {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }
    if (subtree.empty())
    {
        BMCWEB_LOG_DEBUG("No redundant bmc objects");
        return;
    }

    nlohmann::json::object_t redundantObject;
    nlohmann::json::array_t redundancySet;

    // TODO: Redfish path name for sibling interface based on aggregation.
    BMCWEB_LOG_DEBUG("Found {} redundant bmc D-Bus objects.", subtree.size());
    nlohmann::json::object_t item;
    item["@odata.id"] =
        boost::urls::format("/redfish/v1/Managers/{}", managerId);
    redundancySet.emplace_back(std::move(item));
    redundantObject["RedundancySet@odata.count"] = redundancySet.size();
    redundantObject["RedundancySet"] = std::move(redundancySet);

    /* TODO: Will need to determine active BMC by looking at the redundancy
     * state for each one. Need to use async gathering for that.
     * May combine filling in the name in the set with gathering the
     * other redundancy state information.
     *
     * For now, only look at state of this BMC which is always at bmc0
     */
    for (const auto& [path, services] : subtree)
    {
        sdbusplus::message::object_path managerPath(path);
        if (managerPath.filename() == "bmc0")
        {
            // Expect a single service
            if (services.size() != 1 || services[0].first.empty())
            {
                BMCWEB_LOG_ERROR("Unexpected Redundancy D-Bus subtree {}",
                                 services.size());
                messages::internalError(asyncResp->res);
                return;
            }
            auto serviceName = services[0].first;

            BMCWEB_LOG_DEBUG("Getting redundant properties for {} {}", path,
                             serviceName);
            dbus::utility::getAllProperties(
                *crow::connections::systemBus, serviceName, path,
                "xyz.openbmc_project.State.BMC.Redundancy",
                std::bind_front(getRedundantData, asyncResp, managerId,
                                redundantObject));
            return;
        }
    }
}

inline void getManagerRedundancy(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& managerId)
{
    BMCWEB_LOG_DEBUG("Get Redundancy for Manager {}", managerId);

    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.State.BMC.Redundancy"};
    dbus::utility::getSubTree(
        "/xyz/openbmc_project/state", 0, interfaces,
        std::bind_front(handleRedundancySubTree, asyncResp, managerId));
}

} // namespace redfish
