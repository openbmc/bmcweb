// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2018 Intel Corporation

#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/pcie_device.hpp"
#include "generated/enums/pcie_function.hpp"
#include "generated/enums/pcie_slots.hpp"
#include "generated/enums/resource.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/asset_utils.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/pcie_util.hpp"

#include <asm-generic/errno.h>

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>

namespace redfish
{

inline void handlePCIeDevicePath(
    const std::string& pcieDeviceId,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const dbus::utility::MapperGetSubTreePathsResponse& pcieDevicePaths,
    const std::function<void(const std::string& pcieDevicePath,
                             const std::string& service)>& callback)

{
    for (const std::string& pcieDevicePath : pcieDevicePaths)
    {
        std::string pcieDeviceName =
            sdbusplus::message::object_path(pcieDevicePath).filename();
        if (pcieDeviceName.empty() || pcieDeviceName != pcieDeviceId)
        {
            continue;
        }
        static constexpr std::array<std::string_view, 1> pcieDeviceInterface = {
            "xyz.openbmc_project.Inventory.Item.PCIeDevice"};
        dbus::utility::getDbusObject(
            pcieDevicePath, pcieDeviceInterface,
            [pcieDevicePath, asyncResp,
             callback](const boost::system::error_code& ec,
                       const dbus::utility::MapperGetObject& object) {
                if (ec || object.empty())
                {
                    BMCWEB_LOG_ERROR("DBUS response error {}", ec);
                    messages::internalError(asyncResp->res);
                    return;
                }
                callback(pcieDevicePath, object.begin()->first);
            });
        return;
    }

    BMCWEB_LOG_WARNING("PCIe Device not found");
    messages::resourceNotFound(asyncResp->res, "PCIeDevice", pcieDeviceId);
}

inline void getValidPCIeDevicePath(
    const std::string& pcieDeviceId,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::function<void(const std::string& pcieDevicePath,
                             const std::string& service)>& callback)
{
    static constexpr std::array<std::string_view, 1> pcieDeviceInterface = {
        "xyz.openbmc_project.Inventory.Item.PCIeDevice"};

    dbus::utility::getSubTreePaths(
        "/xyz/openbmc_project/inventory", 0, pcieDeviceInterface,
        [pcieDeviceId, asyncResp,
         callback](const boost::system::error_code& ec,
                   const dbus::utility::MapperGetSubTreePathsResponse&
                       pcieDevicePaths) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("D-Bus response error on GetSubTree {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }
            handlePCIeDevicePath(pcieDeviceId, asyncResp, pcieDevicePaths,
                                 callback);
            return;
        });
}

inline void handlePCIeDeviceCollectionGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName)
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
    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    asyncResp->res.addHeader(boost::beast::http::field::link,
                             "</redfish/v1/JsonSchemas/PCIeDeviceCollection/"
                             "PCIeDeviceCollection.json>; rel=describedby");
    asyncResp->res.jsonValue["@odata.type"] =
        "#PCIeDeviceCollection.PCIeDeviceCollection";
    asyncResp->res.jsonValue["@odata.id"] = std::format(
        "/redfish/v1/Systems/{}/PCIeDevices", BMCWEB_REDFISH_SYSTEM_URI_NAME);
    asyncResp->res.jsonValue["Name"] = "PCIe Device Collection";
    asyncResp->res.jsonValue["Description"] = "Collection of PCIe Devices";

    pcie_util::getPCIeDeviceList(asyncResp,
                                 nlohmann::json::json_pointer("/Members"));
}

inline void requestRoutesSystemPCIeDeviceCollection(App& app)
{
    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/PCIeDevices/")
        .privileges(redfish::privileges::getPCIeDeviceCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handlePCIeDeviceCollectionGet, std::ref(app)));
}

