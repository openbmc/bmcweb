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
#include "multi_storage_helper.hpp"
#include "openbmc_dbus_rest.hpp"
#include "utility.hpp"

#include <app.hpp>
#include <dbus_utility.hpp>
#include <query.hpp>
#include <registries/privilege_registry.hpp>
#include <sdbusplus/asio/property.hpp>
#include <utils/location_utils.hpp>

#include <unordered_set>

namespace redfish
{
inline void requestRoutesStorageCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/Storage/")
        .privileges(redfish::privileges::getStorageCollection)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
                {
                    return;
                }
                asyncResp->res.jsonValue["@odata.type"] =
                    "#StorageCollection.StorageCollection";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Systems/system/Storage";
                asyncResp->res.jsonValue["Name"] = "Storage Collection";
                nlohmann::json::array_t members;
                nlohmann::json::object_t member;
                member["@odata.id"] = "/redfish/v1/Systems/system/Storage/1";
                members.emplace_back(member);
                asyncResp->res.jsonValue["Members"] = std::move(members);
                asyncResp->res.jsonValue["Members@odata.count"] = 1;
            });

    BMCWEB_ROUTE(app, "/redfish/v1/Storage/")
        .privileges(redfish::privileges::getStorageCollection)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
                {
                    return;
                }
                asyncResp->res.jsonValue["@odata.type"] =
                    "#StorageCollection.StorageCollection";
                asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/Storage";
                asyncResp->res.jsonValue["Name"] = "Storage Collection";
                collection_util::getCollectionMembers(
                    asyncResp, "/redfish/v1/Storage",
                    {"xyz.openbmc_project.Inventory.Item.Storage"});
            });
}

void getDrivesWithoutAssociation(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::shared_ptr<HealthPopulate>& health,
    const boost::system::error_code ec,
    const std::vector<std::string>& driveList)
{
    if (ec)
    {
        return;
    }

    std::unordered_set<std::string> driveMap(driveList.begin(),
                                             driveList.end());

    nlohmann::json& driveArray = asyncResp->res.jsonValue["Drives"];
    driveArray = nlohmann::json::array();
    auto& count = asyncResp->res.jsonValue["Drives@odata.count"];
    count = 0;

    // /redfish/v1/Systems/system/Storage/1/Drives will only include Drives that
    // is not associated with an existing Storage.
    for (const std::string& drive : driveList)
    {
        sdbusplus::asio::getProperty<std::vector<std::string>>(
            *crow::connections::systemBus, "xyz.openbmc_project.ObjectMapper",
            drive + "/storage", "xyz.openbmc_project.Association", "endpoints",
            [asyncResp, health,
             drive](const boost::system::error_code ec,
                    const std::vector<std::string>& storageAssociations) {
                if (!ec || !storageAssociations.empty())
                {
                    return;
                }

                sdbusplus::message::object_path path(drive);
                const std::string leaf = path.filename();
                if (leaf.empty())
                {
                    BMCWEB_LOG_ERROR << "Empty filename() for " << drive;
                    return;
                }

                health->inventory.emplace_back(drive);

                nlohmann::json::object_t storage;
                storage["@odata.id"] =
                    "/redfish/v1/Systems/system/Storage/1/Drives/" + leaf;
                asyncResp->res.jsonValue["Drives"].push_back(
                    std::move(storage));
                asyncResp->res.jsonValue["Drives@odata.count"] =
                    asyncResp->res.jsonValue["Drives"].size();
            });
    }
}

void getDrivesWithAssociation(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::shared_ptr<HealthPopulate>& health,
    const std::string& storagePath, const std::string& storageId,
    const boost::system::error_code ec,
    const std::vector<std::string>& driveList)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR << "Drive mapper call error";
        messages::internalError(asyncResp->res);
        return;
    }
    sdbusplus::asio::getProperty<std::vector<std::string>>(
        *crow::connections::systemBus, "xyz.openbmc_project.ObjectMapper",
        storagePath + "/drive", "xyz.openbmc_project.Association", "endpoints",
        [asyncResp, health, storagePath, storageId,
         driveList](const boost::system::error_code ec,
                    const std::vector<std::string>& driveAssociations) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << storagePath << " has no Drive association";
                return;
            }

            std::unordered_set<std::string> driveMap(driveList.begin(),
                                                     driveList.end());

            nlohmann::json& driveArray = asyncResp->res.jsonValue["Drives"];
            driveArray = nlohmann::json::array();
            auto& count = asyncResp->res.jsonValue["Drives@odata.count"];
            count = 0;

            for (const std::string& drivePath : driveAssociations)
            {
                sdbusplus::message::object_path path(drivePath);
                const std::string leaf = path.filename();
                if (leaf.empty())
                {
                    BMCWEB_LOG_DEBUG << "filename() is empty for " << drivePath;
                    continue;
                }

                auto findDrive = driveMap.find(drivePath);
                if (findDrive == driveMap.end())
                {
                    BMCWEB_LOG_DEBUG
                        << "Associated Drive is does not have valid Drive interface: "
                        << drivePath;
                    continue;
                }
                health->inventory.emplace_back(drivePath);
                nlohmann::json::object_t drive;
                drive["@odata.id"] = crow::utility::urlFromPieces(
                    "redfish", "v1", "Storage", storageId, "Drives", leaf);
                driveArray.push_back(std::move(drive));
            }
            count = asyncResp->res.jsonValue["Drives"].size();
        });
}

