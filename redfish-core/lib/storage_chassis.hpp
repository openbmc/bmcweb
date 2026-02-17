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

inline void getDrivePresent(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& connectionName,
                            const std::string& path)
{
    dbus::utility::getProperty<bool>(
        connectionName, path, "xyz.openbmc_project.Inventory.Item", "Present",
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
    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Systems/{}/Storage/1/Drives/{}",
                            BMCWEB_REDFISH_SYSTEM_URI_NAME, driveId);
    asyncResp->res.jsonValue["Name"] = driveId;
    asyncResp->res.jsonValue["Description"] = "Drive";
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
                    member["@odata.id"] =
                        boost::urls::format("/redfish/v1/Chassis/{}/Drives/{}",
                                            chassisId, leafName);
                    members.emplace_back(std::move(member));
                    // navigation links will be registered in next patch set
                }
                asyncResp->res.jsonValue["Members@odata.count"] = resp.size();
            }); // end association lambda

    } // end Iterate over all retrieved ObjectPaths
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
    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, chassisInterfaces,
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
        asyncResp->res.jsonValue["Status"]["State"] = resource::State::Enabled;

        nlohmann::json::object_t linkChassisNav;
        linkChassisNav["@odata.id"] =
            boost::urls::format("/redfish/v1/Chassis/{}", chassisId);
        asyncResp->res.jsonValue["Links"]["Chassis"] = linkChassisNav;

        addAllDriveInfo(asyncResp, connectionNames[0].first, path,
                        connectionNames[0].second);
    }
}

inline void matchAndFillDrive(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& driveName,
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

inline void handleChassisDriveGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& driveName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    // mapper call chassis
    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, chassisInterfaces,
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
                        matchAndFillDrive(asyncResp, chassisId, driveName,
                                          resp);
                    });
                return;
            }
            // Couldn't find an object with that name.  return an error
            messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
        });
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
