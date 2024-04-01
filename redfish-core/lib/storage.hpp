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

#include "bmcweb_config.h"

#include "app.hpp"
#include "dbus_utility.hpp"
#include "generated/enums/drive.hpp"
#include "generated/enums/protocol.hpp"
#include "human_sort.hpp"
#include "openbmc_dbus_rest.hpp"
#include "query.hpp"
#include "redfish_util.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/collection.hpp"
#include "utils/dbus_utils.hpp"

#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <ranges>
#include <string_view>

namespace redfish
{

inline void handleSystemsStorageCollectionGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName)
{
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

    constexpr std::array<std::string_view, 1> interface{
        "xyz.openbmc_project.Inventory.Item.Storage"};
    collection_util::getCollectionMembers(
        asyncResp, boost::urls::format("/redfish/v1/Systems/system/Storage"),
        interface, "/xyz/openbmc_project/inventory");
}

inline void handleStorageCollectionGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.jsonValue["@odata.type"] =
        "#StorageCollection.StorageCollection";
    asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/Storage";
    asyncResp->res.jsonValue["Name"] = "Storage Collection";
    constexpr std::array<std::string_view, 1> interface{
        "xyz.openbmc_project.Inventory.Item.Storage"};
    collection_util::getCollectionMembers(
        asyncResp, boost::urls::format("/redfish/v1/Storage"), interface,
        "/xyz/openbmc_project/inventory");
}

inline void requestRoutesStorageCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Storage/")
        .privileges(redfish::privileges::getStorageCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleSystemsStorageCollectionGet, std::ref(app)));
    BMCWEB_ROUTE(app, "/redfish/v1/Storage/")
        .privileges(redfish::privileges::getStorageCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleStorageCollectionGet, std::ref(app)));
}

inline void afterChassisDriveCollectionSubtree(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& driveList)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("Drive mapper call error");
        messages::internalError(asyncResp->res);
        return;
    }

    nlohmann::json& driveArray = asyncResp->res.jsonValue["Drives"];
    driveArray = nlohmann::json::array();
    auto& count = asyncResp->res.jsonValue["Drives@odata.count"];
    count = 0;

    for (const std::string& drive : driveList)
    {
        sdbusplus::message::object_path object(drive);
        if (object.filename().empty())
        {
            BMCWEB_LOG_ERROR("Failed to find filename in {}", drive);
            return;
        }

        nlohmann::json::object_t driveJson;
        driveJson["@odata.id"] = boost::urls::format(
            "/redfish/v1/Systems/system/Storage/1/Drives/{}",
            object.filename());
        driveArray.emplace_back(std::move(driveJson));
    }

    count = driveArray.size();
}
inline void getDrives(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    const std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Drive"};
    dbus::utility::getSubTreePaths(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        std::bind_front(afterChassisDriveCollectionSubtree, asyncResp));
}

inline void afterSystemsStorageGetSubtree(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& storageId, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        BMCWEB_LOG_DEBUG("requestRoutesStorage DBUS response error");
        messages::resourceNotFound(asyncResp->res, "#Storage.v1_13_0.Storage",
                                   storageId);
        return;
    }
    auto storage = std::ranges::find_if(
        subtree,
        [&storageId](const std::pair<std::string,
                                     dbus::utility::MapperServiceMap>& object) {
        return sdbusplus::message::object_path(object.first).filename() ==
               storageId;
    });
    if (storage == subtree.end())
    {
        messages::resourceNotFound(asyncResp->res, "#Storage.v1_13_0.Storage",
                                   storageId);
        return;
    }

    asyncResp->res.jsonValue["@odata.type"] = "#Storage.v1_13_0.Storage";
    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Systems/system/Storage/{}", storageId);
    asyncResp->res.jsonValue["Name"] = "Storage";
    asyncResp->res.jsonValue["Id"] = storageId;
    asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

    getDrives(asyncResp);
    asyncResp->res.jsonValue["Controllers"]["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/system/Storage/{}/Controllers", storageId);
}

inline void
    handleSystemsStorageGet(App& app, const crow::Request& req,
                            const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& systemName,
                            const std::string& storageId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if constexpr (bmcwebEnableMultiHost)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Storage"};
    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        std::bind_front(afterSystemsStorageGetSubtree, asyncResp, storageId));
}

