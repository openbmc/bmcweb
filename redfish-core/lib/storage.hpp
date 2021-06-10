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

#include "app.hpp"
#include "dbus_utility.hpp"
#include "health.hpp"
#include "human_sort.hpp"
#include "openbmc_dbus_rest.hpp"
#include "query.hpp"
#include "redfish_util.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/dbus_utils.hpp"

#include <boost/system/error_code.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <string_view>
#include <unordered_set>

namespace redfish
{
inline void requestRoutesStorageCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Storage/")
        .privileges(redfish::privileges::getStorageCollection)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }

        asyncResp->res.jsonValue["@odata.type"] =
            "#StorageCollection.StorageCollection";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/Storage";
        asyncResp->res.jsonValue["Name"] = "Storage Collection";

        constexpr std::array<std::string_view, 1> interface {
            "xyz.openbmc_project.Inventory.Item.Storage"
        };
        collection_util::getCollectionMembers(
            asyncResp,
            crow::utility::urlFromPieces("redfish", "v1", "Systems", "system",
                                         "Storage"),
            interface);
        });

    BMCWEB_ROUTE(app, "/redfish/v1/Storage/")
        .privileges(redfish::privileges::getStorageCollection)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        asyncResp->res.jsonValue["@odata.type"] =
            "#StorageCollection.StorageCollection";
        asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/Storage";
        asyncResp->res.jsonValue["Name"] = "Storage Collection";
        constexpr std::array<std::string_view, 1> interface {
            "xyz.openbmc_project.Inventory.Item.Storage"
        };
        collection_util::getCollectionMembers(
            asyncResp, crow::utility::urlFromPieces("redfish", "v1", "Storage"),
            interface);
        });
}

inline void getDrives(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::shared_ptr<HealthPopulate>& health,
                      const sdbusplus::message::object_path& storagePath,
                      const std::string& chassisId)
{
    const std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Drive"};
    dbus::utility::getAssociatedSubTreePaths(
        storagePath / "drive",
        sdbusplus::message::object_path("/xyz/openbmc_project/inventory"), 0,
        interfaces,
        [asyncResp, health, chassisId](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreePathsResponse& driveList) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "Drive mapper call error";
            messages::internalError(asyncResp->res);
            return;
        }

        nlohmann::json& driveArray = asyncResp->res.jsonValue["Drives"];
        driveArray = nlohmann::json::array();
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

            nlohmann::json::object_t driveJson;
            driveJson["@odata.id"] = crow::utility::urlFromPieces(
                "redfish", "v1", "Chassis", chassisId, "Drives",
                object.filename());
            driveArray.push_back(std::move(driveJson));
        }

        count = driveArray.size();
        });
}

inline void
    getDriveFromChassis(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::shared_ptr<HealthPopulate>& health,
                        const sdbusplus::message::object_path& storagePath)
{
    const std::array<std::string_view, 2> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Board",
        "xyz.openbmc_project.Inventory.Item.Chassis"};
    dbus::utility::getAssociatedSubTreePaths(
        storagePath / "chassis",
        sdbusplus::message::object_path("/xyz/openbmc_project/inventory"), 0,
        interfaces,
        [asyncResp, health, storagePath](
            const boost::system::error_code ec,
            const dbus::utility::MapperGetSubTreePathsResponse& chassisList) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "Chassis mapper call error";
            messages::internalError(asyncResp->res);
            return;
        }
        if (chassisList.size() != 1)
        {
            BMCWEB_LOG_ERROR
                << "Storage is not associated with only one chassis";
            messages::internalError(asyncResp->res);
            return;
        }

        std::string chassisPath = chassisList.front();
        std::string chassisId =
            sdbusplus::message::object_path(chassisPath).filename();
        if (chassisId.empty())
        {
            BMCWEB_LOG_ERROR << "Failed to find filename in " << chassisPath;
            return;
        }
        getDrives(asyncResp, health, storagePath, chassisId);
        });
}

