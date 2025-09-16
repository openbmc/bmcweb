// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2018 Intel Corporation
#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/action_info.hpp"
#include "generated/enums/chassis.hpp"
#include "generated/enums/resource.hpp"
#include "http_request.hpp"
#include "led.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/asset_utils.hpp"
#include "utils/chassis_utils.hpp"
#include "utils/collection.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"

#include <asm-generic/errno.h>

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <boost/url/url.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <algorithm>
#include <array>
#include <format>
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

inline chassis::ChassisType translateChassisTypeToRedfish(
    const std::string_view& chassisType)
{
    if (chassisType ==
        "xyz.openbmc_project.Inventory.Item.Chassis.ChassisType.Blade")
    {
        return chassis::ChassisType::Blade;
    }
    if (chassisType ==
        "xyz.openbmc_project.Inventory.Item.Chassis.ChassisType.Component")
    {
        return chassis::ChassisType::Component;
    }
    if (chassisType ==
        "xyz.openbmc_project.Inventory.Item.Chassis.ChassisType.Enclosure")
    {
        return chassis::ChassisType::Enclosure;
    }
    if (chassisType ==
        "xyz.openbmc_project.Inventory.Item.Chassis.ChassisType.Module")
    {
        return chassis::ChassisType::Module;
    }
    if (chassisType ==
        "xyz.openbmc_project.Inventory.Item.Chassis.ChassisType.RackMount")
    {
        return chassis::ChassisType::RackMount;
    }
    if (chassisType ==
        "xyz.openbmc_project.Inventory.Item.Chassis.ChassisType.StandAlone")
    {
        return chassis::ChassisType::StandAlone;
    }
    if (chassisType ==
        "xyz.openbmc_project.Inventory.Item.Chassis.ChassisType.StorageEnclosure")
    {
        return chassis::ChassisType::StorageEnclosure;
    }
    if (chassisType ==
        "xyz.openbmc_project.Inventory.Item.Chassis.ChassisType.Zone")
    {
        return chassis::ChassisType::Zone;
    }
    return chassis::ChassisType::Invalid;
}

/**
 * @brief Retrieves resources over dbus to link to the chassis
 *
 * @param[in] asyncResp  - Shared pointer for completing asynchronous
 * calls
 * @param[in] path       - Chassis dbus path to look for the storage.
 *
 * Calls the Association endpoints on the path + "/storage" and add the link of
 * json["Links"]["Storage@odata.count"] =
 *    {"@odata.id", "/redfish/v1/Storage/" + resourceId}
 *
 * @return None.
 */
inline void getStorageLink(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const sdbusplus::message::object_path& path)
{
    dbus::utility::getProperty<std::vector<std::string>>(
        "xyz.openbmc_project.ObjectMapper", (path / "storage").str,
        "xyz.openbmc_project.Association", "endpoints",
        [asyncResp](const boost::system::error_code& ec,
                    const std::vector<std::string>& storageList) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG("getStorageLink got DBUS response error");
                return;
            }

            nlohmann::json::array_t storages;
            for (const std::string& storagePath : storageList)
            {
                std::string id =
                    sdbusplus::message::object_path(storagePath).filename();
                if (id.empty())
                {
                    continue;
                }

                nlohmann::json::object_t storage;
                storage["@odata.id"] =
                    boost::urls::format("/redfish/v1/Systems/{}/Storage/{}",
                                        BMCWEB_REDFISH_SYSTEM_URI_NAME, id);
                storages.emplace_back(std::move(storage));
            }
            asyncResp->res.jsonValue["Links"]["Storage@odata.count"] =
                storages.size();
            asyncResp->res.jsonValue["Links"]["Storage"] = std::move(storages);
        });
}

/**
 * @brief Retrieves chassis state properties over dbus
 *
 * @param[in] asyncResp - Shared pointer for completing asynchronous calls.
 *
 * @return None.
 */
