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

namespace redfish
{
static constexpr std::array<std::string_view, 1> fabricPortInterface{
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

    asyncResp->res.jsonValue["@odata.type"] = "#Port.v1_7_0.Port";
    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Systems/{}/FabricAdapters/{}/Ports/{}",
                            systemName, adapterId, portId);
    asyncResp->res.jsonValue["Id"] = portId;
    asyncResp->res.jsonValue["Name"] = "Fabric Port";
}

inline void getFabricPortPaths(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& fabricAdapterPath,
    const std::function<void(const boost::system::error_code& ec,
                             const dbus::utility::MapperGetSubTreePathsResponse&
                                 fabricPortPaths)>&& callback)
{
    sdbusplus::message::object_path endpointPath{fabricAdapterPath};
    endpointPath /= "connecting";

    dbus::utility::getAssociatedSubTreePaths(
        endpointPath,
        sdbusplus::message::object_path("/xyz/openbmc_project/inventory"), 0,
        fabricPortInterface,
        [asyncResp, callback](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreePathsResponse& subtreePaths) {
        callback(ec, subtreePaths);
    });
}

inline void afterGetValidFabricPortPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& portId,
    const dbus::utility::MapperGetSubTreePathsResponse& fabricPortPaths,
    const std::function<void(const boost::system::error_code& ec,
                             const std::string& portPath)>&& callback)

{
    auto portIter = std::ranges::find_if(
        fabricPortPaths, [&portId](const std::string& portPath) {
        return sdbusplus::message::object_path(portPath).filename() == portId;
    });
    if (portIter == fabricPortPaths.end())
    {
        BMCWEB_LOG_WARNING("Port {} not found", portId);
        messages::resourceNotFound(asyncResp->res, "Port", portId);
        return;
    }

    const std::string& portPath = *portIter;
    dbus::utility::getDbusObject(
        portPath, fabricPortInterface,
        [portPath, asyncResp,
         callback](const boost::system::error_code& ec,
                   const dbus::utility::MapperGetObject& object) {
        if (ec || object.empty())
        {
            BMCWEB_LOG_ERROR("DBUS response error on getDbusObject {}",
                             ec.value());
            messages::internalError(asyncResp->res);
            return;
        }
        callback(ec, portPath);
    });
}

inline void getValidFabricPortPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& fabricAdapterPath, const std::string& portId,
    const std::function<void(const boost::system::error_code& ec,
                             const std::string& portPath)>&& callback)
{
    getFabricPortPaths(asyncResp, fabricAdapterPath,
                       [asyncResp, portId, callback{callback}](
                           const boost::system::error_code& ec,
                           const dbus::utility::MapperGetSubTreePathsResponse&
                               fabricPortPaths) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
                messages::internalError(asyncResp->res);
            }
            return;
        }
        afterGetValidFabricPortPath(asyncResp, portId, fabricPortPaths,
                                    std::move(callback));
    });
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

    getValidFabricAdapterPath(adapterId, systemName, asyncResp,
                              [asyncResp, systemName, adapterId,
                               portId](const boost::system::error_code& ec,
                                       const std::string& fabricAdapterPath,
                                       const std::string& serviceName) {
        if (ec)
        {
            if (ec.value() == boost::system::errc::io_error)
            {
                messages::resourceNotFound(asyncResp->res, "FabricAdapter",
                                           adapterId);
                return;
            }

            BMCWEB_LOG_ERROR("DBus method call failed with error {}",
                             ec.value());
            messages::internalError(asyncResp->res);
            return;
        }
        if (fabricAdapterPath.empty() || serviceName.empty())
        {
            BMCWEB_LOG_WARNING("Adapter not found");
            messages::resourceNotFound(asyncResp->res, "FabricAdapter",
                                       adapterId);
            return;
        }

        getValidFabricPortPath(
            asyncResp, fabricAdapterPath, portId,
            [asyncResp, portId](const boost::system::error_code& ec2,
                                const std::string& portPath) {
            if (ec2)
            {
                BMCWEB_LOG_ERROR("DBUS response error {}", ec2.value());
                messages::internalError(asyncResp->res);
                return;
            }
            if (portPath.empty())
            {
                BMCWEB_LOG_WARNING("FabricPort not found");
                messages::resourceNotFound(asyncResp->res, "Port", portId);
                return;
            }
            asyncResp->res.addHeader(
                boost::beast::http::field::link,
                "</redfish/v1/JsonSchemas/Port/Port.json>; rel=describedby");
        });
    });
}

