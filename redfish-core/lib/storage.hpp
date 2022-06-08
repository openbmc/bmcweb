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
#include <query.hpp>
#include <registries/privilege_registry.hpp>
#include <sdbusplus/asio/property.hpp>

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
                     const dbus::utility::MapperGetSubTreePathsResponse&
                         storageList) {
            nlohmann::json& storageArray = asyncResp->res.jsonValue["Drives"];
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
                                     storageList.begin(), storageList.end());

            for (const std::string& objpath : storageList)
            {
                std::size_t lastPos = objpath.rfind('/');
                if (lastPos == std::string::npos ||
                    (objpath.size() <= lastPos + 1))
                {
                    BMCWEB_LOG_ERROR << "Failed to find '/' in " << objpath;
                    continue;
                }
                nlohmann::json::object_t storage;
                storage["@odata.id"] =
                    "/redfish/v1/Systems/system/Storage/1/Drives/" +
                    objpath.substr(lastPos + 1);
                storageArray.push_back(std::move(storage));
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
                     const dbus::utility::MapperGetSubTreeResponse& subtree) {
            if (ec || subtree.empty())
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

                const std::string& connectionName = interfaceDict.front().first;

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
                        asyncResp->res.jsonValue["StorageControllers"][index]
                                                ["Status"]["State"] =
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
            // Pointer based on |asyncResp->res.jsonValue|
            nlohmann::json::json_pointer rootPtr =
                "/StorageControllers"_json_pointer;
            for (const auto& [path, interfaceDict] : subtree)
            {
                auto subHealth = std::make_shared<HealthPopulate>(
                    asyncResp, rootPtr / count / "Status");
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
        for (const std::pair<std::string, dbus::utility::DbusVariantType>&
                 property : propertiesList)
        {
            // Store DBus properties that are also
            // Redfish properties with same name and a
            // string value
            const std::string& propertyName = property.first;
            if ((propertyName == "PartNumber") ||
                (propertyName == "SerialNumber") ||
                (propertyName == "Manufacturer") || (propertyName == "Model"))
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
                asyncResp->res.jsonValue["PredictedMediaLifeLeftPercent"] =
                    *lifeLeft;
            }
            else if (propertyName == "EncryptionStatus")
            {
                encryptionStatus = std::get_if<std::string>(&property.second);
                if (encryptionStatus == nullptr)
                {
                    BMCWEB_LOG_ERROR << "Illegal property: EncryptionStatus";
                    messages::internalError(asyncResp->res);
                    return;
                }
            }
            else if (propertyName == "Locked")
            {
                isLocked = std::get_if<bool>(&property.second);
                if (isLocked == nullptr)
                {
                    BMCWEB_LOG_ERROR << "Illegal property: EncryptionStatus";
                    messages::internalError(asyncResp->res);
                    return;
                }
            }
        }
        if (*encryptionStatus ==
            "xyz.openbmc_project.Drive.DriveEncryptionState.Encrypted")
        {
            if (*isLocked == true)
            {
                asyncResp->res.jsonValue["EncryptionStatus"] = "Locked";
            }
            else if (*isLocked == false)
            {
                asyncResp->res.jsonValue["EncryptionStatus"] = "Unlocked";
            }
            else
            {
                asyncResp->res.jsonValue["EncryptionStatus"] = "Foreign";
            }
        }
        else
        {
            asyncResp->res.jsonValue["EncryptionStatus"] = "Unencrypted";
        }
        });
}

void addAllDriveInfo(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const std::string& connectionName, const std::string& path,
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
                      const dbus::utility::MapperGetSubTreeResponse& subtree) {
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
                        std::vector<std::pair<
                            std::string, std::vector<std::string>>>>& object) {
                return sdbusplus::message::object_path(object.first)
                           .filename() == driveId;
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
                "/redfish/v1/Systems/system/Storage/1/Drives/" + driveId;
            asyncResp->res.jsonValue["Name"] = driveId;
            asyncResp->res.jsonValue["Id"] = driveId;

            if (connectionNames.size() != 1)
            {
                BMCWEB_LOG_ERROR << "Connection size " << connectionNames.size()
                                 << ", not equal to 1";
                messages::internalError(asyncResp->res);
                return;
            }

            getMainChassisId(
                asyncResp, [](const std::string& chassisId,
                              const std::shared_ptr<bmcweb::AsyncResp>& aRsp) {
                    aRsp->res.jsonValue["Links"]["Chassis"]["@odata.id"] =
                        "/redfish/v1/Chassis/" + chassisId;
                });

            // default it to Enabled
            asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

            auto health = std::make_shared<HealthPopulate>(asyncResp);
            health->inventory.emplace_back(path);
            health->populate();

            addAllDriveInfo(asyncResp, connectionNames[0].first, path,
                            connectionNames[0].second);
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTree",
            "/xyz/openbmc_project/inventory", int32_t(0),
            std::array<const char*, 1>{
                "xyz.openbmc_project.Inventory.Item.Drive"});
        });
}

