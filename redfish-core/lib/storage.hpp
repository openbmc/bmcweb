/*
// Copyright (c) 2019 Intel Corporation
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

#include <node.hpp>

namespace redfish
{
class StorageCollection : public Node
{
  public:
    StorageCollection(CrowApp &app) :
        Node(app, "/redfish/v1/Systems/system/Storage/")
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
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        res.jsonValue["@odata.type"] = "#StorageCollection.StorageCollection";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#StorageCollection.StorageCollection";
        res.jsonValue["@odata.id"] = "/redfish/v1/Systems/system/Storage";
        res.jsonValue["Name"] = "Storage Collection";
        res.jsonValue["Members"] = {
            {"@odata.id", "/redfish/v1/Systems/system/Storage/1"}};
        res.jsonValue["Members@odata.count"] = 1;
        res.end();
    }
};

class Storage : public Node
{
  public:
    Storage(CrowApp &app) : Node(app, "/redfish/v1/Systems/system/Storage/1")
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
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        res.jsonValue["@odata.type"] = "#Storage.v1_2_0.Storage";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#Storage.Storage";
        res.jsonValue["@odata.id"] = "/redfish/v1/Systems/system/Storage/1";
        res.jsonValue["Name"] = "Storage Controller";
        res.jsonValue["Id"] = "Storage Controller";

        auto asyncResp = std::make_shared<AsyncResp>(res);
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        const std::vector<std::string> &storageList) {
                nlohmann::json &storageArray =
                    asyncResp->res.jsonValue["Drives"];
                storageArray = nlohmann::json::array();
                asyncResp->res.jsonValue["Drives@odata.count"] = 0;
                if (ec)
                {
                    return;
                }
                for (const std::string &objpath : storageList)
                {
                    std::size_t lastPos = objpath.rfind("/");
                    if (lastPos == std::string::npos ||
                        objpath.size() <= lastPos + 1)
                    {
                        BMCWEB_LOG_ERROR << "Failed to find '/' in " << objpath;
                        continue;
                    }

                    storageArray.push_back(
                        {{"@odata.id",
                          "/redfish/v1/Systems/system/Storage/1/Drive/" +
                              objpath.substr(lastPos + 1)}});
                }

                asyncResp->res.jsonValue["Drives@odata.count"] =
                    storageArray.size();
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
            "/xyz/openbmc_project/inventory", int32_t(0),
            std::array<const char *, 1>{
                "xyz.openbmc_project.Inventory.Item.Drive"});
    }
};

class Drive : public Node
{
  public:
    Drive(CrowApp &app) :
        Node(app, "/redfish/v1/Systems/system/Storage/1/Drive/<str>/",
             std::string())
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
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        const std::string &driveId = params[0];

        auto asyncResp = std::make_shared<AsyncResp>(res);

        crow::connections::systemBus->async_method_call(
            [asyncResp, driveId](
                const boost::system::error_code ec,
                const std::vector<std::pair<
                    std::string, std::vector<std::pair<
                                     std::string, std::vector<std::string>>>>>
                    &subtree) {
                if (ec)
                {
                    messages::resourceNotFound(asyncResp->res, "Drive",
                                               driveId);
                    return;
                }

                for (const std::pair<
                         std::string,
                         std::vector<
                             std::pair<std::string, std::vector<std::string>>>>
                         &object : subtree)
                {
                    const std::string &path = object.first;
                    const std::vector<
                        std::pair<std::string, std::vector<std::string>>>
                        &connectionNames = object.second;

                    if (!boost::ends_with(path, "/" + driveId))
                    {
                        continue;
                    }

                    asyncResp->res.jsonValue["@odata.type"] =
                        "#Drive.v1_2_0.Drive";
                    asyncResp->res.jsonValue["@odata.context"] =
                        "/redfish/v1/$metadata#Drive.Drive";
                    asyncResp->res.jsonValue["@odata.id"] =
                        "/redfish/v1/Systems/system/Storage/1/Drive/" + driveId;

                    if (connectionNames.size() != 1)
                    {
                        BMCWEB_LOG_ERROR << "Connection size "
                                         << connectionNames.size()
                                         << ", greater than 1";
                        continue;
                    }

                    const std::string &connectionName =
                        connectionNames[0].first;
                    crow::connections::systemBus->async_method_call(
                        [asyncResp, driveId](
                            const boost::system::error_code ec,
                            const std::vector<std::pair<
                                std::string,
                                std::variant<bool, std::string, uint64_t>>>
                                &propertiesList) {
                            for (const std::pair<
                                     std::string,
                                     std::variant<bool, std::string, uint64_t>>
                                     &property : propertiesList)
                            {
                                // Store DBus properties that are also
                                // Redfish properties with same name and a
                                // string value
                                const std::string &propertyName =
                                    property.first;
                                if ((propertyName == "PartNumber") ||
                                    (propertyName == "SerialNumber") ||
                                    (propertyName == "Manufacturer") ||
                                    (propertyName == "Model"))
                                {
                                    const std::string *value =
                                        std::get_if<std::string>(
                                            &property.second);
                                    if (value != nullptr)
                                    {
                                        asyncResp->res.jsonValue[propertyName] =
                                            *value;
                                    }
                                }
                            }
                            asyncResp->res.jsonValue["Name"] = driveId;
                            asyncResp->res.jsonValue["Id"] = driveId;

                            getMainChassisId(
                                asyncResp, [](const std::string &chassisId,
                                              std::shared_ptr<AsyncResp> aRsp) {
                                    aRsp->res.jsonValue["Links"]["Chassis"] = {
                                        {"@odata.id",
                                         "/redfish/v1/Chassis/" + chassisId}};
                                });
                        },
                        connectionName, path, "org.freedesktop.DBus.Properties",
                        "GetAll",
                        "xyz.openbmc_project.Inventory.Decorator.Asset");

                    return;
                }

                // no object found with that name
                messages::resourceNotFound(asyncResp->res, "Drive", driveId);
                return;
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTree",
            "/xyz/openbmc_project/inventory", int32_t(0),
            std::array<const char *, 1>{
                "xyz.openbmc_project.Inventory.Item.Drive"});
    }
};
} // namespace redfish
