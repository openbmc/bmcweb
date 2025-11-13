// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/resource.hpp"
#include "http_request.hpp"
#include "led.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/asset_utils.hpp"
#include "utils/chassis_utils.hpp"
#include "utils/fan_utils.hpp"
#include "utils/json_utils.hpp"

#include <asm-generic/errno.h>

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace redfish
{
inline void updateFanList(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId,
    const dbus::utility::MapperGetSubTreePathsResponse& fanPaths)
{
    nlohmann::json& fanList = asyncResp->res.jsonValue["Members"];
    for (const std::string& fanPath : fanPaths)
    {
        std::string fanName =
            sdbusplus::message::object_path(fanPath).filename();
        if (fanName.empty())
        {
            continue;
        }

        nlohmann::json item = nlohmann::json::object();
        item["@odata.id"] = boost::urls::format(
            "/redfish/v1/Chassis/{}/ThermalSubsystem/Fans/{}", chassisId,
            fanName);

        fanList.emplace_back(std::move(item));
    }
    asyncResp->res.jsonValue["Members@odata.count"] = fanList.size();
}

inline void doFanCollection(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
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
        "</redfish/v1/JsonSchemas/FanCollection/FanCollection.json>; rel=describedby");
    asyncResp->res.jsonValue["@odata.type"] = "#FanCollection.FanCollection";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/ThermalSubsystem/Fans", chassisId);
    asyncResp->res.jsonValue["Name"] = "Fan Collection";
    asyncResp->res.jsonValue["Description"] =
        "The collection of Fan resource instances " + chassisId;
    asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
    asyncResp->res.jsonValue["Members@odata.count"] = 0;

    fan_utils::getFanPaths(
        asyncResp, *validChassisPath,
        std::bind_front(updateFanList, asyncResp, chassisId));
}

inline void handleFanCollectionHead(
    App& app, const crow::Request& req,
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
                "</redfish/v1/JsonSchemas/FanCollection/FanCollection.json>; rel=describedby");
        });
}

inline void handleFanCollectionGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    redfish::chassis_utils::getValidChassisPath(
        asyncResp, chassisId,
        std::bind_front(doFanCollection, asyncResp, chassisId));
}

inline void requestRoutesFanCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/ThermalSubsystem/Fans/")
        .privileges(redfish::privileges::headFanCollection)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleFanCollectionHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/ThermalSubsystem/Fans/")
        .privileges(redfish::privileges::getFanCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleFanCollectionGet, std::ref(app)));
}

inline bool checkFanId(const std::string& fanPath, const std::string& fanId)
{
    std::string fanName = sdbusplus::message::object_path(fanPath).filename();

    return !(fanName.empty() || fanName != fanId);
}

inline void handleFanPath(
    const std::string& fanId,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const dbus::utility::MapperGetSubTreePathsResponse& fanPaths,
    const std::function<void(const std::string& fanPath,
                             const std::string& service)>& callback)
{
    for (const auto& fanPath : fanPaths)
    {
        if (!checkFanId(fanPath, fanId))
        {
            continue;
        }
        dbus::utility::getDbusObject(
            fanPath, fanInterface,
            [fanPath, asyncResp,
             callback](const boost::system::error_code& ec,
                       const dbus::utility::MapperGetObject& object) {
                if (ec || object.empty())
                {
                    BMCWEB_LOG_ERROR("DBUS response error on getDbusObject {}",
                                     ec.value());
                    messages::internalError(asyncResp->res);
                    return;
                }
                callback(fanPath, object.begin()->first);
            });

        return;
    }
    BMCWEB_LOG_WARNING("Fan not found {}", fanId);
    messages::resourceNotFound(asyncResp->res, "Fan", fanId);
}

inline void getValidFanObject(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& validChassisPath, const std::string& fanId,
    const std::function<void(const std::string& fanPath,
                             const std::string& service)>& callback)
{
    fan_utils::getFanPaths(
        asyncResp, validChassisPath,
        [fanId, asyncResp, callback](
            const dbus::utility::MapperGetSubTreePathsResponse& fanPaths) {
            handleFanPath(fanId, asyncResp, fanPaths, callback);
        });
}

inline void addFanCommonProperties(crow::Response& resp,
                                   const std::string& chassisId,
                                   const std::string& fanId)
{
    resp.addHeader(boost::beast::http::field::link,
                   "</redfish/v1/JsonSchemas/Fan/Fan.json>; rel=describedby");
    resp.jsonValue["@odata.type"] = "#Fan.v1_3_0.Fan";
    resp.jsonValue["Name"] = "Fan";
    resp.jsonValue["Id"] = fanId;
    resp.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/ThermalSubsystem/Fans/{}", chassisId, fanId);
    resp.jsonValue["Status"]["State"] = resource::State::Enabled;
    resp.jsonValue["Status"]["Health"] = resource::Health::OK;
}

inline void getFanHealth(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& fanPath, const std::string& service)
{
    dbus::utility::getProperty<bool>(
        service, fanPath,
        "xyz.openbmc_project.State.Decorator.OperationalStatus", "Functional",
        [asyncResp](const boost::system::error_code& ec, const bool value) {
            if (ec)
            {
                if (ec.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR("DBUS response error for Health {}",
                                     ec.value());
                    messages::internalError(asyncResp->res);
                }
                return;
            }

            if (!value)
            {
                asyncResp->res.jsonValue["Status"]["Health"] =
                    resource::Health::Critical;
            }
        });
}

