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

namespace redfish
{

/**
 * DBus types primitives for several generic DBus interfaces
 * TODO consider move this to separate file into boost::dbus
 */
using GetManagedObjectsType = boost::container::flat_map<
    sdbusplus::message::object_path,
    boost::container::flat_map<
        std::string,
        boost::container::flat_map<
            std::string, sdbusplus::message::variant<
                             std::string, bool, uint8_t, int16_t, uint16_t,
                             int32_t, uint32_t, int64_t, uint64_t, double>>>>;

class Manager : public Node
{
  public:
    Manager(CrowApp &app) : Node(app, "/redfish/v1/Managers/openbmc/")
    {
        Node::json["@odata.id"] = "/redfish/v1/Managers/openbmc";
        Node::json["@odata.type"] = "#Manager.v1_3_0.Manager";
        Node::json["@odata.context"] = "/redfish/v1/$metadata#Manager.Manager";
        Node::json["Id"] = "openbmc";
        Node::json["Name"] = "OpenBmc Manager";
        Node::json["Description"] = "Baseboard Management Controller";
        Node::json["PowerState"] = "On";
        Node::json["ManagerType"] = "BMC";
        Node::json["UUID"] =
            app.template getMiddleware<crow::persistent_data::Middleware>()
                .systemUuid;
        Node::json["Model"] = "OpenBmc"; // TODO(ed), get model
        Node::json["EthernetInterfaces"] = {
            {"@odata.id", "/redfish/v1/Managers/openbmc/EthernetInterfaces"}};

        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        asyncResp->res.jsonValue = Node::json;

        Node::json["DateTime"] = getDateTime();
        res.jsonValue = Node::json;
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        const GetManagedObjectsType &resp) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Error while getting Software Version";
                    asyncResp->res.result(
                        boost::beast::http::status::internal_server_error);
                    return;
                }

                for (auto &objpath : resp)
                {
                    for (auto &interface : objpath.second)
                    {
                        // If interface is xyz.openbmc_project.Software.Version,
                        // this is what we're looking for.
                        if (interface.first ==
                            "xyz.openbmc_project.Software.Version")
                        {
                            // Cut out everyting until last "/", ...
                            const std::string &iface_id = objpath.first;
                            for (auto &property : interface.second)
                            {
                                if (property.first == "Version")
                                {
                                    const std::string *value =
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
    ManagerCollection(CrowApp &app) : Node(app, "/redfish/v1/Managers/")
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
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
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