inline void afterSubtree(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& storageId,
                         const boost::system::error_code& ec,
                         const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        BMCWEB_LOG_DEBUG("requestRoutesStorage DBUS response error");
        messages::resourceNotFound(asyncResp->res, "#Storage.v1_13_0.Storage",
                                   storageId);
        return;
    }
    auto storage = std::ranges::find_if(
        subtree,
        [&storageId](const std::pair<std::string,
                                     dbus::utility::MapperServiceMap>& object) {
        return sdbusplus::message::object_path(object.first).filename() ==
               storageId;
    });
    if (storage == subtree.end())
    {
        messages::resourceNotFound(asyncResp->res, "#Storage.v1_13_0.Storage",
                                   storageId);
        return;
    }

    asyncResp->res.jsonValue["@odata.type"] = "#Storage.v1_13_0.Storage";
    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Storage/{}", storageId);
    asyncResp->res.jsonValue["Name"] = "Storage";
    asyncResp->res.jsonValue["Id"] = storageId;
    asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

    // Storage subsystem to Storage link.
    nlohmann::json::array_t storageServices;
    nlohmann::json::object_t storageService;
    storageService["@odata.id"] =
        boost::urls::format("/redfish/v1/Systems/system/Storage/{}", storageId);
    storageServices.emplace_back(storageService);
    asyncResp->res.jsonValue["Links"]["StorageServices"] =
        std::move(storageServices);
    asyncResp->res.jsonValue["Links"]["StorageServices@odata.count"] = 1;
}

inline void
    handleStorageGet(App& app, const crow::Request& req,
                     const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const std::string& storageId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        BMCWEB_LOG_DEBUG("requestRoutesStorage setUpRedfishRoute failed");
        return;
    }

    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Storage"};
    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        std::bind_front(afterSubtree, asyncResp, storageId));
}

inline void requestRoutesStorage(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Storage/<str>/")
        .privileges(redfish::privileges::getStorage)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleSystemsStorageGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Storage/<str>/")
        .privileges(redfish::privileges::getStorage)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleStorageGet, std::ref(app)));
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

inline std::optional<drive::MediaType> convertDriveType(std::string_view type)
{
    if (type == "xyz.openbmc_project.Inventory.Item.Drive.DriveType.HDD")
    {
        return drive::MediaType::HDD;
    }
    if (type == "xyz.openbmc_project.Inventory.Item.Drive.DriveType.SSD")
    {
        return drive::MediaType::SSD;
    }
    if (type == "xyz.openbmc_project.Inventory.Item.Drive.DriveType.Unknown")
    {
        return std::nullopt;
    }

    return drive::MediaType::Invalid;
}

inline std::optional<protocol::Protocol>
    convertDriveProtocol(std::string_view proto)
{
    if (proto == "xyz.openbmc_project.Inventory.Item.Drive.DriveProtocol.SAS")
    {
        return protocol::Protocol::SAS;
    }
    if (proto == "xyz.openbmc_project.Inventory.Item.Drive.DriveProtocol.SATA")
    {
        return protocol::Protocol::SATA;
    }
    if (proto == "xyz.openbmc_project.Inventory.Item.Drive.DriveProtocol.NVMe")
    {
        return protocol::Protocol::NVMe;
    }
    if (proto == "xyz.openbmc_project.Inventory.Item.Drive.DriveProtocol.FC")
    {
        return protocol::Protocol::FC;
    }
    if (proto ==
        "xyz.openbmc_project.Inventory.Item.Drive.DriveProtocol.Unknown")
    {
        return std::nullopt;
    }

    return protocol::Protocol::Invalid;
}

