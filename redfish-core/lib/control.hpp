// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/control.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/chassis_utils.hpp"
#include "utils/collection.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"

#include <asm-generic/errno.h>

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace redfish
{

constexpr std::string_view controlPowerCapInterface =
    "xyz.openbmc_project.Control.Power.Cap";

constexpr std::array<std::string_view, 1> controlInterfaces = {
    controlPowerCapInterface};

inline void afterGetControlObjects(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::function<void(const dbus::utility::MapperGetSubTreeResponse&)>&
        callback,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        if (ec.value() != boost::system::errc::io_error && ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error {}", ec);
            messages::internalError(asyncResp->res);
            return;
        }
        // The chassis has no controlled_by association, which is not an
        // error.  Report it as an empty set of controls.
        callback({});
        return;
    }
    callback(subtree);
}

inline void getControlObjects(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& validChassisPath,
    const std::function<void(const dbus::utility::MapperGetSubTreeResponse&)>&
        callback)
{
    sdbusplus::object_path endpointPath{validChassisPath};
    endpointPath /= "controlled_by";

    dbus::utility::getAssociatedSubTree(
        endpointPath, sdbusplus::object_path("/xyz/openbmc_project/control"), 0,
        controlInterfaces,
        std::bind_front(afterGetControlObjects, asyncResp, callback));
}

inline void updateControlList(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    dbus::utility::MapperGetSubTreePathsResponse controlPaths;
    controlPaths.reserve(subtree.size());
    for (const auto& [controlPath, serviceMap] : subtree)
    {
        controlPaths.emplace_back(controlPath);
    }

    // getControlObjects has already reported any D-Bus failure, so the members
    // are always populated from a successful lookup here.
    collection_util::handleCollectionMembers(
        asyncResp,
        boost::urls::format("/redfish/v1/Chassis/{}/Controls", chassisId),
        nlohmann::json::json_pointer("/Members"), boost::system::error_code{},
        controlPaths);
}

inline void doControlCollection(
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
        "</redfish/v1/JsonSchemas/ControlCollection/ControlCollection.json>; rel=describedby");
    asyncResp->res.jsonValue["@odata.type"] =
        "#ControlCollection.ControlCollection";
    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Chassis/{}/Controls", chassisId);
    asyncResp->res.jsonValue["Name"] = "Controls";
    asyncResp->res.jsonValue["Description"] =
        "The collection of Control resource instances for " + chassisId;

    getControlObjects(asyncResp, *validChassisPath,
                      std::bind_front(updateControlList, asyncResp, chassisId));
}

inline void handleControlCollectionGet(
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
        std::bind_front(doControlCollection, asyncResp, chassisId));
}

inline void handleControlPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& controlId,
    const std::function<void(const std::string& controlPath,
                             const std::string& service)>& callback,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    for (const auto& [controlPath, serviceMap] : subtree)
    {
        if (sdbusplus::object_path(controlPath).filename() != controlId)
        {
            continue;
        }

        if (serviceMap.empty())
        {
            BMCWEB_LOG_ERROR("No service found for control path {}",
                             controlPath);
            messages::internalError(asyncResp->res);
            return;
        }

        callback(controlPath, serviceMap.begin()->first);
        return;
    }
    BMCWEB_LOG_WARNING("Control not found {}", controlId);
    messages::resourceNotFound(asyncResp->res, "Control", controlId);
}

inline void getValidControlObject(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& validChassisPath, const std::string& controlId,
    const std::function<void(const std::string& controlPath,
                             const std::string& service)>& callback)
{
    getControlObjects(
        asyncResp, validChassisPath,
        std::bind_front(handleControlPath, asyncResp, controlId, callback));
}

inline void afterGetPowerCapProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& properties)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error for Power.Cap: {}", ec);
            messages::internalError(asyncResp->res);
        }
        return;
    }

    const uint32_t* maxPowerCapValue = nullptr;
    const uint32_t* minPowerCapValue = nullptr;
    const uint32_t* powerCap = nullptr;
    const bool* powerCapEnable = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "MaxPowerCapValue",
        maxPowerCapValue, "MinPowerCapValue", minPowerCapValue, "PowerCap",
        powerCap, "PowerCapEnable", powerCapEnable);

    if (!success)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    constexpr uint32_t invalidPowerCapValue =
        std::numeric_limits<uint32_t>::max();

    if (maxPowerCapValue != nullptr)
    {
        asyncResp->res.jsonValue["AllowableMax"] = *maxPowerCapValue;
    }

    if (minPowerCapValue != nullptr)
    {
        asyncResp->res.jsonValue["AllowableMin"] = *minPowerCapValue;
    }

    if (powerCapEnable != nullptr)
    {
        if (*powerCapEnable)
        {
            asyncResp->res.jsonValue["ControlMode"] =
                control::ControlMode::Automatic;
        }
        else
        {
            asyncResp->res.jsonValue["ControlMode"] =
                control::ControlMode::Disabled;
        }
    }

    if (powerCap != nullptr)
    {
        if (*powerCap == invalidPowerCapValue)
        {
            asyncResp->res.jsonValue["SetPoint"] = nullptr;
        }
        else
        {
            asyncResp->res.jsonValue["SetPoint"] = *powerCap;
        }
    }
}

