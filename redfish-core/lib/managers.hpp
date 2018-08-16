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
                asyncResp->res.result(
                    boost::beast::http::status::internal_server_error);
                asyncResp->res.jsonValue.clear();
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
                        asyncResp->res.result(
                            boost::beast::http::status::internal_server_error);
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
                                asyncResp->res.result(
                                    boost::beast::http::status::
                                        internal_server_error);
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
                                asyncResp->res.result(
                                    boost::beast::http::status::
                                        internal_server_error);
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
                                    asyncResp->res.result(
                                        boost::beast::http::status::
                                            internal_server_error);
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
                                    asyncResp->res.result(
                                        boost::beast::http::status::
                                            internal_server_error);
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
                                    asyncResp->res.result(
                                        boost::beast::http::status::
                                            internal_server_error);
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

class Manager : public Node
{
  public:
    Manager(CrowApp& app) : Node(app, "/redfish/v1/Managers/bmc/")
    {
        Node::json["@odata.id"] = "/redfish/v1/Managers/openbmc";
        Node::json["@odata.type"] = "#Manager.v1_3_0.Manager";
        Node::json["@odata.context"] = "/redfish/v1/$metadata#Manager.Manager";
        Node::json["Id"] = "openbmc";
        Node::json["Name"] = "OpenBmc Manager";
        Node::json["Description"] = "Baseboard Management Controller";
        Node::json["PowerState"] = "On";
        Node::json["UUID"] =
            app.template getMiddleware<crow::persistent_data::Middleware>()
                .systemUuid;
        Node::json["Model"] = "OpenBmc";              // TODO(ed), get model
        Node::json["FirmwareVersion"] = "1234456789"; // TODO(ed), get fwversion
        Node::json["EthernetInterfaces"] = nlohmann::json(
            {{"@odata.id", "/redfish/v1/Managers/openbmc/"
                           "EthernetInterfaces"}}); // TODO(Pawel),
                                                    // remove this
                                                    // when
                                                    // subroutes
                                                    // will work
                                                    // correctly

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
                    asyncResp->res.result(
                        boost::beast::http::status::internal_server_error);
                    return;
                }

                // create map of <connection, path to objMgr>>
                boost::container::flat_map<std::string, std::string>
                    objectMgrPaths;
                for (const auto& pathGroup : subtree)
                {
                    for (const auto& connectionGroup : pathGroup.second)
                    {
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

        res.jsonValue ["DateTime"] = getDateTime();

        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        const dbus::utility::ManagedObjectType& resp) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Error while getting Software Version";
                    asyncResp->res.result(
                        boost::beast::http::status::internal_server_error);
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

    void doPatch(crow::Response& res, const crow::Request& req,
                 const std::vector<std::string>& params) override
    {
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
        Node::json["Members"] = {
            {{"@odata.id", "/redfish/v1/Managers/openbmc"}}};

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
        // Collections don't include the static data added by SubRoute because
        // it has a duplicate entry for members
        res.jsonValue["@odata.id"] = "/redfish/v1/Managers";
        res.jsonValue["@odata.type"] = "#ManagerCollection.ManagerCollection";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#ManagerCollection.ManagerCollection";
        res.jsonValue["Name"] = "Manager Collection";
        res.jsonValue["Members@odata.count"] = 1;
        res.jsonValue["Members"] = {
            {{"@odata.id", "/redfish/v1/Managers/openbmc"}}};
        res.end();
    }
};
} // namespace redfish
