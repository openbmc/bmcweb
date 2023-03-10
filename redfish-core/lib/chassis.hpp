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

#include "app.hpp"
#include "dbus_utility.hpp"
#include "health.hpp"
#include "led.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/collection.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"

#include <boost/system/error_code.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <string_view>

namespace redfish
{

/**
 * @brief Retrieves chassis state properties over dbus
 *
 * @param[in] aResp - Shared pointer for completing asynchronous calls.
 *
 * @return None.
 */
inline void getChassisState(std::shared_ptr<bmcweb::AsyncResp> aResp)
{
    // crow::connections::systemBus->async_method_call(
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, "xyz.openbmc_project.State.Chassis",
        "/xyz/openbmc_project/state/chassis0",
        "xyz.openbmc_project.State.Chassis", "CurrentPowerState",
        [aResp{std::move(aResp)}](const boost::system::error_code& ec,
                                  const std::string& chassisState) {
        if (ec)
        {
            if (ec == boost::system::errc::host_unreachable)
            {
                // Service not available, no error, just don't return
                // chassis state info
                BMCWEB_LOG_DEBUG << "Service not available " << ec;
                return;
            }
            BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
            messages::internalError(aResp->res);
            return;
        }

        BMCWEB_LOG_DEBUG << "Chassis state: " << chassisState;
        // Verify Chassis State
        if (chassisState == "xyz.openbmc_project.State.Chassis.PowerState.On")
        {
            aResp->res.jsonValue["PowerState"] = "On";
            aResp->res.jsonValue["Status"]["State"] = "Enabled";
        }
        else if (chassisState ==
                 "xyz.openbmc_project.State.Chassis.PowerState.Off")
        {
            aResp->res.jsonValue["PowerState"] = "Off";
            aResp->res.jsonValue["Status"]["State"] = "StandbyOffline";
        }
        });
}

inline void getIntrusionByService(std::shared_ptr<bmcweb::AsyncResp> aResp,
                                  const std::string& service,
                                  const std::string& objPath)
{
    BMCWEB_LOG_DEBUG << "Get intrusion status by service \n";

    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, service, objPath,
        "xyz.openbmc_project.Chassis.Intrusion", "Status",
        [aResp{std::move(aResp)}](const boost::system::error_code& ec,
                                  const std::string& value) {
        if (ec)
        {
            // do not add err msg in redfish response, because this is not
            //     mandatory property
            BMCWEB_LOG_ERROR << "DBUS response error " << ec << "\n";
            return;
        }

        aResp->res.jsonValue["PhysicalSecurity"]["IntrusionSensorNumber"] = 1;
        aResp->res.jsonValue["PhysicalSecurity"]["IntrusionSensor"] = value;
        });
}

/**
 * Retrieves physical security properties over dbus
 */