inline void getChassisState(std::shared_ptr<bmcweb::AsyncResp> asyncResp)
{
    dbus::utility::getProperty<std::string>(
        "xyz.openbmc_project.State.Chassis",
        "/xyz/openbmc_project/state/chassis0",
        "xyz.openbmc_project.State.Chassis", "CurrentPowerState",
        [asyncResp{std::move(asyncResp)}](const boost::system::error_code& ec,
                                          const std::string& chassisState) {
            if (ec)
            {
                if (ec == boost::system::errc::host_unreachable)
                {
                    // Service not available, no error, just don't return
                    // chassis state info
                    BMCWEB_LOG_DEBUG("Service not available {}", ec);
                    return;
                }
                BMCWEB_LOG_DEBUG("DBUS response error {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }

            BMCWEB_LOG_DEBUG("Chassis state: {}", chassisState);
            // Verify Chassis State
            if (chassisState ==
                "xyz.openbmc_project.State.Chassis.PowerState.On")
            {
                asyncResp->res.jsonValue["PowerState"] =
                    resource::PowerState::On;
                asyncResp->res.jsonValue["Status"]["State"] =
                    resource::State::Enabled;
            }
            else if (chassisState ==
                     "xyz.openbmc_project.State.Chassis.PowerState.Off")
            {
                asyncResp->res.jsonValue["PowerState"] =
                    resource::PowerState::Off;
                asyncResp->res.jsonValue["Status"]["State"] =
                    resource::State::StandbyOffline;
            }
        });
}

/**
 * Retrieves physical security properties over dbus
 */
inline void handlePhysicalSecurityGetSubTree(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        // do not add err msg in redfish response, because this is not
        //     mandatory property
        BMCWEB_LOG_INFO("DBUS error: no matched iface {}", ec);
        return;
    }
    // Iterate over all retrieved ObjectPaths.
    for (const auto& object : subtree)
    {
        if (!object.second.empty())
        {
            const auto& service = object.second.front();

            BMCWEB_LOG_DEBUG("Get intrusion status by service ");

            dbus::utility::getProperty<std::string>(
                service.first, object.first,
                "xyz.openbmc_project.Chassis.Intrusion", "Status",
                [asyncResp](const boost::system::error_code& ec1,
                            const std::string& value) {
                    if (ec1)
                    {
                        // do not add err msg in redfish response, because this
                        // is not
                        //     mandatory property
                        BMCWEB_LOG_ERROR("DBUS response error {}", ec1);
                        return;
                    }
                    asyncResp->res.jsonValue["PhysicalSecurity"]
                                            ["IntrusionSensorNumber"] = 1;
                    asyncResp->res
                        .jsonValue["PhysicalSecurity"]["IntrusionSensor"] =
                        value;
                });

            return;
        }
    }
}

inline void handleChassisCollectionGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.jsonValue["@odata.type"] =
        "#ChassisCollection.ChassisCollection";
    asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/Chassis";
    asyncResp->res.jsonValue["Name"] = "Chassis Collection";

    collection_util::getCollectionMembers(
        asyncResp, boost::urls::url("/redfish/v1/Chassis"), chassisInterfaces,
        "/xyz/openbmc_project/inventory");
}

inline void getChassisContainedBy(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& upstreamChassisPaths)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error {}", ec);
            messages::internalError(asyncResp->res);
        }
        return;
    }
    if (upstreamChassisPaths.empty())
    {
        return;
    }
    if (upstreamChassisPaths.size() > 1)
    {
        BMCWEB_LOG_ERROR("{} is contained by multiple chassis", chassisId);
        messages::internalError(asyncResp->res);
        return;
    }

    sdbusplus::message::object_path upstreamChassisPath(
        upstreamChassisPaths[0]);
    std::string upstreamChassis = upstreamChassisPath.filename();
    if (upstreamChassis.empty())
    {
        BMCWEB_LOG_WARNING("Malformed upstream Chassis path {} on {}",
                           upstreamChassisPath.str, chassisId);
        return;
    }

    asyncResp->res.jsonValue["Links"]["ContainedBy"]["@odata.id"] =
        boost::urls::format("/redfish/v1/Chassis/{}", upstreamChassis);
}

