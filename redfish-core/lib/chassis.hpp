/*
// Copyright (c) 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "dbus_utility.hpp"
#include "health.hpp"
#include "led.hpp"
#include "query.hpp"
#include "redfish_util.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/collection.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"

#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <ranges>
#include <string_view>

namespace redfish
{

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
    sdbusplus::asio::getProperty<std::vector<std::string>>(
        *crow::connections::systemBus, "xyz.openbmc_project.ObjectMapper",
        (path / "storage").str, "xyz.openbmc_project.Association", "endpoints",
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
            storage["@odata.id"] = boost::urls::format(
                "/redfish/v1/Systems/system/Storage/{}", id);
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
    // crow::connections::systemBus->async_method_call(
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, "xyz.openbmc_project.State.Chassis",
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
        if (chassisState == "xyz.openbmc_project.State.Chassis.PowerState.On")
        {
            asyncResp->res.jsonValue["PowerState"] = "On";
            asyncResp->res.jsonValue["Status"]["State"] = "Enabled";
        }
        else if (chassisState ==
                 "xyz.openbmc_project.State.Chassis.PowerState.Off")
        {
            asyncResp->res.jsonValue["PowerState"] = "Off";
            asyncResp->res.jsonValue["Status"]["State"] = "StandbyOffline";
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
            const auto service = object.second.front();

            BMCWEB_LOG_DEBUG("Get intrusion status by service ");

            sdbusplus::asio::getProperty<std::string>(
                *crow::connections::systemBus, service.first, object.first,
                "xyz.openbmc_project.Chassis.Intrusion", "Status",
                [asyncResp](const boost::system::error_code& ec1,
                            const std::string& value) {
                if (ec1)
                {
                    // do not add err msg in redfish response, because this is
                    // not
                    //     mandatory property
                    BMCWEB_LOG_ERROR("DBUS response error {}", ec1);
                    return;
                }
                asyncResp->res
                    .jsonValue["PhysicalSecurity"]["IntrusionSensorNumber"] = 1;
                asyncResp->res
                    .jsonValue["PhysicalSecurity"]["IntrusionSensor"] = value;
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

    constexpr std::array<std::string_view, 2> interfaces{
        "xyz.openbmc_project.Inventory.Item.Board",
        "xyz.openbmc_project.Inventory.Item.Chassis"};
    collection_util::getCollectionMembers(
        asyncResp, boost::urls::url("/redfish/v1/Chassis"), interfaces,
        "/xyz/openbmc_project/inventory");
}

inline void getChassisContainedBy(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const boost::system::error_code& ec,
    const dbus::utility::MapperEndPoints& upstreamChassisPaths)
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
    const dbus::utility::MapperEndPoints& downstreamChassisPaths)
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
        link["@odata.id"] = boost::urls::format("/redfish/v1/Chassis/{}",
                                                downstreamChassis);
        jValue.push_back(std::move(link));
    }
    asyncResp->res.jsonValue["Links"]["Contains@odata.count"] = jValue.size();
}

inline void
    getChassisConnectivity(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& chassisId,
                           const std::string& chassisPath)
{
    BMCWEB_LOG_DEBUG("Get chassis connectivity");

    dbus::utility::getAssociationEndPoints(
        chassisPath + "/contained_by",
        std::bind_front(getChassisContainedBy, asyncResp, chassisId));

    dbus::utility::getAssociationEndPoints(
        chassisPath + "/containing",
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

inline void
    getChassisLocationCode(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& connectionName,
                           const std::string& path)
{
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, connectionName, path,
        "xyz.openbmc_project.Inventory.Decorator.LocationCode", "LocationCode",
        [asyncResp](const boost::system::error_code& ec,
                    const std::string& property) {
        if (ec)
        {
            BMCWEB_LOG_ERROR("DBUS response error for Location");
            messages::internalError(asyncResp->res);
            return;
        }

        asyncResp->res.jsonValue["Location"]["PartLocation"]["ServiceLabel"] =
            property;
    });
}

inline void getChassisUUID(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& connectionName,
                           const std::string& path)
{
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, connectionName, path,
        "xyz.openbmc_project.Common.UUID", "UUID",
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
    const std::string* partNumber = nullptr;
    const std::string* serialNumber = nullptr;
    const std::string* manufacturer = nullptr;
    const std::string* model = nullptr;
    const std::string* sparePartNumber = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), propertiesList, "PartNumber",
        partNumber, "SerialNumber", serialNumber, "Manufacturer", manufacturer,
        "Model", model, "SparePartNumber", sparePartNumber);

    if (!success)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    if (partNumber != nullptr)
    {
        asyncResp->res.jsonValue["PartNumber"] = *partNumber;
    }

    if (serialNumber != nullptr)
    {
        asyncResp->res.jsonValue["SerialNumber"] = *serialNumber;
    }

    if (manufacturer != nullptr)
    {
        asyncResp->res.jsonValue["Manufacturer"] = *manufacturer;
    }

    if (model != nullptr)
    {
        asyncResp->res.jsonValue["Model"] = *model;
    }

    // SparePartNumber is optional on D-Bus
    // so skip if it is empty
    if (sparePartNumber != nullptr && !sparePartNumber->empty())
    {
        asyncResp->res.jsonValue["SparePartNumber"] = *sparePartNumber;
    }

    asyncResp->res.jsonValue["Name"] = chassisId;
    asyncResp->res.jsonValue["Id"] = chassisId;
#ifdef BMCWEB_ALLOW_DEPRECATED_POWER_THERMAL
    asyncResp->res.jsonValue["Thermal"]["@odata.id"] =
        boost::urls::format("/redfish/v1/Chassis/{}/Thermal", chassisId);
    // Power object
    asyncResp->res.jsonValue["Power"]["@odata.id"] =
        boost::urls::format("/redfish/v1/Chassis/{}/Power", chassisId);
#endif
#ifdef BMCWEB_NEW_POWERSUBSYSTEM_THERMALSUBSYSTEM
    asyncResp->res.jsonValue["ThermalSubsystem"]["@odata.id"] =
        boost::urls::format("/redfish/v1/Chassis/{}/ThermalSubsystem",
                            chassisId);
    asyncResp->res.jsonValue["PowerSubsystem"]["@odata.id"] =
        boost::urls::format("/redfish/v1/Chassis/{}/PowerSubsystem", chassisId);
    asyncResp->res.jsonValue["EnvironmentMetrics"]["@odata.id"] =
        boost::urls::format("/redfish/v1/Chassis/{}/EnvironmentMetrics",
                            chassisId);
#endif
    // SensorCollection
    asyncResp->res.jsonValue["Sensors"]["@odata.id"] =
        boost::urls::format("/redfish/v1/Chassis/{}/Sensors", chassisId);
    asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

    nlohmann::json::array_t computerSystems;
    nlohmann::json::object_t system;
    system["@odata.id"] = "/redfish/v1/Systems/system";
    computerSystems.emplace_back(std::move(system));
    asyncResp->res.jsonValue["Links"]["ComputerSystems"] =
        std::move(computerSystems);

    nlohmann::json::array_t managedBy;
    nlohmann::json::object_t manager;
    manager["@odata.id"] = "/redfish/v1/Managers/bmc";
    managedBy.emplace_back(std::move(manager));
    asyncResp->res.jsonValue["Links"]["ManagedBy"] = std::move(managedBy);
    getChassisState(asyncResp);
    getStorageLink(asyncResp, path);
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

        auto health = std::make_shared<HealthPopulate>(asyncResp);

        if constexpr (bmcwebEnableHealthPopulate)
        {
            dbus::utility::getAssociationEndPoints(
                path + "/all_sensors",
                [health](const boost::system::error_code& ec2,
                         const dbus::utility::MapperEndPoints& resp) {
                if (ec2)
                {
                    return; // no sensors = no failures
                }
                health->inventory = resp;
            });

            health->populate();
        }

        if (connectionNames.empty())
        {
            BMCWEB_LOG_ERROR("Got 0 Connection names");
            continue;
        }

        asyncResp->res.jsonValue["@odata.type"] = "#Chassis.v1_22_0.Chassis";
        asyncResp->res.jsonValue["@odata.id"] =
            boost::urls::format("/redfish/v1/Chassis/{}", chassisId);
        asyncResp->res.jsonValue["Name"] = "Chassis Collection";
        asyncResp->res.jsonValue["ChassisType"] = "RackMount";
        asyncResp->res.jsonValue["Actions"]["#Chassis.Reset"]["target"] =
            boost::urls::format("/redfish/v1/Chassis/{}/Actions/Chassis.Reset",
                                chassisId);
        asyncResp->res
            .jsonValue["Actions"]["#Chassis.Reset"]["@Redfish.ActionInfo"] =
            boost::urls::format("/redfish/v1/Chassis/{}/ResetActionInfo",
                                chassisId);
        asyncResp->res.jsonValue["PCIeDevices"]["@odata.id"] =
            "/redfish/v1/Systems/system/PCIeDevices";
        asyncResp->res.jsonValue["PCIeSlots"]["@odata.id"] =
            boost::urls::format("/redfish/v1/Chassis/{}/PCIeSlots", chassisId);

        dbus::utility::getAssociationEndPoints(
            path + "/drive",
            [asyncResp, chassisId](const boost::system::error_code& ec3,
                                   const dbus::utility::MapperEndPoints& resp) {
            if (ec3 || resp.empty())
            {
                return; // no drives = no failures
            }

            nlohmann::json reference;
            reference["@odata.id"] =
                boost::urls::format("/redfish/v1/Chassis/{}/Drives", chassisId);
            asyncResp->res.jsonValue["Drives"] = std::move(reference);
        });

        const std::string& connectionName = connectionNames[0].first;

        const std::vector<std::string>& interfaces2 = connectionNames[0].second;
        const std::array<const char*, 2> hasIndicatorLed = {
            "xyz.openbmc_project.Inventory.Item.Panel",
            "xyz.openbmc_project.Inventory.Item.Board.Motherboard"};

        const std::string assetTagInterface =
            "xyz.openbmc_project.Inventory.Decorator.AssetTag";
        const std::string replaceableInterface =
            "xyz.openbmc_project.Inventory.Decorator.Replaceable";
        for (const auto& interface : interfaces2)
        {
            if (interface == assetTagInterface)
            {
                sdbusplus::asio::getProperty<std::string>(
                    *crow::connections::systemBus, connectionName, path,
                    assetTagInterface, "AssetTag",
                    [asyncResp, chassisId](const boost::system::error_code& ec2,
                                           const std::string& property) {
                    if (ec2)
                    {
                        BMCWEB_LOG_ERROR("DBus response error for AssetTag: {}",
                                         ec2);
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    asyncResp->res.jsonValue["AssetTag"] = property;
                });
            }
            else if (interface == replaceableInterface)
            {
                sdbusplus::asio::getProperty<bool>(
                    *crow::connections::systemBus, connectionName, path,
                    replaceableInterface, "HotPluggable",
                    [asyncResp, chassisId](const boost::system::error_code& ec2,
                                           const bool property) {
                    if (ec2)
                    {
                        BMCWEB_LOG_ERROR(
                            "DBus response error for HotPluggable: {}", ec2);
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    asyncResp->res.jsonValue["HotPluggable"] = property;
                });
            }
        }

        for (const char* interface : hasIndicatorLed)
        {
            if (std::ranges::find(interfaces2, interface) != interfaces2.end())
            {
                getIndicatorLedState(asyncResp);
                getSystemLocationIndicatorActive(asyncResp);
                break;
            }
        }

        sdbusplus::asio::getAllProperties(
            *crow::connections::systemBus, connectionName, path,
            "xyz.openbmc_project.Inventory.Decorator.Asset",
            [asyncResp, chassisId,
             path](const boost::system::error_code&,
                   const dbus::utility::DBusPropertiesMap& propertiesList) {
            handleDecoratorAssetProperties(asyncResp, chassisId, path,
                                           propertiesList);
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

inline void
    handleChassisGet(App& app, const crow::Request& req,
                     const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const std::string& chassisId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    constexpr std::array<std::string_view, 2> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Board",
        "xyz.openbmc_project.Inventory.Item.Chassis"};

    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        std::bind_front(handleChassisGetSubTree, asyncResp, chassisId));

    constexpr std::array<std::string_view, 1> interfaces2 = {
        "xyz.openbmc_project.Chassis.Intrusion"};

    dbus::utility::getSubTree(
        "/xyz/openbmc_project", 0, interfaces2,
        std::bind_front(handlePhysicalSecurityGetSubTree, asyncResp));
}

inline void
    handleChassisPatch(App& app, const crow::Request& req,
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

    if (!json_util::readJsonPatch(
            req, asyncResp->res, "LocationIndicatorActive",
            locationIndicatorActive, "IndicatorLED", indicatorLed))
    {
        return;
    }

    // TODO (Gunnar): Remove IndicatorLED after enough time has passed
    if (!locationIndicatorActive && !indicatorLed)
    {
        return; // delete this when we support more patch properties
    }
    if (indicatorLed)
    {
        asyncResp->res.addHeader(
            boost::beast::http::field::warning,
            "299 - \"IndicatorLED is deprecated. Use LocationIndicatorActive instead.\"");
    }

    constexpr std::array<std::string_view, 2> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Board",
        "xyz.openbmc_project.Inventory.Item.Chassis"};

    const std::string& chassisId = param;

    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, interfaces,
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

            if (connectionNames.empty())
            {
                BMCWEB_LOG_ERROR("Got 0 Connection names");
                continue;
            }

            const std::vector<std::string>& interfaces3 =
                connectionNames[0].second;

            const std::array<const char*, 2> hasIndicatorLed = {
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
                    setSystemLocationIndicatorActive(asyncResp,
                                                     *locationIndicatorActive);
                }
                else
                {
                    messages::propertyUnknown(asyncResp->res,
                                              "LocationIndicatorActive");
                }
            }
            if (indicatorLed)
            {
                if (indicatorChassis)
                {
                    setIndicatorLedState(asyncResp, *indicatorLed);
                }
                else
                {
                    messages::propertyUnknown(asyncResp->res, "IndicatorLED");
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

/**
 * Handle error responses from d-bus for chassis power cycles
 */