inline void
    getDriveItemProperties(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& connectionName,
                           const std::string& path)
{
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, connectionName, path,
        "xyz.openbmc_project.Inventory.Item.Drive",
        [asyncResp](const boost::system::error_code& ec,
                    const std::vector<
                        std::pair<std::string, dbus::utility::DbusVariantType>>&
                        propertiesList) {
        if (ec)
        {
            // this interface isn't required
            return;
        }
        const std::string* encryptionStatus = nullptr;
        const bool* isLocked = nullptr;
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
                    BMCWEB_LOG_ERROR("Illegal property: Type");
                    messages::internalError(asyncResp->res);
                    return;
                }

                std::optional<drive::MediaType> mediaType =
                    convertDriveType(*value);
                if (!mediaType)
                {
                    BMCWEB_LOG_WARNING("UnknownDriveType Interface: {}",
                                       *value);
                    continue;
                }
                if (*mediaType == drive::MediaType::Invalid)
                {
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
                    BMCWEB_LOG_ERROR("Illegal property: Capacity");
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
                    BMCWEB_LOG_ERROR("Illegal property: Protocol");
                    messages::internalError(asyncResp->res);
                    return;
                }

                std::optional<protocol::Protocol> proto =
                    convertDriveProtocol(*value);
                if (!proto)
                {
                    BMCWEB_LOG_WARNING("Unknown DrivePrototype Interface: {}",
                                       *value);
                    continue;
                }
                if (*proto == protocol::Protocol::Invalid)
                {
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
                    BMCWEB_LOG_ERROR(
                        "Illegal property: PredictedMediaLifeLeftPercent");
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
            else if (propertyName == "EncryptionStatus")
            {
                encryptionStatus = std::get_if<std::string>(&property.second);
                if (encryptionStatus == nullptr)
                {
                    BMCWEB_LOG_ERROR("Illegal property: EncryptionStatus");
                    messages::internalError(asyncResp->res);
                    return;
                }
            }
            else if (propertyName == "Locked")
            {
                isLocked = std::get_if<bool>(&property.second);
                if (isLocked == nullptr)
                {
                    BMCWEB_LOG_ERROR("Illegal property: Locked");
                    messages::internalError(asyncResp->res);
                    return;
                }
            }
        }

        if (encryptionStatus == nullptr || isLocked == nullptr ||
            *encryptionStatus ==
                "xyz.openbmc_project.Inventory.Item.Drive.DriveEncryptionState.Unknown")
        {
            return;
        }
        if (*encryptionStatus !=
            "xyz.openbmc_project.Inventory.Item.Drive.DriveEncryptionState.Encrypted")
        {
            //"The drive is not currently encrypted."
            asyncResp->res.jsonValue["EncryptionStatus"] =
                drive::EncryptionStatus::Unencrypted;
            return;
        }
        if (*isLocked)
        {
            //"The drive is currently encrypted and the data is not
            // accessible to the user."
            asyncResp->res.jsonValue["EncryptionStatus"] =
                drive::EncryptionStatus::Locked;
            return;
        }
        // if not locked
        // "The drive is currently encrypted but the data is accessible
        // to the user in unencrypted form."
        asyncResp->res.jsonValue["EncryptionStatus"] =
            drive::EncryptionStatus::Unlocked;
    });
}

static void addAllDriveInfo(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& connectionName,
                            const std::string& path,
                            const std::vector<std::string>& interfaces)
{
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
            getDriveState(asyncResp, connectionName, path);
        }
        else if (interface == "xyz.openbmc_project.Inventory.Item.Drive")
        {
            getDriveItemProperties(asyncResp, connectionName, path);
        }
    }
}

inline void afterGetSubtreeSystemsStorageDrive(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& driveId, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("Drive mapper call error");
        messages::internalError(asyncResp->res);
        return;
    }

    auto drive = std::ranges::find_if(
        subtree,
        [&driveId](const std::pair<std::string,
                                   dbus::utility::MapperServiceMap>& object) {
        return sdbusplus::message::object_path(object.first).filename() ==
               driveId;
    });

    if (drive == subtree.end())
    {
        messages::resourceNotFound(asyncResp->res, "Drive", driveId);
        return;
    }

    const std::string& path = drive->first;
    const dbus::utility::MapperServiceMap& connectionNames = drive->second;

    asyncResp->res.jsonValue["@odata.type"] = "#Drive.v1_7_0.Drive";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/system/Storage/1/Drives/{}", driveId);
    asyncResp->res.jsonValue["Name"] = driveId;
    asyncResp->res.jsonValue["Id"] = driveId;

    if (connectionNames.size() != 1)
    {
        BMCWEB_LOG_ERROR("Connection size {}, not equal to 1",
                         connectionNames.size());
        messages::internalError(asyncResp->res);
        return;
    }

    getMainChassisId(asyncResp,
                     [](const std::string& chassisId,
                        const std::shared_ptr<bmcweb::AsyncResp>& aRsp) {
        aRsp->res.jsonValue["Links"]["Chassis"]["@odata.id"] =
            boost::urls::format("/redfish/v1/Chassis/{}", chassisId);
    });

    // default it to Enabled
    asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

    addAllDriveInfo(asyncResp, connectionNames[0].first, path,
                    connectionNames[0].second);
}

inline void handleSystemsStorageDriveGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& driveId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if constexpr (bmcwebEnableMultiHost)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    if (systemName != "system")
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Drive"};
    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        std::bind_front(afterGetSubtreeSystemsStorageDrive, asyncResp,
                        driveId));
}

inline void requestRoutesDrive(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Storage/1/Drives/<str>/")
        .privileges(redfish::privileges::getDrive)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleSystemsStorageDriveGet, std::ref(app)));
}

inline void afterChassisDriveCollectionSubtreeGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        if (ec == boost::system::errc::host_unreachable)
        {
            messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
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
            BMCWEB_LOG_ERROR("Got 0 Connection names");
            continue;
        }

        asyncResp->res.jsonValue["@odata.type"] =
            "#DriveCollection.DriveCollection";
        asyncResp->res.jsonValue["@odata.id"] =
            boost::urls::format("/redfish/v1/Chassis/{}/Drives", chassisId);
        asyncResp->res.jsonValue["Name"] = "Drive Collection";

        // Association lambda
        dbus::utility::getAssociationEndPoints(
            path + "/drive",
            [asyncResp, chassisId](const boost::system::error_code& ec3,
                                   const dbus::utility::MapperEndPoints& resp) {
            if (ec3)
            {
                BMCWEB_LOG_ERROR("Error in chassis Drive association ");
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

            std::ranges::sort(leafNames, AlphanumLess<std::string>());

            for (const auto& leafName : leafNames)
            {
                nlohmann::json::object_t member;
                member["@odata.id"] = boost::urls::format(
                    "/redfish/v1/Chassis/{}/Drives/{}", chassisId, leafName);
                members.emplace_back(std::move(member));
                // navigation links will be registered in next patch set
            }
            asyncResp->res.jsonValue["Members@odata.count"] = resp.size();
        }); // end association lambda

    }       // end Iterate over all retrieved ObjectPaths
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
        std::bind_front(afterChassisDriveCollectionSubtreeGet, asyncResp,
                        chassisId));
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
        BMCWEB_LOG_DEBUG("DBUS response error {}", ec);
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
            BMCWEB_LOG_ERROR("Got 0 Connection names");
            continue;
        }

        asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
            "/redfish/v1/Chassis/{}/Drives/{}", chassisId, driveName);

        asyncResp->res.jsonValue["@odata.type"] = "#Drive.v1_7_0.Drive";
        asyncResp->res.jsonValue["Name"] = driveName;
        asyncResp->res.jsonValue["Id"] = driveName;
        // default it to Enabled
        asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

        nlohmann::json::object_t linkChassisNav;
        linkChassisNav["@odata.id"] =
            boost::urls::format("/redfish/v1/Chassis/{}", chassisId);
        asyncResp->res.jsonValue["Links"]["Chassis"] = linkChassisNav;

        addAllDriveInfo(asyncResp, connectionNames[0].first, path,
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
    constexpr std::array<std::string_view, 2> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Board",
        "xyz.openbmc_project.Inventory.Item.Chassis"};

    // mapper call chassis
    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [asyncResp, chassisId,
         driveName](const boost::system::error_code& ec,
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
                BMCWEB_LOG_ERROR("Got 0 Connection names");
                continue;
            }

            dbus::utility::getAssociationEndPoints(
                path + "/drive",
                [asyncResp, chassisId,
                 driveName](const boost::system::error_code& ec3,
                            const dbus::utility::MapperEndPoints& resp) {
                if (ec3)
                {
                    return; // no drives = no failures
                }
                matchAndFillDrive(asyncResp, chassisId, driveName, resp);
            });
            break;
        }
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