inline void afterGetAssociatedSubTreePaths(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& processorPaths)
{
    if (ec)
    {
        if (ec.value() == EBADR)
        {
            BMCWEB_LOG_DEBUG("No processor association found");
            return;
        }
        BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }

    if (processorPaths.empty())
    {
        BMCWEB_LOG_DEBUG("No association found for processor");
        return;
    }

    nlohmann::json& processorList =
        asyncResp->res.jsonValue["Links"]["Processors"];
    for (const std::string& processorPath : processorPaths)
    {
        std::string processorName =
            sdbusplus::message::object_path(processorPath).filename();
        if (processorName.empty())
        {
            continue;
        }

        nlohmann::json item = nlohmann::json::object();
        item["@odata.id"] =
            boost::urls::format("/redfish/v1/Systems/{}/Processors/{}",
                                BMCWEB_REDFISH_SYSTEM_URI_NAME, processorName);
        processorList.emplace_back(std::move(item));
    }

    asyncResp->res.jsonValue["Links"]["Processors@odata.count"] =
        processorList.size();
}

inline void linkAssociatedProcessor(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& pcieDevicePath)
{
    constexpr std::array<std::string_view, 2> processorInterfaces = {
        "xyz.openbmc_project.Inventory.Item.Cpu",
        "xyz.openbmc_project.Inventory.Item.Accelerator"};

    dbus::utility::getAssociatedSubTreePaths(
        pcieDevicePath + "/connected_to",
        sdbusplus::message::object_path("/xyz/openbmc_project/inventory"), 0,
        processorInterfaces,
        std::bind_front(afterGetAssociatedSubTreePaths, asyncResp));
}

inline void addPCIeSlotProperties(
    crow::Response& res, const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& pcieSlotProperties)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBUS response error for getAllProperties{}",
                         ec.value());
        messages::internalError(res);
        return;
    }
    std::string generation;
    size_t lanes = 0;
    std::string slotType;

    bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), pcieSlotProperties, "Generation",
        generation, "Lanes", lanes, "SlotType", slotType);

    if (!success)
    {
        messages::internalError(res);
        return;
    }

    std::optional<pcie_device::PCIeTypes> pcieType =
        pcie_util::redfishPcieGenerationFromDbus(generation);
    if (!pcieType)
    {
        BMCWEB_LOG_WARNING("Unknown PCIeType: {}", generation);
    }
    else
    {
        if (*pcieType == pcie_device::PCIeTypes::Invalid)
        {
            BMCWEB_LOG_ERROR("Invalid PCIeType: {}", generation);
            messages::internalError(res);
            return;
        }
        res.jsonValue["Slot"]["PCIeType"] = *pcieType;
    }

    if (lanes != 0)
    {
        res.jsonValue["Slot"]["Lanes"] = lanes;
    }

    std::optional<pcie_slots::SlotTypes> redfishSlotType =
        pcie_util::dbusSlotTypeToRf(slotType);
    if (!redfishSlotType)
    {
        BMCWEB_LOG_WARNING("Unknown PCIeSlot Type: {}", slotType);
    }
    else
    {
        if (*redfishSlotType == pcie_slots::SlotTypes::Invalid)
        {
            BMCWEB_LOG_ERROR("Invalid PCIeSlot type: {}", slotType);
            messages::internalError(res);
            return;
        }
        res.jsonValue["Slot"]["SlotType"] = *redfishSlotType;
    }
}

inline void getPCIeDeviceSlotPath(
    const std::string& pcieDevicePath,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    std::function<void(const std::string& pcieDeviceSlot)>&& callback)
{
    std::string associationPath = pcieDevicePath + "/contained_by";
    sdbusplus::message::object_path path("/xyz/openbmc_project/inventory");
    static constexpr std::array<std::string_view, 1> pcieSlotInterface = {
        "xyz.openbmc_project.Inventory.Item.PCIeSlot"};
    dbus::utility::getAssociatedSubTreePaths(
        associationPath, path, 0, pcieSlotInterface,
        [callback = std::move(callback), asyncResp, pcieDevicePath](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreePathsResponse& endpoints) {
            if (ec)
            {
                if (ec.value() == EBADR)
                {
                    // Missing association is not an error
                    return;
                }
                BMCWEB_LOG_ERROR(
                    "DBUS response error for getAssociatedSubTreePaths {}",
                    ec.value());
                messages::internalError(asyncResp->res);
                return;
            }
            if (endpoints.size() > 1)
            {
                BMCWEB_LOG_ERROR(
                    "PCIeDevice {} is associated with more than one PCIeSlot: {}",
                    pcieDevicePath, endpoints.size());
                for (const std::string& slotPath : endpoints)
                {
                    BMCWEB_LOG_ERROR("Invalid PCIeSlotPath: {}", slotPath);
                }
                messages::internalError(asyncResp->res);
                return;
            }
            if (endpoints.empty())
            {
                // If the device doesn't have an association, return without
                // PCIe Slot properties
                BMCWEB_LOG_DEBUG("PCIeDevice is not associated with PCIeSlot");
                return;
            }
            callback(endpoints[0]);
        });
}

