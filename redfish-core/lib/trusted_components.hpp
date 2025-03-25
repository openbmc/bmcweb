// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/chassis_utils.hpp"

#include <asm-generic/errno.h>

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace redfish
{

const std::array<std::string_view, 1> trustedComponentInterfaces = {
    "xyz.openbmc_project.Inventory.Item.Tpm"};

inline void addTrustedComponentCommonProperties(crow::Response& resp,
                                                const std::string& chassisId,
                                                const std::string& componentId)
{
    resp.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/TrustedComponent/TrustedComponent.json>; rel=describedby");
    resp.jsonValue["@odata.type"] = "#TrustedComponent.v1_0_0.TrustedComponent";
    resp.jsonValue["Name"] = componentId;
    resp.jsonValue["Id"] = componentId;
    resp.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/TrustedComponents/{}", chassisId, componentId);
}

inline void updateTrustedComponentList(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId,
    const dbus::utility::MapperGetSubTreePathsResponse& trustedComponentPaths)
{
    nlohmann::json& trustedComponentList = asyncResp->res.jsonValue["Members"];
    for (const std::string& trustedComponentPath : trustedComponentPaths)
    {
        std::string trustedComponentName =
            sdbusplus::message::object_path(trustedComponentPath).filename();
        if (trustedComponentName.empty())
        {
            continue;
        }

        nlohmann::json item = nlohmann::json::object();
        item["@odata.id"] =
            boost::urls::format("/redfish/v1/Chassis/{}/TrustedComponents/{}",
                                chassisId, trustedComponentName);

        trustedComponentList.emplace_back(std::move(item));
    }
    asyncResp->res.jsonValue["Members@odata.count"] =
        trustedComponentList.size();
}

inline void getTrustedComponentPaths(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& validChassisPath,
    const std::function<void(const dbus::utility::MapperGetSubTreePathsResponse&
                                 trustedComponentPaths)>& callback)
{
    sdbusplus::message::object_path endpointPath{validChassisPath};
    endpointPath /= "trusted_components";

    dbus::utility::getAssociatedSubTreePaths(
        endpointPath,
        sdbusplus::message::object_path("/xyz/openbmc_project/inventory"), 0,
        trustedComponentInterfaces,
        [asyncResp, callback](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreePathsResponse& subtreePaths) {
            if (ec)
            {
                if (ec.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR(
                        "DBUS response error for getAssociatedSubTreePaths {}",
                        ec.value());
                    messages::internalError(asyncResp->res);
                }
                return;
            }
            callback(subtreePaths);
        });
}

inline void doTrustedComponentsCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId,
    const std::optional<std::string>& validChassisPath)
{
    if (!validChassisPath)
    {
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
        return;
    }

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/TrustedComponentCollection/TrustedComponentCollection.json>; rel=describedby");
    asyncResp->res.jsonValue["@odata.type"] =
        "#TrustedComponentCollection.TrustedComponentCollection";
    asyncResp->res.jsonValue["Name"] = "Trusted Component Collection";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/TrustedComponents", chassisId);
    asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
    asyncResp->res.jsonValue["Members@odata.count"] = 0;
    getTrustedComponentPaths(
        asyncResp, *validChassisPath,
        std::bind_front(updateTrustedComponentList, asyncResp, chassisId));
}

inline void handleTrustedComponentsCollectionGet(
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
        std::bind_front(doTrustedComponentsCollection, asyncResp, chassisId));
}

inline void handleTrustedComponentsCollectionHead(
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
        [asyncResp,
         chassisId](const std::optional<std::string>& validChassisPath) {
            if (!validChassisPath)
            {
                messages::resourceNotFound(asyncResp->res, "Chassis",
                                           chassisId);
                return;
            }
            asyncResp->res.addHeader(
                boost::beast::http::field::link,
                "</redfish/v1/JsonSchemas/TrustedComponentCollection/TrustedComponentCollection.json>; rel=describedby");
        });
}

inline bool checkTrustedComponentId(const std::string& trustedComponentPath,
                                    const std::string& componentId)
{
    std::string trustedComponentName =
        sdbusplus::message::object_path(trustedComponentPath).filename();

    return !(trustedComponentName.empty() ||
             trustedComponentName != componentId);
}

inline void handleTrustedComponentPath(
    const std::string& componentId,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const dbus::utility::MapperGetSubTreePathsResponse& trustedComponentPaths,
    const std::function<void(const std::string& trustedComponentPath,
                             const std::string& service)>& callback)
{
    for (const auto& trustedComponentPath : trustedComponentPaths)
    {
        if (!checkTrustedComponentId(trustedComponentPath, componentId))
        {
            continue;
        }
        dbus::utility::getDbusObject(
            trustedComponentPath, trustedComponentInterfaces,
            [trustedComponentPath, asyncResp,
             callback](const boost::system::error_code& ec,
                       const dbus::utility::MapperGetObject& object) {
                if (ec || object.empty())
                {
                    BMCWEB_LOG_ERROR("DBUS response error on getDbusObject {}",
                                     ec.value());
                    messages::internalError(asyncResp->res);
                    return;
                }
                callback(trustedComponentPath, object.begin()->first);
            });

        return;
    }
    BMCWEB_LOG_WARNING("TPM not found {}", componentId);
    messages::resourceNotFound(asyncResp->res, "TrustedComponents",
                               componentId);
}

