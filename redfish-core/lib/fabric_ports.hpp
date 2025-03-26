// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/resource.hpp"
#include "http_request.hpp"
#include "human_sort.hpp"
#include "led.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/json_utils.hpp"

#include <asm-generic/errno.h>

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <sdbusplus/asio/property.hpp>

#include <algorithm>
#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace redfish
{
static constexpr std::array<std::string_view, 1> fabricInterfaces{
    "xyz.openbmc_project.Inventory.Item.FabricAdapter"};
static constexpr std::array<std::string_view, 1> portInterfaces{
    "xyz.openbmc_project.Inventory.Connector.Port"};

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

inline void getFabricPortLocation(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& portPath, const std::string& serviceName)
{
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, serviceName, portPath,
        "xyz.openbmc_project.Inventory.Decorator.LocationCode", "LocationCode",
        std::bind_front(afterGetFabricPortLocation, asyncResp));
}

inline void afterGetFabricPortState(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
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
        asyncResp->res.jsonValue["Status"]["State"] = resource::State::Absent;
    }
}

inline void getFabricPortState(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& portPath, const std::string& serviceName)
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
        asyncResp->res.jsonValue["Status"]["Health"] =
            resource::Health::Critical;
    }
}

inline void getFabricPortHealth(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& portPath, const std::string& serviceName)
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
    if (portPath.empty())
    {
        BMCWEB_LOG_WARNING("Port not found");
        messages::resourceNotFound(asyncResp->res, "Port", portId);
        return;
    }

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/Port/Port.json>; rel=describedby");

    asyncResp->res.jsonValue["@odata.type"] = "#Port.v1_11_0.Port";
    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Systems/{}/FabricAdapters/{}/Ports/{}",
                            systemName, adapterId, portId);
    asyncResp->res.jsonValue["Id"] = portId;
    asyncResp->res.jsonValue["Name"] = "Fabric Port";
    asyncResp->res.jsonValue["Status"]["State"] = resource::State::Enabled;
    asyncResp->res.jsonValue["Status"]["Health"] = resource::Health::OK;

    getFabricPortLocation(asyncResp, portPath, serviceName);
    getFabricPortState(asyncResp, portPath, serviceName);
    getFabricPortHealth(asyncResp, portPath, serviceName);
    getLocationIndicatorActive(asyncResp, portPath);
}

inline void afterGetValidFabricPortPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& portId,
    std::function<void(const std::string& portPath,
                       const std::string& portServiceName)>& callback,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& portSubTreePaths)
{
    if (ec)
    {
        if (ec.value() != boost::system::errc::io_error)
        {
            BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
            messages::internalError(asyncResp->res);
            return;
        }
        // Port not found
        callback(std::string(), std::string());
        return;
    }
    const auto& it =
        std::ranges::find_if(portSubTreePaths, [portId](const auto& portPath) {
            return portId ==
                   sdbusplus::message::object_path(portPath).filename();
        });
    if (it == portSubTreePaths.end())
    {
        // Port not found
        callback(std::string(), std::string());
        return;
    }

    const std::string& portPath = *it;
    dbus::utility::getDbusObject(
        portPath, portInterfaces,
        [asyncResp, portPath, callback{std::move(callback)}](
            const boost::system::error_code& ec1,
            const dbus::utility::MapperGetObject& object) {
            if (ec1 || object.empty())
            {
                BMCWEB_LOG_ERROR("DBUS response error on getDbusObject {}",
                                 ec1.value());
                messages::internalError(asyncResp->res);
                return;
            }
            callback(portPath, object.begin()->first);
        });
}

inline void getValidFabricPortPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& adapterId, const std::string& portId,
    std::function<void(const std::string& portPath,
                       const std::string& portServiceName)>&& callback)
{
    dbus::utility::getAssociatedSubTreePathsById(
        adapterId, "/xyz/openbmc_project/inventory", fabricInterfaces,
        "connecting", portInterfaces,
        std::bind_front(afterGetValidFabricPortPath, asyncResp, portId,
                        std::move(callback)));
}

inline void handleFabricPortHead(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& adapterId,
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
    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    getValidFabricPortPath(
        asyncResp, adapterId, portId,
        [asyncResp, portId](const std::string& portPath, const std::string&) {
            if (portPath.empty())
            {
                BMCWEB_LOG_WARNING("Port not found");
                messages::resourceNotFound(asyncResp->res, "Port", portId);
                return;
            }
            asyncResp->res.addHeader(
                boost::beast::http::field::link,
                "</redfish/v1/JsonSchemas/Port/Port.json>; rel=describedby");
        });
}

inline void handleFabricPortGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& adapterId,
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
    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    getValidFabricPortPath(asyncResp, adapterId, portId,
                           std::bind_front(getFabricPortProperties, asyncResp,
                                           systemName, adapterId, portId));
}

inline void afterHandleFabricPortCollectionHead(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& adapterId, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& /* portSubTreePaths */)
{
    if (ec)
    {
        if (ec.value() != boost::system::errc::io_error)
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
    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    dbus::utility::getAssociatedSubTreePathsById(
        adapterId, "/xyz/openbmc_project/inventory", fabricInterfaces,
        "connecting", portInterfaces,
        std::bind_front(afterHandleFabricPortCollectionHead, asyncResp,
                        adapterId));
}

inline void doHandleFabricPortCollectionGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& adapterId,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& portSubTreePaths)
{
    if (ec)
    {
        if (ec.value() != boost::system::errc::io_error)
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
    for (const std::string& portPath : portSubTreePaths)
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
    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    dbus::utility::getAssociatedSubTreePathsById(
        adapterId, "/xyz/openbmc_project/inventory", fabricInterfaces,
        "connecting", portInterfaces,
        std::bind_front(doHandleFabricPortCollectionGet, asyncResp, systemName,
                        adapterId));
}

inline void afterHandlePortPatch(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& portId, const bool locationIndicatorActive,
    const std::string& portPath, const std::string& /*serviceName*/)
{
    if (portPath.empty())
    {
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
    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
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
        getValidFabricPortPath(
            asyncResp, adapterId, portId,
            std::bind_front(afterHandlePortPatch, asyncResp, portId,
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
