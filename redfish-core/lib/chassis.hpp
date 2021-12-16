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
#include <utils/collection.hpp>

#include <variant>

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
    crow::connections::systemBus->async_method_call(
        [aResp{std::move(aResp)}](
            const boost::system::error_code ec,
            const std::variant<std::string>& chassisState) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                messages::internalError(aResp->res);
                return;
            }

            const std::string* s = std::get_if<std::string>(&chassisState);
            BMCWEB_LOG_DEBUG << "Chassis state: " << *s;
            if (s != nullptr)
            {
                // Verify Chassis State
                if (*s == "xyz.openbmc_project.State.Chassis.PowerState.On")
                {
                    aResp->res.jsonValue["PowerState"] = "On";
                    aResp->res.jsonValue["Status"]["State"] = "Enabled";
                }
                else if (*s ==
                         "xyz.openbmc_project.State.Chassis.PowerState.Off")
                {
                    aResp->res.jsonValue["PowerState"] = "Off";
                    aResp->res.jsonValue["Status"]["State"] = "StandbyOffline";
                }
            }
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
// Note, this is not a very useful Variant, but because it isn't used to get
// values, it should be as simple as possible
// TODO(ed) invent a nullvariant type
using VariantType = std::variant<bool, std::string, uint64_t, uint32_t>;
using ManagedObjectsType = std::vector<std::pair<
    sdbusplus::message::object_path,
    std::vector<std::pair<std::string,
                          std::vector<std::pair<std::string, VariantType>>>>>>;

using PropertiesType = boost::container::flat_map<std::string, VariantType>;

inline void getIntrusionByService(std::shared_ptr<bmcweb::AsyncResp> aResp,
                                  const std::string& service,
                                  const std::string& objPath)
{
    BMCWEB_LOG_DEBUG << "Get intrusion status by service \n";

    crow::connections::systemBus->async_method_call(
        [aResp{std::move(aResp)}](const boost::system::error_code ec,
                                  const std::variant<std::string>& value) {
            if (ec)
            {
                // do not add err msg in redfish response, because this is not
                //     mandatory property
                BMCWEB_LOG_ERROR << "DBUS response error " << ec << "\n";
                return;
            }

            const std::string* status = std::get_if<std::string>(&value);

            if (status == nullptr)
            {
                BMCWEB_LOG_ERROR << "intrusion status read error \n";
                return;
            }

            aResp->res.jsonValue["PhysicalSecurity"] = {
                {"IntrusionSensorNumber", 1}, {"IntrusionSensor", *status}};
        },
        service, objPath, "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Chassis.Intrusion", "Status");
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
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec,
                    const std::variant<std::string>& property) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error for Location";
                messages::internalError(asyncResp->res);
                return;
            }

            const std::string* value = std::get_if<std::string>(&property);
            if (value == nullptr)
            {
                BMCWEB_LOG_DEBUG << "Null value returned for locaton code";
                messages::internalError(asyncResp->res);
                return;
            }
            asyncResp->res
                .jsonValue["Location"]["PartLocation"]["ServiceLabel"] = *value;
        },
        connectionName, path, "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Inventory.Decorator.LocationCode", "LocationCode");
}

inline void getChassisUUID(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& connectionName,
                           const std::string& path)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec,
                    const std::variant<std::string>& chassisUUID) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error for UUID";
                messages::internalError(asyncResp->res);
                return;
            }
            const std::string* value = std::get_if<std::string>(&chassisUUID);
            if (value == nullptr)
            {
                BMCWEB_LOG_DEBUG << "Null value returned for UUID";
                messages::internalError(asyncResp->res);
                return;
            }
            asyncResp->res.jsonValue["UUID"] = *value;
        },
        connectionName, path, "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Common.UUID", "UUID");
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

                        crow::connections::systemBus->async_method_call(
                            [health](
                                const boost::system::error_code ec2,
                                std::variant<std::vector<std::string>>& resp) {
                                if (ec2)
                                {
                                    return; // no sensors = no failures
                                }
                                std::vector<std::string>* data =
                                    std::get_if<std::vector<std::string>>(
                                        &resp);
                                if (data == nullptr)
                                {
                                    return;
                                }
                                health->inventory = std::move(*data);
                            },
                            "xyz.openbmc_project.ObjectMapper",
                            path + "/all_sensors",
                            "org.freedesktop.DBus.Properties", "Get",
                            "xyz.openbmc_project.Association", "endpoints");

                        health->populate();

                        if (connectionNames.size() < 1)
                        {
                            BMCWEB_LOG_ERROR << "Got 0 Connection names";
                            continue;
                        }

                        asyncResp->res.jsonValue["@odata.type"] =
                            "#Chassis.v1_14_0.Chassis";
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
                            crow::connections::systemBus->async_method_call(
                                [asyncResp, chassisId(std::string(chassisId))](
                                    const boost::system::error_code ec,
                                    const std::variant<std::string>& property) {
                                    if (ec)
                                    {
                                        BMCWEB_LOG_DEBUG
                                            << "DBus response error for AssetTag";
                                        messages::internalError(asyncResp->res);
                                        return;
                                    }

                                    const std::string* assetTag =
                                        std::get_if<std::string>(&property);
                                    if (assetTag == nullptr)
                                    {
                                        BMCWEB_LOG_DEBUG
                                            << "Null value returned for Chassis AssetTag";
                                        messages::internalError(asyncResp->res);
                                        return;
                                    }
                                    asyncResp->res.jsonValue["AssetTag"] =
                                        *assetTag;
                                },
                                connectionName, path,
                                "org.freedesktop.DBus.Properties", "Get",
                                assetTagInterface, "AssetTag");
                        }

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
                                    std::pair<std::string, VariantType>>&
                                    propertiesList) {
                                for (const std::pair<std::string, VariantType>&
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
                                asyncResp->res.jsonValue["Name"] =
                                    dbus::utility::getResourceName(chassisId);
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
                                asyncResp->res.jsonValue["Status"] = {
                                    {"State", "Enabled"},
                                };

                                asyncResp->res
                                    .jsonValue["Links"]["ComputerSystems"] = {
                                    {{"@odata.id",
                                      "/redfish/v1/Systems/system"}}};
                                asyncResp->res.jsonValue["Links"]["ManagedBy"] =
                                    {{{"@odata.id",
                                       "/redfish/v1/Managers/bmc"}}};
                                getChassisState(asyncResp);
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
                        asyncResp->res, "#Chassis.v1_14_0.Chassis", chassisId);
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
                std::variant<std::string>{propertyValue});
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
