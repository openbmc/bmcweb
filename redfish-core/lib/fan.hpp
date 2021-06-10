#pragma once

#include "app.hpp"
#include "error_messages.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/chassis_utils.hpp"

#include <boost/url/format.hpp>

#include <memory>
#include <optional>
#include <string>

namespace redfish
{

inline void updateFanList(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& chassisId,
                          const std::string& fanPath)
{
    std::string fanName = sdbusplus::message::object_path(fanPath).filename();
    if (fanName.empty())
    {
        return;
    }

    nlohmann::json item;
    item["@odata.id"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/ThermalSubsystem/Fans/{}", chassisId, fanName);

    nlohmann::json& fanList = asyncResp->res.jsonValue["Members"];
    fanList.emplace_back(std::move(item));
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

    dbus::utility::getAssociationEndPoints(
        *validChassisPath + "/cooled_by",
        [asyncResp,
         chassisId](const boost::system::error_code& ec,
                    const dbus::utility::MapperEndPoints& endpoints) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            return;
        }

        for (const auto& endpoint : endpoints)
        {
            updateFanList(asyncResp, chassisId, endpoint);
        }
        });
}

inline void
    handleFanCollectionHead(App& app, const crow::Request& req,
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
            messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
            return;
        }
        asyncResp->res.addHeader(
            boost::beast::http::field::link,
            "</redfish/v1/JsonSchemas/FanCollection/FanCollection.json>; rel=describedby");
        });
}

inline void
    handleFanCollectionGet(App& app, const crow::Request& req,
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

static inline void handleFanPath(
    const std::string& fanId,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const dbus::utility::MapperGetSubTreePathsResponse& fanPaths,
    const std::function<void(const std::string& fanPath,
                             const std::string& service)>& callback)

{
    for (const std::string& fanPath : fanPaths)
    {
        std::string fanName =
            sdbusplus::message::object_path(fanPath).filename();
        if (!checkFanId(fanPath, fanId))
        {
            continue;
        }
        dbus::utility::getDbusObject(
            fanPath, {},
            [fanPath, asyncResp,
             callback](const boost::system::error_code& ec,
                       const dbus::utility::MapperGetObject& object) {
            if (ec || object.empty())
            {
                BMCWEB_LOG_ERROR << "DBUS response error on getDbusObject"
                                 << ec;
                messages::internalError(asyncResp->res);
                return;
            }
            callback(fanPath, object.begin()->first);
            });

        return;
    }
    BMCWEB_LOG_WARNING << "Fan not found";
    messages::resourceNotFound(asyncResp->res, "Fan", fanId);
}

inline void getValidFanPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& validChassisPath, const std::string& fanId,
    const std::function<void(const std::string& fanPath,
                             const std::string& service)>& callback)
{
    std::string endpointPath = validChassisPath + "/cooled_by";
    dbus::utility::getAssociationEndPoints(
        endpointPath, [asyncResp, fanId, callback](
                          const boost::system::error_code& ec,
                          const dbus::utility::MapperEndPoints& fanPaths) {
            if (ec)
            {
                if (ec.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR
                        << "D-Bus response error on getAssociationEndPoints "
                        << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
                return;
            }
            handleFanPath(fanId, asyncResp, fanPaths, callback);
            return;
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
    resp.jsonValue["Status"]["State"] = "Enabled";
    resp.jsonValue["Status"]["Health"] = "OK";
}

inline void
    getFanHealth(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& fanPath, const std::string& service)
{
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, service, fanPath,
        "xyz.openbmc_project.State.Decorator.OperationalStatus", "Functional",
        [asyncResp](const boost::system::error_code& ec, const bool value) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR << "DBUS response error for Health";
                messages::internalError(asyncResp->res);
            }
            return;
        }

        if (!value)
        {
            asyncResp->res.jsonValue["Status"]["Health"] = "Critical";
        }
        });
}

inline void
    getFanState(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& fanPath, const std::string& service)
{
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, service, fanPath,
        "xyz.openbmc_project.Inventory.Item", "Present",
        [asyncResp](const boost::system::error_code& ec, const bool value) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR << "DBUS response error for State";
                messages::internalError(asyncResp->res);
            }
            return;
        }

        if (!value)
        {
            asyncResp->res.jsonValue["Status"]["State"] = "Absent";
        }
        });
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

    getValidFanPath(asyncResp, *validChassisPath, fanId,
                    [asyncResp, chassisId, fanId](const std::string& fanPath,
                                                  const std::string& service) {
        addFanCommonProperties(asyncResp->res, chassisId, fanId);
        getFanState(asyncResp, fanPath, service);
        getFanHealth(asyncResp, fanPath, service);
    });
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
            messages::resourceNotFound(asyncResp->res, "Fan", fanId);
            return;
        }
        getValidFanPath(asyncResp, *validChassisPath, fanId,
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
}

} // namespace redfish
