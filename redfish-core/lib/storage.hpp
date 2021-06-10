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

#include <variant>

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

            nlohmann::json& storageArray = asyncResp->res.jsonValue["Drives"];
            storageArray = nlohmann::json::array();
            auto& count = asyncResp->res.jsonValue["Drives@odata.count"];
            count = 0;

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

            count = storageArray.size();
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

                auto storageControllerPointer =
                    "/StorageControllers"_json_pointer / index;
                name_util::getPrettyName(asyncResp, path, interfaceDict,
                                         storageControllerPointer / "Name");

                location_util::getLocation(asyncResp, path, connectionName,
                                           interfaceDict.front().second,
                                           storageControllerPointer /
                                               "Location");

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
                crow::connections::systemBus->async_method_call(
                    [asyncResp, storageId](
                        const boost::system::error_code ec,
                        const crow::openbmc_mapper::GetSubTreeType& subtree) {
                        if (ec)
                        {
                            BMCWEB_LOG_DEBUG << "DBUS response error";
                            messages::resourceNotFound(
                                asyncResp->res, "#Storage.v1_7_1.Storage",
                                storageId);
                            return;
                        }

                        auto storage = std::find_if(
                            subtree.begin(), subtree.end(),
                            [asyncResp, &storageId](auto& object) {
                                const sdbusplus::message::object_path path(
                                    object.first);

                                if (path.filename() != storageId)
                                {
                                    return false;
                                }

                                return true;
                            });

                        if (storage == subtree.end())
                        {
                            messages::resourceNotFound(
                                asyncResp->res, "#Storage.v1_7_1.Storage",
                                storageId);
                            return;
                        }

                        const std::string& storagePath = storage->first;
                        const std::vector<
                            std::pair<std::string, std::vector<std::string>>>&
                            connectionNames = storage->second;

                        if (connectionNames.size() != 1)
                        {
                            BMCWEB_LOG_ERROR << "Connection size "
                                             << connectionNames.size()
                                             << ", greater than 1";
                            messages::resourceNotFound(
                                asyncResp->res, "#Storage.v1_7_1.Storage",
                                storageId);
                            return;
                        }

                        asyncResp->res.jsonValue["@odata.type"] =
                            "#Storage.v1_7_1.Storage";
                        asyncResp->res.jsonValue["@odata.id"] =
                            "/redfish/v1/Systems/system/Storage/" + storageId;
                        asyncResp->res.jsonValue["Name"] = "Storage";
                        asyncResp->res.jsonValue["Id"] = storageId;
                        asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

                        auto health =
                            std::make_shared<HealthPopulate>(asyncResp);
                        health->populate();

                        sdbusplus::message::object_path path(storagePath);
                        getDrives(asyncResp, health, path, storageId);
                        getStorageControllers(asyncResp, health, path,
                                              storageId);

                        location_util::getLocation(
                            asyncResp, path, connectionNames[0].first,
                            connectionNames[0].second,
                            "/PhysicalLocation"_json_pointer);
                    },
                    "xyz.openbmc_project.ObjectMapper",
                    "/xyz/openbmc_project/object_mapper",
                    "xyz.openbmc_project.ObjectMapper", "GetSubTree",
                    "/xyz/openbmc_project/inventory", 0,
                    std::array<std::string, 1>{
                        "xyz.openbmc_project.Inventory.Item.Storage"});
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
                        [asyncResp, &driveId](auto& object) {
                            const sdbusplus::message::object_path path(
                                object.first);

                            if (path.filename() != driveId)
                            {
                                return false;
                            }

                            name_util::getPrettyName(asyncResp, path,
                                                     object.second,
                                                     "/Name"_json_pointer);
                            return true;
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

                    if (connectionNames.size() != 1)
                    {
                        BMCWEB_LOG_ERROR << "Connection size "
                                         << connectionNames.size()
                                         << ", greater than 1";
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    getChassisId(
                        asyncResp, path,
                        [asyncResp](std::optional<std::string> chassisId) {
                            if (chassisId.has_value())
                            {
                                asyncResp->res.jsonValue["Links"]["Chassis"] = {
                                    {"@odata.id", "/redfish/v1/Chassis/" +
                                                      chassisId.value()}};
                            }
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
                                asyncResp->res.jsonValue["Status"]["State"] =
                                    "Disabled";
                            }
                        },
                        connectionName, path, "org.freedesktop.DBus.Properties",
                        "Get", "xyz.openbmc_project.Inventory.Item", "Present");

                    crow::connections::systemBus->async_method_call(
                        [asyncResp](const boost::system::error_code ec2,
                                    const std::variant<bool> rebuilding) {
                            // this interface isn't necessary, only check it
                            // if we get a good return
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

                            // updating and disabled in the backend
                            // shouldn't be able to be set at the same time,
                            // so we don't need to check for the race
                            // condition of these two calls
                            if ((*updating))
                            {
                                asyncResp->res.jsonValue["Status"]["State"] =
                                    "Updating";
                            }
                        },
                        connectionName, path, "org.freedesktop.DBus.Properties",
                        "Get", "xyz.openbmc_project.State.Drive", "Rebuilding");

                    location_util::getLocation(
                        asyncResp, path, connectionName,
                        connectionNames[0].second,
                        "/PhysicalLocation"_json_pointer);

                    asyncResp->res.jsonValue["Actions"]["#Drive.Reset"] = {
                        {"target", "/redfish/v1/Systems/system/Storage/" +
                                       storageId + "/Drives/" + driveId +
                                       "/Actions/Drive.Reset"},
                        {"@Redfish.ActionInfo",
                         "/redfish/v1/Systems/system/Storage/" + storageId +
                             "/Drives/" + driveId + "/ResetActionInfo/"}};
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
 * Function for the Drive to perform actions.
 *
 * @param[in] asyncResp - Shared pointer for completing asynchronous calls
 * @param[in] driveId   - D-bus filename to identify the Drive
 * @param[in] action    - Action call to the Drive
 */
inline void doDriveAction(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& driveId, const std::string& action)
{
    const char* interfaceName = "xyz.openbmc_project.State.Drive";

    crow::connections::systemBus->async_method_call(
        [asyncResp, driveId, action,
         interfaceName](const boost::system::error_code ec,
                        const crow::openbmc_mapper::GetSubTreeType& subtree) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "Drive mapper call error";
                messages::internalError(asyncResp->res);
                return;
            }

            auto driveState = std::find_if(
                subtree.begin(), subtree.end(), [&driveId](auto& object) {
                    const sdbusplus::message::object_path path(object.first);
                    return path.filename() == driveId;
                });

            if (driveState == subtree.end())
            {
                messages::resourceNotFound(asyncResp->res, "Drive Action",
                                           driveId);
                return;
            }

            const std::string& path = driveState->first;
            const std::vector<std::pair<std::string, std::vector<std::string>>>&
                connectionNames = driveState->second;

            if (connectionNames.size() != 1)
            {
                BMCWEB_LOG_ERROR << "Connection size " << connectionNames.size()
                                 << ", greater than 1";
                messages::internalError(asyncResp->res);
                return;
            }

            const std::string& connectionName = connectionNames[0].first;
            const std::string& propertyValue =
                "xyz.openbmc_project.State.Drive.Transition." + action;
            const char* destProperty = "RequestedDriveTransition";
            std::variant<std::string> dbusPropertyValue(propertyValue);

            crow::connections::systemBus->async_method_call(
                [asyncResp, action](const boost::system::error_code ec) {
                    if (ec)
                    {
                        BMCWEB_LOG_ERROR << "[Set] Bad D-Bus request error for "
                                         << action << " : " << ec;
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    messages::success(asyncResp->res);
                },
                connectionName.c_str(), path.c_str(),
                "org.freedesktop.DBus.Properties", "Set", interfaceName,
                destProperty, dbusPropertyValue);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", int32_t(0),
        std::array<const char*, 1>{interfaceName});
}

/**
 * DriveResetAction class supports the POST method for the Reset (reboot)
 * action.
 */
inline void requestDriveResetAction(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/Storage/<str>/Drives/<str>/"
                      "Actions/Drive.Reset/")
        .privileges(redfish::privileges::postDrive)
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string&, const std::string& driveId) {
                BMCWEB_LOG_DEBUG << "Post Drive Reset.";

                crow::connections::systemBus->async_method_call(
                    [asyncResp, req, driveId](
                        const boost::system::error_code ec,
                        const crow::openbmc_mapper::GetSubTreeType& subtree) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR << "Drive mapper call error";
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        auto drive = std::find_if(
                            subtree.begin(), subtree.end(),
                            [&driveId](auto& object) {
                                const sdbusplus::message::object_path path(
                                    object.first);
                                return path.filename() == driveId;
                            });

                        if (drive == subtree.end())
                        {
                            messages::resourceNotFound(
                                asyncResp->res, "Drive Action Reset", driveId);
                            return;
                        }

                        std::string resetType;

                        if (!json_util::readJson(req, asyncResp->res,
                                                 "ResetType", resetType) ||
                            resetType.empty())
                        {
                            BMCWEB_LOG_DEBUG
                                << "Proceeding with Powercycle (default)";
                            doDriveAction(asyncResp, driveId, "Powercycle");
                            return;
                        }

                        if (resetType == "ForceRestart")
                        {
                            BMCWEB_LOG_DEBUG << "Proceeding with " << resetType;
                            doDriveAction(asyncResp, driveId, "Reboot");
                            return;
                        }
                        if (resetType == "PowerCycle")
                        {
                            BMCWEB_LOG_DEBUG << "Proceeding with " << resetType;
                            doDriveAction(asyncResp, driveId, "Powercycle");
                            return;
                        }

                        BMCWEB_LOG_DEBUG
                            << "Invalid property value for ResetType: "
                            << resetType;
                        messages::actionParameterNotSupported(
                            asyncResp->res, resetType, "ResetType");
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
 * DriveResetActionInfo derived class for delivering Drive
 * ResetType AllowableValues using ResetInfo schema.
 */
inline void requestRoutesDriveResetActionInfo(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/Storage/<str>/Drives/<str>/"
                      "ResetActionInfo/")
        .privileges(redfish::privileges::getActionInfo)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& storageId, const std::string& driveId) {
                crow::connections::systemBus->async_method_call(
                    [asyncResp, storageId, driveId](
                        const boost::system::error_code ec,
                        const crow::openbmc_mapper::GetSubTreeType& subtree) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR << "Drive mapper call error";
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        auto drive = std::find_if(
                            subtree.begin(), subtree.end(),
                            [&driveId](auto& object) {
                                const sdbusplus::message::object_path path(
                                    object.first);
                                return path.filename() == driveId;
                            });

                        if (drive == subtree.end())
                        {
                            messages::resourceNotFound(asyncResp->res,
                                                       "Drive ResetActionInfo",
                                                       driveId);
                            return;
                        }

                        asyncResp->res.jsonValue = {
                            {"@odata.type", "#ActionInfo.v1_1_2.ActionInfo"},
                            {"@odata.id",
                             "/redfish/v1/Systems/system/Storage/" + storageId +
                                 "/Drives/" + driveId + "/ResetActionInfo/"},
                            {"Name", "Reset Action Info"},
                            {"Id", "ResetActionInfo"},
                            {"Parameters",
                             {{{"Name", "ResetType"},
                               {"Required", true},
                               {"DataType", "String"},
                               {"AllowableValues",
                                {"PowerCycle", "ForceRestart"}}}}}};
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
