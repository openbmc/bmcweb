// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2019 Intel Corporation
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "generated/enums/resource.hpp"
#include "http_request.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/systems_utils.hpp"

namespace redfish
{

inline void populateStorageController(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& storageId,
    const std::string& controllerId, const std::string& connectionName,
    const std::string& path)
{
    asyncResp->res.jsonValue["@odata.type"] =
        "#StorageController.v1_6_0.StorageController";
    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Systems/{}/Storage/{}/Controllers/{}",
                            systemName, storageId, controllerId);
    asyncResp->res.jsonValue["Name"] = controllerId;
    asyncResp->res.jsonValue["Id"] = controllerId;
    asyncResp->res.jsonValue["Status"]["State"] = resource::State::Enabled;

    dbus::utility::getProperty<bool>(
        connectionName, path, "xyz.openbmc_project.Inventory.Item", "Present",
        // ast-grep-ignore: long-lambda
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
                asyncResp->res.jsonValue["Status"]["State"] =
                    resource::State::Absent;
            }
        });

    asset_utils::getAssetInfo(asyncResp, connectionName, path, ""_json_pointer,
                              false);
}

inline void getStorageControllerHandler(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& storageId,
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
        sdbusplus::object_path object(path);
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
        populateStorageController(asyncResp, systemName, storageId,
                                  controllerId, connectionName, path);
    }
}

inline void populateStorageControllerCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& storageId,
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
        std::string id = sdbusplus::object_path(path).filename();
        if (id.empty())
        {
            BMCWEB_LOG_ERROR("Failed to find filename in {}", path);
            return;
        }
        nlohmann::json::object_t member;
        member["@odata.id"] = boost::urls::format(
            "/redfish/v1/Systems/{}/Storage/{}/Controllers/{}", systemName,
            storageId, id);
        members.emplace_back(member);
    }
    asyncResp->res.jsonValue["Members@odata.count"] = members.size();
    asyncResp->res.jsonValue["Members"] = std::move(members);
}

inline void afterSystemsCheckStorageControllersGetSubtree(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& storageId,
    const std::string& controllerId, const boost::system::error_code& ec,
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
            return sdbusplus::object_path(object.first).filename() == storageId;
        });
    if (storage == subtree.end())
    {
        messages::resourceNotFound(asyncResp->res, "#Storage.v1_13_0.Storage",
                                   storageId);
        return;
    }

    constexpr std::array<std::string_view, 1> controllerInterface = {
        "xyz.openbmc_project.Inventory.Item.StorageController"};

    dbus::utility::getAssociatedSubTreeById(
        storageId, "/xyz/openbmc_project/inventory", storageInterface,
        "containing", controllerInterface,
        std::bind_front(getStorageControllerHandler, asyncResp, systemName,
                        storageId, controllerId));
}

inline void multiHostHandleSystemsStorageControllersGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& storageId,
    const std::string& controllerId,
    const std::optional<std::string>& systemPath)

{
    if (!systemPath.has_value())
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    dbus::utility::getAssociatedSubTreeById(
        systemName, "/xyz/openbmc_project/inventory", systemInterface,
        "containing", storageInterface,
        std::bind_front(afterSystemsCheckStorageControllersGetSubtree,
                        asyncResp, systemName, storageId, controllerId));
}

inline void handleSystemsStorageControllerGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& storageId,
    const std::string& controllerId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        BMCWEB_LOG_DEBUG("Failed to setup Redfish Route for StorageController");
        return;
    }
    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        systems_utils::getValidSystemsPath(
            asyncResp, systemName,
            std::bind_front(multiHostHandleSystemsStorageControllersGet,
                            asyncResp, systemName, storageId, controllerId));
    }
    else
    {
        if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
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
            std::bind_front(getStorageControllerHandler, asyncResp, systemName,
                            storageId, controllerId));
    }
}

inline void afterSystemsStorageControllersCollectionGetSubtree(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& storageId,
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
            return sdbusplus::object_path(object.first).filename() == storageId;
        });
    if (storage == subtree.end())
    {
        messages::resourceNotFound(asyncResp->res, "#Storage.v1_13_0.Storage",
                                   storageId);
        return;
    }

    asyncResp->res.jsonValue["@odata.type"] =
        "#StorageControllerCollection.StorageControllerCollection";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/{}/Storage/{}/Controllers", systemName, storageId);
    asyncResp->res.jsonValue["Name"] = "Storage Controller Collection";

    constexpr std::array<std::string_view, 1> controllerInterface = {
        "xyz.openbmc_project.Inventory.Item.StorageController"};

    dbus::utility::getAssociatedSubTreePathsById(
        storageId, "/xyz/openbmc_project/inventory", storageInterface,
        "containing", controllerInterface,
        std::bind_front(populateStorageControllerCollection, asyncResp,
                        systemName, storageId));
}

inline void multiHostHandleSystemsStorageControllersCollectionGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& storageId,
    const std::optional<std::string>& systemPath)

{
    if (!systemPath.has_value())
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    dbus::utility::getAssociatedSubTreeById(
        systemName, "/xyz/openbmc_project/inventory", systemInterface,
        "containing", storageInterface,
        std::bind_front(afterSystemsStorageControllersCollectionGetSubtree,
                        asyncResp, systemName, storageId));
}

inline void handleSystemsStorageControllerCollectionGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& storageId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        BMCWEB_LOG_DEBUG(
            "Failed to setup Redfish Route for StorageController Collection");
        return;
    }
    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        systems_utils::getValidSystemsPath(
            asyncResp, systemName,
            std::bind_front(
                multiHostHandleSystemsStorageControllersCollectionGet,
                asyncResp, systemName, storageId));
    }
    else
    {
        if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            BMCWEB_LOG_DEBUG("Failed to find ComputerSystem of {}", systemName);
            return;
        }

        asyncResp->res.jsonValue["@odata.type"] =
            "#StorageControllerCollection.StorageControllerCollection";
        asyncResp->res.jsonValue["@odata.id"] =
            boost::urls::format("/redfish/v1/Systems/{}/Storage/{}/Controllers",
                                BMCWEB_REDFISH_SYSTEM_URI_NAME, storageId);
        asyncResp->res.jsonValue["Name"] = "Storage Controller Collection";

        constexpr std::array<std::string_view, 1> interfaces = {
            "xyz.openbmc_project.Inventory.Item.StorageController"};

        dbus::utility::getSubTreePaths(
            "/xyz/openbmc_project/inventory", 0, interfaces,
            std::bind_front(populateStorageControllerCollection, asyncResp,
                            systemName, storageId));
    }
}

inline void requestRoutesStorageController(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Storage/<str>/Controllers/")
        .privileges(redfish::privileges::getStorageControllerCollection)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleSystemsStorageControllerCollectionGet, std::ref(app)));

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/<str>/Storage/<str>/Controllers/<str>/")
        .privileges(redfish::privileges::getStorageController)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleSystemsStorageControllerGet, std::ref(app)));
}

} // namespace redfish