inline void getChassisContains(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& downstreamChassisPaths)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error {}", ec);
            messages::internalError(asyncResp->res);
        }
        return;
    }
    if (downstreamChassisPaths.empty())
    {
        return;
    }
    nlohmann::json& jValue = asyncResp->res.jsonValue["Links"]["Contains"];
    if (!jValue.is_array())
    {
        // Create the array if it was empty
        jValue = nlohmann::json::array();
    }
    for (const auto& p : downstreamChassisPaths)
    {
        sdbusplus::message::object_path downstreamChassisPath(p);
        std::string downstreamChassis = downstreamChassisPath.filename();
        if (downstreamChassis.empty())
        {
            BMCWEB_LOG_WARNING("Malformed downstream Chassis path {} on {}",
                               downstreamChassisPath.str, chassisId);
            continue;
        }
        nlohmann::json link;
        link["@odata.id"] =
            boost::urls::format("/redfish/v1/Chassis/{}", downstreamChassis);
        jValue.push_back(std::move(link));
    }
    asyncResp->res.jsonValue["Links"]["Contains@odata.count"] = jValue.size();
}

inline void getChassisConnectivity(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& chassisPath)
{
    BMCWEB_LOG_DEBUG("Get chassis connectivity");

    dbus::utility::getAssociatedSubTreePaths(
        chassisPath + "/contained_by",
        sdbusplus::message::object_path("/xyz/openbmc_project/inventory"), 0,
        chassisInterfaces,
        std::bind_front(getChassisContainedBy, asyncResp, chassisId));

    dbus::utility::getAssociatedSubTreePaths(
        chassisPath + "/containing",
        sdbusplus::message::object_path("/xyz/openbmc_project/inventory"), 0,
        chassisInterfaces,
        std::bind_front(getChassisContains, asyncResp, chassisId));
}

/**
 * ChassisCollection derived class for delivering Chassis Collection Schema
 *  Functions triggers appropriate requests on DBus
 */
inline void requestRoutesChassisCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/")
        .privileges(redfish::privileges::getChassisCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleChassisCollectionGet, std::ref(app)));
}

inline void getChassisLocationCode(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& connectionName, const std::string& path)
{
    dbus::utility::getProperty<std::string>(
        connectionName, path,
        "xyz.openbmc_project.Inventory.Decorator.LocationCode", "LocationCode",
        [asyncResp](const boost::system::error_code& ec,
                    const std::string& property) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("DBUS response error for Location");
                messages::internalError(asyncResp->res);
                return;
            }

            asyncResp->res
                .jsonValue["Location"]["PartLocation"]["ServiceLabel"] =
                property;
        });
}

inline void getChassisUUID(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& connectionName,
                           const std::string& path)
{
    dbus::utility::getProperty<std::string>(
        connectionName, path, "xyz.openbmc_project.Common.UUID", "UUID",
        [asyncResp](const boost::system::error_code& ec,
                    const std::string& chassisUUID) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("DBUS response error for UUID");
                messages::internalError(asyncResp->res);
                return;
            }
            asyncResp->res.jsonValue["UUID"] = chassisUUID;
        });
}