inline void getStorageControllerAsset(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const std::vector<std::pair<std::string, dbus::utility::DbusVariantType>>&
        propertiesList)
{
    if (ec)
    {
        // this interface isn't necessary
        BMCWEB_LOG_DEBUG("Failed to get StorageControllerAsset");
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
    const std::string& controllerId, const std::string& connectionName,
    const std::string& path)
{
    asyncResp->res.jsonValue["@odata.type"] =
        "#StorageController.v1_6_0.StorageController";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/system/Storage/1/Controllers/{}", controllerId);
    asyncResp->res.jsonValue["Name"] = controllerId;
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
            BMCWEB_LOG_DEBUG("Failed to get Present property");
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
    const std::string& controllerId, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec || subtree.empty())
    {
        // doesn't have to be there
        BMCWEB_LOG_DEBUG("Failed to handle StorageController");
        return;
    }

    for (const auto& [path, interfaceDict] : subtree)
    {
        sdbusplus::message::object_path object(path);
        std::string id = object.filename();
        if (id.empty())
        {
            BMCWEB_LOG_ERROR("Failed to find filename in {}", path);
            return;
        }
        if (id != controllerId)
        {
            continue;
        }

        if (interfaceDict.size() != 1)
        {
            BMCWEB_LOG_ERROR("Connection size {}, greater than 1",
                             interfaceDict.size());
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string& connectionName = interfaceDict.front().first;
        populateStorageController(asyncResp, controllerId, connectionName,
                                  path);
    }
}

inline void populateStorageControllerCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& controllerList)
{
    nlohmann::json::array_t members;
    if (ec || controllerList.empty())
    {
        asyncResp->res.jsonValue["Members"] = std::move(members);
        asyncResp->res.jsonValue["Members@odata.count"] = 0;
        BMCWEB_LOG_DEBUG("Failed to find any StorageController");
        return;
    }

    for (const std::string& path : controllerList)
    {
        std::string id = sdbusplus::message::object_path(path).filename();
        if (id.empty())
        {
            BMCWEB_LOG_ERROR("Failed to find filename in {}", path);
            return;
        }
        nlohmann::json::object_t member;
        member["@odata.id"] = boost::urls::format(
            "/redfish/v1/Systems/system/Storage/1/Controllers/{}", id);
        members.emplace_back(member);
    }
    asyncResp->res.jsonValue["Members@odata.count"] = members.size();
    asyncResp->res.jsonValue["Members"] = std::move(members);
}

inline void handleSystemsStorageControllerCollectionGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        BMCWEB_LOG_DEBUG(
            "Failed to setup Redfish Route for StorageController Collection");
        return;
    }
    if (systemName != "system")
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        BMCWEB_LOG_DEBUG("Failed to find ComputerSystem of {}", systemName);
        return;
    }

    asyncResp->res.jsonValue["@odata.type"] =
        "#StorageControllerCollection.StorageControllerCollection";
    asyncResp->res.jsonValue["@odata.id"] =
        "/redfish/v1/Systems/system/Storage/1/Controllers";
    asyncResp->res.jsonValue["Name"] = "Storage Controller Collection";

    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Inventory.Item.StorageController"};
    dbus::utility::getSubTreePaths(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [asyncResp](const boost::system::error_code& ec,
                    const dbus::utility::MapperGetSubTreePathsResponse&
                        controllerList) {
        populateStorageControllerCollection(asyncResp, ec, controllerList);
    });
}

inline void handleSystemsStorageControllerGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& controllerId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        BMCWEB_LOG_DEBUG("Failed to setup Redfish Route for StorageController");
        return;
    }
    if (systemName != "system")
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        BMCWEB_LOG_DEBUG("Failed to find ComputerSystem of {}", systemName);
        return;
    }
    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Inventory.Item.StorageController"};
    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [asyncResp,
         controllerId](const boost::system::error_code& ec,
                       const dbus::utility::MapperGetSubTreeResponse& subtree) {
        getStorageControllerHandler(asyncResp, controllerId, ec, subtree);
    });
}

inline void requestRoutesStorageControllerCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Storage/1/Controllers/")
        .privileges(redfish::privileges::getStorageControllerCollection)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleSystemsStorageControllerCollectionGet, std::ref(app)));
}

inline void requestRoutesStorageController(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Storage/1/Controllers/<str>")
        .privileges(redfish::privileges::getStorageController)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleSystemsStorageControllerGet, std::ref(app)));
}

} // namespace redfish
