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
#include <sdbusplus/unpack_properties.hpp>
#include <utils/dbus_utils.hpp>

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
        nlohmann::json::array_t members;
        nlohmann::json::object_t member;
        member["@odata.id"] = "/redfish/v1/Systems/system/Storage/1";
        members.emplace_back(member);
        asyncResp->res.jsonValue["Members"] = std::move(members);
        asyncResp->res.jsonValue["Members@odata.count"] = 1;
        });
}

inline void getDrives(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::shared_ptr<HealthPopulate>& health)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, health](
            const boost::system::error_code ec,
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
            driveJson["@odata.id"] =
                "/redfish/v1/Systems/system/Storage/1/Drives/" +
                object.filename();
            driveArray.push_back(std::move(driveJson));
        }

        count = driveArray.size();
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
                 const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec || subtree.empty())
        {
            // doesn't have to be there
            return;
        }

        nlohmann::json& root = asyncResp->res.jsonValue["StorageControllers"];
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
                BMCWEB_LOG_ERROR << "Connection size " << interfaceDict.size()
                                 << ", greater than 1";
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
                    asyncResp->res.jsonValue["StorageControllers"][index]
                                            ["Status"]["State"] = "Disabled";
                }
                });

            sdbusplus::asio::getAllProperties(
                *crow::connections::systemBus, connectionName, path,
                "xyz.openbmc_project.Inventory.Decorator.Asset",
                [asyncResp, index](
                    const boost::system::error_code ec2,
                    const std::vector<
                        std::pair<std::string, dbus::utility::DbusVariantType>>&
                        propertiesList) {
                if (ec2)
                {
                    // this interface isn't necessary
                    return;
                }

                const std::string* partNumber = nullptr;
                const std::string* serialNumber = nullptr;
                const std::string* manufacturer = nullptr;
                const std::string* model = nullptr;

                const bool success = sdbusplus::unpackPropertiesNoThrow(
                    dbus_utils::UnpackErrorPrinter(), propertiesList,
                    "PartNumber", partNumber, "SerialNumber", serialNumber,
                    "Manufacturer", manufacturer, "Model", model);

                if (!success)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }

                nlohmann::json& controller =
                    asyncResp->res.jsonValue["StorageControllers"][index];

                if (partNumber != nullptr)
                {
                    controller["PartNumber"] = *partNumber;
                }

                if (serialNumber != nullptr)
                {
                    controller["SerialNumber"] = *serialNumber;
                }

                if (manufacturer != nullptr)
                {
                    controller["Manufacturer"] = *manufacturer;
                }

                if (model != nullptr)
                {
                    controller["Model"] = *model;
                }
                });
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
}

inline void requestRoutesStorage(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/Storage/1/")
        .privileges(redfish::privileges::getStorage)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
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

        getDrives(asyncResp, health);
        getStorageControllers(asyncResp, health);
        });
}

inline void getDriveAsset(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& connectionName,
                          const std::string& path)
{
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, connectionName, path,
        "xyz.openbmc_project.Inventory.Decorator.Asset",
        [asyncResp](const boost::system::error_code ec,
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
        }
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

inline void requestRoutesDrive(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Storage/1/Drives/<str>/")
        .privileges(redfish::privileges::getDrive)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName, const std::string& driveId) {
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
                    const std::pair<std::string,
                                    dbus::utility::MapperServiceMap>& object) {
                return sdbusplus::message::object_path(object.first)
                           .filename() == driveId;
                });

            if (drive == subtree.end())
            {
                messages::resourceNotFound(asyncResp->res, "Drive", driveId);
                return;
            }

            const std::string& path = drive->first;
            const dbus::utility::MapperServiceMap& connectionNames =
                drive->second;

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