inline void requestRoutesStorage(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Storage/<str>/")
        .privileges(redfish::privileges::getStorage)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName,
                   const std::string& storageId) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }

        constexpr std::array<std::string_view, 1> interfaces = {
            "xyz.openbmc_project.Inventory.Item.Storage"};
        dbus::utility::getSubTree(
            "/xyz/openbmc_project/inventory", 0, interfaces,
            [asyncResp, storageId](
                const boost::system::error_code ec,
                const dbus::utility::MapperGetSubTreeResponse& subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "requestRoutesStorage DBUS response error";
                messages::resourceNotFound(
                    asyncResp->res, "#Storage.v1_13_0.Storage", storageId);
                return;
            }
            auto storage = std::find_if(
                subtree.begin(), subtree.end(),
                [&storageId](
                    const std::pair<std::string,
                                    dbus::utility::MapperServiceMap>& object) {
                return sdbusplus::message::object_path(object.first)
                           .filename() == storageId;
                });
            if (storage == subtree.end())
            {
                messages::resourceNotFound(
                    asyncResp->res, "#Storage.v1_13_0.Storage", storageId);
                return;
            }

            asyncResp->res.jsonValue["@odata.type"] =
                "#Storage.v1_13_0.Storage";
            asyncResp->res.jsonValue["@odata.id"] =
                crow::utility::urlFromPieces("redfish", "v1", "Systems",
                                             "system", "Storage", storageId);
            asyncResp->res.jsonValue["Name"] = "Storage";
            asyncResp->res.jsonValue["Id"] = storageId;
            asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

            auto health = std::make_shared<HealthPopulate>(asyncResp);
            health->populate();

            getDriveFromChassis(asyncResp, health, storage->first);
            asyncResp->res.jsonValue["Controllers"]["@odata.id"] =
                crow::utility::urlFromPieces("redfish", "v1", "Systems",
                                             "system", "Storage", storageId,
                                             "Controllers");
            });
        });

    BMCWEB_ROUTE(app, "/redfish/v1/Storage/<str>/")
        .privileges(redfish::privileges::getStorage)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& storageId) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            BMCWEB_LOG_DEBUG << "requestRoutesStorage setUpRedfishRoute failed";
            return;
        }

        constexpr std::array<std::string_view, 1> interfaces = {
            "xyz.openbmc_project.Inventory.Item.Storage"};
        dbus::utility::getSubTree(
            "/xyz/openbmc_project/inventory", 0, interfaces,
            [asyncResp, storageId](
                const boost::system::error_code ec,
                const dbus::utility::MapperGetSubTreeResponse& subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "requestRoutesStorage DBUS response error";
                messages::resourceNotFound(
                    asyncResp->res, "#Storage.v1_13_0.Storage", storageId);
                return;
            }
            auto storage = std::find_if(
                subtree.begin(), subtree.end(),
                [&storageId](
                    const std::pair<std::string,
                                    dbus::utility::MapperServiceMap>& object) {
                return sdbusplus::message::object_path(object.first)
                           .filename() == storageId;
                });
            if (storage == subtree.end())
            {
                messages::resourceNotFound(
                    asyncResp->res, "#Storage.v1_13_0.Storage", storageId);
                return;
            }

            asyncResp->res.jsonValue["@odata.type"] =
                "#Storage.v1_13_0.Storage";
            asyncResp->res.jsonValue["@odata.id"] =
                crow::utility::urlFromPieces("redfish", "v1", "Storage",
                                             storageId);
            asyncResp->res.jsonValue["Name"] = "Storage";
            asyncResp->res.jsonValue["Id"] = storageId;
            asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

            // Storage subsystem to Stroage link.
            nlohmann::json::array_t storageServices;
            nlohmann::json::object_t storageService;
            storageService["@odata.id"] = crow::utility::urlFromPieces(
                "redfish", "v1", "Systems", "system", "Storage", storageId);
            storageServices.emplace_back(storageService);
            asyncResp->res.jsonValue["Links"]["StorageServices"] =
                std::move(storageServices);
            asyncResp->res.jsonValue["Links"]["StorageServices@odata.count"] =
                1;
            });
        });
}

inline void getDriveAsset(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& connectionName,
                          const std::string& path)
{
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, connectionName, path,
        "xyz.openbmc_project.Inventory.Decorator.Asset",
        [asyncResp](const boost::system::error_code& ec,
                    const std::vector<
                        std::pair<std::string, dbus::utility::DbusVariantType>>&
                        propertiesList) {
        if (ec)
        {
            // this interface isn't necessary
            return;
        }

        const std::string* partNumber = nullptr;
        const std::string* serialNumber = nullptr;
        const std::string* manufacturer = nullptr;
        const std::string* model = nullptr;

        const bool success = sdbusplus::unpackPropertiesNoThrow(
            dbus_utils::UnpackErrorPrinter(), propertiesList, "PartNumber",
            partNumber, "SerialNumber", serialNumber, "Manufacturer",
            manufacturer, "Model", model);

        if (!success)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        if (partNumber != nullptr)
        {
            asyncResp->res.jsonValue["PartNumber"] = *partNumber;
        }

        if (serialNumber != nullptr)
        {
            asyncResp->res.jsonValue["SerialNumber"] = *serialNumber;
        }

        if (manufacturer != nullptr)
        {
            asyncResp->res.jsonValue["Manufacturer"] = *manufacturer;
        }

        if (model != nullptr)
        {
            asyncResp->res.jsonValue["Model"] = *model;
        }
        });
}

