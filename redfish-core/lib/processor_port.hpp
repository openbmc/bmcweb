// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/port.hpp"
#include "generated/enums/protocol.hpp"
#include "http_request.hpp"
#include "human_sort.hpp"
#include "logging.hpp"
#include "processor.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/dbus_utils.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <format>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace redfish
{

inline std::optional<protocol::Protocol> dbusProcessorPortProtocolToRedfish(
    const std::string& portProtocol)
{
    if (portProtocol ==
        "xyz.openbmc_project.Inventory.Connector.Port.PortProtocol.NVLink")
    {
        return protocol::Protocol::NVLink;
    }
    if (portProtocol ==
        "xyz.openbmc_project.Inventory.Connector.Port.PortProtocol.PCIe")
    {
        return protocol::Protocol::PCIe;
    }
    // Unknown / anything else: omit the property
    return std::nullopt;
}

inline std::optional<port::PortType> dbusProcessorPortTypeToRedfish(
    const std::string& portType)
{
    if (portType ==
        "xyz.openbmc_project.Inventory.Connector.Port.PortType.Bidirectional")
    {
        return port::PortType::BidirectionalPort;
    }
    if (portType ==
        "xyz.openbmc_project.Inventory.Connector.Port.PortType.Downstream")
    {
        return port::PortType::DownstreamPort;
    }
    if (portType ==
        "xyz.openbmc_project.Inventory.Connector.Port.PortType.Upstream")
    {
        return port::PortType::UpstreamPort;
    }
    // Unknown / anything else: omit the property
    return std::nullopt;
}

inline std::optional<port::LinkState> dbusProcessorPortLinkStateToRedfish(
    const std::string& linkState)
{
    if (linkState ==
        "xyz.openbmc_project.Inventory.Connector.Port.LinkState.Enabled")
    {
        return port::LinkState::Enabled;
    }
    if (linkState ==
        "xyz.openbmc_project.Inventory.Connector.Port.LinkState.Disabled")
    {
        return port::LinkState::Disabled;
    }
    // Unknown / anything else: omit the property
    return std::nullopt;
}

inline std::optional<port::LinkStatus> dbusProcessorPortLinkStatusToRedfish(
    const std::string& linkStatus)
{
    if (linkStatus ==
        "xyz.openbmc_project.Inventory.Connector.Port.LinkStatus.LinkUp")
    {
        return port::LinkStatus::LinkUp;
    }
    if (linkStatus ==
        "xyz.openbmc_project.Inventory.Connector.Port.LinkStatus.LinkDown")
    {
        return port::LinkStatus::LinkDown;
    }
    if (linkStatus ==
        "xyz.openbmc_project.Inventory.Connector.Port.LinkStatus.NoLink")
    {
        return port::LinkStatus::NoLink;
    }
    if (linkStatus ==
        "xyz.openbmc_project.Inventory.Connector.Port.LinkStatus.Starting")
    {
        return port::LinkStatus::Starting;
    }
    if (linkStatus ==
        "xyz.openbmc_project.Inventory.Connector.Port.LinkStatus.Training")
    {
        return port::LinkStatus::Training;
    }
    // Unknown / anything else: omit the property
    return std::nullopt;
}

inline void afterGetProcessorPortProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& properties)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBus response error on Port GetAllProperties {}",
                         ec.value());
        messages::internalError(asyncResp->res);
        return;
    }

    std::optional<std::string> portProtocol;
    std::optional<std::string> portType;
    std::optional<uint64_t> speed;
    std::optional<uint64_t> maxSpeed;
    std::optional<std::string> linkState;
    std::optional<std::string> linkStatus;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "PortProtocol",
        portProtocol, "PortType", portType, "Speed", speed, "MaxSpeed",
        maxSpeed, "LinkState", linkState, "LinkStatus", linkStatus);
    if (!success)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    if (portProtocol.has_value())
    {
        std::optional<protocol::Protocol> proto =
            dbusProcessorPortProtocolToRedfish(*portProtocol);
        if (proto.has_value())
        {
            asyncResp->res.jsonValue["PortProtocol"] = *proto;
        }
    }
    if (portType.has_value())
    {
        std::optional<port::PortType> type =
            dbusProcessorPortTypeToRedfish(*portType);
        if (type.has_value())
        {
            asyncResp->res.jsonValue["PortType"] = *type;
        }
    }
    if (speed.has_value() && *speed != std::numeric_limits<uint64_t>::max())
    {
        // Convert from bits per second to Gigabits per second (Gbps)
        asyncResp->res.jsonValue["CurrentSpeedGbps"] =
            static_cast<double>(*speed) / 1e9;
    }
    if (maxSpeed.has_value() &&
        *maxSpeed != std::numeric_limits<uint64_t>::max())
    {
        asyncResp->res.jsonValue["MaxSpeedGbps"] =
            static_cast<double>(*maxSpeed) / 1e9;
    }
    if (linkState.has_value())
    {
        std::optional<port::LinkState> state =
            dbusProcessorPortLinkStateToRedfish(*linkState);
        if (state.has_value())
        {
            asyncResp->res.jsonValue["LinkState"] = *state;
        }
    }
    if (linkStatus.has_value())
    {
        std::optional<port::LinkStatus> status =
            dbusProcessorPortLinkStatusToRedfish(*linkStatus);
        if (status.has_value())
        {
            asyncResp->res.jsonValue["LinkStatus"] = *status;
        }
    }
}

