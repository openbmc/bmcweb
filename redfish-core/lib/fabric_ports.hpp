#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "dbus_utility.hpp"
#include "fabric_adapters.hpp"
#include "human_sort.hpp"
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
static constexpr std::array<std::string_view, 1> portInterfaces{
    "xyz.openbmc_project.Inventory.Connector.Port"};

inline void
    getFabricPortProperties(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& systemName,
                            const std::string& adapterId,
                            const std::string& portId)
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
    dbus::utility::getAssociatedSubTree(
        fabricAdapterPath + "/connecting",
        sdbusplus::message::object_path("/xyz/openbmc_project/inventory"), 0,
        portInterfaces,
        [callback{std::move(callback)}](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreeResponse& portSubTree) {
        callback(ec, portSubTree);
    });
}

inline void getAssociatedFabricPortSubTree(
    const std::string& adapterId,
    std::function<void(const boost::system::error_code&,
                       const dbus::utility::MapperGetSubTreeResponse&)>&&
        callback)
{
    getValidFabricAdapterPath(
        adapterId,
        [callback{std::move(callback)}](const boost::system::error_code& ec,
                                        const std::string& fabricAdapterPath,
                                        const std::string& fabricServiceName) {
        if (ec)
        {
            callback(ec, dbus::utility::MapperGetSubTreeResponse{});
            return;
        }
        afterGetAssociatedFabricPortSubTree(
            fabricAdapterPath, fabricServiceName, std::move(callback));
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
    return;
}

inline void getValidFabricPortPath(
    const std::string& adapterId, const std::string& portId,
    std::function<void(const boost::system::error_code& ec,
                       const std::string& portPath,
                       const std::string& portServiceName)>&& callback)
{
    getAssociatedFabricPortSubTree(
        adapterId, std::bind_front(afterGetValidFabricPortPath, portId,
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
    if constexpr (bmcwebEnableMultiHost)
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
        adapterId, portId,
        [asyncResp, portId](const boost::system::error_code& ec,
                            const std::string& /*unused*/,
                            const std::string& /*unused*/) {
        afterHandleFabricPortHead(asyncResp, portId, ec);
    });
}

inline void afterHandleFabricPortGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& adapterId,
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
    getFabricPortProperties(asyncResp, systemName, adapterId, portId);
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
    if constexpr (bmcwebEnableMultiHost)
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
    getValidFabricPortPath(adapterId, portId,
                           [asyncResp, systemName, adapterId,
                            portId](const boost::system::error_code& ec,
                                    const std::string& /*unused*/,
                                    const std::string& /*unused*/) {
        afterHandleFabricPortGet(asyncResp, systemName, adapterId, portId, ec);
    });
}

inline void afterHandleFarbicPortCollectionHead(
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

inline void handleFarbicPortCollectionHead(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& adapterId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if constexpr (bmcwebEnableMultiHost)
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
        adapterId, std::bind_front(afterHandleFarbicPortCollectionHead,
                                   asyncResp, adapterId));
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
    if constexpr (bmcwebEnableMultiHost)
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
        adapterId, std::bind_front(doHandleFabricPortCollectionGet, asyncResp,
                                   systemName, adapterId));
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
            std::bind_front(handleFarbicPortCollectionHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/FabricAdapters/<str>/Ports/")
        .privileges(redfish::privileges::getPortCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleFabricPortCollectionGet, std::ref(app)));
}

} // namespace redfish
