// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "pcie.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"

#include <asm-generic/errno.h>

#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

static constexpr std::array<std::string_view, 1> pciePortInterface = {
    "xyz.openbmc_project.Inventory.Item.PCIePort"};

namespace redfish
{
inline void handlePCIeDevicePortPathsPortCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& pcieDeviceId,
    const std::vector<std::string>& pcieDevicePorts)
{
    asyncResp->res.jsonValue["@odata.id"] =
        std::format("/redfish/v1/Systems/{}/PCIeDevices/{}/Ports",
                    BMCWEB_REDFISH_SYSTEM_URI_NAME, pcieDeviceId);
    asyncResp->res.jsonValue["@odata.type"] = "#PortCollection.PortCollection";
    asyncResp->res.jsonValue["Name"] = pcieDeviceId + " Port Collection";
    asyncResp->res.jsonValue["Members@odata.count"] = pcieDevicePorts.size();
    nlohmann::json& ports = asyncResp->res.jsonValue["Members"];

    ports = nlohmann::json::array();
    for (const std::string& portPath : pcieDevicePorts)
    {
        std::string portName =
            sdbusplus::message::object_path(portPath).filename();

        nlohmann::json port;
        port["@odata.id"] =
            std::format("/redfish/v1/Systems/{}/PCIeDevices/{}/Ports/{}",
                        BMCWEB_REDFISH_SYSTEM_URI_NAME, pcieDeviceId, portName);
        ports.push_back(std::move(port));
    }
}

inline void afterGetPCIeDevicePortPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& pcieDeviceId,
    const std::function<void(const std::string& pcieDeviceId,
                             const std::vector<std::string>& pcieDevicePorts)>&
        callback,
    const boost::system::error_code& ec,
    const std::vector<std::string>& pcieDevicePorts)
{
    if (ec)
    {
        if (ec.value() == EBADR)
        {
            // Missing association is not an error
            return;
        }
        BMCWEB_LOG_ERROR("DBUS response error for getAssociatedSubTreePaths {}",
                         ec.value());
        messages::internalError(asyncResp->res);
        return;
    }

    callback(pcieDeviceId, pcieDevicePorts);
}

inline void afterGetValidPcieDevice(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& pcieDeviceId,
    const std::function<void(const std::string& pcieDeviceId,
                             const std::vector<std::string>& pcieDevicePorts)>&
        callback,
    const std::string& pcieDevicePath,
    [[maybe_unused]] const std::string& service)
{
    std::string associationPath = pcieDevicePath + "/containing";
    dbus::utility::getAssociatedSubTreePaths(
        associationPath, sdbusplus::message::object_path(inventoryPath), 0,
        pciePortInterface,
        std::bind_front(afterGetPCIeDevicePortPath, asyncResp, pcieDeviceId,
                        callback));
}

inline void handlePCIePortsCollectionGet(
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
        std::bind_front(afterGetValidPcieDevice, asyncResp, pcieDeviceId,
                        std::bind_front(handlePCIeDevicePortPathsPortCollection,
                                        asyncResp)));
}

inline std::string dBusSensorPCIePortTypeToRedfish(const std::string& portType)
{
    if (portType ==
        "xyz.openbmc_project.Inventory.Item.PCIePort.PortType.DownstreamPort")
    {
        return "DownstreamPort";
    }

    if (portType ==
        "xyz.openbmc_project.Inventory.Item.PCIePort.PortType.UpstreamPort")
    {
        return "UpstreamPort";
    }

    return "Unknown";
}

inline void afterGetPCIeDevicePortInfo(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& properties)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBUS response error for getAllProperties {}",
                         ec.value());
        messages::internalError(asyncResp->res);
        return;
    }

    std::optional<double> currentSpeedGbps;
    std::optional<size_t> activeWidth;
    std::optional<std::string> portType;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "CurrentSpeedGbps",
        currentSpeedGbps, "ActiveWidth", activeWidth, "PortType", portType);

    if (!success)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    if (currentSpeedGbps.has_value())
    {
        asyncResp->res.jsonValue["CurrentSpeedGbps"] = *currentSpeedGbps;
    }
    if (activeWidth.has_value())
    {
        asyncResp->res.jsonValue["ActiveWidth"] = *activeWidth;
    }
    if (portType.has_value())
    {
        const std::string portTypeStr =
            dBusSensorPCIePortTypeToRedfish(*portType);
        if (portTypeStr != "Unknown")
        {
            asyncResp->res.jsonValue["PortType"] = portTypeStr;
        }
    }
}

inline void afterGetPCIeDevicePort(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& pcieDeviceId, const std::string& pciePortId,
    const std::string& portPath, const boost::system::error_code& ec,
    const dbus::utility::MapperGetObject& object)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBUS response error for getDbusObject {}",
                         ec.value());
        messages::internalError(asyncResp->res);
        return;
    }

    if (object.size() != 1)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    const auto& portIface = *object.begin();
    const std::string& serviceName = portIface.first;

    asyncResp->res.jsonValue["@odata.type"] = "#Port.v1_4_0.Port";
    asyncResp->res.jsonValue["@odata.id"] =
        std::format("/redfish/v1/Systems/{}/PCIeDevices/{}/Ports/{}",
                    BMCWEB_REDFISH_SYSTEM_URI_NAME, pcieDeviceId, pciePortId);
    asyncResp->res.jsonValue["Id"] = pciePortId;
    asyncResp->res.jsonValue["Name"] =
        std::format("{} {} Port", pcieDeviceId, pciePortId);

    auto& status = asyncResp->res.jsonValue["Status"];
    status["Health"] = resource::Health::OK;
    status["HealthRollup"] = resource::Health::OK;
    status["State"] = resource::State::Enabled;
    status["Conditions"] = nlohmann::json::array();

    asyncResp->res.jsonValue["PortProtocol"] = "PCIe";

    dbus::utility::getAllProperties(
        serviceName, portPath, std::string{pciePortInterface[0]},
        std::bind_front(afterGetPCIeDevicePortInfo, asyncResp));
}

inline void handlePCIeDevicePortPathsPortGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& pciePortId, const std::string& pcieDeviceId,
    const std::vector<std::string>& pcieDevicePorts)
{
    for (const std::string& portPath : pcieDevicePorts)
    {
        std::string portName =
            sdbusplus::message::object_path(portPath).filename();
        if (portName != pciePortId)
        {
            continue;
        }

        dbus::utility::getDbusObject(
            portPath, pciePortInterface,
            std::bind_front(afterGetPCIeDevicePort, asyncResp, pcieDeviceId,
                            pciePortId, portPath));
        return;
    }

    BMCWEB_LOG_DEBUG("PCIeDevice {} Port {} not found", pcieDeviceId,
                     pciePortId);

    messages::resourceNotFound(asyncResp->res, "PCIe Port", pciePortId);
}

inline void handlePCIePortGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& pcieDeviceId,
    const std::string& pciePortId)
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
        std::bind_front(afterGetValidPcieDevice, asyncResp, pcieDeviceId,
                        std::bind_front(handlePCIeDevicePortPathsPortGet,
                                        asyncResp, pciePortId)));
}

inline void requestRoutesSystemPCIePortsCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/PCIeDevices/<str>/Ports/")
        .privileges(redfish::privileges::getPCIeDevice)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handlePCIePortsCollectionGet, std::ref(app)));
}

inline void requestRoutesSystemPCIePortGet(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/<str>/PCIeDevices/<str>/Ports/<str>/")
        .privileges(redfish::privileges::getPCIeDevice)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handlePCIePortGet, std::ref(app)));
}
} // namespace redfish