inline void getDrivePresent(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& connectionName,
                            const std::string& path)
{
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, connectionName, path,
        "xyz.openbmc_project.Inventory.Item", "Present",
        [asyncResp, path](const boost::system::error_code& ec,
                          const bool isPresent) {
        // this interface isn't necessary, only check it if
        // we get a good return
        if (ec)
        {
            return;
        }

        if (!isPresent)
        {
            asyncResp->res.jsonValue["Status"]["State"] = "Absent";
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
        [asyncResp](const boost::system::error_code& ec, const bool updating) {
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

inline void addResetLinks(nlohmann::json& driveReset,
                          const std::string& driveId,
                          const std::string& chassisId)
{
    driveReset["target"] = crow::utility::urlFromPieces(
        "redfish", "v1", "Chassis", chassisId, "Drives", driveId, "Actions",
        "Drive.Reset");
    driveReset["@Redfish.ActionInfo"] =
        crow::utility::urlFromPieces("redfish", "v1", "Chassis", chassisId,
                                     "Drives", driveId, "ResetActionInfo");
    return;
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

inline void getDriveItemProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& driveId, const std::optional<std::string>& chassisId,
    const std::string& connectionName, const std::string& path, bool hasDriveState)
{
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, connectionName, path,
        "xyz.openbmc_project.Inventory.Item.Drive",
        [asyncResp, driveId, chassisId, hasDriveState](
            const boost::system::error_code ec,
            const dbus::utility::DBusPropertiesMap& propertiesList) {
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

                std::optional<std::string> mediaType = convertDriveType(*value);
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

                std::optional<std::string> proto = convertDriveProtocol(*value);
                if (!proto)
                {
                    BMCWEB_LOG_ERROR << "Unsupported DrivePrototype Interface: "
                                     << *value;
                    messages::internalError(asyncResp->res);
                    return;
                }
                asyncResp->res.jsonValue["Protocol"] = *proto;
            }
            else if (propertyName == "PredictedMediaLifeLeftPercent")
            {
                const uint8_t* lifeLeft =
                    std::get_if<uint8_t>(&property.second);
                if (lifeLeft == nullptr)
                {
                    BMCWEB_LOG_ERROR
                        << "Illegal property: PredictedMediaLifeLeftPercent";
                    messages::internalError(asyncResp->res);
                    return;
                }
                // 255 means reading the value is not supported
                if (*lifeLeft != 255)
                {
                    asyncResp->res.jsonValue["PredictedMediaLifeLeftPercent"] =
                        *lifeLeft;
                }
            }
            else if (propertyName == "Resettable" && hasDriveState)
            {
                const bool* value = std::get_if<bool>(&property.second);
                // If Resettable flag is not present, its not considered a
                // failure.
                if (value != nullptr && *value)
                {
                    addResetLinks(
                        asyncResp->res.jsonValue["Actions"]["#Drive.Reset"],
                        driveId, *chassisId);
                }
            }
        }
        });
}

static void
    addAllDriveInfo(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                    const std::string& driveId,
                    const std::string& connectionName, const std::string& path,
                    const std::vector<std::string>& interfaces,
                    const std::optional<std::string> chassisId = std::nullopt)
{
    bool driveInterface = false;
    bool driveStateInterface = false;
    for (const std::string& interface : interfaces)
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
            driveStateInterface = true;
            getDriveState(asyncResp, connectionName, path);
        }
        else if (interface == "xyz.openbmc_project.Inventory.Item.Drive")
        {
            driveInterface = true;
        }
    }

    if (driveInterface)
    {
        getDriveItemProperties(asyncResp, driveId, chassisId, connectionName,
                               path, driveStateInterface);
    }
}

/**
 * Chassis drives, this URL will show all the DriveCollection
 * information
 */
