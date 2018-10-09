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

#include "node.hpp"

#include <boost/algorithm/string/replace.hpp>
#include <dbus_utility.hpp>

namespace redfish
{
static constexpr const char* objectManagerIface =
    "org.freedesktop.DBus.ObjectManager";
static constexpr const char* pidConfigurationIface =
    "xyz.openbmc_project.Configuration.Pid";
static constexpr const char* pidZoneConfigurationIface =
    "xyz.openbmc_project.Configuration.Pid.Zone";

static void asyncPopulatePid(const std::string& connection,
                             const std::string& path,
                             std::shared_ptr<AsyncResp> asyncResp)
{

    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec,
                    const dbus::utility::ManagedObjectType& managedObj) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << ec;
                asyncResp->res.jsonValue.clear();
                messages::internalError(asyncResp->res);
                return;
            }
            nlohmann::json& configRoot =
                asyncResp->res.jsonValue["Oem"]["OpenBmc"]["Fan"];
            nlohmann::json& fans = configRoot["FanControllers"];
            fans["@odata.type"] = "#OemManager.FanControllers";
            fans["@odata.context"] =
                "/redfish/v1/$metadata#OemManager.FanControllers";
            fans["@odata.id"] = "/redfish/v1/Managers/bmc#/Oem/OpenBmc/"
                                "Fan/FanControllers";

            nlohmann::json& pids = configRoot["PidControllers"];
            pids["@odata.type"] = "#OemManager.PidControllers";
            pids["@odata.context"] =
                "/redfish/v1/$metadata#OemManager.PidControllers";
            pids["@odata.id"] =
                "/redfish/v1/Managers/bmc#/Oem/OpenBmc/Fan/PidControllers";

            nlohmann::json& zones = configRoot["FanZones"];
            zones["@odata.id"] =
                "/redfish/v1/Managers/bmc#/Oem/OpenBmc/Fan/FanZones";
            zones["@odata.type"] = "#OemManager.FanZones";
            zones["@odata.context"] =
                "/redfish/v1/$metadata#OemManager.FanZones";
            configRoot["@odata.id"] =
                "/redfish/v1/Managers/bmc#/Oem/OpenBmc/Fan";
            configRoot["@odata.type"] = "#OemManager.Fan";
            configRoot["@odata.context"] =
                "/redfish/v1/$metadata#OemManager.Fan";

            bool propertyError = false;
            for (const auto& pathPair : managedObj)
            {
                for (const auto& intfPair : pathPair.second)
                {
                    if (intfPair.first != pidConfigurationIface &&
                        intfPair.first != pidZoneConfigurationIface)
                    {
                        continue;
                    }
                    auto findName = intfPair.second.find("Name");
                    if (findName == intfPair.second.end())
                    {
                        BMCWEB_LOG_ERROR << "Pid Field missing Name";
                        messages::internalError(asyncResp->res, "Name");
                        return;
                    }
                    const std::string* namePtr =
                        mapbox::getPtr<const std::string>(findName->second);
                    if (namePtr == nullptr)
                    {
                        BMCWEB_LOG_ERROR << "Pid Name Field illegal";
                        return;
                    }

                    std::string name = *namePtr;
                    dbus::utility::escapePathForDbus(name);
                    if (intfPair.first == pidZoneConfigurationIface)
                    {
                        std::string chassis;
                        if (!dbus::utility::getNthStringFromPath(
                                pathPair.first.str, 5, chassis))
                        {
                            chassis = "#IllegalValue";
                        }
                        nlohmann::json& zone = zones[name];
                        zone["Chassis"] = {
                            {"@odata.id", "/redfish/v1/Chassis/" + chassis}};
                        zone["@odata.id"] = "/redfish/v1/Managers/bmc#/Oem/"
                                            "OpenBmc/Fan/FanZones/" +
                                            name;
                        zone["@odata.type"] = "#OemManager.FanZone";
                        zone["@odata.context"] =
                            "/redfish/v1/$metadata#OemManager.FanZone";
                    }

                    for (const auto& propertyPair : intfPair.second)
                    {
                        if (propertyPair.first == "Type" ||
                            propertyPair.first == "Class" ||
                            propertyPair.first == "Name")
                        {
                            continue;
                        }

                        // zones
                        if (intfPair.first == pidZoneConfigurationIface)
                        {
                            const double* ptr = mapbox::getPtr<const double>(
                                propertyPair.second);
                            if (ptr == nullptr)
                            {
                                BMCWEB_LOG_ERROR << "Field Illegal "
                                                 << propertyPair.first;
                                messages::internalError(asyncResp->res);
                                return;
                            }
                            zones[name][propertyPair.first] = *ptr;
                        }

                        // pid and fans are off the same configuration
                        if (intfPair.first == pidConfigurationIface)
                        {
                            const std::string* classPtr = nullptr;
                            auto findClass = intfPair.second.find("Class");
                            if (findClass != intfPair.second.end())
                            {
                                classPtr = mapbox::getPtr<const std::string>(
                                    findClass->second);
                            }
                            if (classPtr == nullptr)
                            {
                                BMCWEB_LOG_ERROR << "Pid Class Field illegal";
                                messages::internalError(asyncResp->res,
                                                        "Class");
                                return;
                            }
                            bool isFan = *classPtr == "fan";
                            nlohmann::json& element =
                                isFan ? fans[name] : pids[name];
                            if (isFan)
                            {
                                element["@odata.id"] =
                                    "/redfish/v1/Managers/bmc#/Oem/"
                                    "OpenBmc/Fan/FanControllers/" +
                                    std::string(name);
                                element["@odata.type"] =
                                    "#OemManager.FanController";

                                element["@odata.context"] =
                                    "/redfish/v1/"
                                    "$metadata#OemManager.FanController";
                            }
                            else
                            {
                                element["@odata.id"] =
                                    "/redfish/v1/Managers/bmc#/Oem/"
                                    "OpenBmc/Fan/PidControllers/" +
                                    std::string(name);
                                element["@odata.type"] =
                                    "#OemManager.PidController";
                                element["@odata.context"] =
                                    "/redfish/v1/$metadata"
                                    "#OemManager.PidController";
                            }

                            if (propertyPair.first == "Zones")
                            {
                                const std::vector<std::string>* inputs =
                                    mapbox::getPtr<
                                        const std::vector<std::string>>(
                                        propertyPair.second);

                                if (inputs == nullptr)
                                {
                                    BMCWEB_LOG_ERROR
                                        << "Zones Pid Field Illegal";
                                    messages::internalError(asyncResp->res,
                                                            "Zones");
                                    return;
                                }
                                auto& data = element[propertyPair.first];
                                data = nlohmann::json::array();
                                for (std::string itemCopy : *inputs)
                                {
                                    dbus::utility::escapePathForDbus(itemCopy);
                                    data.push_back(
                                        {{"@odata.id",
                                          "/redfish/v1/Managers/bmc#/Oem/"
                                          "OpenBmc/Fan/FanZones/" +
                                              itemCopy}});
                                }
                            }
                            // todo(james): may never happen, but this
                            // assumes configuration data referenced in the
                            // PID config is provided by the same daemon, we
                            // could add another loop to cover all cases,
                            // but I'm okay kicking this can down the road a
                            // bit

                            else if (propertyPair.first == "Inputs" ||
                                     propertyPair.first == "Outputs")
                            {
                                auto& data = element[propertyPair.first];
                                const std::vector<std::string>* inputs =
                                    mapbox::getPtr<
                                        const std::vector<std::string>>(
                                        propertyPair.second);

                                if (inputs == nullptr)
                                {
                                    BMCWEB_LOG_ERROR << "Field Illegal "
                                                     << propertyPair.first;
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                data = *inputs;
                            } // doubles
                            else if (propertyPair.first ==
                                         "FFGainCoefficient" ||
                                     propertyPair.first == "FFOffCoefficient" ||
                                     propertyPair.first == "ICoefficient" ||
                                     propertyPair.first == "ILimitMax" ||
                                     propertyPair.first == "ILimitMin" ||
                                     propertyPair.first == "OutLimitMax" ||
                                     propertyPair.first == "OutLimitMin" ||
                                     propertyPair.first == "PCoefficient" ||
                                     propertyPair.first == "SlewNeg" ||
                                     propertyPair.first == "SlewPos")
                            {
                                const double* ptr =
                                    mapbox::getPtr<const double>(
                                        propertyPair.second);
                                if (ptr == nullptr)
                                {
                                    BMCWEB_LOG_ERROR << "Field Illegal "
                                                     << propertyPair.first;
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                element[propertyPair.first] = *ptr;
                            }
                        }
                    }
                }
            }
        },
        connection, path, objectManagerIface, "GetManagedObjects");
}