inline void afterGetDbusObject(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& pcieDeviceSlot, const boost::system::error_code& ec,
    const dbus::utility::MapperGetObject& object)
{
    if (ec || object.empty())
    {
        BMCWEB_LOG_ERROR("DBUS response error for getDbusObject {}",
                         ec.value());
        messages::internalError(asyncResp->res);
        return;
    }
    dbus::utility::getAllProperties(
        object.begin()->first, pcieDeviceSlot,
        "xyz.openbmc_project.Inventory.Item.PCIeSlot",
        [asyncResp](
            const boost::system::error_code& ec2,
            const dbus::utility::DBusPropertiesMap& pcieSlotProperties) {
            addPCIeSlotProperties(asyncResp->res, ec2, pcieSlotProperties);
        });
}

inline void afterGetPCIeDeviceSlotPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& pcieDeviceSlot)
{
    static constexpr std::array<std::string_view, 1> pcieSlotInterface = {
        "xyz.openbmc_project.Inventory.Item.PCIeSlot"};
    dbus::utility::getDbusObject(
        pcieDeviceSlot, pcieSlotInterface,
        [asyncResp,
         pcieDeviceSlot](const boost::system::error_code& ec,
                         const dbus::utility::MapperGetObject& object) {
            afterGetDbusObject(asyncResp, pcieDeviceSlot, ec, object);
        });
}