inline void chassisDriveCollectionGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    // mapper call lambda
    constexpr std::array<std::string_view, 2> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Board",
        "xyz.openbmc_project.Inventory.Item.Chassis"};
    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [asyncResp,
         chassisId](const boost::system::error_code& ec,
                    const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            if (ec == boost::system::errc::host_unreachable)
            {
                messages::resourceNotFound(asyncResp->res, "Chassis",
                                           chassisId);
                return;
            }
            messages::internalError(asyncResp->res);
            return;
        }

        // Iterate over all retrieved ObjectPaths.
        for (const auto& [path, connectionNames] : subtree)
        {
            sdbusplus::message::object_path objPath(path);
            if (objPath.filename() != chassisId)
            {
                continue;
            }

            if (connectionNames.empty())
            {
                BMCWEB_LOG_ERROR << "Got 0 Connection names";
                continue;
            }

            asyncResp->res.jsonValue["@odata.type"] =
                "#DriveCollection.DriveCollection";
            asyncResp->res.jsonValue["@odata.id"] =
                crow::utility::urlFromPieces("redfish", "v1", "Chassis",
                                             chassisId, "Drives");
            asyncResp->res.jsonValue["Name"] = "Drive Collection";

            // Association lambda
            dbus::utility::getAssociationEndPoints(
                path + "/drive",
                [asyncResp,
                 chassisId](const boost::system::error_code& ec3,
                            const dbus::utility::MapperEndPoints& resp) {
                if (ec3)
                {
                    BMCWEB_LOG_ERROR << "Error in chassis Drive association ";
                }
                nlohmann::json& members = asyncResp->res.jsonValue["Members"];
                // important if array is empty
                members = nlohmann::json::array();

                std::vector<std::string> leafNames;
                for (const auto& drive : resp)
                {
                    sdbusplus::message::object_path drivePath(drive);
                    leafNames.push_back(drivePath.filename());
                }

                std::sort(leafNames.begin(), leafNames.end(),
                          AlphanumLess<std::string>());

                for (const auto& leafName : leafNames)
                {
                    nlohmann::json::object_t member;
                    member["@odata.id"] = crow::utility::urlFromPieces(
                        "redfish", "v1", "Chassis", chassisId, "Drives",
                        leafName);
                    members.push_back(std::move(member));
                    // navigation links will be registered in next patch set
                }
                asyncResp->res.jsonValue["Members@odata.count"] = resp.size();
                }); // end association lambda

        } // end Iterate over all retrieved ObjectPaths
        });
}

inline void requestRoutesChassisDrive(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Drives/")
        .privileges(redfish::privileges::getDriveCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(chassisDriveCollectionGet, std::ref(app)));
}

inline void buildDrive(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& chassisId,
                       const std::string& driveName,
                       const boost::system::error_code& ec,
                       const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
        messages::internalError(asyncResp->res);
        return;
    }

    // Iterate over all retrieved ObjectPaths.
    for (const auto& [path, connectionNames] : subtree)
    {
        sdbusplus::message::object_path objPath(path);
        if (objPath.filename() != driveName)
        {
            continue;
        }

        if (connectionNames.empty())
        {
            BMCWEB_LOG_ERROR << "Got 0 Connection names";
            continue;
        }

        asyncResp->res.jsonValue["@odata.id"] = crow::utility::urlFromPieces(
            "redfish", "v1", "Chassis", chassisId, "Drives", driveName);

        asyncResp->res.jsonValue["@odata.type"] = "#Drive.v1_7_0.Drive";
        asyncResp->res.jsonValue["Name"] = driveName;
        asyncResp->res.jsonValue["Id"] = driveName;
        // default it to Enabled
        asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

        nlohmann::json::object_t linkChassisNav;
        linkChassisNav["@odata.id"] =
            crow::utility::urlFromPieces("redfish", "v1", "Chassis", chassisId);
        asyncResp->res.jsonValue["Links"]["Chassis"] = linkChassisNav;

        addAllDriveInfo(asyncResp, driveName, connectionNames[0].first, path,
                        connectionNames[0].second);
    }
}

inline void
    matchAndFillDrive(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& chassisId,
                      const std::string& driveName,
                      const std::vector<std::string>& resp)
{
    for (const std::string& drivePath : resp)
    {
        sdbusplus::message::object_path path(drivePath);
        std::string leaf = path.filename();
        if (leaf != driveName)
        {
            continue;
        }
        //  mapper call drive
        constexpr std::array<std::string_view, 1> driveInterface = {
            "xyz.openbmc_project.Inventory.Item.Drive"};
        dbus::utility::getSubTree(
            "/xyz/openbmc_project/inventory", 0, driveInterface,
            [asyncResp, chassisId, driveName](
                const boost::system::error_code& ec,
                const dbus::utility::MapperGetSubTreeResponse& subtree) {
            buildDrive(asyncResp, chassisId, driveName, ec, subtree);
            });
    }
}

