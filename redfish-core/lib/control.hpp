// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/control.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/chassis_utils.hpp"

#include <boost/beast/http/verb.hpp>
#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <algorithm>
#include <format>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace redfish
{

// Mapping from a D-Bus interface to its Redfish control type prefix.
// To add a new control type, add an entry here and implement its
// populate function in populateControlProperties().
struct ControlTypeMapping
{
    std::string_view prefix;
    std::string_view requiredInterface;
};

constexpr std::array<ControlTypeMapping, 1> controlTypeMappings = {{
    {"ClockLimit", "xyz.openbmc_project.Inventory.Item.Cpu.OperatingConfig"},
    // Future examples:
    // {"PowerLimit", "xyz.openbmc_project.Control.Power.Cap"},
}};

// Information about a discovered control endpoint
struct ControlEndpointInfo
{
    std::string path;
    std::string service;
};

// Context for aggregating async discovery results
struct ControlDiscoveryContext
{
    // Map from control type prefix to list of instances
    std::map<std::string, std::vector<ControlEndpointInfo>> controlInstances;
    size_t remaining{0};
};

// Look up which control type prefix corresponds to a D-Bus interface.
// Returns nullopt if the interface doesn't correspond to any control type.
inline std::optional<std::string_view> getControlPrefixForInterface(
    std::string_view iface)
{
    for (const auto& mapping : controlTypeMappings)
    {
        if (mapping.requiredInterface == iface)
        {
            return mapping.prefix;
        }
    }
    return std::nullopt;
}

inline void afterGetOperatingConfigProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& propertiesList)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("GetAll OperatingConfig failed: {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }
    for (const auto& [propName, propValue] : propertiesList)
    {
        if (propName == "MaxSpeed")
        {
            const uint32_t* val = std::get_if<uint32_t>(&propValue);
            if (val != nullptr)
            {
                asyncResp->res.jsonValue["AllowableMax"] = *val;
            }
        }
        else if (propName == "MinSpeed")
        {
            const uint32_t* val = std::get_if<uint32_t>(&propValue);
            if (val != nullptr)
            {
                asyncResp->res.jsonValue["AllowableMin"] = *val;
            }
        }
        else if (propName == "RequestedSpeedLimitMax")
        {
            const uint32_t* val = std::get_if<uint32_t>(&propValue);
            if (val != nullptr)
            {
                asyncResp->res.jsonValue["SettingMax"] = *val;
            }
        }
        else if (propName == "RequestedSpeedLimitMin")
        {
            const uint32_t* val = std::get_if<uint32_t>(&propValue);
            if (val != nullptr)
            {
                asyncResp->res.jsonValue["SettingMin"] = *val;
            }
        }
        else if (propName == "SpeedLocked")
        {
            const bool* val = std::get_if<bool>(&propValue);
            if (val != nullptr)
            {
                asyncResp->res.jsonValue["ControlMode"] =
                    *val ? control::ControlMode::Disabled
                         : control::ControlMode::Automatic;
            }
        }
    }
}

inline void afterGetAcceleratorProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& propertiesList)
{
    if (ec)
    {
        BMCWEB_LOG_DEBUG("GetAll Accelerator failed: {}", ec);
        return;
    }
    for (const auto& [propName, propValue] : propertiesList)
    {
        if (propName == "Type")
        {
            const std::string* val = std::get_if<std::string>(&propValue);
            if (val != nullptr)
            {
                asyncResp->res.jsonValue["PhysicalContext"] = *val;
            }
        }
    }
}

inline void populateClockLimitFromProcessor(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& processorPath, const std::string& service)
{
    dbus::utility::getAllProperties(
        service, processorPath,
        "xyz.openbmc_project.Inventory.Item.Cpu.OperatingConfig",
        std::bind_front(afterGetOperatingConfigProperties, asyncResp));

    dbus::utility::getAllProperties(
        service, processorPath,
        "xyz.openbmc_project.Inventory.Item.Accelerator",
        std::bind_front(afterGetAcceleratorProperties, asyncResp));
}

