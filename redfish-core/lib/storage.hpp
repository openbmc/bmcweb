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
#include <registries/privilege_registry.hpp>
#include <utils/location_utils.hpp>
#include <utils/name_utils.hpp>

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
                collection_util::getCollectionMembers(
                    asyncResp, "/redfish/v1/Systems/system/Storage",
                    {"xyz.openbmc_project.Inventory.Item.Storage"});
            });
}

inline void getDrives(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::shared_ptr<HealthPopulate>& health,
                      const std::string& parent, const std::string& storageId)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, health, parent,
         storageId](const boost::system::error_code ec,
                    const std::vector<std::string>& driveList) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "Drive mapper call error";
                messages::internalError(asyncResp->res);
                return;
            }

            nlohmann::json storageArray = nlohmann::json::array();

            health->inventory.insert(health->inventory.end(), driveList.begin(),
                                     driveList.end());

            for (const std::string& objpath : driveList)
            {
                if (!validSubpath(objpath, parent))
                {
                    continue;
                }

                sdbusplus::message::object_path object(objpath);
                if (object.filename().empty())
                {
                    BMCWEB_LOG_ERROR << "Failed to find filename in "
                                     << objpath;
                    return;
                }

                storageArray.push_back(
                    {{"@odata.id", "/redfish/v1/Systems/system/Storage/" +
                                       storageId + "/Drives/" +
                                       object.filename()}});
            }

            asyncResp->res.jsonValue["Drives@odata.count"] =
                storageArray.size();
            asyncResp->res.jsonValue["Drives"] = std::move(storageArray);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
        "/xyz/openbmc_project/inventory", int32_t(0),
        std::array<const char*, 1>{"xyz.openbmc_project.Inventory.Item.Drive"});
}

inline void
    getStorageControllers(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::shared_ptr<HealthPopulate>& health,
                          const std::string& parent,
                          const std::string& storageId)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, health, parent,
         storageId](const boost::system::error_code ec,
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
                // Skip path if the object is not under parent object path.
                if (!validSubpath(path, parent))
                {
                    continue;
                }

                sdbusplus::message::object_path object(path);
                if (object.filename().empty())
                {
                    BMCWEB_LOG_ERROR << "Failed to find filename in " << path;
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

                const std::string& connectionName = interfaceDict.front().first;

                size_t index = root.size();
                nlohmann::json& storageController =
                    root.emplace_back(nlohmann::json::object());

                std::string id = object.filename();

                storageController["@odata.type"] =
                    "#Storage.v1_7_0.StorageController";
                storageController["@odata.id"] =
                    "/redfish/v1/Systems/system/Storage/" + storageId +
                    "#/StorageControllers/" + std::to_string(index);
                storageController["Name"] = id;
                storageController["MemberId"] = id;
                storageController["Status"]["State"] = "Enabled";

                name_util::getPrettyName(asyncResp, path,
                                         {"xyz.openbmc_project.Inventory.Item."
                                          "StorageController"},
                                         &storageController["Name"]);

                location_util::getLocation(
                    asyncResp, sdbusplus::message::object_path(path).filename(),
                    std::vector<const char*>{
                        "xyz.openbmc_project.Inventory.Item."
                        "StorageController"},
                    &storageController, "Location");

                crow::connections::systemBus->async_method_call(
                    [asyncResp, index](const boost::system::error_code ec2,
                                       const std::variant<bool> present) {
                        // this interface isn't necessary, only check it
                        // if we get a good return
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
                            asyncResp->res.jsonValue["StorageControllers"]
                                                    [index]["Status"]["State"] =
                                "Disabled";
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
                        for (const std::pair<std::string,
                                             std::variant<bool, std::string,
                                                          uint64_t>>& property :
                             propertiesList)
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
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/Storage/<str>/")
        .privileges(redfish::privileges::getStorage)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& storageId) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#Storage.v1_7_1.Storage";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Systems/system/Storage/" + storageId;
                asyncResp->res.jsonValue["Name"] = "Storage";
                asyncResp->res.jsonValue["Id"] = storageId;
                asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

                auto health = std::make_shared<HealthPopulate>(asyncResp);
                health->populate();

                crow::connections::systemBus->async_method_call(
                    [asyncResp, health,
                     storageId](const boost::system::error_code ec,
                                const std::vector<std::string>& storages) {
                        if (ec)
                        {
                            BMCWEB_LOG_DEBUG << "DBUS response error";
                            return;
                        }

                        bool foundStorage = false;
                        for (const auto& storage : storages)
                        {
                            sdbusplus::message::object_path path(storage);
                            if (path.filename() != storageId)
                            {
                                continue;
                            }

                            foundStorage = true;
                            getDrives(asyncResp, health, storage, storageId);
                            getStorageControllers(asyncResp, health, storage,
                                                  storageId);
                        }

                        if (!foundStorage)
                        {
                            messages::resourceNotFound(asyncResp->res,
                                                       "Storage", storageId);
                        }
                    },
                    "xyz.openbmc_project.ObjectMapper",
                    "/xyz/openbmc_project/object_mapper",
                    "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
                    "/xyz/openbmc_project/inventory", 0,
                    std::array<std::string, 1>{
                        "xyz.openbmc_project.Inventory.Item.Storage"});

                location_util::getLocation(
                    asyncResp, storageId,
                    std::vector<const char*>{
                        "xyz.openbmc_project.Inventory.Item.Storage"},
                    &asyncResp->res.jsonValue, "Location");
            });
}

