// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/control.hpp"
#include "generated/enums/physical_context.hpp"
#include "io_context_singleton.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/chassis_utils.hpp"
#include "utils/collection.hpp"
#include "utils/control_utils.hpp"
#include "utils/dbus_utils.hpp"

#include <boost/asio/steady_timer.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <atomic>
#include <chrono>
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

inline void afterGetControlledByForControl(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("GetAssociatedSubTree error for controlled_by: {}",
                             ec);
            messages::internalError(asyncResp->res);
        }
        return;
    }

    if (subtree.empty() || subtree.front().second.empty())
    {
        return;
    }

    const auto& [controlPath, serviceMap] = subtree.front();
    const std::string& controlService = serviceMap.front().first;

    dbus::utility::getAllProperties(
        controlService, controlPath,
        "xyz.openbmc_project.Control.OperatingClockSpeed",
        std::bind_front(afterGetControlProcessorProps, asyncResp));
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
    const std::string& inventoryPath, const std::string& inventoryService)
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

    constexpr std::array<std::string_view, 1> controlClockSpeedIface = {
        "xyz.openbmc_project.Control.OperatingClockSpeed"};

    dbus::utility::getAssociatedSubTree(
        sdbusplus::object_path(inventoryPath) / "controlled_by",
        sdbusplus::object_path("/xyz/openbmc_project/control"), 0,
        controlClockSpeedIface,
        std::bind_front(afterGetControlledByForControl, asyncResp));
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

inline void afterDiscoverContainingForCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& paths)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR(
                "GetAssociatedSubTreePaths error for containing: {}", ec);
            messages::internalError(asyncResp->res);
        }
        return;
    }

    for (const std::string& objectPath : paths)
    {
        checkControlledByForCollection(asyncResp, chassisId, objectPath);
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

    // Check chassis itself for controls
    sdbusplus::object_path chassisObjPath(*validChassisPath);
    std::string chassisName = chassisObjPath.filename();
    if (chassisName.empty())
    {
        return;
    }

    checkControlledByForCollection(asyncResp, chassisId, *validChassisPath);

    // Check contained objects for controls
    constexpr std::array<std::string_view, 1> acceleratorIface = {
        "xyz.openbmc_project.Inventory.Item.Accelerator"};

    dbus::utility::getAssociatedSubTreePaths(
        chassisObjPath / "containing",
        sdbusplus::object_path("/xyz/openbmc_project/inventory"), 0,
        acceleratorIface,
        std::bind_front(afterDiscoverContainingForCollection, asyncResp,
                        chassisId));
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
    const std::string& inventoryPath, const std::string& inventoryService,
    bool ownedByChassis, const boost::system::error_code& ec,
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

    asyncResp->res.jsonValue["@odata.type"] = "#Control.v1_3_0.Control";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/Controls/{}", chassisId, controlId);
    asyncResp->res.jsonValue["Id"] = controlId;
    asyncResp->res.jsonValue["Name"] =
        "Control for " + endpointName + " " + prefix;

    nlohmann::json::array_t relatedItems;
    nlohmann::json::object_t relatedItem;
    if (ownedByChassis)
    {
        relatedItem["@odata.id"] =
            boost::urls::format("/redfish/v1/Chassis/{}", chassisId);
    }
    else
    {
        relatedItem["@odata.id"] =
            boost::urls::format("/redfish/v1/Systems/{}/Processors/{}",
                                BMCWEB_REDFISH_SYSTEM_URI_NAME, endpointName);
    }
    relatedItems.emplace_back(std::move(relatedItem));
    asyncResp->res.jsonValue["RelatedItem"] = std::move(relatedItems);

    if (prefix == "ClockLimit")
    {
        asyncResp->res.jsonValue["Actions"]["#Control.ResetToDefaults"]
                                ["target"] = boost::urls::format(
            "/redfish/v1/Chassis/{}/Controls/{}/Actions/Control.ResetToDefaults",
            chassisId, controlId);
        populateClockLimitControl(asyncResp, inventoryPath, inventoryService);
    }
}