inline void afterGetProcessorPort(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& processorId, const std::string& portId,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBus response error on Port GetSubTree {}",
                         ec.value());
        messages::internalError(asyncResp->res);
        return;
    }

    for (const auto& [portPath, serviceMap] : subtree)
    {
        if (sdbusplus::object_path(portPath).filename() != portId)
        {
            continue;
        }
        if (serviceMap.empty())
        {
            messages::internalError(asyncResp->res);
            return;
        }

        asyncResp->res.addHeader(
            boost::beast::http::field::link,
            "</redfish/v1/JsonSchemas/Port/Port.json>; rel=describedby");
        asyncResp->res.jsonValue["@odata.type"] = "#Port.v1_4_0.Port";
        asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
            "/redfish/v1/Systems/{}/Processors/{}/Ports/{}",
            BMCWEB_REDFISH_SYSTEM_URI_NAME, processorId, portId);
        asyncResp->res.jsonValue["Id"] = portId;
        asyncResp->res.jsonValue["Name"] =
            std::format("{} {} Port", processorId, portId);

        const std::string& serviceName = serviceMap.begin()->first;
        dbus::utility::getAllProperties(
            serviceName, portPath,
            "xyz.openbmc_project.Inventory.Connector.Port",
            std::bind_front(afterGetProcessorPortProperties, asyncResp));
        return;
    }

    messages::resourceNotFound(asyncResp->res, "Port", portId);
}

inline void getProcessorPort(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& processorId, const std::string& portId,
    const std::string& objectPath,
    const dbus::utility::MapperServiceMap& /*serviceMap*/)
{
    constexpr std::array<std::string_view, 1> processorPortInterfaces = {
        "xyz.openbmc_project.Inventory.Connector.Port"};
    dbus::utility::getSubTree(
        objectPath, 1, processorPortInterfaces,
        std::bind_front(afterGetProcessorPort, asyncResp, processorId, portId));
}

inline void handleProcessorPortGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& processorId,
    const std::string& portId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    getProcessorObject(
        asyncResp, processorId,
        std::bind_front(getProcessorPort, asyncResp, processorId, portId));
}

inline void afterGetProcessorPortCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& processorId, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& portPaths)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBus response error on Port GetSubTreePaths {}",
                         ec.value());
        messages::internalError(asyncResp->res);
        return;
    }

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/PortCollection/PortCollection.json>; rel=describedby");
    asyncResp->res.jsonValue["@odata.type"] = "#PortCollection.PortCollection";
    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Systems/{}/Processors/{}/Ports",
                            BMCWEB_REDFISH_SYSTEM_URI_NAME, processorId);
    asyncResp->res.jsonValue["Name"] =
        std::format("{} Port Collection", processorId);

    std::vector<std::string> portIds;
    portIds.reserve(portPaths.size());
    for (const std::string& portPath : portPaths)
    {
        std::string portId = sdbusplus::object_path(portPath).filename();
        if (!portId.empty())
        {
            portIds.emplace_back(std::move(portId));
        }
    }
    std::ranges::sort(portIds, AlphanumLess<std::string>());

    nlohmann::json::array_t members;
    for (const std::string& portId : portIds)
    {
        nlohmann::json::object_t member;
        member["@odata.id"] = boost::urls::format(
            "/redfish/v1/Systems/{}/Processors/{}/Ports/{}",
            BMCWEB_REDFISH_SYSTEM_URI_NAME, processorId, portId);
        members.emplace_back(std::move(member));
    }
    asyncResp->res.jsonValue["Members@odata.count"] = members.size();
    asyncResp->res.jsonValue["Members"] = std::move(members);
}

inline void getProcessorPortCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& processorId, const std::string& objectPath,
    const dbus::utility::MapperServiceMap& /*serviceMap*/)
{
    constexpr std::array<std::string_view, 1> processorPortInterfaces = {
        "xyz.openbmc_project.Inventory.Connector.Port"};
    dbus::utility::getSubTreePaths(
        objectPath, 1, processorPortInterfaces,
        std::bind_front(afterGetProcessorPortCollection, asyncResp,
                        processorId));
}

inline void handleProcessorPortCollectionGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& processorId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    getProcessorObject(
        asyncResp, processorId,
        std::bind_front(getProcessorPortCollection, asyncResp, processorId));
}

inline void requestRoutesProcessorPortCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Processors/<str>/Ports/")
        .privileges(redfish::privileges::getPortCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleProcessorPortCollectionGet, std::ref(app)));
}

inline void requestRoutesProcessorPort(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Processors/<str>/Ports/<str>/")
        .privileges(redfish::privileges::getPort)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleProcessorPortGet, std::ref(app)));
}

} // namespace redfish
