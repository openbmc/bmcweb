#pragma once

#include "app.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "led.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/chassis_utils.hpp"
#include "utils/json_utils.hpp"

#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/message/types.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace redfish
{
constexpr std::array<std::string_view, 1> fanInterface = {
    "xyz.openbmc_project.Inventory.Item.Fan"};

inline void
    updateFanList(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
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

inline void getFanPaths(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::optional<std::string>& validChassisPath,
    const std::function<void(const dbus::utility::MapperGetSubTreePathsResponse&
                                 fanPaths)>& callback)
{
    sdbusplus::message::object_path endpointPath{*validChassisPath};
    endpointPath /= "cooled_by";

    dbus::utility::getAssociatedSubTreePaths(
        endpointPath,
        sdbusplus::message::object_path("/xyz/openbmc_project/inventory"), 0,
        fanInterface,
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

    getFanPaths(asyncResp, validChassisPath,
                std::bind_front(updateFanList, asyncResp, chassisId));
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

inline void getValidFanPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& validChassisPath, const std::string& fanId,
    const std::function<void(const std::string& fanPath,
                             const std::string& service)>& callback)
{
    getFanPaths(
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
    resp.jsonValue["Status"]["State"] = "Enabled";
    resp.jsonValue["Status"]["Health"] = "OK";
}

inline void getFanHealth(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
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
                BMCWEB_LOG_ERROR("DBUS response error for Health {}",
                                 ec.value());
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

inline void getFanState(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
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
                BMCWEB_LOG_ERROR("DBUS response error for State {}",
                                 ec.value());
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

inline void getFanAsset(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& fanPath, const std::string& service)
{
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, service, fanPath,
        "xyz.openbmc_project.Inventory.Decorator.Asset",
        [fanPath, asyncResp{asyncResp}](
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
        const std::string* model = nullptr;
        const std::string* partNumber = nullptr;
        const std::string* serialNumber = nullptr;
        const std::string* sparePartNumber = nullptr;

        const bool success = sdbusplus::unpackPropertiesNoThrow(
            dbus_utils::UnpackErrorPrinter(), assetList, "Manufacturer",
            manufacturer, "Model", model, "PartNumber", partNumber,
            "SerialNumber", serialNumber, "SparePartNumber", sparePartNumber);
        if (!success)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        if (manufacturer != nullptr)
        {
            asyncResp->res.jsonValue["Manufacturer"] = *manufacturer;
        }
        if (model != nullptr)
        {
            asyncResp->res.jsonValue["Model"] = *model;
        }
        if (partNumber != nullptr)
        {
            asyncResp->res.jsonValue["PartNumber"] = *partNumber;
        }
        if (serialNumber != nullptr)
        {
            asyncResp->res.jsonValue["SerialNumber"] = *serialNumber;
        }
        if (sparePartNumber != nullptr && !sparePartNumber->empty())
        {
            asyncResp->res.jsonValue["SparePartNumber"] = *sparePartNumber;
        }
    });
}

inline void getFanLocation(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& fanPath,
                           const std::string& service)
{
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, service, fanPath,
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
        asyncResp->res.jsonValue["Location"]["PartLocation"]["ServiceLabel"] =
            property;
    });
}

inline void
    afterGetValidFanPath(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& chassisId, const std::string& fanId,
                         const std::string& fanPath, const std::string& service)
{
    addFanCommonProperties(asyncResp->res, chassisId, fanId);
    getFanState(asyncResp, fanPath, service);
    getFanHealth(asyncResp, fanPath, service);
    getFanAsset(asyncResp, fanPath, service);
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

    getValidFanPath(
        asyncResp, *validChassisPath, fanId,
        std::bind_front(afterGetValidFanPath, asyncResp, chassisId, fanId));
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
            messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
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

inline void doPatchFan(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const bool locationIndicatorActive,
                       const std::string& fanPath,
                       const std::string& /* service */)
{
    setLocationIndicatorActive(asyncResp, fanPath, locationIndicatorActive);
}

inline void fanPatchAfterValidateChassis(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& fanId,
    const bool locationIndicatorActive,
    const std::optional<std::string>& validChassisPath)
{
    if (!validChassisPath)
    {
        BMCWEB_LOG_WARNING("Not a valid chassis ID: {}", chassisId);
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
        return;
    }

    // Verify that the fan has the correct chassis and whether fan has a
    // chassis association
    getValidFanPath(
        asyncResp, *validChassisPath, fanId,
        std::bind_front(doPatchFan, asyncResp, locationIndicatorActive));
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
        redfish::chassis_utils::getValidChassisPath(
            asyncResp, chassisId,
            std::bind_front(fanPatchAfterValidateChassis, asyncResp, chassisId,
                            fanId, *locationIndicatorActive));
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