inline void afterDiscoverContainingForGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& controlId,
    const std::string& prefix, const std::string& endpointName,
    std::string_view iface, const boost::system::error_code& ec,
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

    for (const auto& [objectPath, serviceMap] : subtree)
    {
        sdbusplus::object_path objPath(objectPath);
        if (objPath.filename() != endpointName || serviceMap.empty())
        {
            continue;
        }

        std::string inventoryService = serviceMap.front().first;

        // Verify the control exists via controlled_by
        std::array<std::string_view, 1> ifaceArr = {iface};
        dbus::utility::getAssociatedSubTree(
            objPath / "controlled_by",
            sdbusplus::object_path("/xyz/openbmc_project/control"), 0, ifaceArr,
            std::bind_front(afterVerifyControlledByForGet, asyncResp, chassisId,
                            controlId, prefix, endpointName, objectPath,
                            inventoryService, /*ownedByChassis=*/false));
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

    sdbusplus::object_path chassisObjPath(*validChassisPath);

    // If the endpoint name refers to the chassis itself, check whether the
    // control lives directly on the chassis inventory object.
    if (chassisObjPath.filename() == parsed->endpointName)
    {
        std::array<std::string_view, 1> ifaceArr = {*ifaceOpt};
        dbus::utility::getAssociatedSubTree(
            chassisObjPath / "controlled_by",
            sdbusplus::object_path("/xyz/openbmc_project/control"), 0, ifaceArr,
            std::bind_front(afterVerifyControlledByForGet, asyncResp, chassisId,
                            controlId, parsed->prefix, parsed->endpointName,
                            *validChassisPath, std::string(),
                            /*ownedByChassis=*/true));
        return;
    }

    // Otherwise search contained objects (accelerators) for the endpoint.
    constexpr std::array<std::string_view, 1> acceleratorIface = {
        "xyz.openbmc_project.Inventory.Item.Accelerator"};

    dbus::utility::getAssociatedSubTree(
        chassisObjPath / "containing",
        sdbusplus::object_path("/xyz/openbmc_project/inventory"), 0,
        acceleratorIface,
        std::bind_front(afterDiscoverContainingForGet, asyncResp, chassisId,
                        controlId, parsed->prefix, parsed->endpointName,
                        *ifaceOpt));
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

// Wait window for the asynchronous ResetComplete signal. NSM
// SET_CLOCK_LIMIT typically completes in milliseconds; 10 s is a
// conservative bound chosen to accommodate transient MCTP retries.
constexpr std::chrono::seconds clockLimitResetTimeout{10};

// State shared between the ResetComplete signal match, the timeout
// timer, and the D-Bus method-call ack handler. The first of the
// three to fire wins the race via the `completed` flag and tears the
// others down; the rest become no-ops.
struct ClockLimitResetState
{
    std::unique_ptr<sdbusplus::bus::match_t> signalMatch;
    boost::asio::steady_timer timeoutTimer;
    std::atomic_flag completed = ATOMIC_FLAG_INIT;

    explicit ClockLimitResetState(boost::asio::io_context& io) :
        timeoutTimer(io)
    {}
};

inline void dispatchResetWithSignalAwait(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& service, const std::string& objectPath)
{
    auto state = std::make_shared<ClockLimitResetState>(getIoContext());

    // Subscribe to ResetComplete before invoking Reset so a fast
    // hardware response is not lost between method ack and match
    // installation. The match callback captures `state` by weak_ptr to
    // avoid the cycle state -> signalMatch -> match callback -> state,
    // which would also have led to a use-after-free if the callback
    // tried to destroy its own match. The match stays alive for as
    // long as `state` does; `state` is kept alive by the timer's
    // pending wait handler and the async_method_call's pending reply
    // handler, both of which capture `state` by shared_ptr.
    std::weak_ptr<ClockLimitResetState> stateWeak(state);

    std::string matchRule =
        sdbusplus::bus::match::rules::type::signal() +
        sdbusplus::bus::match::rules::sender(service) +
        sdbusplus::bus::match::rules::path(objectPath) +
        sdbusplus::bus::match::rules::interface("xyz.openbmc_project.Control."
                                                "OperatingClockSpeed") +
        sdbusplus::bus::match::rules::member("ResetComplete");

    state->signalMatch = std::make_unique<sdbusplus::bus::match_t>(
        *crow::connections::systemBus, matchRule,
        [asyncResp, stateWeak](sdbusplus::message_t& msg) mutable {
            auto s = stateWeak.lock();
            if (!s)
            {
                return;
            }
            if (s->completed.test_and_set())
            {
                return;
            }
            s->timeoutTimer.cancel();
            uint16_t status = 0;
            try
            {
                msg.read(status);
            }
            catch (const sdbusplus::exception_t& e)
            {
                BMCWEB_LOG_ERROR("Failed to read ResetComplete signal: {}",
                                 e.what());
                status = 0xFFFF;
            }
            if (status != 0)
            {
                BMCWEB_LOG_ERROR("ResetComplete reported failure status {}",
                                 status);
                messages::internalError(asyncResp->res);
                return;
            }
            messages::success(asyncResp->res);
        });

    state->timeoutTimer.expires_after(clockLimitResetTimeout);
    state->timeoutTimer.async_wait(
        [asyncResp, state](const boost::system::error_code& ec) {
            if (ec == boost::asio::error::operation_aborted)
            {
                return;
            }
            if (state->completed.test_and_set())
            {
                return;
            }
            BMCWEB_LOG_ERROR("Timed out waiting for ResetComplete signal");
            messages::operationTimeout(asyncResp->res);
        });

    crow::connections::systemBus->async_method_call(
        [asyncResp, state](const boost::system::error_code& ec,
                           const sdbusplus::message_t& msg) {
            // Success path: the method acked the enqueue; stay parked
            // on the signal + timer.
            if (!ec)
            {
                return;
            }
            // Method failed synchronously (encode / Unavailable / bus
            // error). Cancel the timer and reply immediately. The match
            // and state are torn down naturally once the timer's
            // operation_aborted handler runs.
            if (state->completed.test_and_set())
            {
                return;
            }
            state->timeoutTimer.cancel();

            const sd_bus_error* dbusError = msg.get_error();
            if (dbusError != nullptr && dbusError->name != nullptr)
            {
                std::string_view name(dbusError->name);
                if (name == "xyz.openbmc_project.Common.Error.Unavailable")
                {
                    messages::serviceTemporarilyUnavailable(asyncResp->res,
                                                            "10");
                    return;
                }
            }
            BMCWEB_LOG_ERROR("Reset method call failed: {}", ec);
            messages::internalError(asyncResp->res);
        },
        service, objectPath, "xyz.openbmc_project.Control.OperatingClockSpeed",
        "Reset");
}

inline void afterFindControlForReset(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& controlId, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("controlled_by lookup for reset failed: {}", ec);
            messages::internalError(asyncResp->res);
            return;
        }
        messages::resourceNotFound(asyncResp->res, "Control", controlId);
        return;
    }

    if (subtree.empty() || subtree.front().second.empty())
    {
        messages::resourceNotFound(asyncResp->res, "Control", controlId);
        return;
    }

    if (subtree.size() > 1)
    {
        BMCWEB_LOG_ERROR("More than one control object found for reset of {}",
                         controlId);
        messages::internalError(asyncResp->res);
        return;
    }

    const auto& [controlPath, serviceMap] = subtree.front();
    const std::string& service = serviceMap.front().first;

    dispatchResetWithSignalAwait(asyncResp, service, controlPath);
}

