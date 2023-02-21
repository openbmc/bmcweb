#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "dbus_utility.hpp"
#include "fabric_adapters.hpp"
#include "human_sort.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/port_utils.hpp"

#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace redfish
{

inline bool checkPortId(const std::string& portPath, const std::string& portId)
{
    std::string portName = sdbusplus::message::object_path(portPath).filename();
    return !(portName.empty() || portName != portId);
}

inline void
    getPortProperties(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& systemName,
                      const std::string& adapterId, const std::string& portId,
                      const std::string& /*portPath*/)
{
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/Port/Port.json>; rel=describedby");

    asyncResp->res.jsonValue["@odata.type"] = "#Port.v1_7_0.Port";
    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Systems/{}/FabricAdapters/{}/Ports/{}",
                            systemName, adapterId, portId);
    asyncResp->res.jsonValue["Id"] = portId;
    asyncResp->res.jsonValue["Name"] = portId;
}

inline void getValidPortPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& fabricAdapterPath, const std::string& portId,
    std::function<void(const std::string& portPath)>&& callback)
{
    redfish::port_utils::getPortList(
        asyncResp, fabricAdapterPath,
        [asyncResp, portId, callback{std::move(callback)}](
            const dbus::utility::MapperEndPoints& endpoints) {
        for (const auto& portPath : endpoints)
        {
            if (checkPortId(portPath, portId))
            {
                callback(portPath);
                return;
            }
        }
        BMCWEB_LOG_WARNING("Port {} not found", portId);
        messages::resourceNotFound(asyncResp->res, "Port", portId);
    });
}

inline void handlePortHead(crow::App& app, const crow::Request& req,
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

    getValidFabricAdapterPath(
        adapterId, systemName, asyncResp,
        [asyncResp, portId](const std::string& adapterPath,
                            const std::string&) {
        getValidPortPath(asyncResp, adapterPath, portId,
                         [asyncResp](const std::string&) {
            asyncResp->res.addHeader(
                boost::beast::http::field::link,
                "</redfish/v1/JsonSchemas/Port/Port.json>; rel=describedby");
        });
    });
}

inline void doHandlePortGet(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& systemName,
                            const std::string& adapterId,
                            const std::string& portId,
                            const std::string& adapterPath,
                            const std::string& /*serviceName*/)
{
    getValidPortPath(asyncResp, adapterPath, portId,
                     std::bind_front(getPortProperties, asyncResp, systemName,
                                     adapterId, portId));
}

inline void handlePortGet(App& app, const crow::Request& req,
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

    getValidFabricAdapterPath(adapterId, systemName, asyncResp,
                              std::bind_front(doHandlePortGet, asyncResp,
                                              systemName, adapterId, portId));
}

inline void
    doGetPortCollection(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& systemName,
                        const std::string& adapterId,
                        const dbus::utility::MapperEndPoints& portPaths)
{
    BMCWEB_LOG_DEBUG("Get Port collection members for: {}", adapterId);

    std::vector<std::string> portIdNames;
    for (const auto& path : portPaths)
    {
        std::string portId = sdbusplus::message::object_path(path).filename();
        if (!portId.empty())
        {
            portIdNames.emplace_back(std::move(portId));
        }
    }

    std::sort(portIdNames.begin(), portIdNames.end(),
              AlphanumLess<std::string>());

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

inline void afterHandlePortCollectionGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& adapterId, const std::string& fabricAdapterPath,
    const std::string& systemName)
{
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/PortCollection/PortCollection.json>; rel=describedby");
    asyncResp->res.jsonValue["@odata.type"] = "#PortCollection.PortCollection";
    asyncResp->res.jsonValue["Name"] = "Port Collection";
    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Systems/{}/FabricAdapters/{}/Ports",
                            systemName, adapterId);
    asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
    asyncResp->res.jsonValue["Members@odata.count"] = 0;

    redfish::port_utils::getPortList(
        asyncResp, fabricAdapterPath,
        std::bind_front(doGetPortCollection, asyncResp, systemName, adapterId));
}

inline void handlePortCollectionHead(
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
        [asyncResp](const std::string&, const std::string&) {
        asyncResp->res.addHeader(
            boost::beast::http::field::link,
            "</redfish/v1/JsonSchemas/PortCollection/PortCollection.json>; rel=describedby");
    });
}

inline void
    handlePortCollectionGet(App& app, const crow::Request& req,
                            const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& systemName,
                            const std::string& adapterId)
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
        std::bind_front(afterHandlePortCollectionGet, asyncResp, adapterId));
}

inline void requestRoutesPort(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/<str>/FabricAdapters/<str>/Ports/<str>/")
        .privileges(redfish::privileges::headPort)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handlePortHead, std::ref(app)));

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/<str>/FabricAdapters/<str>/Ports/<str>/")
        .privileges(redfish::privileges::getPort)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handlePortGet, std::ref(app)));
}

inline void requestRoutesPortCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/FabricAdapters/<str>/Ports/")
        .privileges(redfish::privileges::headPortCollection)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handlePortCollectionHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/FabricAdapters/<str>/Ports/")
        .privileges(redfish::privileges::getPortCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handlePortCollectionGet, std::ref(app)));
}

} // namespace redfish
