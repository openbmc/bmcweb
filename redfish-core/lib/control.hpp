// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/control.hpp"
#include "generated/enums/physical_context.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/chassis_utils.hpp"
#include "utils/collection.hpp"
#include "utils/control_utils.hpp"
#include "utils/dbus_utils.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace redfish
{

constexpr uint64_t hzPerMhzControl = 1000000;

inline std::optional<physical_context::PhysicalContext>
    acceleratorTypeToPhysicalContext(std::string_view dbusType)
{
    if (dbusType == "xyz.openbmc_project.Inventory.Item.Accelerator."
                    "AcceleratorType.GPU")
    {
        return physical_context::PhysicalContext::GPU;
    }
    if (dbusType == "xyz.openbmc_project.Inventory.Item.Accelerator."
                    "AcceleratorType.FPGA")
    {
        return physical_context::PhysicalContext::FPGA;
    }
    if (dbusType == "xyz.openbmc_project.Inventory.Item.Accelerator."
                    "AcceleratorType.ASIC")
    {
        return physical_context::PhysicalContext::ASIC;
    }
    return std::nullopt;
}

inline void afterGetControlProcessorProps(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& properties)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("GetAll Control.OperatingClockSpeed failed: {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }

    const uint64_t* presentSpeedLimitMaxHz = nullptr;
    const uint64_t* presentSpeedLimitMinHz = nullptr;
    const uint64_t* requestedSpeedLimitMaxHz = nullptr;
    const uint64_t* requestedSpeedLimitMinHz = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "PresentSpeedLimitMaxHz",
        presentSpeedLimitMaxHz, "PresentSpeedLimitMinHz",
        presentSpeedLimitMinHz, "RequestedSpeedLimitMaxHz",
        requestedSpeedLimitMaxHz, "RequestedSpeedLimitMinHz",
        requestedSpeedLimitMinHz);

    if (!success)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    if (presentSpeedLimitMaxHz != nullptr)
    {
        asyncResp->res.jsonValue["SetPoint"] =
            *presentSpeedLimitMaxHz / hzPerMhzControl;
    }
    if (presentSpeedLimitMaxHz != nullptr && presentSpeedLimitMinHz != nullptr)
    {
        const bool locked = *presentSpeedLimitMaxHz == *presentSpeedLimitMinHz;
        asyncResp->res.jsonValue["ControlMode"] =
            locked ? control::ControlMode::Disabled
                   : control::ControlMode::Automatic;
    }
    if (requestedSpeedLimitMaxHz != nullptr)
    {
        asyncResp->res.jsonValue["SettingMax"] =
            *requestedSpeedLimitMaxHz / hzPerMhzControl;
    }
    if (requestedSpeedLimitMinHz != nullptr)
    {
        asyncResp->res.jsonValue["SettingMin"] =
            *requestedSpeedLimitMinHz / hzPerMhzControl;
    }
}

inline void afterGetAcceleratorPropsForControl(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& properties)
{
    if (ec)
    {
        BMCWEB_LOG_DEBUG("GetAll Accelerator for control failed: {}", ec);
        return;
    }

    const uint64_t* maxSpeedInHz = nullptr;
    const uint64_t* minSpeedInHz = nullptr;
    const std::string* type = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "MaxSpeedInHz",
        maxSpeedInHz, "MinSpeedInHz", minSpeedInHz, "Type", type);

    if (!success)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    if (maxSpeedInHz != nullptr)
    {
        asyncResp->res.jsonValue["AllowableMax"] =
            *maxSpeedInHz / hzPerMhzControl;
    }
    if (minSpeedInHz != nullptr)
    {
        asyncResp->res.jsonValue["AllowableMin"] =
            *minSpeedInHz / hzPerMhzControl;
    }
    if (type != nullptr)
    {
        std::optional<physical_context::PhysicalContext> ctx =
            acceleratorTypeToPhysicalContext(*type);
        if (ctx)
        {
            asyncResp->res.jsonValue["PhysicalContext"] = *ctx;
        }
    }
}

inline void populateClockLimitControl(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& inventoryPath, const std::string& inventoryService,
    const std::string& controlPath, const std::string& controlService)
{
    asyncResp->res.jsonValue["ControlType"] =
        control::ControlType::FrequencyMHz;
    asyncResp->res.jsonValue["SetPointUnits"] = "MHz";

    if (!inventoryService.empty())
    {
        dbus::utility::getAllProperties(
            inventoryService, inventoryPath,
            "xyz.openbmc_project.Inventory.Item.Accelerator",
            std::bind_front(afterGetAcceleratorPropsForControl, asyncResp));
    }

    dbus::utility::getAllProperties(
        controlService, controlPath,
        "xyz.openbmc_project.Control.OperatingClockSpeed",
        std::bind_front(afterGetControlProcessorProps, asyncResp));
}