// Find Chassis with chassisId and the Drives associated to it.
void findChassisDrive(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& chassisId,
                      std::function<void(const boost::system::error_code ec3,
                                         const std::vector<std::string>& resp)>
                          cb)
{
    constexpr std::array<std::string_view, 2> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Board",
        "xyz.openbmc_project.Inventory.Item.Chassis"};
    // mapper call chassis
    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [asyncResp, chassisId,
         cb](const boost::system::error_code& ec,
             const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        // Iterate over all retrieved ObjectPaths.
        for (const auto& [path, connectionNames] : subtree)
        {
            sdbusplus::message::object_path objPath(path);
            if (objPath.filename() != chassisId)
            {
                continue;
            }

            if (connectionNames.empty())
            {
                BMCWEB_LOG_ERROR << "Got 0 Connection names";
                continue;
            }

            dbus::utility::getAssociationEndPoints(path + "/drive", cb);
            break;
        }
        });
}

inline void
    handleChassisDriveGet(crow::App& app, const crow::Request& req,
                          const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& chassisId,
                          const std::string& driveName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    findChassisDrive(asyncResp, chassisId,
                     [asyncResp, chassisId,
                      driveName](const boost::system::error_code ec,
                                 const std::vector<std::string>& resp) {
        if (ec)
        {
            return; // no drives = no failures
        }
        matchAndFillDrive(asyncResp, chassisId, driveName, resp);
    });
}

/**
 * This URL will show the drive interface for the specific drive in the chassis
 */
inline void requestRoutesChassisDriveName(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Drives/<str>/")
        .privileges(redfish::privileges::getChassis)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleChassisDriveGet, std::ref(app)));
}

inline void setResetType(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& driveId, const std::string& action,
                         const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    auto driveState =
        std::find_if(subtree.begin(), subtree.end(), [&driveId](auto& object) {
            const sdbusplus::message::object_path path(object.first);
            return path.filename() == driveId;
        });

    if (driveState == subtree.end())
    {
        messages::resourceNotFound(asyncResp->res, "Drive Action", driveId);
        return;
    }

    const std::string& path = driveState->first;
    const std::vector<std::pair<std::string, std::vector<std::string>>>&
        connectionNames = driveState->second;

    if (connectionNames.size() != 1)
    {
        BMCWEB_LOG_ERROR << "Connection size " << connectionNames.size()
                         << ", not equal to 1";
        messages::internalError(asyncResp->res);
        return;
    }

    sdbusplus::asio::setProperty<std::string>(
        *crow::connections::systemBus, connectionNames[0].first, path,
        "xyz.openbmc_project.State.Drive", "RequestedDriveTransition",
        action.c_str(),
        [asyncResp, action](const boost::system::error_code ec) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "[Set] Bad D-Bus request error for " << action
                             << " : " << ec;
            messages::internalError(asyncResp->res);
            return;
        }
        messages::success(asyncResp->res);
        });
}

/**
 * Performs drive reset action.
 *
 * @param[in] asyncResp - Shared pointer for completing asynchronous calls
 * @param[in] driveId   - D-bus filename to identify the Drive
 * @param[in] resetType - Reset type for the Drive
 */
inline void
    performDriveReset(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& driveId,
                      std::optional<std::string> resetType)
{
    std::string action;
    if (!resetType || *resetType == "PowerCycle")
    {
        action = "xyz.openbmc_project.State.Drive.Transition.Powercycle";
    }
    else if (*resetType == "ForceReset")
    {
        action = "xyz.openbmc_project.State.Drive.Transition.Reboot";
    }
    else
    {
        BMCWEB_LOG_DEBUG << "Invalid property value for ResetType: "
                         << *resetType;
        messages::actionParameterNotSupported(asyncResp->res, *resetType,
                                              "ResetType");
        return;
    }

    BMCWEB_LOG_DEBUG << "Reset Drive with " << action;

    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.State.Drive"};
    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [asyncResp, driveId,
         action](const boost::system::error_code& ec,
                 const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "DBUS response error";
            messages::internalError(asyncResp->res);
            return;
        }
        setResetType(asyncResp, driveId, action, subtree);
        });
}

