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

#include "health.hpp"
#include "openbmc_dbus_rest.hpp"

#include <node.hpp>

namespace redfish
{
class StorageCollection : public Node
{
  public:
    StorageCollection(App& app) :
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
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>&) override
    {
        res.jsonValue["@odata.type"] = "#StorageCollection.StorageCollection";
        res.jsonValue["@odata.id"] = "/redfish/v1/Systems/system/Storage";
        res.jsonValue["Name"] = "Storage Collection";
        res.jsonValue["Members"] = {
            {{"@odata.id", "/redfish/v1/Systems/system/Storage/1"}}};
        res.jsonValue["Members@odata.count"] = 1;
        res.end();
    }
};

class Storage : public Node
{
  public:
    Storage(App& app) : Node(app, "/redfish/v1/Systems/system/Storage/1/")
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
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>&) override
    {
        res.jsonValue["@odata.type"] = "#Storage.v1_7_1.Storage";
        res.jsonValue["@odata.id"] = "/redfish/v1/Systems/system/Storage/1";
        res.jsonValue["Name"] = "Storage";
        res.jsonValue["Id"] = "1";
        res.jsonValue["Status"]["State"] = "Enabled";

        auto asyncResp = std::make_shared<AsyncResp>(res);
        auto health = std::make_shared<HealthPopulate>(asyncResp);
        health->populate();

        crow::connections::systemBus->async_method_call(
            [asyncResp, health](const boost::system::error_code ec,
                                const std::vector<std::string>& storageList) {
                nlohmann::json& storageArray =
                    asyncResp->res.jsonValue["Drives"];
                storageArray = nlohmann::json::array();
                auto& count = asyncResp->res.jsonValue["Drives@odata.count"];
                count = 0;

                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Drive mapper call error";
                    messages::internalError(asyncResp->res);
                    return;
                }

                health->inventory.insert(health->inventory.end(),
                                         storageList.begin(),
                                         storageList.end());

                for (const std::string& objpath : storageList)
                {
                    std::size_t lastPos = objpath.rfind('/');
                    if (lastPos == std::string::npos ||
                        (objpath.size() <= lastPos + 1))
                    {
                        BMCWEB_LOG_ERROR << "Failed to find '/' in " << objpath;
                        continue;
                    }

                    storageArray.push_back(
                        {{"@odata.id",
                          "/redfish/v1/Systems/system/Storage/1/Drives/" +
                              objpath.substr(lastPos + 1)}});
                }

                count = storageArray.size();
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
            "/xyz/openbmc_project/inventory", int32_t(0),
            std::array<const char*, 1>{
                "xyz.openbmc_project.Inventory.Item.Drive"});

        crow::connections::systemBus->async_method_call(
            [asyncResp,
             health](const boost::system::error_code ec,
                     const crow::openbmc_mapper::GetSubTreeType& subtree) {
                if (ec || !subtree.size())
                {
                    // doesn't have to be there
                    return;
                }

                nlohmann::json& root =
                    asyncResp->res.jsonValue["StorageControllers"];
                root = nlohmann::json::array();
                for (const auto& [path, interfaceDict] : subtree)
                {
                    std::size_t lastPos = path.rfind('/');
                    if (lastPos == std::string::npos ||
                        (path.size() <= lastPos + 1))
                    {
                        BMCWEB_LOG_ERROR << "Failed to find '/' in " << path;
                        return;
                    }

                    if (interfaceDict.size() != 1)
                    {
                        BMCWEB_LOG_ERROR << "Connection size "
                                         << interfaceDict.size()
                                         << ", greater than 1";
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    const std::string& connectionName =
                        interfaceDict.front().first;

                    size_t index = root.size();
                    nlohmann::json& storageController =
                        root.emplace_back(nlohmann::json::object());

                    std::string id = path.substr(lastPos + 1);

                    storageController["@odata.type"] =
                        "#Storage.v1_7_0.StorageController";
                    storageController["@odata.id"] =
                        "/redfish/v1/Systems/system/Storage/1"
                        "#/StorageControllers/" +
                        std::to_string(index);
                    storageController["Name"] = id;
                    storageController["MemberId"] = id;
                    storageController["Status"]["State"] = "Enabled";

                    crow::connections::systemBus->async_method_call(
                        [asyncResp, index](const boost::system::error_code ec2,
                                           const std::variant<bool> present) {
                            // this interface isn't necessary, only check it if
                            // we get a good return
                            if (ec2)
                            {
                                return;
                            }
                            const bool* enabled = std::get_if<bool>(&present);
                            if (enabled == nullptr)
                            {
                                BMCWEB_LOG_DEBUG << "Illegal property present";
                                messages::internalError(asyncResp->res);
                                return;
                            }
                            if (!(*enabled))
                            {
                                asyncResp->res
                                    .jsonValue["StorageControllers"][index]
                                              ["Status"]["State"] = "Disabled";
                            }
                        },
                        connectionName, path, "org.freedesktop.DBus.Properties",
                        "Get", "xyz.openbmc_project.Inventory.Item", "Present");

                    crow::connections::systemBus->async_method_call(
                        [asyncResp,
                         index](const boost::system::error_code ec2,
                                const std::vector<std::pair<
                                    std::string,
                                    std::variant<bool, std::string, uint64_t>>>&
                                    propertiesList) {
                            if (ec2)
                            {
                                // this interface isn't necessary
                                return;
                            }
                            for (const std::pair<
                                     std::string,
                                     std::variant<bool, std::string, uint64_t>>&
                                     property : propertiesList)
                            {
                                // Store DBus properties that are also
                                // Redfish properties with same name and a
                                // string value
                                const std::string& propertyName =
                                    property.first;
                                nlohmann::json& object =
                                    asyncResp->res
                                        .jsonValue["StorageControllers"][index];
                                if ((propertyName == "PartNumber") ||
                                    (propertyName == "SerialNumber") ||
                                    (propertyName == "Manufacturer") ||
                                    (propertyName == "Model"))
                                {
                                    const std::string* value =
                                        std::get_if<std::string>(
                                            &property.second);
                                    if (value == nullptr)
                                    {
                                        // illegal property
                                        messages::internalError(asyncResp->res);
                                        continue;
                                    }
                                    object[propertyName] = *value;
                                }
                            }
                        },
                        connectionName, path, "org.freedesktop.DBus.Properties",
                        "GetAll",
                        "xyz.openbmc_project.Inventory.Decorator.Asset");
                }

                // this is done after we know the json array will no longer be
                // resized, as json::array uses vector underneath and we need
                // references to its members that won't change
                size_t count = 0;
                for (const auto& [path, interfaceDict] : subtree)
                {
                    auto subHealth = std::make_shared<HealthPopulate>(
                        asyncResp, root[count]["Status"]);
                    subHealth->inventory.emplace_back(path);
                    health->inventory.emplace_back(path);
                    health->children.emplace_back(subHealth);
                    count++;
                }
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTree",
            "/xyz/openbmc_project/inventory", int32_t(0),
            std::array<const char*, 1>{
                "xyz.openbmc_project.Inventory.Item.StorageController"});
    }
};

class Drive : public Node
{
  public:
    Drive(App& app) :
        Node(app, "/redfish/v1/Systems/system/Storage/1/Drives/<str>/",
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
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>& params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        const std::string& driveId = params[0];

        crow::connections::systemBus->async_method_call(
            [asyncResp,
             driveId](const boost::system::error_code ec,
                      const crow::openbmc_mapper::GetSubTreeType& subtree) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Drive mapper call error";
                    messages::internalError(asyncResp->res);
                    return;
                }

                auto object2 = std::find_if(
                    subtree.begin(), subtree.end(), [&driveId](auto& object) {
                        const std::string& path = object.first;
                        return boost::ends_with(path, "/" + driveId);
                    });

                if (object2 == subtree.end())
                {
                    messages::resourceNotFound(asyncResp->res, "Drive",
                                               driveId);
                    return;
                }

                const std::string& path = object2->first;
                const std::vector<
                    std::pair<std::string, std::vector<std::string>>>&
                    connectionNames = object2->second;

                asyncResp->res.jsonValue["@odata.type"] = "#Drive.v1_7_0.Drive";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Systems/system/Storage/1/Drives/" + driveId;
                asyncResp->res.jsonValue["Name"] = driveId;
                asyncResp->res.jsonValue["Id"] = driveId;

                if (connectionNames.size() != 1)
                {
                    BMCWEB_LOG_ERROR << "Connection size "
                                     << connectionNames.size()
                                     << ", greater than 1";
                    messages::internalError(asyncResp->res);
                    return;
                }

                getMainChassisId(
                    asyncResp, [](const std::string& chassisId,
                                  const std::shared_ptr<AsyncResp>& aRsp) {
                        aRsp->res.jsonValue["Links"]["Chassis"] = {
                            {"@odata.id", "/redfish/v1/Chassis/" + chassisId}};
                    });

                const std::string& connectionName = connectionNames[0].first;
                crow::connections::systemBus->async_method_call(
                    [asyncResp](const boost::system::error_code ec2,
                                const std::vector<std::pair<
                                    std::string,
                                    std::variant<bool, std::string, uint64_t>>>&
                                    propertiesList) {
                        if (ec2)
                        {
                            // this interface isn't necessary
                            return;
                        }
                        for (const std::pair<std::string,
                                             std::variant<bool, std::string,
                                                          uint64_t>>& property :
                             propertiesList)
                        {
                            // Store DBus properties that are also
                            // Redfish properties with same name and a
                            // string value
                            const std::string& propertyName = property.first;
                            if ((propertyName == "PartNumber") ||
                                (propertyName == "SerialNumber") ||
                                (propertyName == "Manufacturer") ||
                                (propertyName == "Model"))
                            {
                                const std::string* value =
                                    std::get_if<std::string>(&property.second);
                                if (value == nullptr)
                                {
                                    // illegal property
                                    messages::internalError(asyncResp->res);
                                    continue;
                                }
                                asyncResp->res.jsonValue[propertyName] = *value;
                            }
                        }
                    },
                    connectionName, path, "org.freedesktop.DBus.Properties",
                    "GetAll", "xyz.openbmc_project.Inventory.Decorator.Asset");

                // default it to Enabled
                asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

                auto health = std::make_shared<HealthPopulate>(asyncResp);
                health->inventory.emplace_back(path);
                health->populate();

                crow::connections::systemBus->async_method_call(
                    [asyncResp, path](const boost::system::error_code ec2,
                                      const std::variant<bool> present) {
                        // this interface isn't necessary, only check it if we
                        // get a good return
                        if (ec2)
                        {
                            return;
                        }
                        const bool* enabled = std::get_if<bool>(&present);
                        if (enabled == nullptr)
                        {
                            BMCWEB_LOG_DEBUG << "Illegal property present";
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        if (!(*enabled))
                        {
                            asyncResp->res.jsonValue["Status"]["State"] =
                                "Disabled";
                        }
                    },
                    connectionName, path, "org.freedesktop.DBus.Properties",
                    "Get", "xyz.openbmc_project.Inventory.Item", "Present");

                crow::connections::systemBus->async_method_call(
                    [asyncResp](const boost::system::error_code ec2,
                                const std::variant<bool> rebuilding) {
                        // this interface isn't necessary, only check it if we
                        // get a good return
                        if (ec2)
                        {
                            return;
                        }
                        const bool* updating = std::get_if<bool>(&rebuilding);
                        if (updating == nullptr)
                        {
                            BMCWEB_LOG_DEBUG << "Illegal property present";
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        // updating and disabled in the backend shouldn't be
                        // able to be set at the same time, so we don't need to
                        // check for the race condition of these two calls
                        if ((*updating))
                        {
                            asyncResp->res.jsonValue["Status"]["State"] =
                                "Updating";
                        }
                    },
                    connectionName, path, "org.freedesktop.DBus.Properties",
                    "Get", "xyz.openbmc_project.State.Drive", "Rebuilding");
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTree",
            "/xyz/openbmc_project/inventory", int32_t(0),
            std::array<const char*, 1>{
                "xyz.openbmc_project.Inventory.Item.Drive"});
    }
};
} // namespace redfish
