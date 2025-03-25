// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2018 Intel Corporation
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/chassis_utils.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace redfish
{

const std::array<std::string_view, 1> trustedComponentInterfaces = {
    "xyz.openbmc_project.Inventory.Item.Tpm"};

inline void addTPMCommonProperties(crow::Response& resp,
                                   const std::string& chassisId,
                                   const std::string& componentID)
{
    resp.jsonValue["@odata.type"] = "#TrustedComponent.v1_0_0.TrustedComponent";
    resp.jsonValue["Name"] = componentID;
    resp.jsonValue["Id"] = componentID;
    resp.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/TrustedComponents/{}", chassisId, componentID);
}

inline void updateTPMList(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId,
    const dbus::utility::MapperGetSubTreePathsResponse& tpmPaths)
{
    nlohmann::json& tpmList = asyncResp->res.jsonValue["Members"];
    for (const std::string& tpmPath : tpmPaths)
    {
        std::string tpmName =
            sdbusplus::message::object_path(tpmPath).filename();
        if (tpmName.empty())
        {
            continue;
        }

        nlohmann::json item = nlohmann::json::object();
        item["@odata.id"] = boost::urls::format(
            "/redfish/v1/Chassis/{}/TrustedComponents/{}", chassisId, tpmName);

        tpmList.emplace_back(std::move(item));
    }
    asyncResp->res.jsonValue["Members@odata.count"] = tpmList.size();
}

inline void getTPMPaths(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& validChassisPath,
    const std::function<void(const dbus::utility::MapperGetSubTreePathsResponse&
                                 tpmPaths)>& callback)
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

inline void doTPMCollection(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& chassisId,
                            const std::optional<std::string>& validChassisPath)
{
    if (!validChassisPath)
    {
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
        return;
    }
    asyncResp->res.jsonValue["@odata.type"] =
        "#TrustedComponentCollection.TrustedComponentCollection";
    asyncResp->res.jsonValue["Name"] = "Trusted Component Collection";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/TrustedComponents", chassisId);
    asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
    asyncResp->res.jsonValue["Members@odata.count"] = 0;
    getTPMPaths(asyncResp, *validChassisPath,
                std::bind_front(updateTPMList, asyncResp, chassisId));
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
        std::bind_front(doTPMCollection, asyncResp, chassisId));
}

inline bool checkTPMId(const std::string& tpmPath,
                       const std::string& componentID)
{
    std::string tpmName = sdbusplus::message::object_path(tpmPath).filename();

    return !(tpmName.empty() || tpmName != componentID);
}

inline void handleTPMPath(
    const std::string& componentID,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const dbus::utility::MapperGetSubTreePathsResponse& tpmPaths,
    const std::function<void(const std::string& tpmPath,
                             const std::string& service)>& callback)
{
    for (const auto& tpmPath : tpmPaths)
    {
        if (!checkTPMId(tpmPath, componentID))
        {
            continue;
        }
        dbus::utility::getDbusObject(
            tpmPath, trustedComponentInterfaces,
            [tpmPath, asyncResp,
             callback](const boost::system::error_code& ec,
                       const dbus::utility::MapperGetObject& object) {
                if (ec || object.empty())
                {
                    BMCWEB_LOG_ERROR("DBUS response error on getDbusObject {}",
                                     ec.value());
                    messages::internalError(asyncResp->res);
                    return;
                }
                callback(tpmPath, object.begin()->first);
            });

        return;
    }
    BMCWEB_LOG_WARNING("TPM not found {}", componentID);
    messages::resourceNotFound(asyncResp->res, "TrustedComponents",
                               componentID);
}

inline void getValidTPMPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& validChassisPath, const std::string& componentID,
    const std::function<void(const std::string& fanPath,
                             const std::string& service)>& callback)
{
    getTPMPaths(
        asyncResp, validChassisPath,
        [componentID, asyncResp, callback](
            const dbus::utility::MapperGetSubTreePathsResponse& tpmPaths) {
            handleTPMPath(componentID, asyncResp, tpmPaths, callback);
        });
}

inline void getTPMAsset(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& tpmPath, const std::string& service)
{
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, service, tpmPath,
        "xyz.openbmc_project.Inventory.Decorator.Asset",
        [tpmPath, asyncResp{asyncResp}](
            const boost::system::error_code& ec,
            const dbus::utility::DBusPropertiesMap& assetList) {
            if (ec)
            {
                if (ec.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR("DBUS response error for Properties{}",
                                     ec.value());
                    messages::internalError(asyncResp->res);
                }
                return;
            }
            const std::string* manufacturer = nullptr;

            const bool success = sdbusplus::unpackPropertiesNoThrow(
                dbus_utils::UnpackErrorPrinter(), assetList, "Manufacturer",
                manufacturer);
            if (!success)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            if (manufacturer != nullptr)
            {
                asyncResp->res.jsonValue["Manufacturer"] = *manufacturer;
            }
        });
}

inline void getTPMVersion(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& tpmPath,
                          const std::string& service)
{
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, service, tpmPath,
        "xyz.openbmc_project.Software.Version", "Version",
        [asyncResp](const boost::system::error_code& ec2,
                    const std::string& version) {
            if (ec2)
            {
                if (ec2.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR("DBUS response error for version{}",
                                     ec2.value());
                    messages::internalError(asyncResp->res);
                }
                return;
            }
            asyncResp->res.jsonValue["FirmwareVersion"] = version;
        });
}

inline void afterGetValidTPMPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& componentID,
    const std::string& tpmPath, const std::string& service)
{
    addTPMCommonProperties(asyncResp->res, chassisId, componentID);
    getTPMVersion(asyncResp, tpmPath, service);
    getTPMAsset(asyncResp, tpmPath, service);
}

inline void doTPMGet(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const std::string& chassisId,
                     const std::string& componentID,
                     const std::optional<std::string>& validChassisPath)
{
    if (!validChassisPath)
    {
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
        return;
    }

    getValidTPMPath(asyncResp, *validChassisPath, componentID,
                    std::bind_front(afterGetValidTPMPath, asyncResp, chassisId,
                                    componentID));
}

inline void handleTrustedComponentGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& componentID)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    redfish::chassis_utils::getValidChassisPath(
        asyncResp, chassisId,
        std::bind_front(doTPMGet, asyncResp, chassisId, componentID));
}

inline void requestRoutesTrustedComponents(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/TrustedComponents/")
        .privileges(redfish::privileges::privilegeSetLogin)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleTrustedComponentsCollectionGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/TrustedComponents/<str>/")
        .privileges(redfish::privileges::privilegeSetLogin)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleTrustedComponentGet, std::ref(app)));
}

} // namespace redfish
