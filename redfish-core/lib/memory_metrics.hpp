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
#include <nlohmann/json.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace redfish
{

/**
 * @brief Get MemoryECC data and populate MemoryMetrics response.
 *
 * Reads ceCount and ueCount from xyz.openbmc_project.Memory.MemoryECC
 * interface and maps to Redfish MemoryMetrics LifeTime properties.
 *
 * @param[in,out] asyncResp  Async HTTP response.
 * @param[in]     service    D-Bus service name.
 * @param[in]     objPath    D-Bus object path.
 */
inline void getMemoryMetricsECCData(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& service, const std::string& objPath)
{
    BMCWEB_LOG_DEBUG("Get MemoryECC data for {}", objPath);

    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, service, objPath,
        "xyz.openbmc_project.Memory.MemoryECC",
        [asyncResp](const boost::system::error_code& ec,
                    const dbus::utility::DBusPropertiesMap& properties) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG("No MemoryECC interface found: {}",
                                 ec.message());
                return;
            }

            const int64_t* ceCount = nullptr;
            const int64_t* ueCount = nullptr;

            const bool success = sdbusplus::unpackPropertiesNoThrow(
                dbus_utils::UnpackErrorPrinter(), properties, "ceCount",
                ceCount, "ueCount", ueCount);

            if (!success)
            {
                BMCWEB_LOG_WARNING("Failed to unpack MemoryECC properties");
                return;
            }

            // Map to Redfish MemoryMetrics LifeTime properties
            nlohmann::json& lifeTime = asyncResp->res.jsonValue["LifeTime"];

            if (ceCount != nullptr)
            {
                lifeTime["CorrectableECCErrorCount"] = *ceCount;
            }

            if (ueCount != nullptr)
            {
                lifeTime["UncorrectableECCErrorCount"] = *ueCount;
            }
        });
}

/**
 * @brief Find Memory D-Bus object and get MemoryMetrics data.
 *
 * Uses ObjectMapper GetSubTree to find Memory objects with MemoryECC interface.
 *
 * @param[in,out] asyncResp  Async HTTP response.
 * @param[in]     memoryId   Memory device ID from URL.
 */
inline void getMemoryObject(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& memoryId)
{
    BMCWEB_LOG_DEBUG("Get Memory object for {}", memoryId);

    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Dimm"};

    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [asyncResp,
         memoryId](const boost::system::error_code& ec,
                   const dbus::utility::MapperGetSubTreeResponse& subtree) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("DBUS response error: {}", ec.message());
                messages::internalError(asyncResp->res);
                return;
            }

            for (const auto& [path, serviceMap] : subtree)
            {
                // Match memory ID (last component of path)
                sdbusplus::message::object_path objPath(path);
                if (objPath.filename() != memoryId)
                {
                    continue;
                }

                // Found matching memory object
                for (const auto& [service, ifaces] : serviceMap)
                {
                    // Check if service provides MemoryECC interface
                    getMemoryMetricsECCData(asyncResp, service, path);
                    return;
                }
            }

            // Memory not found
            messages::resourceNotFound(asyncResp->res, "Memory", memoryId);
        });
}

/**
 * @brief Handle MemoryMetrics GET request.
 *
 * Populates Redfish MemoryMetrics response with ECC error counts.
 *
 * @param[in,out] asyncResp   Async HTTP response.
 * @param[in]     systemName  System name from URL.
 * @param[in]     memoryId    Memory device ID from URL.
 */
inline void doMemoryMetricsGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& memoryId)
{
    // Populate standard Redfish response headers
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/MemoryMetrics/MemoryMetrics.json>; rel=describedby");

    asyncResp->res.jsonValue["@odata.type"] =
        "#MemoryMetrics.v1_7_0.MemoryMetrics";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/{}/Memory/{}/MemoryMetrics", systemName, memoryId);
    asyncResp->res.jsonValue["Id"] = "MemoryMetrics";
    asyncResp->res.jsonValue["Name"] = memoryId + " Memory Metrics";

    // Get ECC data from D-Bus
    getMemoryObject(asyncResp, memoryId);
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

    doMemoryMetricsGet(asyncResp, systemName, memoryId);
}

/**
 * @brief Register MemoryMetrics Redfish routes.
 *
 * Route: /redfish/v1/Systems/<str>/Memory/<str>/MemoryMetrics
 */
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