inline void requestRoutesDrive(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/Storage/<str>/Drives/<str>/")
        .privileges(redfish::privileges::getDrive)
        .methods(
            boost::beast::http::verb::get)([](const crow::Request&,
                                              const std::shared_ptr<
                                                  bmcweb::AsyncResp>& asyncResp,
                                              const std::string& storageId,
                                              const std::string& driveId) {
            crow::connections::systemBus->async_method_call(
                [asyncResp, storageId,
                 driveId](const boost::system::error_code ec,
                          const crow::openbmc_mapper::GetSubTreeType& subtree) {
                    if (ec)
                    {
                        BMCWEB_LOG_ERROR << "Drive mapper call error";
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    auto object2 = std::find_if(
                        subtree.begin(), subtree.end(),
                        [&driveId](auto& object) {
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

                    asyncResp->res.jsonValue["@odata.type"] =
                        "#Drive.v1_7_0.Drive";
                    asyncResp->res.jsonValue["@odata.id"] =
                        "/redfish/v1/Systems/system/Storage/" + storageId +
                        "/Drives/" + driveId;
                    asyncResp->res.jsonValue["Name"] = driveId;
                    asyncResp->res.jsonValue["Id"] = driveId;

                    name_util::getPrettyName(
                        asyncResp, path,
                        {"xyz.openbmc_project.Inventory.Item.Drive"},
                        &asyncResp->res.jsonValue["Name"]);

                    if (connectionNames.size() != 1)
                    {
                        BMCWEB_LOG_ERROR << "Connection size "
                                         << connectionNames.size()
                                         << ", greater than 1";
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    getChassisId(
                        asyncResp,
                        [path](const std::string& chassisPath) {
                            return validSubpath(path, chassisPath);
                        },
                        [](const std::string& chassisId,
                           const std::shared_ptr<bmcweb::AsyncResp>& aRsp) {
                            aRsp->res.jsonValue["Links"]["Chassis"] = {
                                {"@odata.id",
                                 "/redfish/v1/Chassis/" + chassisId}};
                        });

                    const std::string& connectionName =
                        connectionNames[0].first;
                    crow::connections::systemBus->async_method_call(
                        [asyncResp](
                            const boost::system::error_code ec2,
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
                                        return;
                                    }
                                    asyncResp->res.jsonValue[propertyName] =
                                        *value;
                                }
                            }
                        },
                        connectionName, path, "org.freedesktop.DBus.Properties",
                        "GetAll",
                        "xyz.openbmc_project.Inventory.Decorator.Asset");

                    // default it to Enabled
                    asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

                    auto health = std::make_shared<HealthPopulate>(asyncResp);
                    health->inventory.emplace_back(path);
                    health->populate();

                    crow::connections::systemBus->async_method_call(
                        [asyncResp, path](const boost::system::error_code ec2,
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
                                asyncResp->res.jsonValue["Status"]["State"] =
                                    "Disabled";
                            }
                        },
                        connectionName, path, "org.freedesktop.DBus.Properties",
                        "Get", "xyz.openbmc_project.Inventory.Item", "Present");

                    crow::connections::systemBus->async_method_call(
                        [asyncResp](const boost::system::error_code ec2,
                                    const std::variant<bool> rebuilding) {
                            // this interface isn't necessary, only check it if
                            // we get a good return
                            if (ec2)
                            {
                                return;
                            }
                            const bool* updating =
                                std::get_if<bool>(&rebuilding);
                            if (updating == nullptr)
                            {
                                BMCWEB_LOG_DEBUG << "Illegal property present";
                                messages::internalError(asyncResp->res);
                                return;
                            }

                            // updating and disabled in the backend shouldn't be
                            // able to be set at the same time, so we don't need
                            // to check for the race condition of these two
                            // calls
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

            location_util::getLocation(
                asyncResp, driveId,
                std::vector<const char*>{
                    "xyz.openbmc_project.Inventory.Item.Drive"},
                &asyncResp->res.jsonValue, "PhysicalLocation");
        });
}
} // namespace redfish