inline void getPhysicalSecurityData(std::shared_ptr<bmcweb::AsyncResp> aResp)
{
    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Chassis.Intrusion"};
    dbus::utility::getSubTree(
        "/xyz/openbmc_project/Intrusion", 1, interfaces,
        [aResp{std::move(aResp)}](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            // do not add err msg in redfish response, because this is not
            //     mandatory property
            BMCWEB_LOG_INFO << "DBUS error: no matched iface " << ec << "\n";
            return;
        }
        // Iterate over all retrieved ObjectPaths.
        for (const auto& object : subtree)
        {
            for (const auto& service : object.second)
            {
                getIntrusionByService(aResp, service.first, object.first);
                return;
            }
        }
        });
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
        asyncResp, boost::urls::url("/redfish/v1/Chassis"), interfaces);
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
            BMCWEB_LOG_DEBUG << "DBUS response error for Location";
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
            BMCWEB_LOG_DEBUG << "DBUS response error for UUID";
            messages::internalError(asyncResp->res);
            return;
        }
        asyncResp->res.jsonValue["UUID"] = chassisUUID;
        });
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
        [asyncResp, chassisId(std::string(chassisId))](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
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

            auto health = std::make_shared<HealthPopulate>(asyncResp);

            sdbusplus::asio::getProperty<std::vector<std::string>>(
                *crow::connections::systemBus,
                "xyz.openbmc_project.ObjectMapper", path + "/all_sensors",
                "xyz.openbmc_project.Association", "endpoints",
                [health](const boost::system::error_code& ec2,
                         const std::vector<std::string>& resp) {
                if (ec2)
                {
                    return; // no sensors = no failures
                }
                health->inventory = resp;
                });

            health->populate();

            if (connectionNames.empty())
            {
                BMCWEB_LOG_ERROR << "Got 0 Connection names";
                continue;
            }

            asyncResp->res.jsonValue["@odata.type"] =
                "#Chassis.v1_16_0.Chassis";
            asyncResp->res.jsonValue["@odata.id"] =
                crow::utility::urlFromPieces("redfish", "v1", "Chassis",
                                             chassisId);
            asyncResp->res.jsonValue["Name"] = "Chassis Collection";
            asyncResp->res.jsonValue["ChassisType"] = "RackMount";
            asyncResp->res.jsonValue["Actions"]["#Chassis.Reset"]["target"] =
                crow::utility::urlFromPieces("redfish", "v1", "Chassis",
                                             chassisId, "Actions",
                                             "Chassis.Reset");
            asyncResp->res
                .jsonValue["Actions"]["#Chassis.Reset"]["@Redfish.ActionInfo"] =
                crow::utility::urlFromPieces("redfish", "v1", "Chassis",
                                             chassisId, "ResetActionInfo");
            asyncResp->res.jsonValue["PCIeDevices"]["@odata.id"] =
                crow::utility::urlFromPieces("redfish", "v1", "Systems",
                                             "system", "PCIeDevices");

            sdbusplus::asio::getProperty<std::vector<std::string>>(
                *crow::connections::systemBus,
                "xyz.openbmc_project.ObjectMapper", path + "/drive",
                "xyz.openbmc_project.Association", "endpoints",
                [asyncResp, chassisId](const boost::system::error_code& ec3,
                                       const std::vector<std::string>& resp) {
                if (ec3 || resp.empty())
                {
                    return; // no drives = no failures
                }

                nlohmann::json reference;
                reference["@odata.id"] = crow::utility::urlFromPieces(
                    "redfish", "v1", "Chassis", chassisId, "Drives");
                asyncResp->res.jsonValue["Drives"] = std::move(reference);
                });

            const std::string& connectionName = connectionNames[0].first;

            const std::vector<std::string>& interfaces2 =
                connectionNames[0].second;
            const std::array<const char*, 2> hasIndicatorLed = {
                "xyz.openbmc_project.Inventory.Item.Panel",
                "xyz.openbmc_project.Inventory.Item.Board.Motherboard"};

            const std::string assetTagInterface =
                "xyz.openbmc_project.Inventory.Decorator.AssetTag";
            if (std::find(interfaces2.begin(), interfaces2.end(),
                          assetTagInterface) != interfaces2.end())
            {
                sdbusplus::asio::getProperty<std::string>(
                    *crow::connections::systemBus, connectionName, path,
                    assetTagInterface, "AssetTag",
                    [asyncResp, chassisId(std::string(chassisId))](
                        const boost::system::error_code& ec2,
                        const std::string& property) {
                    if (ec2)
                    {
                        BMCWEB_LOG_DEBUG << "DBus response error for AssetTag";
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    asyncResp->res.jsonValue["AssetTag"] = property;
                    });
            }

            for (const char* interface : hasIndicatorLed)
            {
                if (std::find(interfaces2.begin(), interfaces2.end(),
                              interface) != interfaces2.end())
                {
                    getIndicatorLedState(asyncResp);
                    getLocationIndicatorActive(asyncResp);
                    break;
                }
            }

            const std::string thermalDirInterface =
                "xyz.openbmc_project.Inventory.Decorator.ThermalDirection";
            if (std::find(interfaces2.begin(), interfaces2.end(),
                          thermalDirInterface) != interfaces2.end())
            {
                sdbusplus::asio::getProperty<std::string>(
                    *crow::connections::systemBus, connectionName, path,
                    thermalDirInterface, "ThermalDirection",
                    [asyncResp, chassisId(std::string(chassisId))](
                        const boost::system::error_code ec2,
                        const std::string& property) {
                    if (ec2)
                    {
                        BMCWEB_LOG_DEBUG
                            << "DBus response error for thermal Direction";
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    if (!property.empty())
                    {
                        asyncResp->res.jsonValue["ThermalDirection"] = property;
                    }
                    });
            }

            sdbusplus::asio::getAllProperties(
                *crow::connections::systemBus, connectionName, path,
                "xyz.openbmc_project.Inventory.Decorator.Asset",
                [asyncResp, chassisId(std::string(chassisId))](
                    const boost::system::error_code& /*ec2*/,
                    const dbus::utility::DBusPropertiesMap& propertiesList) {
                const std::string* partNumber = nullptr;
                const std::string* serialNumber = nullptr;
                const std::string* manufacturer = nullptr;
                const std::string* model = nullptr;
                const std::string* sparePartNumber = nullptr;

                const bool success = sdbusplus::unpackPropertiesNoThrow(
                    dbus_utils::UnpackErrorPrinter(), propertiesList,
                    "PartNumber", partNumber, "SerialNumber", serialNumber,
                    "Manufacturer", manufacturer, "Model", model,
                    "SparePartNumber", sparePartNumber);

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
                    asyncResp->res.jsonValue["SparePartNumber"] =
                        *sparePartNumber;
                }

                asyncResp->res.jsonValue["Name"] = chassisId;
                asyncResp->res.jsonValue["Id"] = chassisId;
#ifdef BMCWEB_ALLOW_DEPRECATED_POWER_THERMAL
                asyncResp->res.jsonValue["Thermal"]["@odata.id"] =
                    crow::utility::urlFromPieces("redfish", "v1", "Chassis",
                                                 chassisId, "Thermal");
                // Power object
                asyncResp->res.jsonValue["Power"]["@odata.id"] =
                    crow::utility::urlFromPieces("redfish", "v1", "Chassis",
                                                 chassisId, "Power");
#endif
#ifdef BMCWEB_NEW_POWERSUBSYSTEM_THERMALSUBSYSTEM
                asyncResp->res.jsonValue["ThermalSubsystem"]["@odata.id"] =
                    crow::utility::urlFromPieces("redfish", "v1", "Chassis",
                                                 chassisId, "ThermalSubsystem");
                asyncResp->res.jsonValue["PowerSubsystem"]["@odata.id"] =
                    crow::utility::urlFromPieces("redfish", "v1", "Chassis",
                                                 chassisId, "PowerSubsystem");
                asyncResp->res.jsonValue["EnvironmentMetrics"]["@odata.id"] =
                    crow::utility::urlFromPieces("redfish", "v1", "Chassis",
                                                 chassisId,
                                                 "EnvironmentMetrics");
#endif
                // SensorCollection
                asyncResp->res.jsonValue["Sensors"]["@odata.id"] =
                    crow::utility::urlFromPieces("redfish", "v1", "Chassis",
                                                 chassisId, "Sensors");
                asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

                nlohmann::json::array_t computerSystems;
                nlohmann::json::object_t system;
                system["@odata.id"] = "/redfish/v1/Systems/system";
                computerSystems.push_back(std::move(system));
                asyncResp->res.jsonValue["Links"]["ComputerSystems"] =
                    std::move(computerSystems);

                nlohmann::json::array_t managedBy;
                nlohmann::json::object_t manager;
                manager["@odata.id"] = "/redfish/v1/Managers/bmc";
                managedBy.push_back(std::move(manager));
                asyncResp->res.jsonValue["Links"]["ManagedBy"] =
                    std::move(managedBy);
                getChassisState(asyncResp);
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
        });

    getPhysicalSecurityData(asyncResp);
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
                BMCWEB_LOG_ERROR << "Got 0 Connection names";
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
                if (std::find(interfaces3.begin(), interfaces3.end(),
                              interface) != interfaces3.end())
                {
                    indicatorChassis = true;
                    break;
                }
            }
            if (locationIndicatorActive)
            {
                if (indicatorChassis)
                {
                    setLocationIndicatorActive(asyncResp,
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
            BMCWEB_LOG_DEBUG << "[mapper] Bad D-Bus request error: " << ec;
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
        if ((std::find(chassisList.begin(), chassisList.end(), objectPath)) ==
            chassisList.end())
        {
            /* We prefer to reset the full chassis_system, but if it doesn't
             * exist on some platforms, fall back to a host-only power reset
             */
            objectPath = "/xyz/openbmc_project/state/chassis0";
        }

        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code& ec2) {
            // Use "Set" method to set the property value.
            if (ec2)
            {
                BMCWEB_LOG_DEBUG << "[Set] Bad D-Bus request error: " << ec2;
                messages::internalError(asyncResp->res);
                return;
            }

            messages::success(asyncResp->res);
            },
            processName, objectPath, "org.freedesktop.DBus.Properties", "Set",
            interfaceName, destProperty,
            dbus::utility::DbusVariantType{propertyValue});
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
    BMCWEB_LOG_DEBUG << "Post Chassis Reset.";

    std::string resetType;

    if (!json_util::readJsonAction(req, asyncResp->res, "ResetType", resetType))
    {
        return;
    }

    if (resetType != "PowerCycle")
    {
        BMCWEB_LOG_DEBUG << "Invalid property value for ResetType: "
                         << resetType;
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
    asyncResp->res.jsonValue["@odata.id"] = crow::utility::urlFromPieces(
        "redfish", "v1", "Chassis", chassisId, "ResetActionInfo");
    asyncResp->res.jsonValue["Name"] = "Reset Action Info";

    asyncResp->res.jsonValue["Id"] = "ResetActionInfo";
    nlohmann::json::array_t parameters;
    nlohmann::json::object_t parameter;
    parameter["Name"] = "ResetType";
    parameter["Required"] = true;
    parameter["DataType"] = "String";
    nlohmann::json::array_t allowed;
    allowed.push_back("PowerCycle");
    parameter["AllowableValues"] = std::move(allowed);
    parameters.push_back(std::move(parameter));

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