inline void
    handleChassisDriveReset(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& driveId,
                            std::optional<std::string> resetType,
                            const std::vector<std::string>& drives)
{
    std::unordered_set<std::string> drivesMap(drives.begin(), drives.end());
    constexpr std::array<std::string_view, 2> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Drive",
        "xyz.openbmc_project.State.Drive"};
    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [asyncResp, driveId, resetType,
         drivesMap](const boost::system::error_code& ec,
                    const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "Drive mapper call error ";
            messages::internalError(asyncResp->res);
            return;
        }

        auto drive = std::find_if(
            subtree.begin(), subtree.end(),
            [&driveId, &drivesMap](
                const std::pair<std::string, dbus::utility::MapperServiceMap>&
                    object) {
            return sdbusplus::message::object_path(object.first).filename() ==
                       driveId &&
                   drivesMap.contains(object.first);
            });

        if (drive == subtree.end())
        {
            messages::resourceNotFound(asyncResp->res, "Drive Action Reset",
                                       driveId);
            return;
        }

        const std::string& drivePath = drive->first;
        const dbus::utility::MapperServiceMap& driveConnections = drive->second;
        if (driveConnections.size() != 1)
        {
            BMCWEB_LOG_ERROR << "Connection size " << driveConnections.size()
                             << ", not equal to 1";
            messages::internalError(asyncResp->res);
            return;
        }

        bool driveInterface = false;
        bool driveStateInterface = false;
        for (const std::string& interface : driveConnections[0].second)
        {
            if (interface == "xyz.openbmc_project.Inventory.Item.Drive")
            {
                driveInterface = true;
            }
            if (interface == "xyz.openbmc_project.State.Drive")
            {
                driveStateInterface = true;
            }
        }
        if (!driveInterface || !driveStateInterface)
        {
            BMCWEB_LOG_ERROR << "Drive does not have the required interfaces ";
            messages::internalError(asyncResp->res);
            return;
        }

        sdbusplus::asio::getProperty<bool>(
            *crow::connections::systemBus, driveConnections[0].first, drivePath,
            "xyz.openbmc_project.Inventory.Item.Drive", "Resettable",
            [asyncResp, driveId, resetType](
                const boost::system::error_code propEc, bool resettable) {
            if (propEc)
            {
                BMCWEB_LOG_ERROR << "Failed to get resettable property ";
                messages::internalError(asyncResp->res);
                return;
            }
            if (!resettable)
            {
                messages::actionNotSupported(
                    asyncResp->res, "The drive does not support resets.");
                return;
            }
            performDriveReset(asyncResp, driveId, resetType);
            });
        });
}

/**
 * DriveResetAction class supports the POST method for the Reset (reboot)
 * action.
 */
inline void requestDriveResetAction(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Chassis/<str>/Drives/<str>/Actions/Drive.Reset/")
        .privileges(redfish::privileges::postDrive)
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& chassisId, const std::string& driveId) {
        BMCWEB_LOG_DEBUG << "Post Drive Reset.";

        nlohmann::json jsonRequest;
        std::optional<std::string> resetType;
        if (json_util::processJsonFromRequest(asyncResp->res, req,
                                              jsonRequest) &&
            !jsonRequest["ResetType"].empty())
        {
            resetType = jsonRequest["ResetType"];
        }

        findChassisDrive(asyncResp, chassisId,
                         [asyncResp, driveId,
                          resetType](const boost::system::error_code ec,
                                     const std::vector<std::string>& drives) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "failed to find drives";
                messages::internalError(asyncResp->res);
                return; // no drives = no failures
            }
            handleChassisDriveReset(asyncResp, driveId, resetType, drives);
        });
        });
}

inline void handleChassisDriveResetActionInfo(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& driveId,
    const std::vector<std::string>& drives)
{
    std::unordered_set<std::string> drivesMap(drives.begin(), drives.end());

    constexpr std::array<std::string_view, 2> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Drive",
        "xyz.openbmc_project.State.Drive"};
    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [asyncResp, chassisId, driveId,
         drivesMap](const boost::system::error_code ec,
                    const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "Drive mapper call error";
            messages::internalError(asyncResp->res);
            return;
        }

        auto drive = std::find_if(
            subtree.begin(), subtree.end(),
            [&driveId, &drivesMap](
                const std::pair<std::string,
                                std::vector<std::pair<
                                    std::string, std::vector<std::string>>>>&
                    object) {
            return sdbusplus::message::object_path(object.first).filename() ==
                       driveId &&
                   drivesMap.contains(object.first);
            });

        if (drive == subtree.end())
        {
            messages::resourceNotFound(asyncResp->res, "Drive ResetActionInfo",
                                       driveId);
            return;
        }

        const std::string& drivePath = drive->first;
        const dbus::utility::MapperServiceMap& driveConnections = drive->second;

        if (driveConnections.size() != 1)
        {
            BMCWEB_LOG_ERROR << "Connection size " << driveConnections.size()
                             << ", not equal to 1";
            messages::internalError(asyncResp->res);
            return;
        }

        bool driveInterface = false;
        bool driveStateInterface = false;
        for (const std::string& interface : driveConnections[0].second)
        {
            if (interface == "xyz.openbmc_project.Inventory.Item.Drive")
            {
                driveInterface = true;
            }
            if (interface == "xyz.openbmc_project.State.Drive")
            {
                driveStateInterface = true;
            }
        }
        if (!driveInterface || !driveStateInterface)
        {
            BMCWEB_LOG_ERROR << "Drive does not have the required interfaces ";
            messages::internalError(asyncResp->res);
            return;
        }

        sdbusplus::asio::getProperty<bool>(
            *crow::connections::systemBus, driveConnections[0].first, drivePath,
            "xyz.openbmc_project.Inventory.Item.Drive", "Resettable",
            [asyncResp, chassisId,
             driveId](const boost::system::error_code propEc, bool resettable) {
            if (propEc)
            {
                BMCWEB_LOG_ERROR << "Failed to get resettable property ";
                messages::internalError(asyncResp->res);
                return;
            }
            if (!resettable)
            {
                messages::actionNotSupported(
                    asyncResp->res, "The drive does not support resets.");
                return;
            }
            asyncResp->res.jsonValue["@odata.type"] =
                "#ActionInfo.v1_1_2.ActionInfo";
            asyncResp->res.jsonValue["@odata.id"] =
                crow::utility::urlFromPieces("redfish", "v1", "Chassis",
                                             chassisId, "Drives", driveId,
                                             "ResetActionInfo");
            asyncResp->res.jsonValue["Name"] = "Reset Action Info";
            asyncResp->res.jsonValue["Id"] = "ResetActionInfo";
            nlohmann::json::array_t parameters;
            nlohmann::json::object_t parameter;
            parameter["Name"] = "ResetType";
            parameter["Required"] = true;
            parameter["DataType"] = "String";
            nlohmann::json::array_t allowableValues;
            allowableValues.emplace_back("PowerCycle");
            allowableValues.emplace_back("ForceRestart");
            parameter["AllowableValues"] = std::move(allowableValues);
            parameters.emplace_back(parameter);
            asyncResp->res.jsonValue["Parameters"] = std::move(parameters);
            });
        });
}