inline void doHandleFabricPortGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& adapterId,
    const std::string& portId, const boost::system::error_code& ec,
    const std::string& fabricAdapterPath, const std::string&)
{
    if (ec)
    {
        if (ec.value() == boost::system::errc::io_error)
        {
            messages::resourceNotFound(asyncResp->res, "FabricAdapter",
                                       adapterId);
            return;
        }

        BMCWEB_LOG_ERROR("DBus method call failed with error {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }
    getValidFabricPortPath(asyncResp, fabricAdapterPath, portId,
                           [asyncResp, systemName, adapterId,
                            portId](const boost::system::error_code& ec2,
                                    const std::string& portPath) {
        if (ec2)
        {
            BMCWEB_LOG_ERROR("DBUS response error {}", ec2.value());
            messages::internalError(asyncResp->res);
            return;
        }
        if (portPath.empty())
        {
            BMCWEB_LOG_WARNING("FabricPort not found");
            messages::resourceNotFound(asyncResp->res, "Port", portId);
            return;
        }
        getFabricPortProperties(asyncResp, systemName, adapterId, portId);
    });
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

    getValidFabricAdapterPath(adapterId, systemName, asyncResp,
                              std::bind_front(doHandleFabricPortGet, asyncResp,
                                              systemName, adapterId, portId));
}

inline void afterDoHandleFabricPortCollectionGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& adapterId,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& fabricPortPaths)
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
    std::vector<std::string> portIdNames;
    for (const auto& portPath : fabricPortPaths)
    {
        std::string portId =
            sdbusplus::message::object_path(portPath).filename();
        if (portId.empty())
        {
            BMCWEB_LOG_ERROR("Invalid portPath");
            messages::internalError(asyncResp->res);
            return;
        }
        portIdNames.emplace_back(std::move(portId));
    }
    std::sort(portIdNames.begin(), portIdNames.end(),
              AlphanumLess<std::string>());

    nlohmann::json::array_t members;
    for (const std::string& portId : portIdNames)
    {
        nlohmann::json::object_t item;
        item["@odata.id"] = boost::urls::format(
            "/redfish/v1/Systems/{}/FabricAdapters/{}/Ports/{}", systemName,
            adapterId, portId);
        members.emplace_back(std::move(item));
    }

    asyncResp->res.jsonValue["Members@odata.count"] = members.size();
    asyncResp->res.jsonValue["Members"] = std::move(members);
}

inline void doHandleFabricPortCollectionGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& adapterId,
    const boost::system::error_code& ec, const std::string& fabricAdapterPath,
    const std::string& serviceName)
{
    if (ec)
    {
        if (ec.value() == boost::system::errc::io_error)
        {
            messages::resourceNotFound(asyncResp->res, "FabricAdapter",
                                       adapterId);
            return;
        }

        BMCWEB_LOG_ERROR("DBus method call failed with error {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }
    if (fabricAdapterPath.empty() || serviceName.empty())
    {
        BMCWEB_LOG_WARNING("Adapter not found");
        messages::resourceNotFound(asyncResp->res, "FabricAdapter", adapterId);
        return;
    }

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/PortCollection/PortCollection.json>; rel=describedby");

    asyncResp->res.jsonValue["@odata.type"] = "#PortCollection.PortCollection";
    asyncResp->res.jsonValue["Name"] = "Fabric Port Collection";
    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Systems/{}/FabricAdapters/{}/Ports",
                            systemName, adapterId);
    asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
    asyncResp->res.jsonValue["Members@odata.count"] = 0;

    getFabricPortPaths(asyncResp, fabricAdapterPath,
                       std::bind_front(afterDoHandleFabricPortCollectionGet,
                                       asyncResp, systemName, adapterId));
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

    getValidFabricAdapterPath(
        adapterId, systemName, asyncResp,
        [asyncResp, adapterId](const boost::system::error_code& ec,
                               const std::string& fabricAdapterPath,
                               const std::string& serviceName) {
        if (ec)
        {
            if (ec.value() == boost::system::errc::io_error)
            {
                messages::resourceNotFound(asyncResp->res, "FabricAdapter",
                                           adapterId);
                return;
            }

            BMCWEB_LOG_ERROR("DBus method call failed with error {}",
                             ec.value());
            messages::internalError(asyncResp->res);
            return;
        }
        if (fabricAdapterPath.empty() || serviceName.empty())
        {
            BMCWEB_LOG_WARNING("Adapter not found");
            messages::resourceNotFound(asyncResp->res, "FabricAdapter",
                                       adapterId);
            return;
        }
        asyncResp->res.addHeader(
            boost::beast::http::field::link,
            "</redfish/v1/JsonSchemas/PortCollection/PortCollection.json>; rel=describedby");
    });
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

    getValidFabricAdapterPath(adapterId, systemName, asyncResp,
                              std::bind_front(doHandleFabricPortCollectionGet,
                                              asyncResp, systemName,
                                              adapterId));
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
