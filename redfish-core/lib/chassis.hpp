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

#include "health.hpp"
#include "led.hpp"

#include <app.hpp>
#include <boost/container/flat_map.hpp>
#include <dbus_utility.hpp>
#include <registries/privilege_registry.hpp>
#include <sdbusplus/asio/property.hpp>
#include <utils/collection.hpp>

namespace redfish
{

inline std::optional<std::string>
    getChassisStateProperty(const std::variant<std::string>& chassisState)
{
    const std::string* value = std::get_if<std::string>(&chassisState);
    if (!value)
    {
        return std::nullopt;
    }
    if (*value == "xyz.openbmc_project.State.Chassis.PowerState.On")
    {
        return "Enabled";
    }
    if (*value == "xyz.openbmc_project.State.Chassis.PowerState.Off")
    {
        return "StandbyOffline";
    }

    return std::nullopt;
}

inline std::optional<std::string>
    getPowerStateProperty(const std::variant<std::string>& chassisState)
{
    const std::string* value = std::get_if<std::string>(&chassisState);
    if (!value)
    {
        return std::nullopt;
    }
    if (*value == "xyz.openbmc_project.State.Chassis.PowerState.On")
    {
        return "On";
    }
    if (*value == "xyz.openbmc_project.State.Chassis.PowerState.Off")
    {
        return "Off";
    }

    return std::nullopt;
}

/**
 * @brief Retrieves chassis state over dbus
 *
 * @param[in] aResp - Shared pointer for completing asynchronous calls.
 * @param[in] chassisId - Name of the chassis being worked with.
 * @param[in] callback - Callback function 
 *
 * @return None.
 */
inline void getChassisState(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId,
    const std::function<void(const boost::system::error_code ec,
                             const std::optional<std::string>&)>& callback)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, chassisId, callback](const boost::system::error_code ec,
                   const std::variant<std::string>& property) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "Chassis: DBUS response error for Chassis";
                callback(ec, std::nullopt);
                return;
            }

            callback(ec, getChassisStateProperty(property));
        },
        "xyz.openbmc_project.State.Chassis",
        "/xyz/openbmc_project/state/chassis0",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.State.Chassis", "CurrentPowerState");
}

/**
 * @brief Retrieves chassis power state over dbus
 *
 * @param[in] aResp - Shared pointer for completing asynchronous calls.
 * @param[in] chassisId - Name of the chassis being worked with.
 * @param[in] callback - Callback function 
 *
 * @return None.
 */
inline void getPowerState(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId,
    const std::function<void(const boost::system::error_code ec,
                             const std::optional<std::string>&)>& callback)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, chassisId,
         callback](const boost::system::error_code ec,
                   const std::variant<std::string>& property) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "Chassis: DBUS response error for Chassis";
                callback(ec, std::nullopt);
                return;
            }

            callback(ec, getPowerStateProperty(property));
        },
        "xyz.openbmc_project.State.Chassis",
        "/xyz/openbmc_project/state/chassis0",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.State.Chassis", "CurrentPowerState");
}

/**
 * DBus types primitives for several generic DBus interfaces
 * TODO(Pawel) consider move this to separate file into boost::dbus
 */
using ManagedObjectsType = std::vector<std::pair<
    sdbusplus::message::object_path,
    std::vector<std::pair<
        std::string,
        std::vector<std::pair<std::string, dbus::utility::DbusVariantType>>>>>>;

using PropertiesType =
    boost::container::flat_map<std::string, dbus::utility::DbusVariantType>;

inline void getIntrusionByService(std::shared_ptr<bmcweb::AsyncResp> aResp,
                                  const std::string& service,
                                  const std::string& objPath)
{
    BMCWEB_LOG_DEBUG << "Get intrusion status by service \n";

    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, service, objPath,
        "xyz.openbmc_project.Chassis.Intrusion", "Status",
        [aResp{std::move(aResp)}](const boost::system::error_code ec,
                                  const std::string& value) {
            if (ec)
            {
                // do not add err msg in redfish response, because this is not
                //     mandatory property
                BMCWEB_LOG_ERROR << "DBUS response error " << ec << "\n";
                return;
            }

            aResp->res.jsonValue["PhysicalSecurity"] = {
                {"IntrusionSensorNumber", 1}, {"IntrusionSensor", value}};
        });
}