/**
 * DriveResetActionInfo derived class for delivering Drive
 * ResetType AllowableValues using ResetInfo schema.
 */
inline void requestRoutesDriveResetActionInfo(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Drives/<str>/ResetActionInfo/")
        .privileges(redfish::privileges::getActionInfo)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& chassisId, const std::string& driveId) {
        findChassisDrive(asyncResp, chassisId,
                         [asyncResp, chassisId,
                          driveId](const boost::system::error_code ec,
                                   const std::vector<std::string>& drives) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "failed to find drives";
                messages::internalError(asyncResp->res);
                return; // no drives = no failures
            }
            handleChassisDriveResetActionInfo(asyncResp, chassisId, driveId,
                                              drives);
        });
        });
}

inline void getStorageControllerAsset(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const std::vector<std::pair<std::string, dbus::utility::DbusVariantType>>&
        propertiesList)
{
    if (ec)
    {
        // this interface isn't necessary
        BMCWEB_LOG_DEBUG << "Failed to get StorageControllerAsset";
        return;
    }

    const std::string* partNumber = nullptr;
    const std::string* serialNumber = nullptr;
    const std::string* manufacturer = nullptr;
    const std::string* model = nullptr;
    if (!sdbusplus::unpackPropertiesNoThrow(
            dbus_utils::UnpackErrorPrinter(), propertiesList, "PartNumber",
            partNumber, "SerialNumber", serialNumber, "Manufacturer",
            manufacturer, "Model", model))
    {
        messages::internalError(asyncResp->res);
        return;
    }

    if (partNumber != nullptr)
    {
        asyncResp->res.jsonValue["PartNumber"] = *partNumber;
    }

    if (serialNumber != nullptr)
    {
        asyncResp->res.jsonValue["SerialNumber"] = *serialNumber;
    }

    if (manufacturer != nullptr)
    {
        asyncResp->res.jsonValue["Manufacturer"] = *manufacturer;
    }

    if (model != nullptr)
    {
        asyncResp->res.jsonValue["Model"] = *model;
    }
}

inline void populateStorageController(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& storageId, const std::string& controllerId,
    const std::string& connectionName, const std::string& path)
{
    asyncResp->res.jsonValue["@odata.type"] =
        "#StorageController.v1_6_0.StorageController";
    asyncResp->res.jsonValue["@odata.id"] = crow::utility::urlFromPieces(
        "redfish", "v1", "Systems", "system", "Storage", storageId,
        "Controllers", controllerId);
    asyncResp->res.jsonValue["Name"] = storageId + " " + controllerId;
    asyncResp->res.jsonValue["Id"] = controllerId;
    asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, connectionName, path,
        "xyz.openbmc_project.Inventory.Item", "Present",
        [asyncResp](const boost::system::error_code& ec, bool isPresent) {
        // this interface isn't necessary, only check it
        // if we get a good return
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "Failed to get Present property";
            return;
        }
        if (!isPresent)
        {
            asyncResp->res.jsonValue["Status"]["State"] = "Absent";
        }
        });

    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, connectionName, path,
        "xyz.openbmc_project.Inventory.Decorator.Asset",
        [asyncResp](const boost::system::error_code& ec,
                    const std::vector<
                        std::pair<std::string, dbus::utility::DbusVariantType>>&
                        propertiesList) {
        getStorageControllerAsset(asyncResp, ec, propertiesList);
        });
}

