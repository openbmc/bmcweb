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

inline bool assignDriveAsset(
    nlohmann::json& jsonData,
    const std::vector<std::pair<std::string, dbus::utility::DbusVariantType>>&
        properties)
{
    std::optional<std::string> partNumber, serialNumber, manufacturer, model;

    std::optional<std::string> badProperty = sdbusplus::unpackPropertiesNoThrow(
        properties, "PartNumber", partNumber, "SerialNumber", serialNumber,
        "Manufacturer", manufacturer, "Model", model);

    if (badProperty)
    {
        return false;
    }

    json_util::assignIf(jsonData, "PartNumber", partNumber);
    json_util::assignIf(jsonData, "SerialNumber", serialNumber);
    json_util::assignIf(jsonData, "Manufacturer", manufacturer);
    json_util::assignIf(jsonData, "Model", model);

    return true;
}

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

inline void requestRoutesStorage(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/Storage/1/")
        .privileges(redfish::privileges::getStorage)
        .methods(
            boost::beast::http::verb::
                get)([](const crow::Request&,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
            asyncResp->res.jsonValue["@odata.type"] = "#Storage.v1_7_1.Storage";
            asyncResp->res.jsonValue["@odata.id"] =
                "/redfish/v1/Systems/system/Storage/1";
            asyncResp->res.jsonValue["Name"] = "Storage";
            asyncResp->res.jsonValue["Id"] = "1";
            asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

            auto health = std::make_shared<HealthPopulate>(asyncResp);
            health->populate();

            crow::connections::systemBus->async_method_call(
                [asyncResp,
                 health](const boost::system::error_code ec,
                         const std::vector<std::string>& storageList) {
                    nlohmann::json& storageArray =
                        asyncResp->res.jsonValue["Drives"];
                    storageArray = nlohmann::json::array();
                    auto& count =
                        asyncResp->res.jsonValue["Drives@odata.count"];
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
                            BMCWEB_LOG_ERROR << "Failed to find '/' in "
                                             << objpath;
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
                            BMCWEB_LOG_ERROR << "Failed to find '/' in "
                                             << path;
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
                            "/redfish/v1/Systems/system/Storage/1#/StorageControllers/" +
                            std::to_string(index);
                        storageController["Name"] = id;
                        storageController["MemberId"] = id;
                        storageController["Status"]["State"] = "Enabled";

                        sdbusplus::asio::getProperty<bool>(
                            *crow::connections::systemBus, connectionName, path,
                            "xyz.openbmc_project.Inventory.Item", "Present",
                            [asyncResp,
                             index](const boost::system::error_code ec2,
                                    bool enabled) {
                                // this interface isn't necessary, only check it
                                // if we get a good return
                                if (ec2)
                                {
                                    return;
                                }
                                if (!enabled)
                                {
                                    asyncResp->res
                                        .jsonValue["StorageControllers"][index]
                                                  ["Status"]["State"] =
                                        "Disabled";
                                }
                            });

                        sdbusplus::asio::getAllProperties(
                            *crow::connections::systemBus, connectionName, path,
                            "xyz.openbmc_project.Inventory.Decorator.Asset",
                            [asyncResp, index](
                                const boost::system::error_code ec2,
                                const std::vector<
                                    std::pair<std::string,
                                              dbus::utility::DbusVariantType>>&
                                    propertiesList) {
                                if (ec2)
                                {
                                    // this interface isn't necessary
                                    return;
                                }

                                nlohmann::json& object =
                                    asyncResp->res
                                        .jsonValue["StorageControllers"][index];

                                if (!assignDriveAsset(object, propertiesList))
                                {
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                            });
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

            if (!assignDriveAsset(asyncResp->res.jsonValue, propertiesList))
            {
                messages::internalError(asyncResp->res);
                return;
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
            // this interface isn't necessary, only check it if:
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