inline void buildDrive(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& chassisId,
                       const std::string& driveName,
                       const boost::system::error_code ec,
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
        const std::array<const char*, 1> driveInterface = {
            "xyz.openbmc_project.Inventory.Item.Drive"};

        crow::connections::systemBus->async_method_call(
            [asyncResp, chassisId, driveName](
                const boost::system::error_code ec,
                const dbus::utility::MapperGetSubTreeResponse& subtree) {
            buildDrive(asyncResp, chassisId, driveName, ec, subtree);
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTree",
            "/xyz/openbmc_project/inventory", 0, driveInterface);
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
                });
            break;
        }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", 0, interfaces);
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

/**
 * @brief Fetch the current active software for gssd-ddbus
 *
 * @return std::string Returns the object path of the current active software
 */
inline std::string fetchActiveSoftwareForDrive()
{
    std::string currentActiveSoftwareForDrive;
    sdbusplus::asio::getProperty<std::vector<std::string>>(
        *crow::connections::systemBus, "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/software/functional",
        "xyz.openbmc_project.Association", "endpoints",
        [currentActiveSoftwareForDrive](
            const boost::system::error_code ec,
            const std::vector<std::string>& functionalSw) mutable {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "error_code = " << ec;
            BMCWEB_LOG_ERROR << "error msg = " << ec.message();
            return;
        }

        if (functionalSw.empty())
        {
            BMCWEB_LOG_ERROR << "Zero functional software in system";
            return;
        }

        for (const auto& sw : functionalSw)
        {
            sdbusplus::message::object_path path(sw);
            // Get functional software for drive
            if (path.filename().find("/xyz/openbmc_project/software/chassis") !=
                std::string::npos)
            {
                currentActiveSoftwareForDrive = path.filename();
                break;
            }
        }
        return;
        });
    return currentActiveSoftwareForDrive;
}

/**
 * @brief Removes the active software for drive based on the objPath provided.
 * Deletes the association in the provided object path to
 * "/xyz/openbmc_project/software/functional" where
 * "/xyz/openbmc_project/software/functional" represents all the active software
 *
 *  @param[i, o] asyncResp Response object
 *
 *  @param[i] newActiveSoftwareObjPath object path to the new active software
 * for drive
 *
 * @return void
 */
inline void removeActiveSoftwareForDrive(std::string& objPath)
{
    sdbusplus::asio::getProperty<
        std::vector<std::tuple<std::string, std::string, std::string>>>(
        *crow::connections::systemBus, "com.google.gbmc.ssd", objPath,
        "xyz.openbmc_project.Association.Definitions", "Associations",
        [objPath](const boost::system::error_code ec,
                  const std::vector<std::tuple<std::string, std::string,
                                               std::string>>& swAssociations) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
            return;
        }

        if (swAssociations.empty())
        {
            BMCWEB_LOG_DEBUG << "No software associations present";
        }

        std::vector<std::tuple<std::string, std::string, std::string>>
            newSwAssociations;

        for (const auto& swAssociation : swAssociations)
        {
            if (get<2>(swAssociation) !=
                "/xyz/openbmc_project/software/functional")
            {
                newSwAssociations.push_back(swAssociation);
            }
        }

        // newSwAssociations is a vector with all the old associations for
        // the objPath except the "functional" association
        // It can also be an empty vector if no other associations (than
        // "functional") were present
        sdbusplus::asio::setProperty(
            *crow::connections::systemBus, "com.google.gbmc.ssd", objPath,
            "xyz.openbmc_project.Association.Definitions", "Associations",
            dbus::utility::DbusVariantType(std::move(newSwAssociations)),
            [objPath](const boost::system::error_code e) {
            if (e)
            {
                BMCWEB_LOG_ERROR << "DBUS response error: " << e;
                return;
            }
            BMCWEB_LOG_DEBUG << "Removed functional association for - "
                             << objPath;
            });
        });
}