inline void handleChassisPowerCycleError(const boost::system::error_code& ec,
                                         const sdbusplus::message_t& eMsg,
                                         crow::Response& res)
{
    if (eMsg.get_error() == nullptr)
    {
        BMCWEB_LOG_ERROR("D-Bus response error: {}", ec);
        messages::internalError(res);
        return;
    }
    std::string_view errorMessage = eMsg.get_error()->name;

    // If operation failed due to BMC not being in Ready state, tell
    // user to retry in a bit
    if (errorMessage ==
        std::string_view("xyz.openbmc_project.State.Chassis.Error.BMCNotReady"))
    {
        BMCWEB_LOG_DEBUG("BMC not ready, operation not allowed right now");
        messages::serviceTemporarilyUnavailable(res, "10");
        return;
    }

    BMCWEB_LOG_ERROR("Chassis Power Cycle fail {} sdbusplus:{}", ec,
                     errorMessage);
    messages::internalError(res);
}

inline void
    doChassisPowerCycle(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
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
        std::string objectPath = "/xyz/openbmc_project/state/chassis_system0";

        /* Look for system reset chassis path */
        if ((std::ranges::find(chassisList, objectPath)) == chassisList.end())
        {
            /* We prefer to reset the full chassis_system, but if it doesn't
             * exist on some platforms, fall back to a host-only power reset
             */
            objectPath = "/xyz/openbmc_project/state/chassis0";
        }

        sdbusplus::asio::setProperty(
            *crow::connections::systemBus, processName, objectPath,
            interfaceName, destProperty, propertyValue,
            [asyncResp](const boost::system::error_code& ec2,
                        sdbusplus::message_t& sdbusErrMsg) {
            // Use "Set" method to set the property value.
            if (ec2)
            {
                handleChassisPowerCycleError(ec2, sdbusErrMsg, asyncResp->res);

                return;
            }

            messages::success(asyncResp->res);
        });
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
    parameter["DataType"] = "String";
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
