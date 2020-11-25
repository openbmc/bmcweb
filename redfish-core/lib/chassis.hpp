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
#include "node.hpp"

#include <boost/container/flat_map.hpp>
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
inline void getChassisState(std::shared_ptr<AsyncResp> aResp)
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

inline void getIntrusionByService(std::shared_ptr<AsyncResp> aResp,
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
inline void getPhysicalSecurityData(std::shared_ptr<AsyncResp> aResp)
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
                BMCWEB_LOG_ERROR << "DBUS error: no matched iface " << ec
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
 */
class ChassisCollection : public Node
{
  public:
    ChassisCollection(App& app) : Node(app, "/redfish/v1/Chassis/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }

  private:
    /**
     * Functions triggers appropriate requests on DBus
     */
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>&) override
    {
        res.jsonValue["@odata.type"] = "#ChassisCollection.ChassisCollection";
        res.jsonValue["@odata.id"] = "/redfish/v1/Chassis";
        res.jsonValue["Name"] = "Chassis Collection";

        auto asyncResp = std::make_shared<AsyncResp>(res);

        collection_util::getCollectionMembers(
            asyncResp, "/redfish/v1/Chassis",
            {"xyz.openbmc_project.Inventory.Item.Board",
             "xyz.openbmc_project.Inventory.Item.Chassis"});
    }
};

/**
 * Chassis override class for delivering Chassis Schema
 */
class Chassis : public Node
{
  public:
    Chassis(App& app) : Node(app, "/redfish/v1/Chassis/<str>/", std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }

  private:
    /**
     * Functions triggers appropriate requests on DBus
     */
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>& params) override
    {
        const std::array<const char*, 2> interfaces = {
            "xyz.openbmc_project.Inventory.Item.Board",
            "xyz.openbmc_project.Inventory.Item.Chassis"};

        // Check if there is required param, truly entering this shall be
        // impossible.
        if (params.size() != 1)
        {
            messages::internalError(res);
            res.end();
            return;
        }
        const std::string& chassisId = params[0];

        auto asyncResp = std::make_shared<AsyncResp>(res);
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
                         std::vector<
                             std::pair<std::string, std::vector<std::string>>>>&
                         object : subtree)
                {
                    const std::string& path = object.first;
                    const std::vector<
                        std::pair<std::string, std::vector<std::string>>>&
                        connectionNames = object.second;

                    if (!boost::ends_with(path, chassisId))
                    {
                        continue;
                    }

                    auto health = std::make_shared<HealthPopulate>(asyncResp);

                    crow::connections::systemBus->async_method_call(
                        [health](const boost::system::error_code ec2,
                                 std::variant<std::vector<std::string>>& resp) {
                            if (ec2)
                            {
                                return; // no sensors = no failures
                            }
                            std::vector<std::string>* data =
                                std::get_if<std::vector<std::string>>(&resp);
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
                    asyncResp->res.jsonValue["Actions"]["#Chassis.Reset"] = {
                        {"target", "/redfish/v1/Chassis/" + chassisId +
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

                    crow::connections::systemBus->async_method_call(
                        [asyncResp, chassisId(std::string(chassisId))](
                            const boost::system::error_code /*ec2*/,
                            const std::vector<std::pair<
                                std::string, VariantType>>& propertiesList) {
                            for (const std::pair<std::string, VariantType>&
                                     property : propertiesList)
                            {
                                // Store DBus properties that are also Redfish
                                // properties with same name and a string value
                                const std::string& propertyName =
                                    property.first;
                                if ((propertyName == "PartNumber") ||
                                    (propertyName == "SerialNumber") ||
                                    (propertyName == "Manufacturer") ||
                                    (propertyName == "Model"))
                                {
                                    const std::string* value =
                                        std::get_if<std::string>(
                                            &property.second);
                                    if (value != nullptr)
                                    {
                                        asyncResp->res.jsonValue[propertyName] =
                                            *value;
                                    }
                                }
                            }
                            asyncResp->res.jsonValue["Name"] = chassisId;
                            asyncResp->res.jsonValue["Id"] = chassisId;
                            asyncResp->res.jsonValue["Thermal"] = {
                                {"@odata.id", "/redfish/v1/Chassis/" +
                                                  chassisId + "/Thermal"}};
                            // Power object
                            asyncResp->res.jsonValue["Power"] = {
                                {"@odata.id", "/redfish/v1/Chassis/" +
                                                  chassisId + "/Power"}};
                            // SensorCollection
                            asyncResp->res.jsonValue["Sensors"] = {
                                {"@odata.id", "/redfish/v1/Chassis/" +
                                                  chassisId + "/Sensors"}};
                            asyncResp->res.jsonValue["Status"] = {
                                {"State", "Enabled"},
                            };

                            asyncResp->res
                                .jsonValue["Links"]["ComputerSystems"] = {
                                {{"@odata.id", "/redfish/v1/Systems/system"}}};
                            asyncResp->res.jsonValue["Links"]["ManagedBy"] = {
                                {{"@odata.id", "/redfish/v1/Managers/bmc"}}};
                            getChassisState(asyncResp);
                        },
                        connectionName, path, "org.freedesktop.DBus.Properties",
                        "GetAll",
                        "xyz.openbmc_project.Inventory.Decorator.Asset");
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
    }

    void doPatch(crow::Response& res, const crow::Request& req,
                 const std::vector<std::string>& params) override
    {
        std::optional<bool> locationIndicatorActive;
        std::optional<std::string> indicatorLed;
        auto asyncResp = std::make_shared<AsyncResp>(res);

        if (params.size() != 1)
        {
            return;
        }

        if (!json_util::readJson(req, res, "LocationIndicatorActive",
                                 locationIndicatorActive, "IndicatorLED",
                                 indicatorLed))
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
            res.addHeader(boost::beast::http::field::warning,
                          "299 - \"IndicatorLED is deprecated. Use "
                          "LocationIndicatorActive instead.\"");
        }

        const std::array<const char*, 2> interfaces = {
            "xyz.openbmc_project.Inventory.Item.Board",
            "xyz.openbmc_project.Inventory.Item.Chassis"};

        const std::string& chassisId = params[0];

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
                         std::vector<
                             std::pair<std::string, std::vector<std::string>>>>&
                         object : subtree)
                {
                    const std::string& path = object.first;
                    const std::vector<
                        std::pair<std::string, std::vector<std::string>>>&
                        connectionNames = object.second;

                    if (!boost::ends_with(path, chassisId))
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
                        "xyz.openbmc_project.Inventory.Item.Board."
                        "Motherboard"};
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
    }
};

inline void doChassisPowerCycle(const std::shared_ptr<AsyncResp>& asyncResp)
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
 */
class ChassisResetAction : public Node
{
  public:
    ChassisResetAction(App& app) :
        Node(app, "/redfish/v1/Chassis/<str>/Actions/Chassis.Reset/",
             std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }

  private:
    /**
     * Function handles POST method request.
     * Analyzes POST body before sending Reset request data to D-Bus.
     */
    void doPost(crow::Response& res, const crow::Request& req,
                const std::vector<std::string>&) override
    {
        BMCWEB_LOG_DEBUG << "Post Chassis Reset.";

        std::string resetType;
        auto asyncResp = std::make_shared<AsyncResp>(res);

        if (!json_util::readJson(req, asyncResp->res, "ResetType", resetType))
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
};

/**
 * ChassisResetActionInfo derived class for delivering Chassis
 * ResetType AllowableValues using ResetInfo schema.
 */
class ChassisResetActionInfo : public Node
{
  public:
    /*
     * Default Constructor
     */
    ChassisResetActionInfo(App& app) :
        Node(app, "/redfish/v1/Chassis/<str>/ResetActionInfo/", std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }

  private:
    /**
     * Functions triggers appropriate requests on DBus
     */
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>& params) override
    {
        if (params.size() != 1)
        {
            messages::internalError(res);
            res.end();
            return;
        }
        const std::string& chassisId = params[0];

        res.jsonValue = {{"@odata.type", "#ActionInfo.v1_1_2.ActionInfo"},
                         {"@odata.id", "/redfish/v1/Chassis/" + chassisId +
                                           "/ResetActionInfo"},
                         {"Name", "Reset Action Info"},
                         {"Id", "ResetActionInfo"},
                         {"Parameters",
                          {{{"Name", "ResetType"},
                            {"Required", true},
                            {"DataType", "String"},
                            {"AllowableValues", {"PowerCycle"}}}}}};
        res.end();
    }
};

} // namespace redfish