// Dispatch to the appropriate populate function based on control type prefix.
// To add a new control type, add an else-if branch here.
inline void populateControlProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& controlPrefix, const std::string& objectPath,
    const std::string& service)
{
    if (controlPrefix == "ClockLimit")
    {
        asyncResp->res.jsonValue["ControlType"] =
            control::ControlType::FrequencyMHz;
        asyncResp->res.jsonValue["SetPointUnits"] = "MHz";
        populateClockLimitFromProcessor(asyncResp, objectPath, service);
        return;
    }
    // Future: else if (controlPrefix == "PowerLimit") { ... }
}

inline void afterGetEndpointObject(
    const std::shared_ptr<ControlDiscoveryContext>& ctx,
    const std::string& endpointPath,
    const std::shared_ptr<std::function<void(
        std::map<std::string, std::vector<ControlEndpointInfo>>&&)>>& sharedCb,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetObject& objType)
{
    if (!ec && !objType.empty())
    {
        for (const auto& [service, interfaces] : objType)
        {
            for (const auto& iface : interfaces)
            {
                std::optional<std::string_view> prefix =
                    getControlPrefixForInterface(iface);
                if (prefix)
                {
                    ctx->controlInstances[std::string(*prefix)].push_back(
                        {endpointPath, service});
                }
            }
        }
    }
    ctx->remaining--;
    if (ctx->remaining == 0)
    {
        // Sort instances per type for deterministic ordering
        for (auto& [pfx, instances] : ctx->controlInstances)
        {
            std::ranges::sort(instances, [](const ControlEndpointInfo& a,
                                            const ControlEndpointInfo& b) {
                return a.path < b.path;
            });
        }
        (*sharedCb)(std::move(ctx->controlInstances));
    }
}

inline void afterGetAssociationEndpoints(
    const std::shared_ptr<bmcweb::AsyncResp>& /*unused*/,
    const std::shared_ptr<std::function<void(
        std::map<std::string, std::vector<ControlEndpointInfo>>&&)>>& sharedCb,
    const boost::system::error_code& ec,
    const dbus::utility::MapperEndPoints& endpoints)
{
    if (ec || endpoints.empty())
    {
        BMCWEB_LOG_DEBUG("No all_processors association endpoints: {}", ec);
        (*sharedCb)({});
        return;
    }

    auto ctx = std::make_shared<ControlDiscoveryContext>();
    ctx->remaining = endpoints.size();

    constexpr std::array<std::string_view, 0> noInterfaces{};

    for (const auto& endpointPath : endpoints)
    {
        dbus::utility::getDbusObject(
            endpointPath, noInterfaces,
            std::bind_front(afterGetEndpointObject, ctx, endpointPath,
                            sharedCb));
    }
}

// Discover all control endpoints under a chassis by reading the
// all_processors association and checking each endpoint's D-Bus interfaces
// against the controlTypeMappings registry.
inline void discoverControlEndpoints(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisPath,
    std::function<void(
        std::map<std::string, std::vector<ControlEndpointInfo>>&&)>&& callback)
{
    auto sharedCb = std::make_shared<std::function<void(
        std::map<std::string, std::vector<ControlEndpointInfo>>&&)>>(
        std::move(callback));

    dbus::utility::getAssociationEndPoints(
        chassisPath + "/all_processors",
        std::bind_front(afterGetAssociationEndpoints, asyncResp, sharedCb));
}

inline void afterDiscoverControlsForCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId,
    const std::map<std::string, std::vector<ControlEndpointInfo>>&
        controlInstances)
{
    nlohmann::json::array_t members;
    for (const auto& [prefix, instances] : controlInstances)
    {
        for (const auto& instance : instances)
        {
            sdbusplus::message::object_path objPath(instance.path);
            std::string endpointName = objPath.filename();
            nlohmann::json::object_t member;
            const std::string controlName =
                std::format("{}_{}", prefix, endpointName);
            member["@odata.id"] = boost::urls::format(
                "/redfish/v1/Chassis/{}/Controls/{}", chassisId, controlName);
            members.emplace_back(std::move(member));
        }
    }
    asyncResp->res.jsonValue["Members@odata.count"] = members.size();
    asyncResp->res.jsonValue["Members"] = std::move(members);
}

