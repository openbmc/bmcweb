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
    const std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Drive"};
    dbus::utility::getSubTreePaths(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [asyncResp, health](
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
                "redfish", "v1", "Systems", "system", "Storage", "1", "Drives",
                object.filename());
            driveArray.push_back(std::move(driveJson));
        }

        count = driveArray.size();
        });
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
        asyncResp->res.jsonValue["@odata.type"] = "#Storage.v1_13_0.Storage";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/Storage/1";
        asyncResp->res.jsonValue["Name"] = "Storage";
        asyncResp->res.jsonValue["Id"] = "1";
        asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

        auto health = std::make_shared<HealthPopulate>(asyncResp);
        health->populate();

        getDrives(asyncResp, health);
        asyncResp->res.jsonValue["Controllers"]["@odata.id"] =
            "/redfish/v1/Systems/system/Storage/1/Controllers";
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
        [asyncResp](const boost::system::error_code& ec,
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

        constexpr std::array<std::string_view, 1> interfaces = {
            "xyz.openbmc_project.Inventory.Item.Drive"};
        dbus::utility::getSubTree(
            "/xyz/openbmc_project/inventory", 0, interfaces,
            [asyncResp,
             driveId](const boost::system::error_code& ec,
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
                crow::utility::urlFromPieces("redfish", "v1", "Systems",
                                             "system", "Storage", "1", "Drives",
                                             driveId);
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
                        crow::utility::urlFromPieces("redfish", "v1", "Chassis",
                                                     chassisId);
                });

            // default it to Enabled
            asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

            auto health = std::make_shared<HealthPopulate>(asyncResp);
            health->inventory.emplace_back(path);
            health->populate();

            addAllDriveInfo(asyncResp, connectionNames[0].first, path,
                            connectionNames[0].second);
            });
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
            sdbusplus::asio::getProperty<std::vector<std::string>>(
                *crow::connections::systemBus,
                "xyz.openbmc_project.ObjectMapper", path + "/drive",
                "xyz.openbmc_project.Association", "endpoints",
                [asyncResp, chassisId](const boost::system::error_code& ec3,
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
                BMCWEB_LOG_ERROR << "Got 0 Connection names";
                continue;
            }

            sdbusplus::asio::getProperty<std::vector<std::string>>(
                *crow::connections::systemBus,
                "xyz.openbmc_project.ObjectMapper", path + "/drive",
                "xyz.openbmc_project.Association", "endpoints",
                [asyncResp, chassisId,
                 driveName](const boost::system::error_code& ec3,
                            const std::vector<std::string>& resp) {
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

inline void
    getStorageController(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& controllerId)
{
    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Inventory.Item.StorageController"};
    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [asyncResp,
         controllerId](const boost::system::error_code& ec,
                       const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec || subtree.empty())
        {
            // doesn't have to be there
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

            asyncResp->res.jsonValue["@odata.type"] =
                "#StorageController.v1_6_0.StorageController";
            asyncResp->res.jsonValue["@odata.id"] =
                crow::utility::urlFromPieces("redfish", "v1", "Systems",
                                             "system", "Storage", "1",
                                             "Controllers", controllerId);
            asyncResp->res.jsonValue["Name"] = controllerId;
            asyncResp->res.jsonValue["Id"] = controllerId;
            asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

            sdbusplus::asio::getProperty<bool>(
                *crow::connections::systemBus, connectionName, path,
                "xyz.openbmc_project.Inventory.Item", "Present",
                [asyncResp](const boost::system::error_code& ec2,
                            bool isPresent) {
                // this interface isn't necessary, only check it
                // if we get a good return
                if (ec2)
                {
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
                [asyncResp](
                    const boost::system::error_code& ec2,
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
        });
}

inline void requestRoutesStorageControllerCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Storage/1/Controllers/")
        .privileges(redfish::privileges::getStorageControllerCollection)
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
            nlohmann::json::array_t members;
            if (ec || controllerList.empty())
            {
                asyncResp->res.jsonValue["Members"] = std::move(members);
                asyncResp->res.jsonValue["Members@odata.count"] = 0;
                return;
            }

            for (const std::string& path : controllerList)
            {
                sdbusplus::message::object_path object(path);
                std::string id = object.filename();
                if (id.empty())
                {
                    BMCWEB_LOG_ERROR << "Failed to find filename in " << path;
                    return;
                }
                nlohmann::json::object_t member;
                member["@odata.id"] = crow::utility::urlFromPieces(
                    "redfish", "v1", "Systems", "system", "Storage", "1",
                    "Controllers", id);
                members.emplace_back(member);
            }
            asyncResp->res.jsonValue["Members@odata.count"] = members.size();
            asyncResp->res.jsonValue["Members"] = std::move(members);
            });
        });
}

inline void requestRoutesStorageController(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Storage/1/Controllers/<str>")
        .privileges(redfish::privileges::getStorageController)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName,
                   const std::string& controllerId) {
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
        getStorageController(asyncResp, controllerId);
        });
}

} // namespace redfish