inline void afterDiscoverContainingForReset(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& controlId, const std::string& endpointName,
    std::string_view iface, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& paths)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("containing lookup for reset failed: {}", ec);
            messages::internalError(asyncResp->res);
            return;
        }
        messages::resourceNotFound(asyncResp->res, "Control", controlId);
        return;
    }

    for (const std::string& objectPath : paths)
    {
        sdbusplus::object_path objPath(objectPath);
        if (objPath.filename() != endpointName)
        {
            continue;
        }

        std::array<std::string_view, 1> ifaceArr = {iface};
        dbus::utility::getAssociatedSubTree(
            objPath / "controlled_by",
            sdbusplus::object_path("/xyz/openbmc_project/control"), 0, ifaceArr,
            std::bind_front(afterFindControlForReset, asyncResp, controlId));
        return;
    }

    messages::resourceNotFound(asyncResp->res, "Control", controlId);
}

inline void doControlResetToDefaults(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& controlId,
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

    sdbusplus::object_path chassisObjPath(*validChassisPath);

    // Chassis itself owns the control
    if (chassisObjPath.filename() == parsed->endpointName)
    {
        std::array<std::string_view, 1> ifaceArr = {*ifaceOpt};
        dbus::utility::getAssociatedSubTree(
            chassisObjPath / "controlled_by",
            sdbusplus::object_path("/xyz/openbmc_project/control"), 0, ifaceArr,
            std::bind_front(afterFindControlForReset, asyncResp, controlId));
        return;
    }

    // Control is owned by a contained accelerator
    constexpr std::array<std::string_view, 1> acceleratorIface = {
        "xyz.openbmc_project.Inventory.Item.Accelerator"};

    dbus::utility::getAssociatedSubTreePaths(
        chassisObjPath / "containing",
        sdbusplus::object_path("/xyz/openbmc_project/inventory"), 0,
        acceleratorIface,
        std::bind_front(afterDiscoverContainingForReset, asyncResp, controlId,
                        parsed->endpointName, *ifaceOpt));
}

inline void handleControlResetToDefaults(
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
        std::bind_front(doControlResetToDefaults, asyncResp, chassisId,
                        controlId));
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

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Chassis/<str>/Controls/<str>/Actions/Control.ResetToDefaults/")
        .privileges(redfish::privileges::postControl)
        .methods(boost::beast::http::verb::post)(
            std::bind_front(handleControlResetToDefaults, std::ref(app)));
}

} // namespace redfish