inline void
    getDrives(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
              const std::shared_ptr<HealthPopulate>& health,
              const std::optional<std::string>& storagePath = std::nullopt,
              const std::optional<std::string>& storageId = std::nullopt)
{
    // /redfish/v1/Storage will contain all Drives with association to Storage
    // /redfish/v1/System/system/Storage will contain all Drives without
    //   association to Storage
    if (storagePath == std::nullopt || storageId == std::nullopt)
    {
        crow::connections::systemBus->async_method_call(
            [asyncResp, health](const boost::system::error_code ec,
                                const std::vector<std::string>& driveList) {
                getDrivesWithoutAssociation(asyncResp, health, ec, driveList);
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
            "/xyz/openbmc_project/inventory", int32_t(0),
            std::array<const char*, 1>{
                "xyz.openbmc_project.Inventory.Item.Drive"});
    }
    else
    {
        crow::connections::systemBus->async_method_call(
            [asyncResp, health, storagePath,
             storageId](const boost::system::error_code ec,
                        const std::vector<std::string>& driveList) {
                getDrivesWithAssociation(asyncResp, health, *storagePath,
                                         *storageId, ec, driveList);
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
            "/xyz/openbmc_project/inventory", int32_t(0),
            std::array<const char*, 1>{
                "xyz.openbmc_project.Inventory.Item.Drive"});
    }
}

inline void getStorageControllerLocation(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& path, const std::string& service,
    const std::vector<std::string>& interfaces, size_t index)
{
    nlohmann::json_pointer<nlohmann::json> locationPtr =
        "/StorageControllers"_json_pointer / index / "Location";
    for (const std::string& interface : interfaces)
    {
        if (interface == "xyz.openbmc_project.Inventory.Decorator.LocationCode")
        {
            location_util::getLocationCode(asyncResp, service, path,
                                           locationPtr);
        }
        if (location_util::isConnector(interface))
        {
            std::optional<std::string> locationType =
                location_util::getLocationType(interface);
            if (!locationType)
            {
                BMCWEB_LOG_DEBUG
                    << "getLocationType for StorageController failed for "
                    << interface;
                continue;
            }
            asyncResp->res
                .jsonValue[locationPtr]["PartLocation"]["LocationType"] =
                *locationType;
        }
    }
}

void populateStorageController(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::shared_ptr<HealthPopulate>& health, const std::string& service,
    const std::vector<std::string>& interfaces,
    const std::string& storageController,
    std::optional<std::string> storageId = std::nullopt)
{
    sdbusplus::message::object_path path(storageController);
    const std::string& id = path.filename();

    nlohmann::json& root = asyncResp->res.jsonValue["StorageControllers"];
    size_t index = root.size();
    nlohmann::json& storageControllerJson =
        root.emplace_back(nlohmann::json::object());
    storageControllerJson["@odata.type"] = "#Storage.v1_7_0.StorageController";
    storageControllerJson["@odata.id"] =
        storageId == std::nullopt
            ? "/redfish/v1/Systems/system/Storage/1#/StorageControllers/" +
                  std::to_string(index)
            : "/redfish/v1/Storage/" + *storageId + "#/StorageControllers/" +
                  std::to_string(index);
    storageControllerJson["Name"] = id;
    storageControllerJson["MemberId"] = id;
    storageControllerJson["Status"]["State"] = "Enabled";

    getStorageControllerLocation(asyncResp, storageController, service,
                                 interfaces, index);

    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, service, path,
        "xyz.openbmc_project.Inventory.Item", "Present",
        [asyncResp, index](const boost::system::error_code ec2, bool enabled) {
            // this interface isn't necessary, only check it
            // if we get a good return
            if (ec2)
            {
                return;
            }
            if (!enabled)
            {
                asyncResp->res
                    .jsonValue["StorageControllers"][index]["Status"]["State"] =
                    "Disabled";
            }
        });

    crow::connections::systemBus->async_method_call(
        [asyncResp, index](
            const boost::system::error_code ec2,
            const std::vector<std::pair<
                std::string, dbus::utility::DbusVariantType>>& propertiesList) {
            if (ec2)
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
                nlohmann::json& object =
                    asyncResp->res.jsonValue["StorageControllers"][index];
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
        service, storageController, "org.freedesktop.DBus.Properties", "GetAll",
        "xyz.openbmc_project.Inventory.Decorator.Asset");

    auto subHealth = std::make_shared<HealthPopulate>(
        asyncResp, storageControllerJson["Status"]);
    subHealth->inventory.emplace_back(path);
    health->inventory.emplace_back(path);
    health->children.emplace_back(subHealth);
}

void getStorageControllersWithoutAssociation(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::shared_ptr<HealthPopulate>& health,
    const boost::system::error_code ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec || subtree.empty())
    {
        return;
    }

    for (const auto& [storageControllerPath, interfaceDict] : subtree)
    {
        if (interfaceDict.size() != 1)
        {
            BMCWEB_LOG_ERROR << "Connection size " << interfaceDict.size()
                             << ", not equal to 1";
            continue;
        }

        if (sdbusplus::message::object_path(storageControllerPath)
                .filename()
                .empty())
        {
            BMCWEB_LOG_ERROR << "filename() is empty in "
                             << storageControllerPath;
            continue;
        }

        nlohmann::json& root = asyncResp->res.jsonValue["StorageControllers"];
        root = nlohmann::json::array();
        sdbusplus::asio::getProperty<std::vector<std::string>>(
            *crow::connections::systemBus, "xyz.openbmc_project.ObjectMapper",
            storageControllerPath + "/storage",
            "xyz.openbmc_project.Association", "endpoints",
            [asyncResp, health, storageControllerPath = storageControllerPath,
             interfaceDict =
                 interfaceDict](const boost::system::error_code ec,
                                const std::vector<std::string>& storageList) {
                if (!ec || !storageList.empty())
                {
                    return;
                }

                populateStorageController(
                    asyncResp, health, interfaceDict[0].first,
                    interfaceDict[0].second, storageControllerPath);
            });
    }
}

inline void getStorageControllersWithAssociation(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::shared_ptr<HealthPopulate>& health,
    const std::string& storagePath, const std::string& storageId,
    const boost::system::error_code ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec || subtree.empty())
    {
        return;
    }

    std::unordered_map<std::string,
                       std::pair<std::string, std::vector<std::string>>>
        storageControllerServices;
    for (const auto& [storagePath, interfaceDict] : subtree)
    {
        if (interfaceDict.size() != 1)
        {
            BMCWEB_LOG_ERROR << "Connection size " << interfaceDict.size()
                             << ", not equal to 1";
            continue;
        }
        storageControllerServices.emplace(
            storagePath,
            std::make_pair(interfaceDict[0].first, interfaceDict[0].second));
    }

    sdbusplus::asio::getProperty<std::vector<std::string>>(
        *crow::connections::systemBus, "xyz.openbmc_project.ObjectMapper",
        storagePath + "/storage_controller", "xyz.openbmc_project.Association",
        "endpoints",
        [asyncResp, storageId, storageControllerServices, storagePath,
         health](const boost::system::error_code ec,
                 const std::vector<std::string>& storageControllerList) {
            if (ec)
            {
                return;
            }

            if (storageControllerList.empty())
            {
                BMCWEB_LOG_DEBUG << storagePath << " has no storage controller";
                return;
            }

            nlohmann::json& root =
                asyncResp->res.jsonValue["StorageControllers"];
            root = nlohmann::json::array();

            for (const std::string& storageController : storageControllerList)
            {
                auto storageControllerService =
                    storageControllerServices.find(storageController);
                if (storageControllerService == storageControllerServices.end())
                {
                    continue;
                }

                if (sdbusplus::message::object_path(storageController)
                        .filename()
                        .empty())
                {
                    BMCWEB_LOG_ERROR << "filename() is empty in "
                                     << storageController;
                    continue;
                }
                populateStorageController(
                    asyncResp, health, storageControllerService->second.first,
                    storageControllerService->second.second, storageController,
                    storageId);
            }
        });
}