inline void handleDecoratorAssetProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& path,
    const dbus::utility::DBusPropertiesMap& propertiesList)
{
    asset_utils::extractAssetInfo(asyncResp, ""_json_pointer, propertiesList,
                                  true);

    asyncResp->res.jsonValue["Name"] = chassisId;
    asyncResp->res.jsonValue["Id"] = chassisId;

    if constexpr (BMCWEB_REDFISH_ALLOW_DEPRECATED_POWER_THERMAL)
    {
        asyncResp->res.jsonValue["Thermal"]["@odata.id"] =
            boost::urls::format("/redfish/v1/Chassis/{}/Thermal", chassisId);
        // Power object
        asyncResp->res.jsonValue["Power"]["@odata.id"] =
            boost::urls::format("/redfish/v1/Chassis/{}/Power", chassisId);
    }

    if constexpr (BMCWEB_REDFISH_NEW_POWERSUBSYSTEM_THERMALSUBSYSTEM)
    {
        asyncResp->res.jsonValue["ThermalSubsystem"]["@odata.id"] =
            boost::urls::format("/redfish/v1/Chassis/{}/ThermalSubsystem",
                                chassisId);
        asyncResp->res.jsonValue["PowerSubsystem"]["@odata.id"] =
            boost::urls::format("/redfish/v1/Chassis/{}/PowerSubsystem",
                                chassisId);
        asyncResp->res.jsonValue["EnvironmentMetrics"]["@odata.id"] =
            boost::urls::format("/redfish/v1/Chassis/{}/EnvironmentMetrics",
                                chassisId);
    }

    asyncResp->res.jsonValue["Assembly"]["@odata.id"] =
        boost::urls::format("/redfish/v1/Chassis/{}/Assembly", chassisId);

    // SensorCollection
    asyncResp->res.jsonValue["Sensors"]["@odata.id"] =
        boost::urls::format("/redfish/v1/Chassis/{}/Sensors", chassisId);
    asyncResp->res.jsonValue["Status"]["State"] = resource::State::Enabled;

    nlohmann::json::array_t computerSystems;

    if constexpr (!BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        nlohmann::json::object_t system;
        system["@odata.id"] = boost::urls::format(
            "/redfish/v1/Systems/{}", BMCWEB_REDFISH_SYSTEM_URI_NAME);
        computerSystems.emplace_back(std::move(system));
    }

    asyncResp->res.jsonValue["Links"]["ComputerSystems"] =
        std::move(computerSystems);

    nlohmann::json::array_t managedBy;
    nlohmann::json::object_t manager;
    manager["@odata.id"] = boost::urls::format("/redfish/v1/Managers/{}",
                                               BMCWEB_REDFISH_MANAGER_URI_NAME);
    managedBy.emplace_back(std::move(manager));
    asyncResp->res.jsonValue["Links"]["ManagedBy"] = std::move(managedBy);
    getChassisState(asyncResp);
    getStorageLink(asyncResp, path);
}

inline void handleChassisProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const dbus::utility::DBusPropertiesMap& propertiesList)
{
    const std::string* type = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), propertiesList, "Type", type);

    if (!success)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    // Chassis Type is a required property in Redfish
    // If there is an error or some enum we don't support just sit it to Rack
    // Mount
    asyncResp->res.jsonValue["ChassisType"] = chassis::ChassisType::RackMount;

    if (type != nullptr)
    {
        auto chassisType = translateChassisTypeToRedfish(*type);
        if (chassisType != chassis::ChassisType::Invalid)
        {
            asyncResp->res.jsonValue["ChassisType"] = chassisType;
        }
    }
}

