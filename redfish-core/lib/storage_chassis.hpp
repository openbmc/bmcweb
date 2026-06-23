// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2019 Intel Corporation
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "error_messages.hpp"
#include "generated/enums/drive.hpp"
#include "generated/enums/protocol.hpp"
#include "generated/enums/resource.hpp"
#include "http_request.hpp"
#include "query.hpp"
#include "redfish_util.hpp"
#include "registries/privilege_registry.hpp"

namespace redfish
{

constexpr std::array<std::string_view, 1> driveInterface = {
    "xyz.openbmc_project.Inventory.Item.Drive"};

constexpr std::array<std::string_view, 1> chassisInterface{
    "xyz.openbmc_project.Inventory.Item.Chassis",
};

inline void getDrivePresent(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& connectionName,
                            const std::string& path)
{
    dbus::utility::getProperty<bool>(
        connectionName, path, "xyz.openbmc_project.Inventory.Item", "Present",
        // ast-grep-ignore: long-lambda
        [asyncResp,
         path](const boost::system::error_code& ec, const bool isPresent) {
            // this interface isn't necessary, only check it if
            // we get a good return
            if (ec)
            {
                return;
            }

            if (!isPresent)
            {
                asyncResp->res.jsonValue["Status"]["State"] =
                    resource::State::Absent;
            }
        });
}

inline void getDriveState(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& connectionName,
                          const std::string& path)
{
    dbus::utility::getProperty<bool>(
        connectionName, path, "xyz.openbmc_project.State.Drive", "Rebuilding",
        // ast-grep-ignore: long-lambda
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
                asyncResp->res.jsonValue["Status"]["State"] =
                    resource::State::Updating;
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

inline std::optional<protocol::Protocol> convertDriveProtocol(
    std::string_view proto)
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

inline void getDriveItemProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& connectionName, const std::string& path)
{
    dbus::utility::getAllProperties(
        connectionName, path, "xyz.openbmc_project.Inventory.Item.Drive",
        // ast-grep-ignore: long-lambda
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
                        BMCWEB_LOG_WARNING(
                            "Unknown DrivePrototype Interface: {}", *value);
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
                        asyncResp->res
                            .jsonValue["PredictedMediaLifeLeftPercent"] =
                            *lifeLeft;
                    }
                }
                else if (propertyName == "EncryptionStatus")
                {
                    encryptionStatus =
                        std::get_if<std::string>(&property.second);
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

inline void addAllDriveInfo(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& connectionName,
                            const std::string& path,
                            const std::vector<std::string>& interfaces)
{
    for (const std::string& interface : interfaces)
    {
        if (interface == "xyz.openbmc_project.Inventory.Decorator.Asset")
        {
            asset_utils::getAssetInfo(asyncResp, connectionName, path,
                                      ""_json_pointer, false);
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
    const std::string& driveId, const std::string& systemName,
    const boost::system::error_code& ec,
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
            return sdbusplus::object_path(object.first).filename() == driveId;
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
        "/redfish/v1/Systems/{}/Storage/1/Drives/{}", systemName, driveId);
    asyncResp->res.jsonValue["Name"] = driveId;
    asyncResp->res.jsonValue["Id"] = driveId;

    if (connectionNames.size() != 1)
    {
        BMCWEB_LOG_ERROR("Connection size {}, not equal to 1",
                         connectionNames.size());
        messages::internalError(asyncResp->res);
        return;
    }

    getMainChassisId(
        asyncResp, [](const std::string& chassisId,
                      const std::shared_ptr<bmcweb::AsyncResp>& aRsp) {
            aRsp->res.jsonValue["Links"]["Chassis"]["@odata.id"] =
                boost::urls::format("/redfish/v1/Chassis/{}", chassisId);
        });

    // default it to Enabled
    asyncResp->res.jsonValue["Status"]["State"] = resource::State::Enabled;

    addAllDriveInfo(asyncResp, connectionNames[0].first, path,
                    connectionNames[0].second);
}

inline void afterhandleChassisDriveGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& driveId,
    const boost::system::error_code& ec,
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
            return sdbusplus::object_path(object.first).filename() == driveId;
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
        "/redfish/v1/Chassis/{}/Drives/{}", chassisId, driveId);
    asyncResp->res.jsonValue["Name"] = driveId;
    asyncResp->res.jsonValue["Id"] = driveId;

    if (connectionNames.size() != 1)
    {
        BMCWEB_LOG_ERROR("Connection size {}, not equal to 1",
                         connectionNames.size());
        messages::internalError(asyncResp->res);
        return;
    }

    asyncResp->res.jsonValue["Links"]["Chassis"]["@odata.id"] =
        boost::urls::format("/redfish/v1/Chassis/{}", chassisId);

    // default it to Enabled
    asyncResp->res.jsonValue["Status"]["State"] = resource::State::Enabled;

    addAllDriveInfo(asyncResp, connectionNames[0].first, path,
                    connectionNames[0].second);
}

inline void afterChassisDriveCollectionSubtreeGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId,
    const std::optional<std::string>& validChassisPath)
{
    if (!validChassisPath)
    {
        BMCWEB_LOG_WARNING("Not a valid chassis ID{}", chassisId);
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
        return;
    }

    asyncResp->res.jsonValue["@odata.type"] =
        "#DriveCollection.DriveCollection";
    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Chassis/{}/Drives", chassisId);
    asyncResp->res.jsonValue["Name"] = "Drive Collection";

    dbus::utility::getAssociatedSubTreePathsById(
        chassisId, "/xyz/openbmc_project/inventory", chassisInterface,
        // ast-grep-ignore: long-lambda
        "containing", driveInterface,
        std::bind_front(
            collection_util::handleCollectionMembers, asyncResp,
            boost::urls::format("/redfish/v1/Chassis/{}/Drives", chassisId),
            nlohmann::json::json_pointer("/Members")));
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

    redfish::chassis_utils::getValidChassisPath(
        asyncResp, chassisId,
        std::bind_front(afterChassisDriveCollectionSubtreeGet, asyncResp,
                        chassisId));
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
        sdbusplus::object_path objPath(path);
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
        asyncResp->res.jsonValue["Status"]["State"] = resource::State::Enabled;

        nlohmann::json::object_t linkChassisNav;
        linkChassisNav["@odata.id"] =
            boost::urls::format("/redfish/v1/Chassis/{}", chassisId);
        asyncResp->res.jsonValue["Links"]["Chassis"] = linkChassisNav;

        addAllDriveInfo(asyncResp, connectionNames[0].first, path,
                        connectionNames[0].second);
    }
}

inline void handleChassisDriveGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& driveId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, driveInterface,
        std::bind_front(buildDrive, asyncResp, chassisId, driveId));
}

/**
 * This URL will show the drive interface for the specific drive in the chassis
 */
inline void requestRoutesChassisDrive(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Drives/")
        .privileges(redfish::privileges::getDriveCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(chassisDriveCollectionGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Drives/<str>/")
        .privileges(redfish::privileges::getChassis)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleChassisDriveGet, std::ref(app)));
}

} // namespace redfish