/**
 * Retrieves physical security properties over dbus
 */
inline void getPhysicalSecurityData(std::shared_ptr<bmcweb::AsyncResp> aResp)
{
    crow::connections::systemBus->async_method_call(
        [aResp{std::move(aResp)}](
            const boost::system::error_code ec,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>&
                subtree) {
            if (ec)
            {
                // do not add err msg in redfish response, because this is not
                //     mandatory property
                BMCWEB_LOG_INFO << "DBUS error: no matched iface " << ec
                                << "\n";
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
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/Intrusion", 1,
        std::array<const char*, 1>{"xyz.openbmc_project.Chassis.Intrusion"});
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
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#ChassisCollection.ChassisCollection";
                asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/Chassis";
                asyncResp->res.jsonValue["Name"] = "Chassis Collection";

                collection_util::getCollectionMembers(
                    asyncResp, "/redfish/v1/Chassis",
                    {"xyz.openbmc_project.Inventory.Item.Board",
                     "xyz.openbmc_project.Inventory.Item.Chassis"});
            });
}

inline void
    getChassisLocationCode(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& connectionName,
                           const std::string& path)
{
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, connectionName, path,
        "xyz.openbmc_project.Inventory.Decorator.LocationCode", "LocationCode",
        [asyncResp](const boost::system::error_code ec,
                    const std::string& property) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error for Location";
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
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, connectionName, path,
        "xyz.openbmc_project.Common.UUID", "UUID",
        [asyncResp](const boost::system::error_code ec,
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

/**
 * Chassis override class for delivering Chassis Schema
 * Functions triggers appropriate requests on DBus
 */
inline void requestRoutesChassis(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/")
        .privileges(redfish::privileges::getChassis)
        .methods(
            boost::beast::http::verb::get)([](const crow::Request&,
                                              const std::shared_ptr<
                                                  bmcweb::AsyncResp>& asyncResp,
                                              const std::string& chassisId) {
            const std::array<const char*, 2> interfaces = {
                "xyz.openbmc_project.Inventory.Item.Board",
                "xyz.openbmc_project.Inventory.Item.Chassis"};

            crow::connections::systemBus->async_method_call(
                [asyncResp, chassisId(std::string(chassisId))](
                    const boost::system::error_code ec,
                    const crow::openbmc_mapper::GetSubTreeType& subtree) {
                    if (ec)
                    {
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    // Iterate over all retrieved ObjectPaths.
                    for (const std::pair<
                             std::string,
                             std::vector<std::pair<std::string,
                                                   std::vector<std::string>>>>&
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

                        auto health =
                            std::make_shared<HealthPopulate>(asyncResp);

                        sdbusplus::asio::getProperty<std::vector<std::string>>(
                            *crow::connections::systemBus,
                            "xyz.openbmc_project.ObjectMapper",
                            path + "/all_sensors",
                            "xyz.openbmc_project.Association", "endpoints",
                            [health](const boost::system::error_code ec2,
                                     const std::vector<std::string>& resp) {
                                if (ec2)
                                {
                                    return; // no sensors = no failures
                                }
                                health->inventory = resp;
                            });

                        health->populate();

                        if (connectionNames.size() < 1)
                        {
                            BMCWEB_LOG_ERROR << "Got 0 Connection names";
                            continue;
                        }

                        asyncResp->res.jsonValue["@odata.type"] =
                            "#Chassis.v1_16_0.Chassis";
                        asyncResp->res.jsonValue["@odata.id"] =
                            "/redfish/v1/Chassis/" + chassisId;
                        asyncResp->res.jsonValue["Name"] = "Chassis Collection";
                        asyncResp->res.jsonValue["ChassisType"] = "RackMount";
                        asyncResp->res.jsonValue["Actions"]["#Chassis.Reset"] =
                            {{"target", "/redfish/v1/Chassis/" + chassisId +
                                            "/Actions/Chassis.Reset"},
                             {"@Redfish.ActionInfo", "/redfish/v1/Chassis/" +
                                                         chassisId +
                                                         "/ResetActionInfo"}};
                        asyncResp->res.jsonValue["PCIeDevices"] = {
                            {"@odata.id",
                             "/redfish/v1/Systems/system/PCIeDevices"}};

                        const std::string& connectionName =
                            connectionNames[0].first;

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
                                *crow::connections::systemBus, connectionName,
                                path, assetTagInterface, "AssetTag",
                                [asyncResp, chassisId(std::string(chassisId))](
                                    const boost::system::error_code ec,
                                    const std::string& property) {
                                    if (ec)
                                    {
                                        BMCWEB_LOG_DEBUG
                                            << "DBus response error for AssetTag";
                                        messages::internalError(asyncResp->res);
                                        return;
                                    }
                                    asyncResp->res.jsonValue["AssetTag"] =
                                        property;
                                });
                        }

                        crow::connections::systemBus->async_method_call(
                            [asyncResp, connectionName, path,
                                chassisId(std::string(chassisId))](
                                const boost::system::error_code ec,
                                const std::variant<bool>& property) {
                                if (ec)
                                {
                                    BMCWEB_LOG_DEBUG
                                        << "DBus response error for Present";
                                    messages::internalError(asyncResp->res);
                                    return;
                                }

                                getPowerState(asyncResp, chassisId,
                                    [asyncResp, chassisId](
                                        const boost::system::error_code ec,
                                        const std::optional<std::string>&
                                            chassisPowerState) {
                                        if (ec)
                                        {
                                            messages::internalError(
                                                asyncResp->res);
                                        }
                                        if (chassisPowerState)
                                        {
                                            asyncResp->res.jsonValue
                                                ["PowerState"] =
                                                *chassisPowerState;
                                        }
                                        
                                    });

                                const bool* present =
                                    std::get_if<bool>(&property);
                                if (present == nullptr)
                                {
                                    BMCWEB_LOG_DEBUG
                                        << "Null value returned "
                                           "for Chassis present";
                                    messages::internalError(asyncResp->res);
                                    return;
                                }

                                if (*present) {
                                    getChassisState(asyncResp, chassisId,
                                        [asyncResp, chassisId](
                                        const boost::system::error_code ec,
                                        const std::optional<std::string>&
                                            chassisState) {
                                        if (ec)
                                        {
                                            messages::internalError(
                                                asyncResp->res);
                                        }
                                        if (chassisState)
                                        {
                                            asyncResp->res.jsonValue["Status"]
                                            ["State"] = *chassisState;
                                        }
                                    });
                                } else {
                                    asyncResp->res.jsonValue["Status"]["State"]
                                        = "Absent";
                                }
                            },
                            connectionName, path,
                            "org.freedesktop.DBus.Properties", "Get",
                            "xyz.openbmc_project.Inventory.Item", "Present");

                        for (const char* interface : hasIndicatorLed)
                        {
                            if (std::find(interfaces2.begin(),
                                          interfaces2.end(),
                                          interface) != interfaces2.end())
                            {
                                getIndicatorLedState(asyncResp);
                                getLocationIndicatorActive(asyncResp);
                                break;
                            }
                        }

                        crow::connections::systemBus->async_method_call(
                            [asyncResp, chassisId(std::string(chassisId))](
                                const boost::system::error_code /*ec2*/,
                                const std::vector<
                                    std::pair<std::string,
                                              dbus::utility::DbusVariantType>>&
                                    propertiesList) {
                                for (const std::pair<
                                         std::string,
                                         dbus::utility::DbusVariantType>&
                                         property : propertiesList)
                                {
                                    // Store DBus properties that are also
                                    // Redfish properties with same name and a
                                    // string value
                                    const std::string& propertyName =
                                        property.first;
                                    if ((propertyName == "PartNumber") ||
                                        (propertyName == "SerialNumber") ||
                                        (propertyName == "Manufacturer") ||
                                        (propertyName == "Model") ||
                                        (propertyName == "SparePartNumber"))
                                    {
                                        const std::string* value =
                                            std::get_if<std::string>(
                                                &property.second);
                                        if (value == nullptr)
                                        {
                                            BMCWEB_LOG_ERROR
                                                << "Null value returned for "
                                                << propertyName;
                                            messages::internalError(
                                                asyncResp->res);
                                            return;
                                        }
                                        // SparePartNumber is optional on D-Bus
                                        // so skip if it is empty
                                        if (propertyName == "SparePartNumber")
                                        {
                                            if (*value == "")
                                            {
                                                continue;
                                            }
                                        }
                                        asyncResp->res.jsonValue[propertyName] =
                                            *value;
                                    }
                                }
                                asyncResp->res.jsonValue["Name"] = chassisId;
                                asyncResp->res.jsonValue["Id"] = chassisId;
#ifdef BMCWEB_ALLOW_DEPRECATED_POWER_THERMAL
                                asyncResp->res.jsonValue["Thermal"] = {
                                    {"@odata.id", "/redfish/v1/Chassis/" +
                                                      chassisId + "/Thermal"}};
                                // Power object
                                asyncResp->res.jsonValue["Power"] = {
                                    {"@odata.id", "/redfish/v1/Chassis/" +
                                                      chassisId + "/Power"}};
#endif
                                // SensorCollection
                                asyncResp->res.jsonValue["Sensors"] = {
                                    {"@odata.id", "/redfish/v1/Chassis/" +
                                                      chassisId + "/Sensors"}};
                                asyncResp->res
                                    .jsonValue["Links"]["ComputerSystems"] = {
                                    {{"@odata.id",
                                      "/redfish/v1/Systems/system"}}};
                                asyncResp->res.jsonValue["Links"]["ManagedBy"] =
                                    {{{"@odata.id",
                                       "/redfish/v1/Managers/bmc"}}};
                            },
                            connectionName, path,
                            "org.freedesktop.DBus.Properties", "GetAll",
                            "xyz.openbmc_project.Inventory.Decorator.Asset");

                        for (const auto& interface : interfaces2)
                        {
                            if (interface == "xyz.openbmc_project.Common.UUID")
                            {
                                getChassisUUID(asyncResp, connectionName, path);
                            }
                            else if (
                                interface ==
                                "xyz.openbmc_project.Inventory.Decorator.LocationCode")
                            {
                                getChassisLocationCode(asyncResp,
                                                       connectionName, path);
                            }
                        }

                        return;
                    }

                    // Couldn't find an object with that name.  return an error
                    messages::resourceNotFound(
                        asyncResp->res, "#Chassis.v1_16_0.Chassis", chassisId);
                },
                "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/object_mapper",
                "xyz.openbmc_project.ObjectMapper", "GetSubTree",
                "/xyz/openbmc_project/inventory", 0, interfaces);

            getPhysicalSecurityData(asyncResp);
        });

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/")
        .privileges(redfish::privileges::patchChassis)
        .methods(
            boost::beast::http::verb::
                patch)([](const crow::Request& req,
                          const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& param) {
            std::optional<bool> locationIndicatorActive;
            std::optional<std::string> indicatorLed;

            if (param.empty())
            {
                return;
            }

            if (!json_util::readJson(
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

            const std::array<const char*, 2> interfaces = {
                "xyz.openbmc_project.Inventory.Item.Board",
                "xyz.openbmc_project.Inventory.Item.Chassis"};

            const std::string& chassisId = param;

            crow::connections::systemBus->async_method_call(
                [asyncResp, chassisId, locationIndicatorActive, indicatorLed](
                    const boost::system::error_code ec,
                    const crow::openbmc_mapper::GetSubTreeType& subtree) {
                    if (ec)
                    {
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    // Iterate over all retrieved ObjectPaths.
                    for (const std::pair<
                             std::string,
                             std::vector<std::pair<std::string,
                                                   std::vector<std::string>>>>&
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

                        if (connectionNames.size() < 1)
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
                            if (std::find(interfaces3.begin(),
                                          interfaces3.end(),
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
                                setLocationIndicatorActive(
                                    asyncResp, *locationIndicatorActive);
                            }
                            else
                            {
                                messages::propertyUnknown(
                                    asyncResp->res, "LocationIndicatorActive");
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
                                messages::propertyUnknown(asyncResp->res,
                                                          "IndicatorLED");
                            }
                        }
                        return;
                    }

                    messages::resourceNotFound(
                        asyncResp->res, "#Chassis.v1_14_0.Chassis", chassisId);
                },
                "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/object_mapper",
                "xyz.openbmc_project.ObjectMapper", "GetSubTree",
                "/xyz/openbmc_project/inventory", 0, interfaces);
        });
}

inline void
    doChassisPowerCycle(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    const char* busName = "xyz.openbmc_project.ObjectMapper";
    const char* path = "/xyz/openbmc_project/object_mapper";
    const char* interface = "xyz.openbmc_project.ObjectMapper";
    const char* method = "GetSubTreePaths";

    const std::array<const char*, 1> interfaces = {
        "xyz.openbmc_project.State.Chassis"};

    // Use mapper to get subtree paths.
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec,
                    const std::vector<std::string>& chassisList) {
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
            std::string objectPath =
                "/xyz/openbmc_project/state/chassis_system0";

            /* Look for system reset chassis path */
            if ((std::find(chassisList.begin(), chassisList.end(),
                           objectPath)) == chassisList.end())
            {
                /* We prefer to reset the full chassis_system, but if it doesn't
                 * exist on some platforms, fall back to a host-only power reset
                 */
                objectPath = "/xyz/openbmc_project/state/chassis0";
            }

            crow::connections::systemBus->async_method_call(
                [asyncResp](const boost::system::error_code ec) {
                    // Use "Set" method to set the property value.
                    if (ec)
                    {
                        BMCWEB_LOG_DEBUG << "[Set] Bad D-Bus request error: "
                                         << ec;
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    messages::success(asyncResp->res);
                },
                processName, objectPath, "org.freedesktop.DBus.Properties",
                "Set", interfaceName, destProperty,
                dbus::utility::DbusVariantType{propertyValue});
        },
        busName, path, interface, method, "/", 0, interfaces);
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
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string&) {
                BMCWEB_LOG_DEBUG << "Post Chassis Reset.";

                std::string resetType;

                if (!json_util::readJson(req, asyncResp->res, "ResetType",
                                         resetType))
                {
                    return;
                }

                if (resetType != "PowerCycle")
                {
                    BMCWEB_LOG_DEBUG << "Invalid property value for ResetType: "
                                     << resetType;
                    messages::actionParameterNotSupported(
                        asyncResp->res, resetType, "ResetType");

                    return;
                }
                doChassisPowerCycle(asyncResp);
            });
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
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& chassisId)

            {
                asyncResp->res.jsonValue = {
                    {"@odata.type", "#ActionInfo.v1_1_2.ActionInfo"},
                    {"@odata.id",
                     "/redfish/v1/Chassis/" + chassisId + "/ResetActionInfo"},
                    {"Name", "Reset Action Info"},
                    {"Id", "ResetActionInfo"},
                    {"Parameters",
                     {{{"Name", "ResetType"},
                       {"Required", true},
                       {"DataType", "String"},
                       {"AllowableValues", {"PowerCycle"}}}}}};
            });
}

} // namespace redfish
