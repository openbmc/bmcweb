// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2018 Intel Corporation
#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/collection.hpp"
#include "utils/dbus_utils.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace redfish
{

// OperatingConfig D-Bus Types
using TurboProfileProperty = std::vector<std::tuple<uint32_t, size_t>>;
using BaseSpeedPrioritySettingsProperty =
    std::vector<std::tuple<uint32_t, std::vector<uint32_t>>>;
// uint32_t and size_t may or may not be the same type, requiring a dedup'd
// variant

/**
 * Request all the properties for the given D-Bus object and fill out the
 * related entries in the Redfish OperatingConfig response.
 *
 * @param[in,out]   asyncResp       Async HTTP response.
 * @param[in]       service     D-Bus service name to query.
 * @param[in]       objPath     D-Bus object to query.
 */
inline void getOperatingConfigData(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& service, const std::string& objPath)
{
    dbus::utility::getAllProperties(
        service, objPath,
        "xyz.openbmc_project.Inventory.Item.Cpu.OperatingConfig",
        [asyncResp](const boost::system::error_code& ec,
                    const dbus::utility::DBusPropertiesMap& properties) {
            if (ec)
            {
                BMCWEB_LOG_WARNING("D-Bus error: {}, {}", ec, ec.message());
                messages::internalError(asyncResp->res);
                return;
            }

            const size_t* availableCoreCount = nullptr;
            const uint32_t* baseSpeed = nullptr;
            const uint32_t* maxJunctionTemperature = nullptr;
            const uint32_t* maxSpeed = nullptr;
            const uint32_t* powerLimit = nullptr;
            const TurboProfileProperty* turboProfile = nullptr;
            const BaseSpeedPrioritySettingsProperty* baseSpeedPrioritySettings =
                nullptr;

            const bool success = sdbusplus::unpackPropertiesNoThrow(
                dbus_utils::UnpackErrorPrinter(), properties,
                "AvailableCoreCount", availableCoreCount, "BaseSpeed",
                baseSpeed, "MaxJunctionTemperature", maxJunctionTemperature,
                "MaxSpeed", maxSpeed, "PowerLimit", powerLimit, "TurboProfile",
                turboProfile, "BaseSpeedPrioritySettings",
                baseSpeedPrioritySettings);

            if (!success)
            {
                messages::internalError(asyncResp->res);
                return;
            }

            nlohmann::json& json = asyncResp->res.jsonValue;

            if (availableCoreCount != nullptr)
            {
                json["TotalAvailableCoreCount"] = *availableCoreCount;
            }

            if (baseSpeed != nullptr)
            {
                json["BaseSpeedMHz"] = *baseSpeed;
            }

            if (maxJunctionTemperature != nullptr)
            {
                json["MaxJunctionTemperatureCelsius"] = *maxJunctionTemperature;
            }

            if (maxSpeed != nullptr)
            {
                json["MaxSpeedMHz"] = *maxSpeed;
            }

            if (powerLimit != nullptr)
            {
                json["TDPWatts"] = *powerLimit;
            }

            if (turboProfile != nullptr)
            {
                nlohmann::json& turboArray = json["TurboProfile"];
                turboArray = nlohmann::json::array();
                for (const auto& [turboSpeed, coreCount] : *turboProfile)
                {
                    nlohmann::json::object_t turbo;
                    turbo["ActiveCoreCount"] = coreCount;
                    turbo["MaxSpeedMHz"] = turboSpeed;
                    turboArray.emplace_back(std::move(turbo));
                }
            }

            if (baseSpeedPrioritySettings != nullptr)
            {
                nlohmann::json& baseSpeedArray =
                    json["BaseSpeedPrioritySettings"];
                baseSpeedArray = nlohmann::json::array();
                for (const auto& [baseSpeedMhz, coreList] :
                     *baseSpeedPrioritySettings)
                {
                    nlohmann::json::object_t speed;
                    speed["CoreCount"] = coreList.size();
                    speed["CoreIDs"] = coreList;
                    speed["BaseSpeedMHz"] = baseSpeedMhz;
                    baseSpeedArray.emplace_back(std::move(speed));
                }
            }
        });
}

inline void handleOperatingConfigCollectionGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& cpuName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    asyncResp->res.jsonValue["@odata.type"] =
        "#OperatingConfigCollection.OperatingConfigCollection";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/{}/Processors/{}/OperatingConfigs",
        BMCWEB_REDFISH_SYSTEM_URI_NAME, cpuName);
    asyncResp->res.jsonValue["Name"] = "Operating Config Collection";

    // First find the matching CPU object so we know how to
    // constrain our search for related Config objects.
    const std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Control.Processor.CurrentOperatingConfig"};
    dbus::utility::getSubTreePaths(
        dbus_utils::inventoryPath, 0, interfaces,
        [asyncResp,
         cpuName](const boost::system::error_code& ec,
                  const dbus::utility::MapperGetSubTreePathsResponse& objects) {
            if (ec)
            {
                BMCWEB_LOG_WARNING("D-Bus error: {}, {}", ec, ec.message());
                messages::internalError(asyncResp->res);
                return;
            }

            for (const std::string& object : objects)
            {
                if (!object.ends_with(cpuName))
                {
                    continue;
                }

                // Not expected that there will be multiple matching
                // CPU objects, but if there are just use the first
                // one.

                // Use the common search routine to construct the
                // Collection of all Config objects under this CPU.
                constexpr std::array<std::string_view, 1> interface{
                    "xyz.openbmc_project.Inventory.Item.Cpu.OperatingConfig"};
                collection_util::getCollectionMembers(
                    asyncResp,
                    boost::urls::format(
                        "/redfish/v1/Systems/{}/Processors/{}/OperatingConfigs",
                        BMCWEB_REDFISH_SYSTEM_URI_NAME, cpuName),
                    interface, object);
                return;
            }
        });
}