inline void getPCIeDeviceHealth(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& pcieDevicePath, const std::string& service)
{
    dbus::utility::getProperty<bool>(
        service, pcieDevicePath,
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

inline void getPCIeDeviceState(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& pcieDevicePath, const std::string& service)
{
    dbus::utility::getProperty<bool>(
        service, pcieDevicePath, "xyz.openbmc_project.Inventory.Item",
        "Present",
        [asyncResp](const boost::system::error_code& ec, bool value) {
            if (ec)
            {
                if (ec.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR("DBUS response error for State");
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

inline void addPCIeDeviceProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& pcieDeviceId,
    const dbus::utility::DBusPropertiesMap& pcieDevProperties)
{
    const std::string* generationInUse = nullptr;
    const std::string* generationSupported = nullptr;
    const size_t* lanesInUse = nullptr;
    const size_t* maxLanes = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), pcieDevProperties, "GenerationInUse",
        generationInUse, "GenerationSupported", generationSupported,
        "LanesInUse", lanesInUse, "MaxLanes", maxLanes);

    if (!success)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    if (generationInUse != nullptr)
    {
        std::optional<pcie_device::PCIeTypes> redfishGenerationInUse =
            pcie_util::redfishPcieGenerationFromDbus(*generationInUse);

        if (!redfishGenerationInUse)
        {
            BMCWEB_LOG_WARNING("Unknown PCIe Device Generation: {}",
                               *generationInUse);
        }
        else
        {
            if (*redfishGenerationInUse == pcie_device::PCIeTypes::Invalid)
            {
                BMCWEB_LOG_ERROR("Invalid PCIe Device Generation: {}",
                                 *generationInUse);
                messages::internalError(asyncResp->res);
                return;
            }
            asyncResp->res.jsonValue["PCIeInterface"]["PCIeType"] =
                *redfishGenerationInUse;
        }
    }

    if (generationSupported != nullptr)
    {
        std::optional<pcie_device::PCIeTypes> redfishGenerationSupported =
            pcie_util::redfishPcieGenerationFromDbus(*generationSupported);

        if (!redfishGenerationSupported)
        {
            BMCWEB_LOG_WARNING("Unknown PCIe Device Generation: {}",
                               *generationSupported);
        }
        else
        {
            if (*redfishGenerationSupported == pcie_device::PCIeTypes::Invalid)
            {
                BMCWEB_LOG_ERROR("Invalid PCIe Device Generation: {}",
                                 *generationSupported);
                messages::internalError(asyncResp->res);
                return;
            }
            asyncResp->res.jsonValue["PCIeInterface"]["MaxPCIeType"] =
                *redfishGenerationSupported;
        }
    }

    if (lanesInUse != nullptr)
    {
        if (*lanesInUse == std::numeric_limits<size_t>::max())
        {
            // The default value of LanesInUse is "maxint", and the field will
            // be null if it is a default value.
            asyncResp->res.jsonValue["PCIeInterface"]["LanesInUse"] = nullptr;
        }
        else
        {
            asyncResp->res.jsonValue["PCIeInterface"]["LanesInUse"] =
                *lanesInUse;
        }
    }
    // The default value of MaxLanes is 0, and the field will be
    // left as off if it is a default value.
    if (maxLanes != nullptr && *maxLanes != 0)
    {
        asyncResp->res.jsonValue["PCIeInterface"]["MaxLanes"] = *maxLanes;
    }

    asyncResp->res.jsonValue["PCIeFunctions"]["@odata.id"] =
        boost::urls::format(
            "/redfish/v1/Systems/{}/PCIeDevices/{}/PCIeFunctions",
            BMCWEB_REDFISH_SYSTEM_URI_NAME, pcieDeviceId);
}

inline void getPCIeDeviceProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& pcieDevicePath, const std::string& service,
    const std::function<void(
        const dbus::utility::DBusPropertiesMap& pcieDevProperties)>&& callback)
{
    dbus::utility::getAllProperties(
        service, pcieDevicePath,
        "xyz.openbmc_project.Inventory.Item.PCIeDevice",
        [asyncResp,
         callback](const boost::system::error_code& ec,
                   const dbus::utility::DBusPropertiesMap& pcieDevProperties) {
            if (ec)
            {
                if (ec.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR("DBUS response error for Properties");
                    messages::internalError(asyncResp->res);
                }
                return;
            }
            callback(pcieDevProperties);
        });
}

inline void addPCIeDeviceCommonProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& pcieDeviceId)
{
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/PCIeDevice/PCIeDevice.json>; rel=describedby");
    asyncResp->res.jsonValue["@odata.type"] = "#PCIeDevice.v1_19_0.PCIeDevice";
    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Systems/{}/PCIeDevices/{}",
                            BMCWEB_REDFISH_SYSTEM_URI_NAME, pcieDeviceId);
    asyncResp->res.jsonValue["Name"] = "PCIe Device";
    asyncResp->res.jsonValue["Id"] = pcieDeviceId;
    asyncResp->res.jsonValue["Status"]["State"] = resource::State::Enabled;
    asyncResp->res.jsonValue["Status"]["Health"] = resource::Health::OK;
}

inline void afterGetValidPcieDevicePath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& pcieDeviceId, const std::string& pcieDevicePath,
    const std::string& service)
{
    addPCIeDeviceCommonProperties(asyncResp, pcieDeviceId);
    asset_utils::getAssetInfo(asyncResp, service, pcieDevicePath,
                              ""_json_pointer, true);
    getPCIeDeviceState(asyncResp, pcieDevicePath, service);
    getPCIeDeviceHealth(asyncResp, pcieDevicePath, service);
    getPCIeDeviceProperties(
        asyncResp, pcieDevicePath, service,
        std::bind_front(addPCIeDeviceProperties, asyncResp, pcieDeviceId));
    linkAssociatedProcessor(asyncResp, pcieDevicePath);
    getPCIeDeviceSlotPath(
        pcieDevicePath, asyncResp,
        std::bind_front(afterGetPCIeDeviceSlotPath, asyncResp));
}

