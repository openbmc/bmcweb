// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/network_device_function.hpp"
#include "logging.hpp"
#include "network_adapter.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/collection.hpp"

#include <boost/beast/http/verb.hpp>
#include <boost/url/format.hpp>

#include <array>
#include <cerrno>
#include <format>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>

namespace redfish
{

inline constexpr std::array<std::string_view, 1> ndfInterface = {
    "xyz.openbmc_project.Inventory.Item.NetworkInterface"};

inline constexpr std::array<std::string_view, 1> portInterface = {
    "xyz.openbmc_project.Inventory.Connector.Port"};

inline void afterGetNDFPortProtocol(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec, const std::string& portProtocol)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("D-Bus response error for PortProtocol: {}",
                             ec.message());
            messages::internalError(asyncResp->res);
        }
        return;
    }

    if (portProtocol ==
        "xyz.openbmc_project.Inventory.Connector.Port.PortProtocol.Ethernet")
    {
        asyncResp->res.jsonValue["NetDevFuncType"] =
            network_device_function::NetworkDeviceTechnology::Ethernet;
    }
}

inline void afterGetAssignablePort(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& networkAdapterId,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& object)
{
    if (ec)
    {
        if (ec.value() != boost::system::errc::io_error && ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR(
                "D-Bus response error on assignable_physical_network_ports: {}",
                ec.message());
            messages::internalError(asyncResp->res);
        }
        return;
    }

    if (object.empty())
    {
        return;
    }

    const auto& [portPath, services] = *object.begin();
    if (services.size() != 1)
    {
        messages::internalError(asyncResp->res);
        return;
    }
    const std::string& serviceName = services.begin()->first;

    nlohmann::json::array_t ports;
    nlohmann::json::object_t portRef;
    const std::string portId = sdbusplus::object_path(portPath).filename();
    portRef["@odata.id"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/NetworkAdapters/{}/Ports/{}", chassisId,
        networkAdapterId, portId);
    ports.emplace_back(std::move(portRef));
    asyncResp->res.jsonValue["AssignablePhysicalNetworkPorts"] =
        std::move(ports);

    dbus::utility::getProperty<std::string>(
        serviceName, portPath, "xyz.openbmc_project.Inventory.Connector.Port",
        "PortProtocol", std::bind_front(afterGetNDFPortProtocol, asyncResp));
}

inline void afterGetNDFMacAddress(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec, const std::string& macAddress)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("D-Bus response error for MACAddress: {}",
                             ec.message());
            messages::internalError(asyncResp->res);
        }
        return;
    }

    asyncResp->res.jsonValue["Ethernet"]["PermanentMACAddress"] = macAddress;
}

inline void handleNDFFromPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& networkAdapterId,
    const std::string& ndfId, const std::string& ndfPath,
    const std::string& serviceName)
{
    asyncResp->res.jsonValue["@odata.type"] =
        "#NetworkDeviceFunction.v1_9_0.NetworkDeviceFunction";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/NetworkAdapters/{}/NetworkDeviceFunctions/{}",
        chassisId, networkAdapterId, ndfId);
    asyncResp->res.jsonValue["Id"] = ndfId;
    asyncResp->res.jsonValue["Name"] =
        std::format("{} Network Device Function", ndfId);

    dbus::utility::getProperty<std::string>(
        serviceName, ndfPath,
        "xyz.openbmc_project.Inventory.Item.NetworkInterface", "MACAddress",
        std::bind_front(afterGetNDFMacAddress, asyncResp));

    const sdbusplus::object_path associationPath =
        sdbusplus::object_path(ndfPath) / "assignable_physical_network_ports";
    dbus::utility::getAssociatedSubTree(
        associationPath,
        sdbusplus::object_path{"/xyz/openbmc_project/inventory"}, 0,
        portInterface,
        std::bind_front(afterGetAssignablePort, asyncResp, chassisId,
                        networkAdapterId));
}

inline void handleNDFGet(crow::App& app, const crow::Request& req,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& chassisId,
                         const std::string& networkAdapterId,
                         const std::string& ndfId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    getNetworkAdapterPath(
        asyncResp, chassisId, networkAdapterId,
        std::bind_front(getNetworkAdapterAssociatedSubResource, asyncResp,
                        std::string_view{"exposing"},
                        std::span<const std::string_view>(ndfInterface),
                        std::string_view{"NetworkDeviceFunction"}, ndfId,
                        std::bind_front(handleNDFFromPath, asyncResp, chassisId,
                                        networkAdapterId, ndfId)));
}

inline void handleNDFCollectionPaths(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& networkAdapterId,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& objects)
{
    asyncResp->res.jsonValue["@odata.type"] =
        "#NetworkDeviceFunctionCollection.NetworkDeviceFunctionCollection";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/NetworkAdapters/{}/NetworkDeviceFunctions",
        chassisId, networkAdapterId);
    asyncResp->res.jsonValue["Name"] =
        std::format("{} Network Device Function Collection", networkAdapterId);

    collection_util::handleCollectionMembers(
        asyncResp,
        boost::urls::format(
            "/redfish/v1/Chassis/{}/NetworkAdapters/{}/NetworkDeviceFunctions",
            chassisId, networkAdapterId),
        nlohmann::json::json_pointer("/Members"), ec, objects);
}

inline void handleNDFCollectionGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& networkAdapterId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    getNetworkAdapterPath(
        asyncResp, chassisId, networkAdapterId,
        [asyncResp, chassisId,
         networkAdapterId](const std::string& adapterPath) {
            const sdbusplus::object_path associationPath =
                sdbusplus::object_path(adapterPath) / "exposing";
            dbus::utility::getAssociatedSubTreePaths(
                associationPath,
                sdbusplus::object_path{"/xyz/openbmc_project/inventory"}, 0,
                ndfInterface,
                std::bind_front(handleNDFCollectionPaths, asyncResp, chassisId,
                                networkAdapterId));
        });
}

inline void requestRoutesNetworkDeviceFunction(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Chassis/<str>/NetworkAdapters/<str>/NetworkDeviceFunctions/")
        .privileges(redfish::privileges::getNetworkDeviceFunctionCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleNDFCollectionGet, std::ref(app)));

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Chassis/<str>/NetworkAdapters/<str>/NetworkDeviceFunctions/<str>/")
        .privileges(redfish::privileges::getNetworkDeviceFunction)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleNDFGet, std::ref(app)));
}

} // namespace redfish
