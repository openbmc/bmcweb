// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/dbus_utils.hpp"

#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <sdbusplus/message/native_types.hpp>
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

inline void afterGetMemoryEccProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& properties)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("D-Bus response error for MemoryECC: {}",
                             ec.message());
            messages::internalError(asyncResp->res);
        }
        return;
    }

    const int64_t* ceCount = nullptr;
    const int64_t* ueCount = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "ceCount", ceCount,
        "ueCount", ueCount);

    if (!success)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    if (ceCount != nullptr)
    {
        asyncResp->res.jsonValue["LifeTime"]["CorrectableECCErrorCount"] =
            *ceCount;
    }

    if (ueCount != nullptr)
    {
        asyncResp->res.jsonValue["LifeTime"]["UncorrectableECCErrorCount"] =
            *ueCount;
    }
}

inline void afterGetMemoryMetricsSubTree(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& memoryId, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("D-Bus response error for MemoryMetrics: {}",
                             ec.message());
            messages::internalError(asyncResp->res);
            return;
        }
        messages::resourceNotFound(asyncResp->res, "MemoryMetrics", memoryId);
        return;
    }

    for (const auto& [objectPath, serviceMap] : subtree)
    {
        if (sdbusplus::message::object_path(objectPath).filename() != memoryId)
        {
            continue;
        }

        if (serviceMap.size() != 1)
        {
            BMCWEB_LOG_ERROR("Expected exactly one service for {}, got {}",
                             objectPath, serviceMap.size());
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string& service = serviceMap.begin()->first;
        dbus::utility::getAllProperties(
            service, objectPath, "xyz.openbmc_project.Memory.MemoryECC",
            std::bind_front(afterGetMemoryEccProperties, asyncResp));

        dbus::utility::getProperty<uint16_t>(
            service, objectPath, "xyz.openbmc_project.Inventory.Item.Dimm",
            "MemoryConfiguredSpeedInMhz",
            [asyncResp](const boost::system::error_code& ec2,
                        const uint16_t speed) {
                if (ec2)
                {
                    if (ec2.value() != EBADR &&
                        ec2 != boost::system::errc::io_error)
                    {
                        BMCWEB_LOG_ERROR(
                            "DBus error on GetProperty for MemoryConfiguredSpeedInMhz: {}",
                            ec2.message());
                        messages::internalError(asyncResp->res);
                    }
                    return;
                }
                asyncResp->res.jsonValue["OperatingSpeedMHz"] = speed;
            });
        return;
    }

    messages::resourceNotFound(asyncResp->res, "MemoryMetrics", memoryId);
}

inline void handleMemoryMetricsHead(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& /*memoryId*/)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
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

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/MemoryMetrics/MemoryMetrics.json>; rel=describedby");
}

inline void handleMemoryMetricsGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& memoryId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
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

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/MemoryMetrics/MemoryMetrics.json>; rel=describedby");

    asyncResp->res.jsonValue["@odata.type"] =
        "#MemoryMetrics.v1_7_0.MemoryMetrics";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/{}/Memory/{}/MemoryMetrics", systemName, memoryId);
    asyncResp->res.jsonValue["Id"] = "MemoryMetrics";
    asyncResp->res.jsonValue["Name"] =
        std::format("{} Memory Metrics", memoryId);

    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Memory.MemoryECC"};
    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        std::bind_front(afterGetMemoryMetricsSubTree, asyncResp, memoryId));
}

inline void requestRoutesMemoryMetrics(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Memory/<str>/MemoryMetrics/")
        .privileges(redfish::privileges::headMemoryMetrics)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleMemoryMetricsHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Memory/<str>/MemoryMetrics/")
        .privileges(redfish::privileges::getMemoryMetrics)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleMemoryMetricsGet, std::ref(app)));
}

} // namespace redfish
