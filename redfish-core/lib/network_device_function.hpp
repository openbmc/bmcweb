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
#include "utils/dbus_utils.hpp"

#include <boost/beast/http/verb.hpp>
#include <boost/url/format.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <cerrno>
#include <format>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace redfish
{

inline void afterGetNDFMacAddress(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& properties)
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

    const std::string* macAddress = nullptr;
    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "MACAddress", macAddress);

    if (!success || macAddress == nullptr)
    {
        return;
    }

    asyncResp->res.jsonValue["Ethernet"]["PermanentMACAddress"] = *macAddress;
}

inline void afterGetNDFPortProtocol(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& properties)
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

    const std::string* portProtocol = nullptr;
    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "PortProtocol",
        portProtocol);

    if (!success || portProtocol == nullptr)
    {
        return;
    }

    if (*portProtocol ==
        "xyz.openbmc_project.Inventory.Connector.Port.PortProtocol.Ethernet")
    {
        asyncResp->res.jsonValue["NetDevFuncType"] =
            network_device_function::NetworkDeviceTechnology::Ethernet;
    }
}

inline void handleNDFPortProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& networkAdapterId,
    const std::string& ndfId, const std::string& portPath,
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

    nlohmann::json::array_t ports;
    nlohmann::json::object_t portRef;
    portRef["@odata.id"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/NetworkAdapters/{}/Ports/{}", chassisId,
        networkAdapterId, ndfId);
    ports.emplace_back(std::move(portRef));
    asyncResp->res.jsonValue["AssignablePhysicalNetworkPorts"] =
        std::move(ports);

    dbus::utility::getAllProperties(
        serviceName, portPath, "xyz.openbmc_project.Inventory.Connector.Port",
        std::bind_front(afterGetNDFPortProtocol, asyncResp));

    dbus::utility::getAllProperties(
        serviceName, portPath,
        "xyz.openbmc_project.Inventory.Item.NetworkInterface",
        std::bind_front(afterGetNDFMacAddress, asyncResp));
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
            const sdbusplus::message::object_path associationPath =
                sdbusplus::message::object_path(adapterPath) / "connecting";
            dbus::utility::getAssociatedSubTreePaths(
                associationPath,
                sdbusplus::message::object_path{
                    "/xyz/openbmc_project/inventory"},
                0,
                std::array<std::string_view, 1>{
                    "xyz.openbmc_project.Inventory.Item.NetworkInterface"},
                std::bind_front(handleNDFCollectionPaths, asyncResp, chassisId,
                                networkAdapterId));
        });
}

inline void getNDFPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& ndfId,
    std::function<void(const std::string& ndfPath,
                       const std::string& serviceName)>&& callback,
    const std::string& adapterPath)
{
    const std::string associationPath = adapterPath + "/connecting";
    dbus::utility::getAssociatedSubTree(
        associationPath,
        sdbusplus::message::object_path{"/xyz/openbmc_project/inventory"}, 0,
        std::array<std::string_view, 1>{
            "xyz.openbmc_project.Inventory.Item.NetworkInterface"},
        std::bind_front(afterNetworkAdapterPortPaths, asyncResp, ndfId,
                        std::move(callback)));
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
        std::bind_front(getNDFPath, asyncResp, ndfId,
                        std::bind_front(handleNDFPortProperties, asyncResp,
                                        chassisId, networkAdapterId, ndfId)));
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