inline void doControlCollectionGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId,
    const std::optional<std::string>& validChassisPath)
{
    if (!validChassisPath)
    {
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
        return;
    }

    asyncResp->res.jsonValue["@odata.type"] =
        "#ControlCollection.ControlCollection";
    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Chassis/{}/Controls", chassisId);
    asyncResp->res.jsonValue["Name"] = "Controls";
    asyncResp->res.jsonValue["Description"] =
        "The collection of Control resource instances for " + chassisId;
    asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
    asyncResp->res.jsonValue["Members@odata.count"] = 0;

    discoverControlEndpoints(asyncResp, *validChassisPath,
                             std::bind_front(afterDiscoverControlsForCollection,
                                             asyncResp, chassisId));
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
        std::bind_front(doControlCollectionGet, asyncResp, chassisId));
}

inline void afterDiscoverControlsForGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& controlId,
    const std::string& prefix, const std::string& endpointName,
    const std::map<std::string, std::vector<ControlEndpointInfo>>&
        controlInstances)
{
    auto it = controlInstances.find(prefix);
    if (it == controlInstances.end())
    {
        messages::resourceNotFound(asyncResp->res, "Control", controlId);
        return;
    }

    // Find the instance whose D-Bus object name matches endpointName
    const ControlEndpointInfo* matched = nullptr;
    for (const auto& instance : it->second)
    {
        sdbusplus::message::object_path objPath(instance.path);
        if (objPath.filename() == endpointName)
        {
            matched = &instance;
            break;
        }
    }
    if (matched == nullptr)
    {
        messages::resourceNotFound(asyncResp->res, "Control", controlId);
        return;
    }

    const std::string& processorName = endpointName;

    asyncResp->res.jsonValue["@odata.type"] = "#Control.v1_3_0.Control";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/Controls/{}", chassisId, controlId);
    asyncResp->res.jsonValue["Id"] = controlId;
    asyncResp->res.jsonValue["Name"] =
        "Control for " + processorName + " " + controlId;

    nlohmann::json::array_t relatedItems;
    nlohmann::json::object_t relatedItem;
    relatedItem["@odata.id"] =
        boost::urls::format("/redfish/v1/Systems/{}/Processors/{}",
                            BMCWEB_REDFISH_SYSTEM_URI_NAME, processorName);
    relatedItems.emplace_back(std::move(relatedItem));
    asyncResp->res.jsonValue["RelatedItem"] = std::move(relatedItems);

    populateControlProperties(asyncResp, prefix, matched->path,
                              matched->service);
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

    // Parse controlId: e.g. "ClockLimit_GPU_0_0" -> prefix="ClockLimit",
    // endpointName="GPU_0_0". Match against known prefixes from the registry.
    std::string prefix;
    std::string endpointName;
    for (const auto& mapping : controlTypeMappings)
    {
        std::string candidate = std::string(mapping.prefix) + "_";
        if (controlId.starts_with(candidate) &&
            controlId.size() > candidate.size())
        {
            prefix = mapping.prefix;
            endpointName = controlId.substr(candidate.size());
            break;
        }
    }
    if (prefix.empty())
    {
        messages::resourceNotFound(asyncResp->res, "Control", controlId);
        return;
    }

    discoverControlEndpoints(
        asyncResp, *validChassisPath,
        std::bind_front(afterDiscoverControlsForGet, asyncResp, chassisId,
                        controlId, prefix, endpointName));
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

inline void requestRoutesControlCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Controls/")
        .privileges(redfish::privileges::getControlCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleControlCollectionGet, std::ref(app)));
}

inline void requestRoutesControl(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Controls/<str>/")
        .privileges(redfish::privileges::getControl)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleControlGet, std::ref(app)));
}

} // namespace redfish
