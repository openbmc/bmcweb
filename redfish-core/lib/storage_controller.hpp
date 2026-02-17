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

namespace redfish
{

inline void populateStorageController(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& controllerId, const std::string& connectionName,
    const std::string& path)
{
    asyncResp->res.jsonValue["@odata.type"] =
        "#StorageController.v1_6_0.StorageController";
    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Systems/{}/Storage/1/Controllers/{}",
                            BMCWEB_REDFISH_SYSTEM_URI_NAME, controllerId);
    asyncResp->res.jsonValue["Name"] = controllerId;
    asyncResp->res.jsonValue["Id"] = controllerId;
    asyncResp->res.jsonValue["Description"] = "Storage Controller";
    asyncResp->res.jsonValue["Status"]["State"] = resource::State::Enabled;

    dbus::utility::getProperty<bool>(
        connectionName, path, "xyz.openbmc_project.Inventory.Item", "Present",
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
            "/redfish/v1/Systems/{}/Storage/1/Controllers/{}",
            BMCWEB_REDFISH_SYSTEM_URI_NAME, id);
        members.emplace_back(member);
    }
    asyncResp->res.jsonValue["Members@odata.count"] = members.size();
    asyncResp->res.jsonValue["Members"] = std::move(members);
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
        [asyncResp,
         controllerId](const boost::system::error_code& ec,
                       const dbus::utility::MapperGetSubTreeResponse& subtree) {
            getStorageControllerHandler(asyncResp, controllerId, ec, subtree);
        });
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
        boost::urls::format("/redfish/v1/Systems/{}/Storage/1/Controllers",
                            BMCWEB_REDFISH_SYSTEM_URI_NAME);
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

inline void requestRoutesStorageController(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Storage/1/Controllers/")
        .privileges(redfish::privileges::getStorageControllerCollection)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleSystemsStorageControllerCollectionGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Storage/1/Controllers/<str>")
        .privileges(redfish::privileges::getStorageController)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleSystemsStorageControllerGet, std::ref(app)));
}

} // namespace redfish