/**
 * Chassis drives, this URL will show all the DriveCollection
 * information
 */
void chassisDriveCollectionGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
    {
        return;
    }

    // mapper call lambda
    crow::connections::systemBus->async_method_call(
        [asyncResp,
         chassisId](const boost::system::error_code ec,
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
        for (const std::pair<
                 std::string,
                 std::vector<std::pair<std::string, std::vector<std::string>>>>&
                 object : subtree)
        {
            const std::string& path = object.first;
            const dbus::utility::MapperGetObject& connectionNames =
                object.second;

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
                crow::utility::urlFromPieces("redfish", "v1",
                                             "Chassis" + chassisId + "Drives");
            asyncResp->res.jsonValue["Name"] = "Drive Collection";

            // Association lambda
            sdbusplus::asio::getProperty<std::vector<std::string>>(
                *crow::connections::systemBus,
                "xyz.openbmc_project.ObjectMapper", path + "/drive",
                "xyz.openbmc_project.Association", "endpoints",
                [asyncResp, chassisId](const boost::system::error_code ec3,
                                       const std::vector<std::string>& resp) {
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
                    sdbusplus::message::object_path path(drive);
                    leafNames.push_back(path.filename());
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
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", 0,
        std::array<const char*, 2>{
            "xyz.openbmc_project.Inventory.Item.Board",
            "xyz.openbmc_project.Inventory.Item.Chassis"});
}

inline void requestRoutesChassisDrive(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Drives/")
        .privileges(redfish::privileges::getDriveCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(chassisDriveCollectionGet, std::ref(app)));
}

void matchAndFillDrive(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
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
        const std::array<const char*, 1> driveInterface = {
            "xyz.openbmc_project.Inventory.Item.Drive"};

        crow::connections::systemBus->async_method_call(
            [asyncResp, chassisId, driveName](
                const boost::system::error_code ec,
                const dbus::utility::MapperGetSubTreeResponse& subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                messages::internalError(asyncResp->res);
                return;
            }

            // Iterate over all retrieved ObjectPaths.
            for (const std::pair<std::string,
                                 std::vector<std::pair<
                                     std::string, std::vector<std::string>>>>&
                     object : subtree)
            {
                const std::string& path = object.first;
                const std::vector<
                    std::pair<std::string, std::vector<std::string>>>&
                    connectionNames = object.second;

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
                asyncResp->res.jsonValue = {
                    {"@odata.context", "/redfish/v1/$metadata#Drive.Drive"},
                    {"@odata.id", "/redfish/v1/Chassis/" +
                                      (chassisId + "/Drives/" += driveName)}};

                asyncResp->res.jsonValue["@odata.type"] = "#Drive.v1_7_0.Drive";
                asyncResp->res.jsonValue["Name"] = driveName;
                asyncResp->res.jsonValue["Id"] = driveName;
                // default it to Enabled
                asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

                addAllDriveInfo(asyncResp, connectionNames[0].first, path,
                                connectionNames[0].second);
            } // end Iterate over drives
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTree",
            "/xyz/openbmc_project/inventory", 0, driveInterface);
    } // end Iterate over associations
}

void chassisDriveNameGet(crow::App& app, const crow::Request& req,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& chassisId,
                         const std::string& driveName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
    {
        return;
    }
    const std::array<const char*, 2> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Board",
        "xyz.openbmc_project.Inventory.Item.Chassis"};

    // mapper call chassis
    crow::connections::systemBus->async_method_call(
        [asyncResp, chassisId,
         driveName](const boost::system::error_code ec,
                    const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        // Iterate over all retrieved ObjectPaths.
        for (const std::pair<
                 std::string,
                 std::vector<std::pair<std::string, std::vector<std::string>>>>&
                 object : subtree)
        {
            const std::string& path = object.first;
            const std::vector<std::pair<std::string, std::vector<std::string>>>&
                connectionNames = object.second;

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

            sdbusplus::asio::getProperty<std::vector<std::string>>(
                *crow::connections::systemBus,
                "xyz.openbmc_project.ObjectMapper", path + "/drive",
                "xyz.openbmc_project.Association", "endpoints",
                [asyncResp, chassisId,
                 driveName](const boost::system::error_code ec3,
                            const std::vector<std::string>& resp) {
                if (ec3)
                {
                    return; // no drives = no failures
                }
                matchAndFillDrive(asyncResp, chassisId, driveName, resp);
                }); // end association lambda
            break;  // only return result from first machine chassis
        }           // end Iterate over all chassis ObjectPaths
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", 0,
        interfaces); // end mapper chassis
} // end handler

/**
 * This URL will show the drive interface for the specific drive in the chassis
 */
inline void requestRoutesChassisDriveName(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Drives/<str>/")
        .privileges(redfish::privileges::getChassis)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(chassisDriveNameGet, std::ref(app)));
}

} // namespace redfish
