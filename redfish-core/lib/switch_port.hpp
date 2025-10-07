// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "fabric.hpp"
#include "generated/enums/port.hpp"
#include "generated/enums/resource.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"

#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <cstddef>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

static constexpr const char* inventoryPath = "/xyz/openbmc_project/inventory";
static constexpr std::array<std::string_view, 1> portInterface = {
    "xyz.openbmc_project.Inventory.Connector.Port"};

namespace redfish
{
inline port::PortType dBusSensorPortTypeToRedfish(const std::string& portType)
{
    if (portType ==
        "xyz.openbmc_project.Inventory.Connector.Port.PortType.DownstreamPort")
    {
        return port::PortType::DownstreamPort;
    }

    if (portType ==
        "xyz.openbmc_project.Inventory.Connector.Port.PortType.UpstreamPort")
    {
        return port::PortType::UpstreamPort;
    }

    return port::PortType::Invalid;
}

inline std::string dBusSensorPortProtocolToRedfish(
    const std::string& portProtocol)
{
    if (portProtocol ==
        "xyz.openbmc_project.Inventory.Connector.Port.PortProtocol.PCIe")
    {
        return "PCIe";
    }

    return "Unknown";
}

inline void afterGetFabricSwitchPortInfo(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& properties)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBus response error on GetAllProperties {}",
                         ec.value());
        messages::internalError(asyncResp->res);
        return;
    }

    std::optional<double> currentSpeedGbps;
    std::optional<size_t> activeWidth;
    std::optional<std::string> portType;
    std::optional<std::string> portProtocol;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "Speed", currentSpeedGbps,
        "Lanes", activeWidth, "PortType", portType, "PortProtocol",
        portProtocol);

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
        const port::PortType portTypeEnum =
            dBusSensorPortTypeToRedfish(*portType);
        if (portTypeEnum != port::PortType::Invalid)
        {
            asyncResp->res.jsonValue["PortType"] = portTypeEnum;
        }
    }
    if (portProtocol.has_value())
    {
        const std::string portProtocolStr =
            dBusSensorPortProtocolToRedfish(*portProtocol);
        if (portProtocolStr != "Unknown")
        {
            asyncResp->res.jsonValue["PortProtocol"] = portProtocolStr;
        }
    }
}

inline void handleFabricSwitchPortPathPortMetricsGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& fabricId, const std::string& switchId,
    const std::string& portId, [[maybe_unused]] const std::string& portPath,
    [[maybe_unused]] const std::string& serviceName)
{
    asyncResp->res.jsonValue["@odata.type"] = "#PortMetrics.v1_3_0.PortMetrics";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Fabrics/{}/Switches/{}/Ports/{}/Metrics", fabricId,
        switchId, portId);
    asyncResp->res.jsonValue["Id"] = "Metrics";
    asyncResp->res.jsonValue["Name"] =
        std::format("{} {} Port Metrics", switchId, portId);
}

inline void handleFabricSwitchPortPathPortGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& fabricId, const std::string& switchId,
    const std::string& portId, const std::string& portPath,
    const std::string& serviceName)
{
    asyncResp->res.jsonValue["@odata.type"] = "#Port.v1_4_0.Port";
    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Fabrics/{}/Switches/{}/Ports/{}",
                            fabricId, switchId, portId);
    asyncResp->res.jsonValue["Id"] = portId;
    asyncResp->res.jsonValue["Name"] =
        std::format("{} {} Port", switchId, portId);

    auto& status = asyncResp->res.jsonValue["Status"];
    status["Health"] = resource::Health::OK;
    status["State"] = resource::State::Enabled;

    asyncResp->res.jsonValue["Metrics"]["@odata.id"] = boost::urls::format(
        "/redfish/v1/Fabrics/{}/Switches/{}/Ports/{}/Metrics", fabricId,
        switchId, portId);

    dbus::utility::getAllProperties(
        serviceName, portPath, std::string{portInterface[0]},
        std::bind_front(afterGetFabricSwitchPortInfo, asyncResp));
}

inline void afterHandleFabricSwitchPortPaths(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& portId,
    const std::function<void(const std::string& portPath,
                             const std::string& serviceName)>& callback,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& object)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBus response error on GetAssociatedSubTreeById {}",
                         ec);
        messages::internalError(asyncResp->res);
        return;
    }

    std::string portPath;
    std::string serviceName;
    for (const auto& [path, service] : object)
    {
        std::string portName = sdbusplus::message::object_path(path).filename();
        if (portName == portId)
        {
            portPath = path;
            if (service.size() != 1)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            serviceName = service.begin()->first;
            break;
        }
    }

    if (portPath.empty())
    {
        messages::resourceNotFound(asyncResp->res, "Port", portId);
        return;
    }

    callback(portPath, serviceName);
}

