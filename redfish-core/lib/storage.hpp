// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2019 Intel Corporation
#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/resource.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"

#include "utils/collection.hpp"

#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <algorithm>
#include <array>
#include <format>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

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
    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    asyncResp->res.jsonValue["@odata.type"] =
        "#StorageCollection.StorageCollection";
    asyncResp->res.jsonValue["@odata.id"] = std::format(
        "/redfish/v1/Systems/{}/Storage", BMCWEB_REDFISH_SYSTEM_URI_NAME);
    asyncResp->res.jsonValue["Name"] = "Storage Collection";

    constexpr std::array<std::string_view, 1> interface{
        "xyz.openbmc_project.Inventory.Item.Storage"};
    collection_util::getCollectionMembers(
        asyncResp,
        boost::urls::format("/redfish/v1/Systems/{}/Storage",
                            BMCWEB_REDFISH_SYSTEM_URI_NAME),
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
            "/redfish/v1/Systems/{}/Storage/1/Drives/{}",
            BMCWEB_REDFISH_SYSTEM_URI_NAME, object.filename());
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
        boost::urls::format("/redfish/v1/Systems/{}/Storage/{}",
                            BMCWEB_REDFISH_SYSTEM_URI_NAME, storageId);
    asyncResp->res.jsonValue["Name"] = "Storage";
    asyncResp->res.jsonValue["Id"] = storageId;
    asyncResp->res.jsonValue["Status"]["State"] = resource::State::Enabled;

    getDrives(asyncResp);
    asyncResp->res.jsonValue["Controllers"]["@odata.id"] =
        boost::urls::format("/redfish/v1/Systems/{}/Storage/{}/Controllers",
                            BMCWEB_REDFISH_SYSTEM_URI_NAME, storageId);
}

inline void handleSystemsStorageGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& storageId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
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
    asyncResp->res.jsonValue["Status"]["State"] = resource::State::Enabled;

    // Storage subsystem to Storage link.
    nlohmann::json::array_t storageServices;
    nlohmann::json::object_t storageService;
    storageService["@odata.id"] =
        boost::urls::format("/redfish/v1/Systems/{}/Storage/{}",
                            BMCWEB_REDFISH_SYSTEM_URI_NAME, storageId);
    storageServices.emplace_back(storageService);
    asyncResp->res.jsonValue["Links"]["StorageServices"] =
        std::move(storageServices);
    asyncResp->res.jsonValue["Links"]["StorageServices@odata.count"] = 1;
}

inline void handleStorageGet(
    App& app, const crow::Request& req,
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

} // namespace redfish