inline void getStorageControllers(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::shared_ptr<HealthPopulate>& health,
    const std::optional<std::string>& storagePath = std::nullopt,
    const std::optional<std::string>& storageId = std::nullopt)
{
    // /redfish/v1/Storage will contain all Drives with association to Storage
    // /redfish/v1/System/system/Storage will contain all Drives without
    //   association to Storage
    if (storagePath == std::nullopt || storageId == std::nullopt)
    {
        crow::connections::systemBus->async_method_call(
            [asyncResp,
             health](const boost::system::error_code ec,
                     const dbus::utility::MapperGetSubTreeResponse& subtree) {
                getStorageControllersWithoutAssociation(asyncResp, health, ec,
                                                        subtree);
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTree",
            "/xyz/openbmc_project/inventory", int32_t(0),
            std::array<const char*, 1>{
                "xyz.openbmc_project.Inventory.Item.StorageController"});
    }
    else
    {
        crow::connections::systemBus->async_method_call(
            [asyncResp, health, storagePath, storageId](
                const boost::system::error_code ec,
                const dbus::utility::MapperGetSubTreeResponse& subtree) {
                getStorageControllersWithAssociation(
                    asyncResp, health, *storagePath, *storageId, ec, subtree);
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTree",
            "/xyz/openbmc_project/inventory", int32_t(0),
            std::array<const char*, 1>{
                "xyz.openbmc_project.Inventory.Item.StorageController"});
    }
}

inline void
    getStorageLocation(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& path, const std::string& service,
                       const std::vector<std::string>& interfaces)
{
    for (const auto& interface : interfaces)
    {
        if (interface == "xyz.openbmc_project.Inventory.Decorator.LocationCode")
        {
            location_util::getLocationCode(asyncResp, service, path,
                                           "/PhysicalLocation"_json_pointer);
        }
        if (location_util::isConnector(interface))
        {
            std::optional<std::string> locationType =
                location_util::getLocationType(interface);
            if (!locationType)
            {
                BMCWEB_LOG_DEBUG << "getLocationType for Storage failed for "
                                 << interface;
                continue;
            }
            asyncResp->res
                .jsonValue["PhysicalLocation"]["PartLocation"]["LocationType"] =
                *locationType;
        }
    }
}

inline void requestRoutesStorage(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/Storage/1/")
        .privileges(redfish::privileges::getStorage)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
                {
                    return;
                }
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

    BMCWEB_ROUTE(app, "/redfish/v1/Storage/<str>/")
        .privileges(redfish::privileges::getStorage)
        .methods(
            boost::beast::http::verb::
                get)([&app](const crow::Request& req,
                            const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& storageId) {
            if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
            {
                return;
            }
            crow::connections::systemBus->async_method_call(
                [asyncResp, storageId](
                    const boost::system::error_code ec,
                    const dbus::utility::MapperGetSubTreeResponse& subtree) {
                    if (ec)
                    {
                        BMCWEB_LOG_DEBUG
                            << "requestRoutesStorage DBUS response error";
                        messages::resourceNotFound(asyncResp->res,
                                                   "#Storage.v1_7_1.Storage",
                                                   storageId);
                        return;
                    }

                    auto storage = std::find_if(
                        subtree.begin(), subtree.end(),
                        [&storageId](
                            const std::pair<std::string,
                                            dbus::utility::MapperServiceMap>&
                                object) {
                            return sdbusplus::message::object_path(object.first)
                                       .filename() == storageId;
                        });
                    if (storage == subtree.end())
                    {
                        messages::resourceNotFound(asyncResp->res,
                                                   "#Storage.v1_7_1.Storage",
                                                   storageId);
                        return;
                    }

                    const std::string& storagePath = storage->first;
                    const dbus::utility::MapperServiceMap& connectionNames =
                        storage->second;

                    if (connectionNames.size() != 1)
                    {
                        BMCWEB_LOG_ERROR << "Connection size "
                                         << connectionNames.size()
                                         << ", greater than 1";
                        messages::resourceNotFound(asyncResp->res,
                                                   "#Storage.v1_7_1.Storage",
                                                   storageId);
                        return;
                    }

                    asyncResp->res.jsonValue["@odata.type"] =
                        "#Storage.v1_7_1.Storage";
                    asyncResp->res.jsonValue["@odata.id"] =
                        "/redfish/v1/Storage/" + storageId;
                    asyncResp->res.jsonValue["Name"] = "Storage";
                    asyncResp->res.jsonValue["Id"] = storageId;
                    asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

                    auto health = std::make_shared<HealthPopulate>(asyncResp);
                    health->populate();

                    getDrives(asyncResp, health, storagePath, storageId);
                    getStorageControllers(asyncResp, health, storagePath,
                                          storageId);
                    getStorageLocation(asyncResp, connectionNames[0].first,
                                       storagePath, connectionNames[0].second);
                },
                "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/object_mapper",
                "xyz.openbmc_project.ObjectMapper", "GetSubTree",
                "/xyz/openbmc_project/inventory", 0,
                std::array<std::string, 1>{
                    "xyz.openbmc_project.Inventory.Item.Storage"});
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

inline std::optional<std::string> convertDriveType(const std::string& type)
{
    if (type == "xyz.openbmc_project.Inventory.Item.Drive.DriveType.HDD")
    {
        return "HDD";
    }
    if (type == "xyz.openbmc_project.Inventory.Item.Drive.DriveType.SSD")
    {
        return "SSD";
    }

    return std::nullopt;
}

inline std::optional<std::string> convertDriveProtocol(const std::string& proto)
{
    if (proto == "xyz.openbmc_project.Inventory.Item.Drive.DriveProtocol.SAS")
    {
        return "SAS";
    }
    if (proto == "xyz.openbmc_project.Inventory.Item.Drive.DriveProtocol.SATA")
    {
        return "SATA";
    }
    if (proto == "xyz.openbmc_project.Inventory.Item.Drive.DriveProtocol.NVMe")
    {
        return "NVMe";
    }
    if (proto == "xyz.openbmc_project.Inventory.Item.Drive.DriveProtocol.FC")
    {
        return "FC";
    }

    return std::nullopt;
}

inline void
    getDriveItemProperties(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& connectionName,
                           const std::string& path)
{
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, connectionName, path,
        "xyz.openbmc_project.Inventory.Item.Drive",
        [asyncResp](const boost::system::error_code ec,
                    const std::vector<
                        std::pair<std::string, dbus::utility::DbusVariantType>>&
                        propertiesList) {
            if (ec)
            {
                // this interface isn't required
                return;
            }
            for (const std::pair<std::string, dbus::utility::DbusVariantType>&
                     property : propertiesList)
            {
                const std::string& propertyName = property.first;
                if (propertyName == "Type")
                {
                    const std::string* value =
                        std::get_if<std::string>(&property.second);
                    if (value == nullptr)
                    {
                        // illegal property
                        BMCWEB_LOG_ERROR << "Illegal property: Type";
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    std::optional<std::string> mediaType =
                        convertDriveType(*value);
                    if (!mediaType)
                    {
                        BMCWEB_LOG_ERROR << "Unsupported DriveType Interface: "
                                         << *value;
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    asyncResp->res.jsonValue["MediaType"] = *mediaType;
                }
                else if (propertyName == "Capacity")
                {
                    const uint64_t* capacity =
                        std::get_if<uint64_t>(&property.second);
                    if (capacity == nullptr)
                    {
                        BMCWEB_LOG_ERROR << "Illegal property: Capacity";
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    if (*capacity == 0)
                    {
                        // drive capacity not known
                        continue;
                    }

                    asyncResp->res.jsonValue["CapacityBytes"] = *capacity;
                }
                else if (propertyName == "Protocol")
                {
                    const std::string* value =
                        std::get_if<std::string>(&property.second);
                    if (value == nullptr)
                    {
                        BMCWEB_LOG_ERROR << "Illegal property: Protocol";
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    std::optional<std::string> proto =
                        convertDriveProtocol(*value);
                    if (!proto)
                    {
                        BMCWEB_LOG_ERROR
                            << "Unsupported DrivePrototype Interface: "
                            << *value;
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    asyncResp->res.jsonValue["Protocol"] = *proto;
                }
            }
        });
}

void requestDriveHandler(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::optional<std::string>& storageId,
                         const std::string& driveId,
                         const boost::system::error_code ec,
                         const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR << "Drive mapper call error";
        messages::internalError(asyncResp->res);
        return;
    }

    auto drive = std::find_if(
        subtree.begin(), subtree.end(),
        [&driveId](
            const std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>&
                object) {
            return sdbusplus::message::object_path(object.first).filename() ==
                   driveId;
        });

    if (drive == subtree.end())
    {
        messages::resourceNotFound(asyncResp->res, "Drive", driveId);
        return;
    }

    const std::string& path = drive->first;
    const std::vector<std::pair<std::string, std::vector<std::string>>>&
        connectionNames = drive->second;

    asyncResp->res.jsonValue["@odata.type"] = "#Drive.v1_7_0.Drive";
    asyncResp->res.jsonValue["@odata.id"] =
        storageId != std::nullopt
            ? "/redfish/v1/Storage/" + *storageId + "/Drives/" + driveId
            : "/redfish/v1/Systems/system/Storage/1/Drives/" + driveId;
    asyncResp->res.jsonValue["Name"] = driveId;
    asyncResp->res.jsonValue["Id"] = driveId;

    if (connectionNames.size() != 1)
    {
        BMCWEB_LOG_ERROR << "Connection size " << connectionNames.size()
                         << ", not equal to 1";
        messages::internalError(asyncResp->res);
        return;
    }

    if (storageId == std::nullopt)
    {
        getMainChassisId(
            asyncResp, [](const std::string& chassisId,
                          const std::shared_ptr<bmcweb::AsyncResp>& aRsp) {
                aRsp->res.jsonValue["Links"]["Chassis"]["@odata.id"] =
                    "/redfish/v1/Chassis/" + chassisId;
            });
    }
    else
    {
        getChassisId(asyncResp, path,
                     [asyncResp](const std::string& chassisId) {
                         asyncResp->res.jsonValue["Links"]["Chassis"] = {
                             {"@odata.id", "/redfish/v1/Chassis/" + chassisId}};
                     });
    }

    // default it to Enabled
    asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

    auto health = std::make_shared<HealthPopulate>(asyncResp);
    health->inventory.emplace_back(path);
    health->populate();

    const std::string& connectionName = connectionNames[0].first;

    for (const std::string& interface : connectionNames[0].second)
    {
        if (interface == "xyz.openbmc_project.Inventory.Decorator.Asset")
        {
            getDriveAsset(asyncResp, connectionName, path);
        }
        else if (interface == "xyz.openbmc_project.Inventory.Item")
        {
            getDrivePresent(asyncResp, connectionName, path);
        }
        else if (interface == "xyz.openbmc_project.State.Drive")
        {
            getDriveState(asyncResp, connectionName, path);
        }
        else if (interface == "xyz.openbmc_project.Inventory.Item.Drive")
        {
            getDriveItemProperties(asyncResp, connectionName, path);
        }
        else if (interface ==
                 "xyz.openbmc_project.Inventory.Decorator.LocationCode")
        {
            location_util::getLocationCode(asyncResp, connectionName, path,
                                           "/PhysicalLocation"_json_pointer);
        }
        else
        {
            std::optional<std::string> locationType =
                location_util::getLocationType(interface);
            if (!locationType)
            {
                BMCWEB_LOG_DEBUG << "getLocationType for Drive failed for "
                                 << interface;
                continue;
            }
            asyncResp->res
                .jsonValue["PhysicalLocation"]["PartLocation"]["LocationType"] =
                *locationType;
        }
    }
}

inline void requestRoutesDrive(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/Storage/1/Drives/<str>/")
        .privileges(redfish::privileges::getDrive)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& driveId) {
                if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
                {
                    return;
                }
                crow::connections::systemBus->async_method_call(
                    [asyncResp,
                     driveId](const boost::system::error_code ec,
                              const dbus::utility::MapperGetSubTreeResponse&
                                  subtree) {
                        requestDriveHandler(asyncResp, std::nullopt, driveId,
                                            ec, subtree);
                    },
                    "xyz.openbmc_project.ObjectMapper",
                    "/xyz/openbmc_project/object_mapper",
                    "xyz.openbmc_project.ObjectMapper", "GetSubTree",
                    "/xyz/openbmc_project/inventory", int32_t(0),
                    std::array<const char*, 1>{
                        "xyz.openbmc_project.Inventory.Item.Drive"});
            });

    BMCWEB_ROUTE(app, "/redfish/v1/Storage/<str>/Drives/<str>/")
        .privileges(redfish::privileges::getDrive)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& storageId, const std::string& driveId) {
                if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
                {
                    return;
                }
                crow::connections::systemBus->async_method_call(
                    [asyncResp, storageId,
                     driveId](const boost::system::error_code ec,
                              const dbus::utility::MapperGetSubTreeResponse&
                                  subtree) {
                        requestDriveHandler(asyncResp, storageId, driveId, ec,
                                            subtree);
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