inline void handlePCIeDeviceGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& pcieDeviceId)
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
    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    getValidPCIeDevicePath(
        pcieDeviceId, asyncResp,
        std::bind_front(afterGetValidPcieDevicePath, asyncResp, pcieDeviceId));
}

inline void requestRoutesSystemPCIeDevice(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/PCIeDevices/<str>/")
        .privileges(redfish::privileges::getPCIeDevice)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handlePCIeDeviceGet, std::ref(app)));
}

inline void addPCIeFunctionPropertiesFromInterface(
    crow::Response& resp, const dbus::utility::DBusPropertiesMap& props)
{
    uint16_t vendorId = 0;
    uint16_t deviceId = 0;
    uint16_t subsystemVendorId = 0;
    uint16_t subsystemId = 0;
    std::string deviceClass;
    std::string functionType;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), props, "VendorId", vendorId,
        "DeviceId", deviceId, "SubsystemVendorId", subsystemVendorId,
        "SubsystemId", subsystemId, "DeviceClass", deviceClass, "FunctionType",
        functionType);

    if (!success)
    {
        messages::internalError(resp);
        return;
    }

    resp.jsonValue["VendorId"] = std::format("0x{:04X}", vendorId);
    resp.jsonValue["DeviceId"] = std::format("0x{:04X}", deviceId);
    resp.jsonValue["SubsystemVendorId"] =
        std::format("0x{:04X}", subsystemVendorId);
    resp.jsonValue["SubsystemId"] = std::format("0x{:04X}", subsystemId);

    size_t pos = deviceClass.rfind('.');
    if (pos != std::string::npos)
    {
        pcie_function::DeviceClass dc =
            nlohmann::json(deviceClass.substr(pos + 1))
                .get<pcie_function::DeviceClass>();
        if (dc == pcie_function::DeviceClass::Invalid)
        {
            BMCWEB_LOG_WARNING("Unknown PCIeFunction DeviceClass: {}",
                               deviceClass);
        }
        else
        {
            resp.jsonValue["DeviceClass"] = dc;
        }
    }

    pos = functionType.rfind('.');
    if (pos != std::string::npos)
    {
        pcie_function::FunctionType ft =
            nlohmann::json(functionType.substr(pos + 1))
                .get<pcie_function::FunctionType>();
        if (ft == pcie_function::FunctionType::Invalid)
        {
            BMCWEB_LOG_WARNING("Unknown PCIeFunction FunctionType: {}",
                               functionType);
        }
        else
        {
            resp.jsonValue["FunctionType"] = ft;
        }
    }
}

inline void afterGetPCIeFunctionPropsForCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& pcieDeviceId,
    const std::shared_ptr<size_t>& pendingCount,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& props)
{
    (*pendingCount)--;
    if (!ec)
    {
        uint8_t functionNumber = 0;
        if (sdbusplus::unpackPropertiesNoThrow(dbus_utils::UnpackErrorPrinter(),
                                               props, "FunctionNumber",
                                               functionNumber))
        {
            nlohmann::json::object_t pcieFunction;
            pcieFunction["@odata.id"] = boost::urls::format(
                "/redfish/v1/Systems/{}/PCIeDevices/{}/PCIeFunctions/{}",
                BMCWEB_REDFISH_SYSTEM_URI_NAME, pcieDeviceId,
                std::to_string(functionNumber));
            asyncResp->res.jsonValue["Members"].emplace_back(
                std::move(pcieFunction));
        }
    }
    if (*pendingCount == 0)
    {
        asyncResp->res.jsonValue["Members@odata.count"] =
            asyncResp->res.jsonValue["Members"].size();
    }
}

