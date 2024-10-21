#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "dbus_utility.hpp"
#include "fabric_adapters.hpp"
#include "human_sort.hpp"
#include "led.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"

#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>

#include <algorithm>
#include <array>
#include <functional>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>
#include <system_error>

namespace redfish
{
inline void afterGetFabricPortLocation(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec, const std::string& value)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
            messages::internalError(asyncResp->res);
        }
        return;
    }
    asyncResp->res.jsonValue["Location"]["PartLocation"]["ServiceLabel"] =
        value;
}

inline void
    getFabricPortLocation(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& portPath,
                          const std::string& serviceName)
{
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, serviceName, portPath,
        "xyz.openbmc_project.Inventory.Decorator.LocationCode", "LocationCode",
        std::bind_front(afterGetFabricPortLocation, asyncResp));
}

inline void
    afterGetFabricPortState(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const boost::system::error_code& ec, bool present)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error for State, ec {}",
                             ec.value());
            messages::internalError(asyncResp->res);
        }
        return;
    }

    if (!present)
    {
        asyncResp->res.jsonValue["Status"]["State"] = "Absent";
    }
}

inline void
    getFabricPortState(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& portPath,
                       const std::string& serviceName)
{
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, serviceName, portPath,
        "xyz.openbmc_project.Inventory.Item", "Present",
        std::bind_front(afterGetFabricPortState, asyncResp));
}

inline void afterGetFabricPortHealth(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec, bool functional)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error for Health, ec {}",
                             ec.value());
            messages::internalError(asyncResp->res);
        }
        return;
    }

    if (!functional)
    {
        asyncResp->res.jsonValue["Status"]["Health"] = "Critical";
    }
}

inline void
    getFabricPortHealth(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& portPath,
                        const std::string& serviceName)
{
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, serviceName, portPath,
        "xyz.openbmc_project.State.Decorator.OperationalStatus", "Functional",
        std::bind_front(afterGetFabricPortHealth, asyncResp));
}

inline void getFabricPortProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& adapterId,
    const std::string& portId, const std::string& portPath,
    const std::string& serviceName)
{
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/Port/Port.json>; rel=describedby");

    asyncResp->res.jsonValue["@odata.type"] = "#Port.v1_11_0.Port";
    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Systems/{}/FabricAdapters/{}/Ports/{}",
                            systemName, adapterId, portId);
    asyncResp->res.jsonValue["Id"] = portId;
    asyncResp->res.jsonValue["Name"] = "Fabric Port";
    asyncResp->res.jsonValue["Status"]["State"] = "Enabled";
    asyncResp->res.jsonValue["Status"]["Health"] = "OK";

    getFabricPortLocation(asyncResp, portPath, serviceName);
    getFabricPortState(asyncResp, portPath, serviceName);
    getFabricPortHealth(asyncResp, portPath, serviceName);
    getLocationIndicatorActive(asyncResp, portPath);
}

inline void afterGetAssociatedFabricPortSubTree(
    const std::string& fabricAdapterPath, const std::string& fabricServiceName,
    const std::function<void(const boost::system::error_code&,
                             const dbus::utility::MapperGetSubTreeResponse&)>&
        callback)
{
    if (fabricAdapterPath.empty() || fabricServiceName.empty())
    {
        // Adapter Not found
        boost::system::error_code ec1{EBADR, boost::system::system_category()};
        callback(ec1, dbus::utility::MapperGetSubTreeResponse{});
        return;
    }

    constexpr std::array<std::string_view, 1> portInterfaces{
        "xyz.openbmc_project.Inventory.Connector.Port"};
    dbus::utility::getAssociatedSubTree(
        fabricAdapterPath + "/connecting",
        sdbusplus::message::object_path("/xyz/openbmc_project/inventory"), 0,
        portInterfaces,
        [callback](const boost::system::error_code& ec,
                   const dbus::utility::MapperGetSubTreeResponse& portSubTree) {
        callback(ec, portSubTree);
    });
}