/**
 * @brief Sets the active software for drive. Creates an association in the
 * provided object path to  "/xyz/openbmc_project/software/functional" where
 * "/xyz/openbmc_project/software/functional" represents all the active software
 *
 *  @param[i, o] asyncResp Response object
 *
 *  @param[i] newActiveSoftwareObjPath object path to the new active software
 * for drive
 *
 * @return bool status to report whether the operation was successful or not
 */
inline bool setNewActiveSoftwareForDrive(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& newActiveSoftwareObjPath)
{
    // Creating Association Property Value for setting newActiveSoftware path
    std::tuple<std::string, std::string, std::string> propertyValue(
        "functional", "gssd_software",
        "/xyz/openbmc_project/software/functional");
    std::vector<std::tuple<std::string, std::string, std::string>>
        associationPropertyValue{std::move(propertyValue)};

    bool status = false;
    // Setting a new association in the "com.google.gbmc.ssd" subtree for the
    // required new Active software object path
    sdbusplus::asio::setProperty(
        *crow::connections::systemBus, "com.google.gbmc.ssd",
        newActiveSoftwareObjPath, "xyz.openbmc_project.Association.Definitions",
        "Associations",
        dbus::utility::DbusVariantType(std::move(associationPropertyValue)),
        [newActiveSoftwareObjPath, asyncResp,
         status](const boost::system::error_code ec) mutable {
        if (ec)
        {
            asyncResp->res.jsonValue["data"]["description"] =
                "Failed to update the new active software for drive";
            asyncResp->res.jsonValue["message"] = "500 Internal Server Error";
            asyncResp->res.jsonValue["status"] = "error";
            BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
            return;
        }
        BMCWEB_LOG_DEBUG
            << "Set- " << newActiveSoftwareObjPath
            << R"( Property: xyz.openbmc_project.Association.Definitions to: "functional" "gssd_software" "/xyz/openbmc_project/software/functional")";

        status = true;
        });
    return status;
}

inline void setActiveSoftwareForDriveHandler(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    std::optional<std::string> newActiveSoftwareObjPath;

    if (!json_util::readJsonPatch(req, asyncResp->res,
                                  "newActiveSoftwareForDrive",
                                  newActiveSoftwareObjPath))
    {
        asyncResp->res.jsonValue["data"]["description"] =
            "Unable to parse request body";
        asyncResp->res.jsonValue["message"] = "400 Bad Request";
        asyncResp->res.jsonValue["status"] = "error";
        return;
    }

    if (!newActiveSoftwareObjPath)
    {
        asyncResp->res.jsonValue["data"]["description"] =
            "No New Active Software Object Path found in request";
        asyncResp->res.jsonValue["message"] = "400 Bad Request";
        asyncResp->res.jsonValue["status"] = "error";
        return;
    }

    // Fetching the currentActiveSoftware for Drive
    std::string currentActiveSoftwarePath = fetchActiveSoftwareForDrive();
    BMCWEB_LOG_DEBUG << "Current Active Software object path: "
                     << currentActiveSoftwarePath;

    // Set association for the newActiveSoftwareObjPath to functional
    bool success =
        setNewActiveSoftwareForDrive(asyncResp, *newActiveSoftwareObjPath);

    if (success)
    {
        // Removing association for the currentActiveSoftware if exists
        if (!currentActiveSoftwarePath.empty())
        {
            removeActiveSoftwareForDrive(currentActiveSoftwarePath);
        }

        asyncResp->res.jsonValue["data"]["description"] =
            "New active software for drive successfully set";
        asyncResp->res.jsonValue["message"] = "200 OK";
        asyncResp->res.jsonValue["status"] = "ok";
    }
}

inline void requestSetActiveSoftwareForDrive(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/Drives/SetActiveSoftware/")
        .privileges(redfish::privileges::privilegeSetConfigureManager)
        .methods(boost::beast::http::verb::patch)(
            std::bind_front(setActiveSoftwareForDriveHandler, std::ref(app)));
}

} // namespace redfish