inline void handleOperationConfigGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& cpuName,
    const std::string& configName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    // Ask for all objects implementing OperatingConfig so we can search
    // for one with a matching name
    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Cpu.OperatingConfig"};
    dbus::utility::getSubTree(
        dbus_utils::inventoryPath, 0, interfaces,
        [asyncResp, cpuName,
         configName](const boost::system::error_code& ec,
                     const dbus::utility::MapperGetSubTreeResponse& subtree) {
            if (ec)
            {
                BMCWEB_LOG_WARNING("D-Bus error: {}, {}", ec, ec.message());
                messages::internalError(asyncResp->res);
                return;
            }
            const std::string expectedEnding = cpuName + '/' + configName;
            for (const auto& [objectPath, serviceMap] : subtree)
            {
                // Ignore any configs without matching cpuX/configY
                if (!objectPath.ends_with(expectedEnding) || serviceMap.empty())
                {
                    continue;
                }

                nlohmann::json& json = asyncResp->res.jsonValue;
                json["@odata.type"] = "#OperatingConfig.v1_0_0.OperatingConfig";
                json["@odata.id"] = boost::urls::format(
                    "/redfish/v1/Systems/{}/Processors/{}/OperatingConfigs/{}",
                    BMCWEB_REDFISH_SYSTEM_URI_NAME, cpuName, configName);
                json["Name"] = "Processor Profile";
                json["Id"] = configName;

                // Just use the first implementation of the object - not
                // expected that there would be multiple matching
                // services
                getOperatingConfigData(asyncResp, serviceMap.begin()->first,
                                       objectPath);
                return;
            }
            messages::resourceNotFound(asyncResp->res, "OperatingConfig",
                                       configName);
        });
}

inline void requestRoutesOperatingConfig(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/<str>/Processors/<str>/OperatingConfigs/")
        .privileges(redfish::privileges::getOperatingConfigCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleOperatingConfigCollectionGet, std::ref(app)));

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/<str>/Processors/<str>/OperatingConfigs/<str>/")
        .privileges(redfish::privileges::getOperatingConfig)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleOperationConfigGet, std::ref(app)));
}
} // namespace redfish