inline void getAssociatedPortPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& portId,
    std::function<void(const std::string& portPath,
                       const std::string& serviceName)>&& callback,
    const std::string& switchPath)
{
    std::string associationPath = switchPath + "/connecting";
    dbus::utility::getAssociatedSubTree(
        associationPath, sdbusplus::message::object_path{inventoryPath}, 0,
        portInterface,
        std::bind_front(afterHandleFabricSwitchPortPaths, asyncResp, portId,
                        std::move(callback)));
}

inline void handleFabricSwitchPathPortCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& fabricId, const std::string& switchId,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& object)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBus response error on GetSubTreePaths {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }

    asyncResp->res.jsonValue["@odata.type"] = "#PortCollection.PortCollection";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Fabrics/{}/Switches/{}/Ports", fabricId, switchId);
    asyncResp->res.jsonValue["Name"] = switchId + " Port Collection";

    asyncResp->res.jsonValue["Members@odata.count"] = object.size();

    nlohmann::json& members = asyncResp->res.jsonValue["Members"];
    members = nlohmann::json::array();
    for (const std::string& path : object)
    {
        std::string name = sdbusplus::message::object_path(path).filename();
        nlohmann::json member;
        member["@odata.id"] =
            boost::urls::format("/redfish/v1/Fabrics/{}/Switches/{}/Ports/{}",
                                fabricId, switchId, name);
        members.push_back(std::move(member));
    }
}

inline void getFabricSwitchPortPaths(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& fabricId, const std::string& switchId,
    const std::string& switchPath)
{
    std::string associationPath = switchPath + "/connecting";
    dbus::utility::getAssociatedSubTreePaths(
        associationPath, sdbusplus::message::object_path{inventoryPath}, 0,
        portInterface,
        std::bind_front(handleFabricSwitchPathPortCollection, asyncResp,
                        fabricId, switchId));
}

inline void handleFabricSwitchPortPathsSwitchCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& fabricId, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& object)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBus response error on GetSubTreePaths {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }

    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Fabrics/{}/Switches", fabricId);
    asyncResp->res.jsonValue["@odata.type"] =
        "#SwitchCollection.SwitchCollection";
    asyncResp->res.jsonValue["Name"] = fabricId + " Switch Collection";

    asyncResp->res.jsonValue["Members@odata.count"] = object.size();

    nlohmann::json& members = asyncResp->res.jsonValue["Members"];
    members = nlohmann::json::array();
    for (const std::string& path : object)
    {
        std::string name = sdbusplus::message::object_path(path).filename();
        nlohmann::json member;
        member["@odata.id"] = boost::urls::format(
            "/redfish/v1/Fabrics/{}/Switches/{}", fabricId, name);
        members.push_back(std::move(member));
    }
}

inline void handleFabricSwitchPortMetricsGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& fabricId, const std::string& switchId,
    const std::string& portId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    getFabricSwitchPath(
        asyncResp, fabricId, switchId,
        std::bind_front(
            getAssociatedPortPath, asyncResp, portId,
            std::bind_front(handleFabricSwitchPortPathPortMetricsGet, asyncResp,
                            fabricId, switchId, portId)));
}

inline void handleFabricSwitchPortGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& fabricId, const std::string& switchId,
    const std::string& portId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    getFabricSwitchPath(
        asyncResp, fabricId, switchId,
        std::bind_front(
            getAssociatedPortPath, asyncResp, portId,
            std::bind_front(handleFabricSwitchPortPathPortGet, asyncResp,
                            fabricId, switchId, portId)));
}

inline void handleFabricSwitchPortsCollectionGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& fabricId, const std::string& switchId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    getFabricSwitchPath(asyncResp, fabricId, switchId,
                        std::bind_front(getFabricSwitchPortPaths, asyncResp,
                                        fabricId, switchId));
}

inline void requestRoutesFabricSwitchPort(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Fabrics/<str>/Switches/<str>/Ports/")
        .privileges(redfish::privileges::getPortCollection)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleFabricSwitchPortsCollectionGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Fabrics/<str>/Switches/<str>/Ports/<str>/")
        .privileges(redfish::privileges::getPort)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleFabricSwitchPortGet, std::ref(app)));

    BMCWEB_ROUTE(
        app, "/redfish/v1/Fabrics/<str>/Switches/<str>/Ports/<str>/Metrics/")
        .privileges(redfish::privileges::getPortMetrics)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleFabricSwitchPortMetricsGet, std::ref(app)));
}
} // namespace redfish
