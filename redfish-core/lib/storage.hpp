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

#include <app.hpp>
#include <dbus_utility.hpp>
#include <registries/privilege_registry.hpp>
#include <sdbusplus/asio/property.hpp>

namespace redfish
{
inline void requestRoutesStorageCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/Storage/")
        .privileges(redfish::privileges::getStorageCollection)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#StorageCollection.StorageCollection";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Systems/system/Storage";
                asyncResp->res.jsonValue["Name"] = "Storage Collection";
                asyncResp->res.jsonValue["Members"] = {
                    {{"@odata.id", "/redfish/v1/Systems/system/Storage/1"}}};
                asyncResp->res.jsonValue["Members@odata.count"] = 1;
            });
}

inline void getDrives(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::shared_ptr<HealthPopulate>& health)
{

    crow::connections::systemBus->async_method_call(
        [asyncResp, health](const boost::system::error_code ec,
                            const std::vector<std::string>& driveList) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "Drive mapper call error";
                messages::internalError(asyncResp->res);
                return;
            }

            nlohmann::json& storageArray = asyncResp->res.jsonValue["Drives"];
            storageArray = nlohmann::json::array();
            auto& count = asyncResp->res.jsonValue["Drives@odata.count"];
            count = 0;

            health->inventory.insert(health->inventory.end(), driveList.begin(),
                                     driveList.end());

            for (const std::string& drive : driveList)
            {
                sdbusplus::message::object_path object(drive);
                if (object.filename().empty())
                {
                    BMCWEB_LOG_ERROR << "Failed to find filename in " << drive;
                    return;
                }
                storageArray.push_back(
                    {{"@odata.id",
                      "/redfish/v1/Systems/system/Storage/1/Drives/" +
                          object.filename()}});
            }

            count = storageArray.size();
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
        "/xyz/openbmc_project/inventory", int32_t(0),
        std::array<const char*, 1>{"xyz.openbmc_project.Inventory.Item.Drive"});
}

inline void
    getStorageControllers(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::shared_ptr<HealthPopulate>& health)
{
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
                sdbusplus::message::object_path object(path);
                std::string id = object.filename();
                if (id.empty())
                {
                    BMCWEB_LOG_ERROR << "Failed to find filename in " << path;
                    return;
                }

                if (interfaceDict.size() != 1)
                {
                    BMCWEB_LOG_ERROR << "Connection size "
                                     << interfaceDict.size()
                                     << ", not equal to 1";
                    messages::internalError(asyncResp->res);
                    return;
                }

                const std::string& connectionName = interfaceDict.front().first;

                size_t index = root.size();
                nlohmann::json& storageController =
                    root.emplace_back(nlohmann::json::object());

                storageController["@odata.type"] =
                    "#Storage.v1_7_0.StorageController";
                storageController["@odata.id"] =
                    "/redfish/v1/Systems/system/Storage/1#/StorageControllers/" +
                    std::to_string(index);
                storageController["Name"] = id;
                storageController["MemberId"] = id;
                storageController["Status"]["State"] = "Enabled";

                sdbusplus::asio::getProperty<bool>(
                    *crow::connections::systemBus, connectionName, path,
                    "xyz.openbmc_project.Inventory.Item", "Present",
                    [asyncResp, index](const boost::system::error_code ec2,
                                       bool enabled) {
                        // this interface isn't necessary, only check it
                        // if we get a good return
                        if (ec2)
                        {
                            return;
                        }
                        if (!enabled)
                        {
                            asyncResp->res.jsonValue["StorageControllers"]
                                                    [index]["Status"]["State"] =
                                "Disabled";
                        }
                    });

                crow::connections::systemBus->async_method_call(
                    [asyncResp,
                     index](const boost::system::error_code ec2,
                            const std::vector<std::pair<
                                std::string, dbus::utility::DbusVariantType>>&
                                propertiesList) {
                        if (ec2)
                        {
                            // this interface isn't necessary
                            return;
                        }
                        for (const std::pair<std::string,
                                             dbus::utility::DbusVariantType>&
                                 property : propertiesList)
                        {
                            // Store DBus properties that are also
                            // Redfish properties with same name and a
                            // string value
                            const std::string& propertyName = property.first;
                            nlohmann::json& object =
                                asyncResp->res
                                    .jsonValue["StorageControllers"][index];
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
                                    return;
                                }
                                object[propertyName] = *value;
                            }
                        }
                    },
                    connectionName, path, "org.freedesktop.DBus.Properties",
                    "GetAll", "xyz.openbmc_project.Inventory.Decorator.Asset");
            }

            // this is done after we know the json array will no longer
            // be resized, as json::array uses vector underneath and we
            // need references to its members that won't change
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