inline void afterGetValidControlObject(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& controlId,
    const std::string& controlPath, const std::string& service)
{
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/Control/Control.json>; rel=describedby");
    asyncResp->res.jsonValue["@odata.type"] = "#Control.v1_3_0.Control";
    asyncResp->res.jsonValue["Name"] = "Control";
    asyncResp->res.jsonValue["Id"] = controlId;
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/Controls/{}", chassisId, controlId);
    asyncResp->res.jsonValue["ControlType"] = control::ControlType::Power;
    asyncResp->res.jsonValue["SetPointUnits"] = "W";

    dbus::utility::getAllProperties(
        service, controlPath, std::string(controlPowerCapInterface),
        std::bind_front(afterGetPowerCapProperties, asyncResp));
}

inline void doControlGet(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& chassisId,
                         const std::string& controlId,
                         const std::optional<std::string>& validChassisPath)
{
    if (!validChassisPath)
    {
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
        return;
    }

    getValidControlObject(asyncResp, *validChassisPath, controlId,
                          std::bind_front(afterGetValidControlObject, asyncResp,
                                          chassisId, controlId));
}

inline void handleControlGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& controlId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    redfish::chassis_utils::getValidChassisPath(
        asyncResp, chassisId,
        std::bind_front(doControlGet, asyncResp, chassisId, controlId));
}

// Power.Cap can only be turned on or off, so Automatic is the only active
// mode that can be represented.  Returns std::nullopt for the modes that
// Power.Cap cannot express.
inline std::optional<bool> powerCapEnableFromControlMode(
    std::string_view controlMode)
{
    if (controlMode == "Automatic")
    {
        return true;
    }
    if (controlMode == "Disabled")
    {
        return false;
    }
    return std::nullopt;
}

inline void afterGetValidControlObjectForPatch(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::optional<uint32_t>& setPoint,
    const std::optional<bool>& powerCapEnable, const std::string& controlPath,
    const std::string& service)
{
    if (setPoint)
    {
        setDbusProperty(asyncResp, "SetPoint", service,
                        sdbusplus::object_path(controlPath),
                        controlPowerCapInterface, "PowerCap", *setPoint);
    }

    if (powerCapEnable)
    {
        setDbusProperty(asyncResp, "ControlMode", service,
                        sdbusplus::object_path(controlPath),
                        controlPowerCapInterface, "PowerCapEnable",
                        *powerCapEnable);
    }
}

inline void doControlPatch(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& controlId,
    const std::optional<uint32_t>& setPoint,
    const std::optional<bool>& powerCapEnable,
    const std::optional<std::string>& validChassisPath)
{
    if (!validChassisPath)
    {
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
        return;
    }

    getValidControlObject(asyncResp, *validChassisPath, controlId,
                          std::bind_front(afterGetValidControlObjectForPatch,
                                          asyncResp, setPoint, powerCapEnable));
}

inline void handleControlPatch(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& controlId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    std::optional<uint32_t> setPoint;
    std::optional<std::string> controlMode;
    if (!json_util::readJsonPatch(req, asyncResp->res, "SetPoint", setPoint,
                                  "ControlMode", controlMode))
    {
        return;
    }

    std::optional<bool> powerCapEnable;
    if (controlMode)
    {
        powerCapEnable = powerCapEnableFromControlMode(*controlMode);
        if (!powerCapEnable)
        {
            messages::propertyValueNotInList(asyncResp->res, *controlMode,
                                             "ControlMode");
            return;
        }
    }

    if (!setPoint && !powerCapEnable)
    {
        return;
    }

    redfish::chassis_utils::getValidChassisPath(
        asyncResp, chassisId,
        std::bind_front(doControlPatch, asyncResp, chassisId, controlId,
                        setPoint, powerCapEnable));
}

inline void requestRoutesControl(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Controls/")
        .privileges(redfish::privileges::getControlCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleControlCollectionGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Controls/<str>/")
        .privileges(redfish::privileges::getControl)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleControlGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Controls/<str>/")
        .privileges(redfish::privileges::patchControl)
        .methods(boost::beast::http::verb::patch)(
            std::bind_front(handleControlPatch, std::ref(app)));
}

} // namespace redfish