inline void handleChassisGetSubTree(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBUS response error {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }
    // Iterate over all retrieved ObjectPaths.
    for (const std::pair<
             std::string,
             std::vector<std::pair<std::string, std::vector<std::string>>>>&
             object : subtree)
    {
        const std::string& path = object.first;
        const std::vector<std::pair<std::string, std::vector<std::string>>>&
            connectionNames = object.second;

        sdbusplus::message::object_path objPath(path);
        if (objPath.filename() != chassisId)
        {
            continue;
        }

        getChassisConnectivity(asyncResp, chassisId, path);

        if (connectionNames.empty())
        {
            BMCWEB_LOG_ERROR("Got 0 Connection names");
            continue;
        }

        asyncResp->res.jsonValue["@odata.type"] = "#Chassis.v1_22_0.Chassis";
        asyncResp->res.jsonValue["@odata.id"] =
            boost::urls::format("/redfish/v1/Chassis/{}", chassisId);
        asyncResp->res.jsonValue["Name"] = "Chassis Collection";
        asyncResp->res.jsonValue["Actions"]["#Chassis.Reset"]["target"] =
            boost::urls::format("/redfish/v1/Chassis/{}/Actions/Chassis.Reset",
                                chassisId);
        asyncResp->res
            .jsonValue["Actions"]["#Chassis.Reset"]["@Redfish.ActionInfo"] =
            boost::urls::format("/redfish/v1/Chassis/{}/ResetActionInfo",
                                chassisId);
        dbus::utility::getAssociationEndPoints(
            path + "/drive",
            [asyncResp, chassisId](const boost::system::error_code& ec3,
                                   const dbus::utility::MapperEndPoints& resp) {
                if (ec3 || resp.empty())
                {
                    return; // no drives = no failures
                }

                nlohmann::json reference;
                reference["@odata.id"] = boost::urls::format(
                    "/redfish/v1/Chassis/{}/Drives", chassisId);
                asyncResp->res.jsonValue["Drives"] = std::move(reference);
            });

        const std::string& connectionName = connectionNames[0].first;

        const std::vector<std::string>& interfaces2 = connectionNames[0].second;
        const std::array<const char*, 3> hasIndicatorLed = {
            "xyz.openbmc_project.Inventory.Item.Chassis",
            "xyz.openbmc_project.Inventory.Item.Panel",
            "xyz.openbmc_project.Inventory.Item.Board.Motherboard"};

        const std::string assetTagInterface =
            "xyz.openbmc_project.Inventory.Decorator.AssetTag";
        const std::string replaceableInterface =
            "xyz.openbmc_project.Inventory.Decorator.Replaceable";
        const std::string revisionInterface =
            "xyz.openbmc_project.Inventory.Decorator.Revision";
        for (const auto& interface : interfaces2)
        {
            if (interface == assetTagInterface)
            {
                dbus::utility::getProperty<std::string>(
                    connectionName, path, assetTagInterface, "AssetTag",
                    [asyncResp, chassisId](const boost::system::error_code& ec2,
                                           const std::string& property) {
                        if (ec2)
                        {
                            BMCWEB_LOG_ERROR(
                                "DBus response error for AssetTag: {}", ec2);
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        asyncResp->res.jsonValue["AssetTag"] = property;
                    });
            }
            else if (interface == replaceableInterface)
            {
                dbus::utility::getProperty<bool>(
                    connectionName, path, replaceableInterface, "HotPluggable",
                    [asyncResp, chassisId](const boost::system::error_code& ec2,
                                           const bool property) {
                        if (ec2)
                        {
                            BMCWEB_LOG_ERROR(
                                "DBus response error for HotPluggable: {}",
                                ec2);
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        asyncResp->res.jsonValue["HotPluggable"] = property;
                    });
            }
            else if (interface == revisionInterface)
            {
                dbus::utility::getProperty<std::string>(
                    connectionName, path, revisionInterface, "Version",
                    [asyncResp, chassisId](const boost::system::error_code& ec2,
                                           const std::string& property) {
                        if (ec2)
                        {
                            BMCWEB_LOG_ERROR(
                                "DBus response error for Version: {}", ec2);
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        asyncResp->res.jsonValue["Version"] = property;
                    });
            }
        }

        for (const char* interface : hasIndicatorLed)
        {
            if (std::ranges::find(interfaces2, interface) != interfaces2.end())
            {
                if constexpr (BMCWEB_REDFISH_ALLOW_DEPRECATED_INDICATORLED)
                {
                    getIndicatorLedState(asyncResp);
                }
                getLocationIndicatorActive(asyncResp, objPath);
                break;
            }
        }

        dbus::utility::getAllProperties(
            *crow::connections::systemBus, connectionName, path,
            "xyz.openbmc_project.Inventory.Decorator.Asset",
            [asyncResp, chassisId,
             path](const boost::system::error_code&,
                   const dbus::utility::DBusPropertiesMap& propertiesList) {
                handleDecoratorAssetProperties(asyncResp, chassisId, path,
                                               propertiesList);
            });

        dbus::utility::getAllProperties(
            *crow::connections::systemBus, connectionName, path,
            "xyz.openbmc_project.Inventory.Item.Chassis",
            [asyncResp](
                const boost::system::error_code&,
                const dbus::utility::DBusPropertiesMap& propertiesList) {
                handleChassisProperties(asyncResp, propertiesList);
            });

        for (const auto& interface : interfaces2)
        {
            if (interface == "xyz.openbmc_project.Common.UUID")
            {
                getChassisUUID(asyncResp, connectionName, path);
            }
            else if (interface ==
                     "xyz.openbmc_project.Inventory.Decorator.LocationCode")
            {
                getChassisLocationCode(asyncResp, connectionName, path);
            }
        }

        return;
    }

    // Couldn't find an object with that name.  return an error
    messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
}

inline void handleChassisGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, chassisInterfaces,
        std::bind_front(handleChassisGetSubTree, asyncResp, chassisId));

    constexpr std::array<std::string_view, 1> interfaces2 = {
        "xyz.openbmc_project.Chassis.Intrusion"};

    dbus::utility::getSubTree(
        "/xyz/openbmc_project", 0, interfaces2,
        std::bind_front(handlePhysicalSecurityGetSubTree, asyncResp));
}

inline void handleChassisPatch(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& param)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    std::optional<bool> locationIndicatorActive;
    std::optional<std::string> indicatorLed;

    if (param.empty())
    {
        return;
    }

    if (!json_util::readJsonPatch(                             //
            req, asyncResp->res,                               //
            "IndicatorLED", indicatorLed,                      //
            "LocationIndicatorActive", locationIndicatorActive //
            ))
    {
        return;
    }

    if (!locationIndicatorActive && !indicatorLed)
    {
        return; // delete this when we support more patch properties
    }
    if (indicatorLed)
    {
        if constexpr (BMCWEB_REDFISH_ALLOW_DEPRECATED_INDICATORLED)
        {
            asyncResp->res.addHeader(
                boost::beast::http::field::warning,
                "299 - \"IndicatorLED is deprecated. Use LocationIndicatorActive instead.\"");
        }
        else
        {
            messages::propertyUnknown(asyncResp->res, "IndicatorLED");
            return;
        }
    }

    const std::string& chassisId = param;

    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, chassisInterfaces,
        [asyncResp, chassisId, locationIndicatorActive,
         indicatorLed](const boost::system::error_code& ec,
                       const dbus::utility::MapperGetSubTreeResponse& subtree) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("DBUS response error {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }

            // Iterate over all retrieved ObjectPaths.
            for (const std::pair<std::string,
                                 std::vector<std::pair<
                                     std::string, std::vector<std::string>>>>&
                     object : subtree)
            {
                const std::string& path = object.first;
                const std::vector<
                    std::pair<std::string, std::vector<std::string>>>&
                    connectionNames = object.second;

                sdbusplus::message::object_path objPath(path);
                if (objPath.filename() != chassisId)
                {
                    continue;
                }

                if (connectionNames.empty())
                {
                    BMCWEB_LOG_ERROR("Got 0 Connection names");
                    continue;
                }

                const std::vector<std::string>& interfaces3 =
                    connectionNames[0].second;

                const std::array<const char*, 3> hasIndicatorLed = {
                    "xyz.openbmc_project.Inventory.Item.Chassis",
                    "xyz.openbmc_project.Inventory.Item.Panel",
                    "xyz.openbmc_project.Inventory.Item.Board.Motherboard"};
                bool indicatorChassis = false;
                for (const char* interface : hasIndicatorLed)
                {
                    if (std::ranges::find(interfaces3, interface) !=
                        interfaces3.end())
                    {
                        indicatorChassis = true;
                        break;
                    }
                }
                if (locationIndicatorActive)
                {
                    if (indicatorChassis)
                    {
                        setLocationIndicatorActive(asyncResp, path,
                                                   *locationIndicatorActive);
                    }
                    else
                    {
                        messages::propertyUnknown(asyncResp->res,
                                                  "LocationIndicatorActive");
                    }
                }
                if constexpr (BMCWEB_REDFISH_ALLOW_DEPRECATED_INDICATORLED)
                {
                    if (indicatorLed)
                    {
                        if (indicatorChassis)
                        {
                            setIndicatorLedState(asyncResp, *indicatorLed);
                        }
                        else
                        {
                            messages::propertyUnknown(asyncResp->res,
                                                      "IndicatorLED");
                        }
                    }
                }
                return;
            }

            messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
        });
}