inline void afterGetControlledByForCollectionItem(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& endpointName,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("controlled_by lookup failed: {}", ec);
            messages::internalError(asyncResp->res);
        }
        return;
    }

    for (const auto& [controlPath, serviceMap] : subtree)
    {
        for (const auto& [service, interfaces] : serviceMap)
        {
            for (const std::string& iface : interfaces)
            {
                auto prefix =
                    control_utils::getControlPrefixForInterface(iface);
                if (prefix)
                {
                    nlohmann::json::object_t member;
                    member["@odata.id"] = boost::urls::format(
                        "/redfish/v1/Chassis/{}/Controls/{}_{}", chassisId,
                        *prefix, endpointName);
                    nlohmann::json::array_t& members =
                        collection_util::getJsonArray(asyncResp->res.jsonValue,
                                                      "Members");
                    members.emplace_back(std::move(member));
                    asyncResp->res.jsonValue["Members@odata.count"] =
                        members.size();
                }
            }
        }
    }
}

inline void checkControlledByForCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& objectPath)
{
    sdbusplus::object_path objPath(objectPath);
    std::string endpointName = objPath.filename();
    if (endpointName.empty())
    {
        return;
    }
    dbus::utility::getAssociatedSubTree(
        objPath / "controlled_by",
        sdbusplus::object_path("/xyz/openbmc_project/control"), 0,
        control_utils::controlRequiredInterfaces,
        std::bind_front(afterGetControlledByForCollectionItem, asyncResp,
                        chassisId, endpointName));
}

inline void afterDiscoverInventory(
    std::vector<std::pair<std::string, std::string>> candidates,
    std::function<void(const boost::system::error_code&,
                       std::vector<std::pair<std::string, std::string>>)>
        callback,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        // EBADR / io_error from the mapper means there are no contained
        // inventory items. That is a legitimate state — fall through to the
        // chassis-self candidate that is already in the list.
        if (ec.value() != EBADR && ec != boost::system::errc::io_error)
        {
            BMCWEB_LOG_ERROR("GetAssociatedSubTree containing error: {}", ec);
            callback(ec, std::move(candidates));
            return;
        }
        callback({}, std::move(candidates));
        return;
    }

    for (const auto& [path, serviceMap] : subtree)
    {
        if (serviceMap.empty())
        {
            continue;
        }
        candidates.emplace_back(path, serviceMap.front().first);
    }
    callback({}, std::move(candidates));
}

// Discover every inventory path under a chassis that may host controls:
// the chassis itself (with empty service) plus every contained inventory
// item matched by the Accelerator interface. Both doControlGet and
// doControlCollectionGet consume the same list — there is no separate
// chassis-vs-contained branch at the call sites.
inline void discoverInventoryCandidates(
    const std::string& chassisPath,
    std::function<void(const boost::system::error_code&,
                       std::vector<std::pair<std::string, std::string>>)>
        callback)
{
    sdbusplus::object_path chassisObjPath(chassisPath);

    std::vector<std::pair<std::string, std::string>> candidates;
    if (!chassisObjPath.filename().empty())
    {
        // Chassis-self: no inventory item interface to query, so service is
        // left empty. Caller distinguishes chassis from contained by
        // comparing path against the original chassisPath.
        candidates.emplace_back(chassisPath, std::string{});
    }

    constexpr std::array<std::string_view, 1> acceleratorIface = {
        "xyz.openbmc_project.Inventory.Item.Accelerator"};

    dbus::utility::getAssociatedSubTree(
        chassisObjPath / "containing",
        sdbusplus::object_path("/xyz/openbmc_project/inventory"), 0,
        acceleratorIface,
        std::bind_front(afterDiscoverInventory, std::move(candidates),
                        std::move(callback)));
}

inline void afterDiscoverInventoryForCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const boost::system::error_code& ec,
    std::vector<std::pair<std::string, std::string>> candidates)
{
    if (ec)
    {
        messages::internalError(asyncResp->res);
        return;
    }
    for (const auto& [path, service] : candidates)
    {
        checkControlledByForCollection(asyncResp, chassisId, path);
    }
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

    discoverInventoryCandidates(
        *validChassisPath, std::bind_front(afterDiscoverInventoryForCollection,
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

inline void afterVerifyControlledByForGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& controlId,
    const std::string& prefix, const std::string& endpointName,
    const std::string& relatedItemUri, const std::string& inventoryPath,
    const std::string& inventoryService, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        messages::resourceNotFound(asyncResp->res, "Control", controlId);
        return;
    }

    if (subtree.empty())
    {
        messages::resourceNotFound(asyncResp->res, "Control", controlId);
        return;
    }

    if (subtree.size() > 1)
    {
        BMCWEB_LOG_ERROR(
            "More than one control object found in controlled_by association for {} {}",
            endpointName, prefix);
        messages::internalError(asyncResp->res);
        return;
    }

    const auto& [controlPath, serviceMap] = subtree.front();
    if (serviceMap.empty())
    {
        BMCWEB_LOG_ERROR("Empty serviceMap for control object {}", controlPath);
        messages::internalError(asyncResp->res);
        return;
    }
    if (serviceMap.size() > 1)
    {
        BMCWEB_LOG_ERROR("More than one service hosting control object {}",
                         controlPath);
        messages::internalError(asyncResp->res);
        return;
    }
    const std::string& controlService = serviceMap.front().first;

    asyncResp->res.jsonValue["@odata.type"] = "#Control.v1_3_0.Control";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/Controls/{}", chassisId, controlId);
    asyncResp->res.jsonValue["Id"] = controlId;
    asyncResp->res.jsonValue["Name"] =
        "Control for " + endpointName + " " + prefix;

    nlohmann::json::array_t relatedItems;
    nlohmann::json::object_t relatedItem;
    relatedItem["@odata.id"] = relatedItemUri;
    relatedItems.emplace_back(std::move(relatedItem));
    asyncResp->res.jsonValue["RelatedItem"] = std::move(relatedItems);

    if (prefix == "ClockLimit")
    {
        populateClockLimitControl(asyncResp, inventoryPath, inventoryService,
                                  controlPath, controlService);
    }
}

inline void afterDiscoverInventoryForGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& controlId,
    const std::string& validChassisPath,
    const control_utils::ParsedControlId& parsed,
    const std::string& controlInterface, const boost::system::error_code& ec,
    std::vector<std::pair<std::string, std::string>> candidates)
{
    if (ec)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    for (auto& [path, service] : candidates)
    {
        sdbusplus::object_path objPath(path);
        if (objPath.filename() != parsed.endpointName)
        {
            continue;
        }

        const bool isChassis = (path == validChassisPath);
        std::string relatedItemUri =
            isChassis
                ? boost::urls::format("/redfish/v1/Chassis/{}", chassisId)
                      .buffer()
                : boost::urls::format("/redfish/v1/Systems/{}/Processors/{}",
                                      BMCWEB_REDFISH_SYSTEM_URI_NAME,
                                      parsed.endpointName)
                      .buffer();

        std::array<std::string_view, 1> ifaceArr = {controlInterface};
        dbus::utility::getAssociatedSubTree(
            objPath / "controlled_by",
            sdbusplus::object_path("/xyz/openbmc_project/control"), 0, ifaceArr,
            std::bind_front(afterVerifyControlledByForGet, asyncResp, chassisId,
                            controlId, parsed.prefix, parsed.endpointName,
                            std::move(relatedItemUri), std::move(path),
                            std::move(service)));
        return;
    }

    messages::resourceNotFound(asyncResp->res, "Control", controlId);
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

    std::optional<control_utils::ParsedControlId> parsed =
        control_utils::parseControlId(controlId);
    if (!parsed)
    {
        messages::resourceNotFound(asyncResp->res, "Control", controlId);
        return;
    }

    std::optional<std::string_view> ifaceOpt =
        control_utils::getInterfaceForPrefix(parsed->prefix);
    if (!ifaceOpt)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    discoverInventoryCandidates(
        *validChassisPath,
        std::bind_front(afterDiscoverInventoryForGet, asyncResp, chassisId,
                        controlId, *validChassisPath, *parsed,
                        std::string(*ifaceOpt)));
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

inline void handleControlCollectionHead(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& /* chassisId */)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.addHeader(boost::beast::http::field::link,
                             "</redfish/v1/JsonSchemas/ControlCollection/"
                             "ControlCollection.json>; rel=describedby");
}

inline void handleControlHead(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& /* chassisId */, const std::string& /* controlId */)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.addHeader(boost::beast::http::field::link,
                             "</redfish/v1/JsonSchemas/Control/Control.json>;"
                             " rel=describedby");
}

inline void requestRoutesControl(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Controls/")
        .privileges(redfish::privileges::headControlCollection)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleControlCollectionHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Controls/")
        .privileges(redfish::privileges::getControlCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleControlCollectionGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Controls/<str>/")
        .privileges(redfish::privileges::headControl)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleControlHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Controls/<str>/")
        .privileges(redfish::privileges::getControl)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleControlGet, std::ref(app)));
}

} // namespace redfish