inline void getFanState(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& fanPath, const std::string& service)
{
    dbus::utility::getProperty<bool>(
        service, fanPath, "xyz.openbmc_project.Inventory.Item", "Present",
        [asyncResp](const boost::system::error_code& ec, const bool value) {
            if (ec)
            {
                if (ec.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR("DBUS response error for State {}",
                                     ec.value());
                    messages::internalError(asyncResp->res);
                }
                return;
            }

            if (!value)
            {
                asyncResp->res.jsonValue["Status"]["State"] =
                    resource::State::Absent;
            }
        });
}

inline void getFanLocation(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& fanPath,
                           const std::string& service)
{
    dbus::utility::getProperty<std::string>(
        service, fanPath,
        "xyz.openbmc_project.Inventory.Decorator.LocationCode", "LocationCode",
        [asyncResp](const boost::system::error_code& ec,
                    const std::string& property) {
            if (ec)
            {
                if (ec.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR("DBUS response error for Location{}",
                                     ec.value());
                    messages::internalError(asyncResp->res);
                }
                return;
            }
            asyncResp->res
                .jsonValue["Location"]["PartLocation"]["ServiceLabel"] =
                property;
        });
}

inline void afterGetValidFanObject(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& fanId,
    const std::string& fanPath, const std::string& service)
{
    addFanCommonProperties(asyncResp->res, chassisId, fanId);
    getFanState(asyncResp, fanPath, service);
    getFanHealth(asyncResp, fanPath, service);
    asset_utils::getAssetInfo(asyncResp, service, fanPath, ""_json_pointer,
                              true);
    getFanLocation(asyncResp, fanPath, service);
    getLocationIndicatorActive(asyncResp, fanPath);
}

inline void doFanGet(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const std::string& chassisId, const std::string& fanId,
                     const std::optional<std::string>& validChassisPath)
{
    if (!validChassisPath)
    {
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
        return;
    }

    getValidFanObject(
        asyncResp, *validChassisPath, fanId,
        std::bind_front(afterGetValidFanObject, asyncResp, chassisId, fanId));
}

inline void handleFanHead(App& app, const crow::Request& req,
                          const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& chassisId,
                          const std::string& fanId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    redfish::chassis_utils::getValidChassisPath(
        asyncResp, chassisId,
        [asyncResp, chassisId,
         fanId](const std::optional<std::string>& validChassisPath) {
            if (!validChassisPath)
            {
                messages::resourceNotFound(asyncResp->res, "Chassis",
                                           chassisId);
                return;
            }
            getValidFanObject(
                asyncResp, *validChassisPath, fanId,
                [asyncResp](const std::string&, const std::string&) {
                    asyncResp->res.addHeader(
                        boost::beast::http::field::link,
                        "</redfish/v1/JsonSchemas/Fan/Fan.json>; rel=describedby");
                });
        });
}

inline void handleFanGet(App& app, const crow::Request& req,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& chassisId, const std::string& fanId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    redfish::chassis_utils::getValidChassisPath(
        asyncResp, chassisId,
        std::bind_front(doFanGet, asyncResp, chassisId, fanId));
}

inline void handleSetFanPathById(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& fanId,
    bool locationIndicatorActive, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& fanPaths)
{
    if (ec)
    {
        if (ec.value() == boost::system::errc::io_error)
        {
            BMCWEB_LOG_WARNING("Chassis {} not found", chassisId);
            messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
            return;
        }

        BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }

    for (const auto& fanPath : fanPaths)
    {
        if (checkFanId(fanPath, fanId))
        {
            setLocationIndicatorActive(asyncResp, fanPath,
                                       locationIndicatorActive);
            return;
        }
    }
    BMCWEB_LOG_WARNING("Fan {} not found", fanId);
    messages::resourceNotFound(asyncResp->res, "Fan", fanId);
}

inline void handleFanPatch(App& app, const crow::Request& req,
                           const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& chassisId,
                           const std::string& fanId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    std::optional<bool> locationIndicatorActive;
    if (!json_util::readJsonPatch(req, asyncResp->res,
                                  "LocationIndicatorActive",
                                  locationIndicatorActive))
    {
        return;
    }

    if (locationIndicatorActive)
    {
        dbus::utility::getAssociatedSubTreePathsById(
            chassisId, "/xyz/openbmc_project/inventory", chassisInterfaces,
            "cooled_by", fanInterface,
            [asyncResp, chassisId, fanId, locationIndicatorActive](
                const boost::system::error_code& ec,
                const dbus::utility::MapperGetSubTreePathsResponse&
                    subtreePaths) {
                handleSetFanPathById(asyncResp, chassisId, fanId,
                                     *locationIndicatorActive, ec,
                                     subtreePaths);
            });
    }
}

inline void requestRoutesFan(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/ThermalSubsystem/Fans/<str>/")
        .privileges(redfish::privileges::headFan)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleFanHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/ThermalSubsystem/Fans/<str>/")
        .privileges(redfish::privileges::getFan)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleFanGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/ThermalSubsystem/Fans/<str>/")
        .privileges(redfish::privileges::patchFan)
        .methods(boost::beast::http::verb::patch)(
            std::bind_front(handleFanPatch, std::ref(app)));
}

} // namespace redfish