inline void getAssociatedFabricPortSubTree(
    const std::string& adapterId,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    std::function<void(const boost::system::error_code&,
                       const dbus::utility::MapperGetSubTreeResponse&)>&&
        callback)
{
    getValidFabricAdapterPath(
        adapterId, asyncResp,
        [callback{std::move(callback)}](
            const boost::system::error_code& ec,
            const std::string& fabricAdapterPath,
            const std::string& fabricServiceName,
            const dbus::utility::InterfaceList& /*unused*/) {
        if (ec)
        {
            callback(ec, dbus::utility::MapperGetSubTreeResponse{});
            return;
        }
        afterGetAssociatedFabricPortSubTree(fabricAdapterPath,
                                            fabricServiceName, callback);
    });
}

inline void afterGetValidFabricPortPath(
    const std::string& portId,
    std::function<void(const boost::system::error_code&,
                       const std::string& portPath,
                       const std::string& portServiceName)>& callback,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& portSubTree)
{
    if (ec)
    {
        callback(ec, std::string{}, std::string{});
        return;
    }
    for (const auto& [portPath, serviceMap] : portSubTree)
    {
        if (portId == sdbusplus::message::object_path(portPath).filename())
        {
            callback(ec, portPath, serviceMap.begin()->first);
            return;
        }
    }
    boost::system::error_code ec2{EBADR, boost::system::system_category()};
    callback(ec2, std::string{}, std::string{});
}

inline void getValidFabricPortPath(
    const std::string& adapterId, const std::string& portId,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    std::function<void(const boost::system::error_code& ec,
                       const std::string& portPath,
                       const std::string& portServiceName)>&& callback)
{
    getAssociatedFabricPortSubTree(adapterId, asyncResp,
                                   std::bind_front(afterGetValidFabricPortPath,
                                                   portId,
                                                   std::move(callback)));
}

inline void afterHandleFabricPortHead(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& portId, const boost::system::error_code& ec)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
            messages::internalError(asyncResp->res);
            return;
        }
        BMCWEB_LOG_WARNING("Port not found");
        messages::resourceNotFound(asyncResp->res, "Port", portId);
        return;
    }
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/Port/Port.json>; rel=describedby");
}

inline void
    handleFabricPortHead(crow::App& app, const crow::Request& req,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& systemName,
                         const std::string& adapterId,
                         const std::string& portId)
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
    if (systemName != "system")
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    getValidFabricPortPath(
        adapterId, portId, asyncResp,
        [asyncResp, portId](const boost::system::error_code& ec,
                            const std::string& /*unused*/,
                            const std::string& /*unused*/) {
        afterHandleFabricPortHead(asyncResp, portId, ec);
    });
}

inline void afterHandleFabricPortGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& adapterId,
    const std::string& portId, const boost::system::error_code& ec,
    const std::string& portPath, const std::string& portServiceName)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
            messages::internalError(asyncResp->res);
            return;
        }
        BMCWEB_LOG_WARNING("Port not found");
        messages::resourceNotFound(asyncResp->res, "Port", portId);
        return;
    }
    getFabricPortProperties(asyncResp, systemName, adapterId, portId, portPath,
                            portServiceName);
}

inline void
    handleFabricPortGet(App& app, const crow::Request& req,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& systemName,
                        const std::string& adapterId, const std::string& portId)
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
    if (systemName != "system")
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    getValidFabricPortPath(adapterId, portId, asyncResp,
                           std::bind_front(afterHandleFabricPortGet, asyncResp,
                                           systemName, adapterId, portId));
}

inline void afterHandleFabricPortCollectionHead(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& adapterId, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& /*unused*/)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
            messages::internalError(asyncResp->res);
            return;
        }
        BMCWEB_LOG_WARNING("Adapter not found");
        messages::resourceNotFound(asyncResp->res, "Adapter", adapterId);
        return;
    }
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/PortCollection/PortCollection.json>; rel=describedby");
}