enum class CreatePIDRet
{
    fail,
    del,
    patch
};

static CreatePIDRet createPidInterface(
    const std::shared_ptr<AsyncResp>& response, const std::string& type,
    const nlohmann::json& record, const std::string& path,
    const dbus::utility::ManagedObjectType& managedObj, bool createNewObject,
    boost::container::flat_map<std::string, dbus::utility::DbusVariantType>&
        output,
    std::string& chassis)
{

    if (type == "PidControllers" || type == "FanControllers")
    {
        if (createNewObject)
        {
            output["Class"] = type == "PidControllers" ? std::string("temp")
                                                       : std::string("fan");
            output["Type"] = std::string("Pid");
        }
        else if (record == nullptr)
        {
            // delete interface
            crow::connections::systemBus->async_method_call(
                [response,
                 path{std::string(path)}](const boost::system::error_code ec) {
                    if (ec)
                    {
                        BMCWEB_LOG_ERROR << "Error patching " << path << ": "
                                         << ec;
                        messages::internalError(response->res);
                    }
                },
                "xyz.openbmc_project.EntityManager", path,
                pidConfigurationIface, "Delete");
            return CreatePIDRet::del;
        }

        for (auto& field : record.items())
        {
            if (field.key() == "Zones")
            {
                if (!field.value().is_array())
                {
                    BMCWEB_LOG_ERROR << "Illegal Type " << field.key();
                    messages::propertyValueFormatError(
                        response->res, field.value(), field.key());
                    return CreatePIDRet::fail;
                }
                std::vector<std::string> inputs;
                for (const auto& odata : field.value().items())
                {
                    for (const auto& value : odata.value().items())
                    {
                        const std::string* path =
                            value.value().get_ptr<const std::string*>();
                        if (path == nullptr)
                        {
                            BMCWEB_LOG_ERROR << "Illegal Type " << field.key();
                            messages::propertyValueFormatError(
                                response->res, field.value().dump(),
                                field.key());
                            return CreatePIDRet::fail;
                        }
                        std::string input;
                        if (!dbus::utility::getNthStringFromPath(*path, 4,
                                                                 input))
                        {
                            BMCWEB_LOG_ERROR << "Got invalid path " << *path;
                            messages::propertyValueFormatError(
                                response->res, field.value().dump(),
                                field.key());
                            return CreatePIDRet::fail;
                        }
                        boost::replace_all(input, "_", " ");
                        inputs.emplace_back(std::move(input));
                    }
                }
                output["Zones"] = std::move(inputs);
            }
            else if (field.key() == "Inputs" || field.key() == "Outputs")
            {
                if (!field.value().is_array())
                {
                    BMCWEB_LOG_ERROR << "Illegal Type " << field.key();
                    messages::propertyValueFormatError(
                        response->res, field.value().dump(), field.key());
                    return CreatePIDRet::fail;
                }
                std::vector<std::string> inputs;
                for (const auto& value : field.value().items())
                {
                    const std::string* sensor =
                        value.value().get_ptr<const std::string*>();

                    if (sensor == nullptr)
                    {
                        BMCWEB_LOG_ERROR << "Illegal Type "
                                         << field.value().dump();
                        messages::propertyValueFormatError(
                            response->res, field.value().dump(), field.key());
                        return CreatePIDRet::fail;
                    }

                    std::string input =
                        boost::replace_all_copy(*sensor, "_", " ");
                    inputs.push_back(std::move(input));
                    // try to find the sensor in the
                    // configuration
                    if (chassis.empty())
                    {
                        std::find_if(
                            managedObj.begin(), managedObj.end(),
                            [&chassis, sensor](const auto& obj) {
                                if (boost::algorithm::ends_with(obj.first.str,
                                                                *sensor))
                                {
                                    return dbus::utility::getNthStringFromPath(
                                        obj.first.str, 5, chassis);
                                }
                                return false;
                            });
                    }
                }
                output[field.key()] = inputs;
            }

            // doubles
            else if (field.key() == "FFGainCoefficient" ||
                     field.key() == "FFOffCoefficient" ||
                     field.key() == "ICoefficient" ||
                     field.key() == "ILimitMax" || field.key() == "ILimitMin" ||
                     field.key() == "OutLimitMax" ||
                     field.key() == "OutLimitMin" ||
                     field.key() == "PCoefficient" ||
                     field.key() == "SetPoint" || field.key() == "SlewNeg" ||
                     field.key() == "SlewPos")
            {
                const double* ptr = field.value().get_ptr<const double*>();
                if (ptr == nullptr)
                {
                    BMCWEB_LOG_ERROR << "Illegal Type " << field.key();
                    messages::propertyValueFormatError(
                        response->res, field.value().dump(), field.key());
                    return CreatePIDRet::fail;
                }
                output[field.key()] = *ptr;
            }

            else
            {
                BMCWEB_LOG_ERROR << "Illegal Type " << field.key();
                messages::propertyUnknown(response->res, field.key());
                return CreatePIDRet::fail;
            }
        }
    }
    else if (type == "FanZones")
    {
        if (!createNewObject && record == nullptr)
        {
            // delete interface
            crow::connections::systemBus->async_method_call(
                [response,
                 path{std::string(path)}](const boost::system::error_code ec) {
                    if (ec)
                    {
                        BMCWEB_LOG_ERROR << "Error patching " << path << ": "
                                         << ec;
                        messages::internalError(response->res);
                    }
                },
                "xyz.openbmc_project.EntityManager", path,
                pidZoneConfigurationIface, "Delete");
            return CreatePIDRet::del;
        }
        output["Type"] = std::string("Pid.Zone");

        for (auto& field : record.items())
        {
            if (field.key() == "Chassis")
            {
                const std::string* chassisId = nullptr;
                for (const auto& id : field.value().items())
                {
                    if (id.key() != "@odata.id")
                    {
                        BMCWEB_LOG_ERROR << "Illegal Type " << id.key();
                        messages::propertyUnknown(response->res, field.key());
                        return CreatePIDRet::fail;
                    }
                    chassisId = id.value().get_ptr<const std::string*>();
                    if (chassisId == nullptr)
                    {
                        messages::createFailedMissingReqProperties(
                            response->res, field.key());
                        return CreatePIDRet::fail;
                    }
                }

                // /refish/v1/chassis/chassis_name/
                if (!dbus::utility::getNthStringFromPath(*chassisId, 3,
                                                         chassis))
                {
                    BMCWEB_LOG_ERROR << "Got invalid path " << *chassisId;
                    messages::invalidObject(response->res, *chassisId);
                    return CreatePIDRet::fail;
                }
            }
            else if (field.key() == "FailSafePercent" ||
                     field.key() == "MinThermalRpm")
            {
                const double* ptr = field.value().get_ptr<const double*>();
                if (ptr == nullptr)
                {
                    BMCWEB_LOG_ERROR << "Illegal Type " << field.key();
                    messages::propertyValueFormatError(
                        response->res, field.value().dump(), field.key());
                    return CreatePIDRet::fail;
                }
                output[field.key()] = *ptr;
            }
            else
            {
                BMCWEB_LOG_ERROR << "Illegal Type " << field.key();
                messages::propertyUnknown(response->res, field.key());
                return CreatePIDRet::fail;
            }
        }
    }
    else
    {
        BMCWEB_LOG_ERROR << "Illegal Type " << type;
        messages::propertyUnknown(response->res, type);
        return CreatePIDRet::fail;
    }
    return CreatePIDRet::patch;
}