inline void requestRoutesStorage(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/Storage/1/")
        .privileges(redfish::privileges::getStorage)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#Storage.v1_7_1.Storage";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Systems/system/Storage/1";
                asyncResp->res.jsonValue["Name"] = "Storage";
                asyncResp->res.jsonValue["Id"] = "1";
                asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

                auto health = std::make_shared<HealthPopulate>(asyncResp);
                health->populate();

                getDrives(asyncResp, health);
                getStorageControllers(asyncResp, health);
            });
}

inline void getDriveAsset(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& connectionName,
                          const std::string& path)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec,
                    const std::vector<
                        std::pair<std::string, dbus::utility::DbusVariantType>>&
                        propertiesList) {
            if (ec)
            {
                // this interface isn't necessary
                return;
            }
            for (const std::pair<std::string, dbus::utility::DbusVariantType>&
                     property : propertiesList)
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
                        return;
                    }
                    asyncResp->res.jsonValue[propertyName] = *value;
                }
            }
        },
        connectionName, path, "org.freedesktop.DBus.Properties", "GetAll",
        "xyz.openbmc_project.Inventory.Decorator.Asset");
}

inline void getDrivePresent(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& connectionName,
                            const std::string& path)
{
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, connectionName, path,
        "xyz.openbmc_project.Inventory.Item", "Present",
        [asyncResp, path](const boost::system::error_code ec,
                          const bool enabled) {
            // this interface isn't necessary, only check it if
            // we get a good return
            if (ec)
            {
                return;
            }

            if (!enabled)
            {
                asyncResp->res.jsonValue["Status"]["State"] = "Disabled";
            }
        });
}

inline void getDriveState(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& connectionName,
                          const std::string& path)
{
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, connectionName, path,
        "xyz.openbmc_project.State.Drive", "Rebuilding",
        [asyncResp](const boost::system::error_code ec, const bool updating) {
            // this interface isn't necessary, only check it
            // if we get a good return
            if (ec)
            {
                return;
            }

            // updating and disabled in the backend shouldn't be
            // able to be set at the same time, so we don't need
            // to check for the race condition of these two
            // calls
            if (updating)
            {
                asyncResp->res.jsonValue["Status"]["State"] = "Updating";
            }
        });
}

inline void requestRoutesDrive(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/Storage/1/Drives/<str>/")
        .privileges(redfish::privileges::getDrive)
        .methods(
            boost::beast::http::verb::get)([](const crow::Request&,
                                              const std::shared_ptr<
                                                  bmcweb::AsyncResp>& asyncResp,
                                              const std::string& driveId) {
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

                    auto drive = std::find_if(
                        subtree.begin(), subtree.end(),
                        [&driveId](const std::pair<
                                   std::string,
                                   std::vector<std::pair<
                                       std::string, std::vector<std::string>>>>&
                                       object) {
                            return sdbusplus::message::object_path(object.first)
                                       .filename() == driveId;
                        });

                    if (drive == subtree.end())
                    {
                        messages::resourceNotFound(asyncResp->res, "Drive",
                                                   driveId);
                        return;
                    }

                    const std::string& path = drive->first;
                    const std::vector<
                        std::pair<std::string, std::vector<std::string>>>&
                        connectionNames = drive->second;

                    asyncResp->res.jsonValue["@odata.type"] =
                        "#Drive.v1_7_0.Drive";
                    asyncResp->res.jsonValue["@odata.id"] =
                        "/redfish/v1/Systems/system/Storage/1/Drives/" +
                        driveId;
                    asyncResp->res.jsonValue["Name"] = driveId;
                    asyncResp->res.jsonValue["Id"] = driveId;

                    if (connectionNames.size() != 1)
                    {
                        BMCWEB_LOG_ERROR << "Connection size "
                                         << connectionNames.size()
                                         << ", not equal to 1";
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    getMainChassisId(
                        asyncResp,
                        [](const std::string& chassisId,
                           const std::shared_ptr<bmcweb::AsyncResp>& aRsp) {
                            aRsp->res.jsonValue["Links"]["Chassis"] = {
                                {"@odata.id",
                                 "/redfish/v1/Chassis/" + chassisId}};
                        });

                    // default it to Enabled
                    asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

                    auto health = std::make_shared<HealthPopulate>(asyncResp);
                    health->inventory.emplace_back(path);
                    health->populate();

                    const std::string& connectionName =
                        connectionNames[0].first;

                    getDriveAsset(asyncResp, connectionName, path);
                    getDrivePresent(asyncResp, connectionName, path);
                    getDriveState(asyncResp, connectionName, path);
                },
                "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/object_mapper",
                "xyz.openbmc_project.ObjectMapper", "GetSubTree",
                "/xyz/openbmc_project/inventory", int32_t(0),
                std::array<const char*, 1>{
                    "xyz.openbmc_project.Inventory.Item.Drive"});
        });
}
} // namespace redfish