/**
 * Chassis override class for delivering Chassis Schema
 * Functions triggers appropriate requests on DBus
 */
inline void requestRoutesChassis(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/")
        .privileges(redfish::privileges::getChassis)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleChassisGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/")
        .privileges(redfish::privileges::patchChassis)
        .methods(boost::beast::http::verb::patch)(
            std::bind_front(handleChassisPatch, std::ref(app)));
}

inline void doChassisPowerCycle(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.State.Chassis"};

    // Use mapper to get subtree paths.
    dbus::utility::getSubTreePaths(
        "/", 0, interfaces,
        [asyncResp](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreePathsResponse& chassisList) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("[mapper] Bad D-Bus request error: {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }

            const char* processName = "xyz.openbmc_project.State.Chassis";
            const char* interfaceName = "xyz.openbmc_project.State.Chassis";
            const char* destProperty = "RequestedPowerTransition";
            const std::string propertyValue =
                "xyz.openbmc_project.State.Chassis.Transition.PowerCycle";
            std::string objectPath =
                "/xyz/openbmc_project/state/chassis_system0";

            /* Look for system reset chassis path */
            if ((std::ranges::find(chassisList, objectPath)) ==
                chassisList.end())
            {
                /* We prefer to reset the full chassis_system, but if it doesn't
                 * exist on some platforms, fall back to a host-only power reset
                 */
                objectPath = "/xyz/openbmc_project/state/chassis0";
            }

            setDbusProperty(asyncResp, "ResetType", processName, objectPath,
                            interfaceName, destProperty, propertyValue);
        });
}