class Manager : public Node
{
  public:
    Manager(CrowApp& app) : Node(app, "/redfish/v1/Managers/bmc/")
    {
        Node::json["@odata.id"] = "/redfish/v1/Managers/bmc";
        Node::json["@odata.type"] = "#Manager.v1_3_0.Manager";
        Node::json["@odata.context"] = "/redfish/v1/$metadata#Manager.Manager";
        Node::json["Id"] = "bmc";
        Node::json["Name"] = "OpenBmc Manager";
        Node::json["Description"] = "Baseboard Management Controller";
        Node::json["PowerState"] = "On";
        Node::json["ManagerType"] = "BMC";
        Node::json["UUID"] =
            app.template getMiddleware<crow::persistent_data::Middleware>()
                .systemUuid;
        Node::json["Model"] = "OpenBmc"; // TODO(ed), get model
        Node::json["EthernetInterfaces"] = {
            {"@odata.id", "/redfish/v1/Managers/bmc/EthernetInterfaces"}};

        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};

        // default oem data
        nlohmann::json& oem = Node::json["Oem"];
        nlohmann::json& oemOpenbmc = oem["OpenBmc"];
        oem["@odata.type"] = "#OemManager.Oem";
        oem["@odata.id"] = "/redfish/v1/Managers/bmc#/Oem";
        oem["@odata.context"] = "/redfish/v1/$metadata#OemManager.Oem";
        oemOpenbmc["@odata.type"] = "#OemManager.OpenBmc";
        oemOpenbmc["@odata.id"] = "/redfish/v1/Managers/bmc#/Oem/OpenBmc";
        oemOpenbmc["@odata.context"] =
            "/redfish/v1/$metadata#OemManager.OpenBmc";
    }

  private:
    void getPidValues(std::shared_ptr<AsyncResp> asyncResp)
    {
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        const crow::openbmc_mapper::GetSubTreeType& subtree) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }

                // create map of <connection, path to objMgr>>
                boost::container::flat_map<std::string, std::string>
                    objectMgrPaths;
                boost::container::flat_set<std::string> calledConnections;
                for (const auto& pathGroup : subtree)
                {
                    for (const auto& connectionGroup : pathGroup.second)
                    {
                        auto findConnection =
                            calledConnections.find(connectionGroup.first);
                        if (findConnection != calledConnections.end())
                        {
                            break;
                        }
                        for (const std::string& interface :
                             connectionGroup.second)
                        {
                            if (interface == objectManagerIface)
                            {
                                objectMgrPaths[connectionGroup.first] =
                                    pathGroup.first;
                            }
                            // this list is alphabetical, so we
                            // should have found the objMgr by now
                            if (interface == pidConfigurationIface ||
                                interface == pidZoneConfigurationIface)
                            {
                                auto findObjMgr =
                                    objectMgrPaths.find(connectionGroup.first);
                                if (findObjMgr == objectMgrPaths.end())
                                {
                                    BMCWEB_LOG_DEBUG << connectionGroup.first
                                                     << "Has no Object Manager";
                                    continue;
                                }

                                calledConnections.insert(connectionGroup.first);

                                asyncPopulatePid(findObjMgr->first,
                                                 findObjMgr->second, asyncResp);
                                break;
                            }
                        }
                    }
                }
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTree", "/", 0,
            std::array<const char*, 3>{pidConfigurationIface,
                                       pidZoneConfigurationIface,
                                       objectManagerIface});
    }

    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        asyncResp->res.jsonValue = Node::json;

        Node::json["DateTime"] = getDateTime();
        res.jsonValue = Node::json;

        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        const dbus::utility::ManagedObjectType& resp) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Error while getting Software Version";
                    messages::internalError(asyncResp->res);
                    return;
                }

                for (auto& objpath : resp)
                {
                    for (auto& interface : objpath.second)
                    {
                        // If interface is xyz.openbmc_project.Software.Version,
                        // this is what we're looking for.
                        if (interface.first ==
                            "xyz.openbmc_project.Software.Version")
                        {
                            // Cut out everyting until last "/", ...
                            const std::string& iface_id = objpath.first;
                            for (auto& property : interface.second)
                            {
                                if (property.first == "Version")
                                {
                                    const std::string* value =
                                        mapbox::getPtr<const std::string>(
                                            property.second);
                                    if (value == nullptr)
                                    {
                                        continue;
                                    }
                                    asyncResp->res
                                        .jsonValue["FirmwareVersion"] = *value;
                                }
                            }
                        }
                    }
                }
            },
            "xyz.openbmc_project.Software.BMC.Updater",
            "/xyz/openbmc_project/software",
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
        getPidValues(asyncResp);
    }
    void setPidValues(std::shared_ptr<AsyncResp> response,
                      const nlohmann::json& data)
    {
        // todo(james): might make sense to do a mapper call here if this
        // interface gets more traction
        crow::connections::systemBus->async_method_call(
            [response,
             data](const boost::system::error_code ec,
                   const dbus::utility::ManagedObjectType& managedObj) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Error communicating to Entity Manager";
                    messages::internalError(response->res);
                    return;
                }
                for (const auto& type : data.items())
                {
                    if (!type.value().is_object())
                    {
                        BMCWEB_LOG_ERROR << "Illegal Type " << type.key();
                        messages::propertyValueFormatError(
                            response->res, type.value(), type.key());
                        return;
                    }
                    for (const auto& record : type.value().items())
                    {
                        const std::string& name = record.key();
                        auto pathItr =
                            std::find_if(managedObj.begin(), managedObj.end(),
                                         [&name](const auto& obj) {
                                             return boost::algorithm::ends_with(
                                                 obj.first.str, name);
                                         });
                        boost::container::flat_map<
                            std::string, dbus::utility::DbusVariantType>
                            output;

                        output.reserve(16); // The pid interface length

                        // determines if we're patching entity-manager or
                        // creating a new object
                        bool createNewObject = (pathItr == managedObj.end());
                        if (type.key() == "PidControllers" ||
                            type.key() == "FanControllers")
                        {
                            if (!createNewObject &&
                                pathItr->second.find(pidConfigurationIface) ==
                                    pathItr->second.end())
                            {
                                createNewObject = true;
                            }
                        }
                        else if (!createNewObject &&
                                 pathItr->second.find(
                                     pidZoneConfigurationIface) ==
                                     pathItr->second.end())
                        {
                            createNewObject = true;
                        }
                        output["Name"] =
                            boost::replace_all_copy(name, "_", " ");

                        std::string chassis;
                        CreatePIDRet ret = createPidInterface(
                            response, type.key(), record.value(),
                            pathItr->first.str, managedObj, createNewObject,
                            output, chassis);
                        if (ret == CreatePIDRet::fail)
                        {
                            return;
                        }
                        else if (ret == CreatePIDRet::del)
                        {
                            continue;
                        }

                        if (!createNewObject)
                        {
                            for (const auto& property : output)
                            {
                                const char* iface =
                                    type.key() == "FanZones"
                                        ? pidZoneConfigurationIface
                                        : pidConfigurationIface;
                                crow::connections::systemBus->async_method_call(
                                    [response,
                                     propertyName{std::string(property.first)}](
                                        const boost::system::error_code ec) {
                                        if (ec)
                                        {
                                            BMCWEB_LOG_ERROR
                                                << "Error patching "
                                                << propertyName << ": " << ec;
                                            messages::internalError(
                                                response->res);
                                        }
                                    },
                                    "xyz.openbmc_project.EntityManager",
                                    pathItr->first.str,
                                    "org.freedesktop.DBus.Properties", "Set",
                                    std::string(iface), property.first,
                                    property.second);
                            }
                        }
                        else
                        {
                            if (chassis.empty())
                            {
                                BMCWEB_LOG_ERROR
                                    << "Failed to get chassis from config";
                                messages::invalidObject(response->res, name);
                                return;
                            }

                            bool foundChassis = false;
                            for (const auto& obj : managedObj)
                            {
                                if (boost::algorithm::ends_with(obj.first.str,
                                                                chassis))
                                {
                                    chassis = obj.first.str;
                                    foundChassis = true;
                                    break;
                                }
                            }
                            if (!foundChassis)
                            {
                                BMCWEB_LOG_ERROR
                                    << "Failed to find chassis on dbus";
                                messages::resourceMissingAtURI(
                                    response->res,
                                    "/redfish/v1/Chassis/" + chassis);
                                return;
                            }

                            crow::connections::systemBus->async_method_call(
                                [response](const boost::system::error_code ec) {
                                    if (ec)
                                    {
                                        BMCWEB_LOG_ERROR
                                            << "Error Adding Pid Object " << ec;
                                        messages::internalError(response->res);
                                    }
                                },
                                "xyz.openbmc_project.EntityManager", chassis,
                                "xyz.openbmc_project.AddObject", "AddObject",
                                output);
                        }
                    }
                }
            },
            "xyz.openbmc_project.EntityManager", "/", objectManagerIface,
            "GetManagedObjects");
    }

    void doPatch(crow::Response& res, const crow::Request& req,
                 const std::vector<std::string>& params) override
    {
        nlohmann::json patch;
        if (!json_util::processJsonFromRequest(res, req, patch))
        {
            return;
        }
        std::shared_ptr<AsyncResp> response = std::make_shared<AsyncResp>(res);
        for (const auto& topLevel : patch.items())
        {
            if (topLevel.key() == "Oem")
            {
                if (!topLevel.value().is_object())
                {
                    BMCWEB_LOG_ERROR << "Bad Patch " << topLevel.key();
                    messages::propertyValueFormatError(
                        response->res, topLevel.key(), "OemManager.Oem");
                    return;
                }
            }
            else
            {
                BMCWEB_LOG_ERROR << "Bad Patch " << topLevel.key();
                messages::propertyUnknown(response->res, topLevel.key());
                return;
            }
            for (const auto& oemLevel : topLevel.value().items())
            {
                if (oemLevel.key() == "OpenBmc")
                {
                    if (!oemLevel.value().is_object())
                    {
                        BMCWEB_LOG_ERROR << "Bad Patch " << oemLevel.key();
                        messages::propertyValueFormatError(
                            response->res, topLevel.key(),
                            "OemManager.OpenBmc");
                        return;
                    }
                    for (const auto& typeLevel : oemLevel.value().items())
                    {

                        if (typeLevel.key() == "Fan")
                        {
                            if (!typeLevel.value().is_object())
                            {
                                BMCWEB_LOG_ERROR << "Bad Patch "
                                                 << typeLevel.key();
                                messages::propertyValueFormatError(
                                    response->res, typeLevel.value().dump(),
                                    typeLevel.key());
                                return;
                            }
                            setPidValues(response,
                                         std::move(typeLevel.value()));
                        }
                        else
                        {
                            BMCWEB_LOG_ERROR << "Bad Patch " << typeLevel.key();
                            messages::propertyUnknown(response->res,
                                                      typeLevel.key());
                            return;
                        }
                    }
                }
                else
                {
                    BMCWEB_LOG_ERROR << "Bad Patch " << oemLevel.key();
                    messages::propertyUnknown(response->res, oemLevel.key());
                    return;
                }
            }
        }
    }

    std::string getDateTime() const
    {
        std::array<char, 128> dateTime;
        std::string redfishDateTime("0000-00-00T00:00:00Z00:00");
        std::time_t time = std::time(nullptr);

        if (std::strftime(dateTime.begin(), dateTime.size(), "%FT%T%z",
                          std::localtime(&time)))
        {
            // insert the colon required by the ISO 8601 standard
            redfishDateTime = std::string(dateTime.data());
            redfishDateTime.insert(redfishDateTime.end() - 2, ':');
        }

        return redfishDateTime;
    }
};

class ManagerCollection : public Node
{
  public:
    ManagerCollection(CrowApp& app) : Node(app, "/redfish/v1/Managers/")
    {
        Node::json["@odata.id"] = "/redfish/v1/Managers";
        Node::json["@odata.type"] = "#ManagerCollection.ManagerCollection";
        Node::json["@odata.context"] =
            "/redfish/v1/$metadata#ManagerCollection.ManagerCollection";
        Node::json["Name"] = "Manager Collection";
        Node::json["Members@odata.count"] = 1;
        Node::json["Members"] = {{{"@odata.id", "/redfish/v1/Managers/bmc"}}};

        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        // Collections don't include the static data added by SubRoute
        // because it has a duplicate entry for members
        res.jsonValue["@odata.id"] = "/redfish/v1/Managers";
        res.jsonValue["@odata.type"] = "#ManagerCollection.ManagerCollection";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#ManagerCollection.ManagerCollection";
        res.jsonValue["Name"] = "Manager Collection";
        res.jsonValue["Members@odata.count"] = 1;
        res.jsonValue["Members"] = {
            {{"@odata.id", "/redfish/v1/Managers/bmc"}}};
        res.end();
    }
};
} // namespace redfish