inline void afterGetPCIeFunctionPathsFromSubTree(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& pcieDeviceId, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec && ec.value() != EBADR)
    {
        BMCWEB_LOG_ERROR("DBUS response error for PCIeFunction subtree: {}",
                         ec);
        messages::internalError(asyncResp->res);
        return;
    }

    nlohmann::json& pcieFunctionList = asyncResp->res.jsonValue["Members"];
    pcieFunctionList = nlohmann::json::array();

    if (subtree.empty())
    {
        asyncResp->res.jsonValue["Members@odata.count"] = 0;
        return;
    }

    auto pendingCount = std::make_shared<size_t>(subtree.size());
    for (const auto& [path, serviceMap] : subtree)
    {
        if (serviceMap.empty())
        {
            (*pendingCount)--;
            if (*pendingCount == 0)
            {
                asyncResp->res.jsonValue["Members@odata.count"] =
                    pcieFunctionList.size();
            }
            continue;
        }
        const std::string& service = serviceMap.begin()->first;
        dbus::utility::getAllProperties(
            service, path, "xyz.openbmc_project.Inventory.Item.PCIeFunction",
            std::bind_front(afterGetPCIeFunctionPropsForCollection, asyncResp,
                            pcieDeviceId, pendingCount));
    }
}

inline void handlePCIeFunctionCollectionGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& pcieDeviceId)
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

    getValidPCIeDevicePath(
        pcieDeviceId, asyncResp,
        [asyncResp, pcieDeviceId](const std::string& pcieDevicePath,
                                  const std::string& /*service*/) {
            asyncResp->res.addHeader(
                boost::beast::http::field::link,
                "</redfish/v1/JsonSchemas/PCIeFunctionCollection/PCIeFunctionCollection.json>; rel=describedby");
            asyncResp->res.jsonValue["@odata.type"] =
                "#PCIeFunctionCollection.PCIeFunctionCollection";
            asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
                "/redfish/v1/Systems/{}/PCIeDevices/{}/PCIeFunctions",
                BMCWEB_REDFISH_SYSTEM_URI_NAME, pcieDeviceId);
            asyncResp->res.jsonValue["Name"] = "PCIe Function Collection";
            asyncResp->res.jsonValue["Description"] =
                "Collection of PCIe Functions for PCIe Device " + pcieDeviceId;

            static constexpr std::array<std::string_view, 1>
                pcieFunctionInterface = {
                    "xyz.openbmc_project.Inventory.Item.PCIeFunction"};
            dbus::utility::getAssociatedSubTree(
                pcieDevicePath + "/exposing",
                sdbusplus::message::object_path(
                    "/xyz/openbmc_project/inventory"),
                0, pcieFunctionInterface,
                std::bind_front(afterGetPCIeFunctionPathsFromSubTree, asyncResp,
                                pcieDeviceId));
        });
}

inline void requestRoutesSystemPCIeFunctionCollection(App& app)
{
    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/<str>/PCIeDevices/<str>/PCIeFunctions/")
        .privileges(redfish::privileges::getPCIeFunctionCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handlePCIeFunctionCollectionGet, std::ref(app)));
}

inline void addPCIeFunctionCommonProperties(crow::Response& resp,
                                            const std::string& pcieDeviceId,
                                            uint64_t pcieFunctionId)
{
    resp.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/PCIeFunction/PCIeFunction.json>; rel=describedby");
    resp.jsonValue["@odata.type"] = "#PCIeFunction.v1_2_3.PCIeFunction";
    resp.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/{}/PCIeDevices/{}/PCIeFunctions/{}",
        BMCWEB_REDFISH_SYSTEM_URI_NAME, pcieDeviceId,
        std::to_string(pcieFunctionId));
    resp.jsonValue["Name"] = "PCIe Function";
    resp.jsonValue["Id"] = std::to_string(pcieFunctionId);
    resp.jsonValue["FunctionId"] = pcieFunctionId;
    resp.jsonValue["Links"]["PCIeDevice"]["@odata.id"] =
        boost::urls::format("/redfish/v1/Systems/{}/PCIeDevices/{}",
                            BMCWEB_REDFISH_SYSTEM_URI_NAME, pcieDeviceId);
}