inline void handleFabricPortCollectionHead(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& adapterId)
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
    if (systemName != "system")
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    getAssociatedFabricPortSubTree(
        adapterId, asyncResp,
        std::bind_front(afterHandleFabricPortCollectionHead, asyncResp,
                        adapterId));
}

inline void doHandleFabricPortCollectionGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& adapterId,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& portSubTree)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
            messages::internalError(asyncResp->res);
            return;
        }
        BMCWEB_LOG_WARNING("Adapter not found");
        messages::resourceNotFound(asyncResp->res, "Adapter", adapterId);
        return;
    }
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/PortCollection/PortCollection.json>; rel=describedby");

    asyncResp->res.jsonValue["@odata.type"] = "#PortCollection.PortCollection";
    asyncResp->res.jsonValue["Name"] = "Port Collection";
    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Systems/{}/FabricAdapters/{}/Ports",
                            systemName, adapterId);
    asyncResp->res.jsonValue["Members"] = nlohmann::json::array();

    std::vector<std::string> portIdNames;
    for (const auto& [portPath, serviceMap] : portSubTree)
    {
        std::string portId =
            sdbusplus::message::object_path(portPath).filename();
        if (!portId.empty())
        {
            portIdNames.emplace_back(std::move(portId));
        }
    }

    std::ranges::sort(portIdNames, AlphanumLess<std::string>());

    nlohmann::json& members = asyncResp->res.jsonValue["Members"];
    for (const std::string& portId : portIdNames)
    {
        nlohmann::json item;
        item["@odata.id"] = boost::urls::format(
            "/redfish/v1/Systems/{}/FabricAdapters/{}/Ports/{}", systemName,
            adapterId, portId);
        members.emplace_back(std::move(item));
    }
    asyncResp->res.jsonValue["Members@odata.count"] = members.size();
}

inline void handleFabricPortCollectionGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& adapterId)
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
    if (systemName != "system")
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    getAssociatedFabricPortSubTree(
        adapterId, asyncResp,
        std::bind_front(doHandleFabricPortCollectionGet, asyncResp, systemName,
                        adapterId));
}

inline void afterHandlePortPatch(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& portId, const bool locationIndicatorActive,
    const boost::system::error_code& ec, const std::string& portPath,
    const std::string& /*unused*/)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
            messages::internalError(asyncResp->res);
            return;
        }
        BMCWEB_LOG_WARNING("Port not found");
        messages::resourceNotFound(asyncResp->res, "Port", portId);
        return;
    }

    setLocationIndicatorActive(asyncResp, portPath, locationIndicatorActive);
}

inline void handlePortPatch(App& app, const crow::Request& req,
                            const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& systemName,
                            const std::string& adapterId,
                            const std::string& portId)
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
    if (systemName != "system")
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    std::optional<bool> locationIndicatorActive;
    if (!json_util::readJsonPatch(req, asyncResp->res,
                                  "LocationIndicatorActive",
                                  locationIndicatorActive))
    {
        return;
    }
    if (locationIndicatorActive)
    {
        getValidFabricPortPath(adapterId, portId, asyncResp,
                               std::bind_front(afterHandlePortPatch, asyncResp,
                                               portId,
                                               *locationIndicatorActive));
    }
}

inline void requestRoutesFabricPort(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/<str>/FabricAdapters/<str>/Ports/<str>/")
        .privileges(redfish::privileges::headPort)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleFabricPortHead, std::ref(app)));

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/<str>/FabricAdapters/<str>/Ports/<str>/")
        .privileges(redfish::privileges::getPort)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleFabricPortGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/FabricAdapters/<str>/Ports/")
        .privileges(redfish::privileges::headPortCollection)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleFabricPortCollectionHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/FabricAdapters/<str>/Ports/")
        .privileges(redfish::privileges::getPortCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleFabricPortCollectionGet, std::ref(app)));

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/<str>/FabricAdapters/<str>/Ports/<str>/")
        .privileges(redfish::privileges::patchPort)
        .methods(boost::beast::http::verb::patch)(
            std::bind_front(handlePortPatch, std::ref(app)));
}

} // namespace redfish