inline void getStorageControllerHandler(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& storageId, const std::string& controllerId,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec || subtree.empty())
    {
        // doesn't have to be there
        BMCWEB_LOG_DEBUG << "Failed to handle StorageController";
        return;
    }

    for (const auto& [path, interfaceDict] : subtree)
    {
        sdbusplus::message::object_path object(path);
        std::string id = object.filename();
        if (id.empty())
        {
            BMCWEB_LOG_ERROR << "Failed to find filename in " << path;
            return;
        }
        if (id != controllerId)
        {
            continue;
        }

        if (interfaceDict.size() != 1)
        {
            BMCWEB_LOG_ERROR << "Connection size " << interfaceDict.size()
                             << ", greater than 1";
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string& connectionName = interfaceDict.front().first;
        populateStorageController(asyncResp, storageId, controllerId,
                                  connectionName, path);
    }
}

inline void populateStorageControllerCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec, const std::string& storageId,
    const dbus::utility::MapperGetSubTreePathsResponse& controllerList)
{
    nlohmann::json::array_t members;
    if (ec || controllerList.empty())
    {
        asyncResp->res.jsonValue["Members"] = std::move(members);
        asyncResp->res.jsonValue["Members@odata.count"] = 0;
        BMCWEB_LOG_DEBUG << "Failed to find any StorageController";
        return;
    }

    for (const std::string& path : controllerList)
    {
        std::string id = sdbusplus::message::object_path(path).filename();
        if (id.empty())
        {
            BMCWEB_LOG_ERROR << "Failed to find filename in " << path;
            return;
        }
        nlohmann::json::object_t member;
        member["@odata.id"] = crow::utility::urlFromPieces(
            "redfish", "v1", "Systems", "system", "Storage", storageId,
            "Controllers", id);
        members.emplace_back(member);
    }
    asyncResp->res.jsonValue["Members@odata.count"] = members.size();
    asyncResp->res.jsonValue["Members"] = std::move(members);
}

inline void storageControllerCollectionHandler(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& storageId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        BMCWEB_LOG_DEBUG
            << "Failed to setup Redfish Route for StorageController Collection";
        return;
    }
    if (systemName != "system")
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        BMCWEB_LOG_DEBUG << "Failed to find ComputerSystem of " << systemName;
        return;
    }

    asyncResp->res.jsonValue["@odata.type"] =
        "#StorageControllerCollection.StorageControllerCollection";
    asyncResp->res.jsonValue["@odata.id"] =
        crow::utility::urlFromPieces("redfish", "v1", "Systems", "system",
                                     "Storage", storageId, "Controllers");
    asyncResp->res.jsonValue["Name"] = "Storage Controller Collection";

    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Inventory.Item.StorageController"};
    dbus::utility::getSubTreePaths(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [asyncResp,
         storageId](const boost::system::error_code& ec,
                    const dbus::utility::MapperGetSubTreePathsResponse&
                        controllerList) {
        populateStorageControllerCollection(asyncResp, ec, storageId,
                                            controllerList);
        });
}

inline void storageControllerHandler(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& storageId,
    const std::string& controllerId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        BMCWEB_LOG_DEBUG
            << "Failed to setup Redfish Route for StorageController";
        return;
    }
    if (systemName != "system")
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        BMCWEB_LOG_DEBUG << "Failed to find ComputerSystem of " << systemName;
        return;
    }
    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Inventory.Item.StorageController"};
    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [asyncResp, storageId,
         controllerId](const boost::system::error_code& ec,
                       const dbus::utility::MapperGetSubTreeResponse& subtree) {
        getStorageControllerHandler(asyncResp, storageId, controllerId, ec,
                                    subtree);
        });
}

inline void requestRoutesStorageControllerCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Storage/<str>/Controllers/")
        .privileges(redfish::privileges::getStorageControllerCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(storageControllerCollectionHandler, std::ref(app)));
}

inline void requestRoutesStorageController(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/<str>/Storage/<str>/Controllers/<str>")
        .privileges(redfish::privileges::getStorageController)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(storageControllerHandler, std::ref(app)));
}

} // namespace redfish