inline void afterGetPCIeFunctionPropsForSearch(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& pcieDeviceId, uint64_t pcieFunctionId,
    const std::shared_ptr<size_t>& pendingCount,
    const std::shared_ptr<bool>& found, const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& props)
{
    (*pendingCount)--;
    if (*found)
    {
        return;
    }
    if (ec)
    {
        if (*pendingCount == 0)
        {
            messages::resourceNotFound(asyncResp->res, "PCIeFunction",
                                       std::to_string(pcieFunctionId));
        }
        return;
    }
    uint8_t functionNumber = 0;
    bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), props, "FunctionNumber",
        functionNumber);
    if (!success || static_cast<uint64_t>(functionNumber) != pcieFunctionId)
    {
        if (*pendingCount == 0)
        {
            messages::resourceNotFound(asyncResp->res, "PCIeFunction",
                                       std::to_string(pcieFunctionId));
        }
        return;
    }
    *found = true;
    addPCIeFunctionCommonProperties(asyncResp->res, pcieDeviceId,
                                    pcieFunctionId);
    addPCIeFunctionPropertiesFromInterface(asyncResp->res, props);
}

inline void afterGetPCIeFunctionSubTreeForSearch(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& pcieDeviceId, uint64_t pcieFunctionId,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec || subtree.empty())
    {
        messages::resourceNotFound(asyncResp->res, "PCIeFunction",
                                   std::to_string(pcieFunctionId));
        return;
    }

    std::shared_ptr<size_t> pendingCount =
        std::make_shared<size_t>(subtree.size());
    std::shared_ptr<bool> found = std::make_shared<bool>(false);
    for (const auto& [path, serviceMap] : subtree)
    {
        if (serviceMap.empty())
        {
            (*pendingCount)--;
            if (*pendingCount == 0 && !*found)
            {
                messages::resourceNotFound(asyncResp->res, "PCIeFunction",
                                           std::to_string(pcieFunctionId));
            }
            continue;
        }
        const std::string& service = serviceMap.begin()->first;
        dbus::utility::getAllProperties(
            service, path, "xyz.openbmc_project.Inventory.Item.PCIeFunction",
            std::bind_front(afterGetPCIeFunctionPropsForSearch, asyncResp,
                            pcieDeviceId, pcieFunctionId, pendingCount, found));
    }
}

inline void afterGetPCIeDevicePathForFunction(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& pcieDeviceId, uint64_t pcieFunctionId,
    const std::string& pcieDevicePath, const std::string& /*service*/)
{
    static constexpr std::array<std::string_view, 1> pcieFunctionInterface = {
        "xyz.openbmc_project.Inventory.Item.PCIeFunction"};
    dbus::utility::getAssociatedSubTree(
        pcieDevicePath + "/exposing",
        sdbusplus::message::object_path("/xyz/openbmc_project/inventory"), 0,
        pcieFunctionInterface,
        std::bind_front(afterGetPCIeFunctionSubTreeForSearch, asyncResp,
                        pcieDeviceId, pcieFunctionId));
}

inline void handlePCIeFunctionGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& pcieDeviceId,
    const std::string& pcieFunctionIdStr)
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
    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    std::string_view pcieFunctionIdView = pcieFunctionIdStr;

    uint64_t pcieFunctionId = 0;
    std::from_chars_result result = std::from_chars(
        pcieFunctionIdView.begin(), pcieFunctionIdView.end(), pcieFunctionId);
    if (result.ec != std::errc{} || result.ptr != pcieFunctionIdView.end())
    {
        messages::resourceNotFound(asyncResp->res, "PCIeFunction",
                                   pcieFunctionIdStr);
        return;
    }

    getValidPCIeDevicePath(
        pcieDeviceId, asyncResp,
        std::bind_front(afterGetPCIeDevicePathForFunction, asyncResp,
                        pcieDeviceId, pcieFunctionId));
}

inline void requestRoutesSystemPCIeFunction(App& app)
{
    BMCWEB_ROUTE(
        app, "/redfish/v1/Systems/<str>/PCIeDevices/<str>/PCIeFunctions/<str>/")
        .privileges(redfish::privileges::getPCIeFunction)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handlePCIeFunctionGet, std::ref(app)));
}

} // namespace redfish