inline void handleChassisResetActionInfoPost(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& /*chassisId*/)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    BMCWEB_LOG_DEBUG("Post Chassis Reset.");

    std::string resetType;

    if (!json_util::readJsonAction(req, asyncResp->res, "ResetType", resetType))
    {
        return;
    }

    if (resetType != "PowerCycle")
    {
        BMCWEB_LOG_DEBUG("Invalid property value for ResetType: {}", resetType);
        messages::actionParameterNotSupported(asyncResp->res, resetType,
                                              "ResetType");

        return;
    }
    doChassisPowerCycle(asyncResp);
}

/**
 * ChassisResetAction class supports the POST method for the Reset
 * action.
 * Function handles POST method request.
 * Analyzes POST body before sending Reset request data to D-Bus.
 */

inline void requestRoutesChassisResetAction(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Actions/Chassis.Reset/")
        .privileges(redfish::privileges::postChassis)
        .methods(boost::beast::http::verb::post)(
            std::bind_front(handleChassisResetActionInfoPost, std::ref(app)));
}

inline void handleChassisResetActionInfoGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.jsonValue["@odata.type"] = "#ActionInfo.v1_1_2.ActionInfo";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/ResetActionInfo", chassisId);
    asyncResp->res.jsonValue["Name"] = "Reset Action Info";

    asyncResp->res.jsonValue["Id"] = "ResetActionInfo";
    nlohmann::json::array_t parameters;
    nlohmann::json::object_t parameter;
    parameter["Name"] = "ResetType";
    parameter["Required"] = true;
    parameter["DataType"] = action_info::ParameterTypes::String;
    nlohmann::json::array_t allowed;
    allowed.emplace_back("PowerCycle");
    parameter["AllowableValues"] = std::move(allowed);
    parameters.emplace_back(std::move(parameter));

    asyncResp->res.jsonValue["Parameters"] = std::move(parameters);
}

/**
 * ChassisResetActionInfo derived class for delivering Chassis
 * ResetType AllowableValues using ResetInfo schema.
 */
inline void requestRoutesChassisResetActionInfo(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/ResetActionInfo/")
        .privileges(redfish::privileges::getActionInfo)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleChassisResetActionInfoGet, std::ref(app)));
}

} // namespace redfish