inline void getValidTrustedComponentPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& validChassisPath, const std::string& componentId,
    const std::function<void(const std::string& trustedComponentPath,
                             const std::string& service)>& callback)
{
    getTrustedComponentPaths(
        asyncResp, validChassisPath,
        [componentId, asyncResp, callback](
            const dbus::utility::MapperGetSubTreePathsResponse&
                trustedComponentPaths) {
            handleTrustedComponentPath(componentId, asyncResp,
                                       trustedComponentPaths, callback);
        });
}

inline void getTrustedComponentAsset(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& path, const std::string& service)
{
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, service, path,
        "xyz.openbmc_project.Inventory.Decorator.Asset",
        [path, asyncResp{asyncResp}](
            const boost::system::error_code& ec,
            const dbus::utility::DBusPropertiesMap& assetList) {
            if (ec)
            {
                if (ec.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR("DBUS response error for Properties {}",
                                     ec.value());
                    messages::internalError(asyncResp->res);
                }
                return;
            }
            const std::string* manufacturer = nullptr;
            const std::string* serialNumber = nullptr;

            const bool success = sdbusplus::unpackPropertiesNoThrow(
                dbus_utils::UnpackErrorPrinter(), assetList, "Manufacturer",
                manufacturer, "SerialNumber", serialNumber);
            if (!success)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            if (manufacturer != nullptr && !manufacturer->empty())
            {
                asyncResp->res.jsonValue["Manufacturer"] = *manufacturer;
            }
            if (serialNumber != nullptr && !serialNumber->empty())
            {
                asyncResp->res.jsonValue["SerialNumber"] = *serialNumber;
            }
        });
}

inline void getTrustedComponentVersion(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& path, const std::string& service)
{
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, service, path,
        "xyz.openbmc_project.Software.Version", "Version",
        [asyncResp](const boost::system::error_code& ec,
                    const std::string& version) {
            if (ec)
            {
                if (ec.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR("DBUS response error for version {}",
                                     ec.value());
                    messages::internalError(asyncResp->res);
                }
                return;
            }
            asyncResp->res.jsonValue["FirmwareVersion"] = version;
        });
}

inline void getTrustedComponentType(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    [[maybe_unused]] const std::string& path,
    [[maybe_unused]] const std::string& service)
{
    // TPMs are always a Discrete components
    asyncResp->res.jsonValue["TrustedComponentType"] = "Discrete";
}

inline void afterGetValidTrustedComponentPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& componentId,
    const std::string& path, const std::string& service)
{
    addTrustedComponentCommonProperties(asyncResp->res, chassisId, componentId);
    getTrustedComponentVersion(asyncResp, path, service);
    getTrustedComponentAsset(asyncResp, path, service);
    getTrustedComponentType(asyncResp, path, service);
}

inline void doTrustedComponentGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& componentId,
    const std::optional<std::string>& validChassisPath)
{
    if (!validChassisPath)
    {
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
        return;
    }

    getValidTrustedComponentPath(
        asyncResp, *validChassisPath, componentId,
        std::bind_front(afterGetValidTrustedComponentPath, asyncResp, chassisId,
                        componentId));
}

inline void handleTrustedComponentGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& componentId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    redfish::chassis_utils::getValidChassisPath(
        asyncResp, chassisId,
        std::bind_front(doTrustedComponentGet, asyncResp, chassisId,
                        componentId));
}

inline void handleTrustedComponentHead(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& componentId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    redfish::chassis_utils::getValidChassisPath(
        asyncResp, chassisId,
        [asyncResp, chassisId,
         componentId](const std::optional<std::string>& validChassisPath) {
            if (!validChassisPath)
            {
                messages::resourceNotFound(asyncResp->res, "Chassis",
                                           chassisId);
                return;
            }
            getValidTrustedComponentPath(
                asyncResp, *validChassisPath, componentId,
                [asyncResp](const std::string&, const std::string&) {
                    asyncResp->res.addHeader(
                        boost::beast::http::field::link,
                        "</redfish/v1/JsonSchemas/TrustedComponent/TrustedComponent.json>; rel=describedby");
                });
        });
}

inline void requestRoutesTrustedComponents(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/TrustedComponents/")
        .privileges(redfish::privileges::headTrustedComponentCollection)
        .methods(boost::beast::http::verb::head)(std::bind_front(
            handleTrustedComponentsCollectionHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/TrustedComponents/")
        .privileges(redfish::privileges::getTrustedComponentCollection)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleTrustedComponentsCollectionGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/TrustedComponents/<str>/")
        .privileges(redfish::privileges::headTrustedComponent)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleTrustedComponentHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/TrustedComponents/<str>/")
        .privileges(redfish::privileges::getTrustedComponent)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleTrustedComponentGet, std::ref(app)));
}

} // namespace redfish
