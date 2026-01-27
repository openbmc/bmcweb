// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "processor.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/dbus_utils.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace redfish
{

/**
 * @brief Get processor memory ECC data for ProcessorMetrics
 *
 * @param[in,out]   asyncResp   Async HTTP response.
 * @param[in]       service     D-Bus service to query.
 * @param[in]       objPath     D-Bus object path.
 */
inline void getProcessorMetricsECCData(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& service, const std::string& objPath)
{
    BMCWEB_LOG_DEBUG("Get processor metrics ECC data for {}", objPath);

    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, service, objPath,
        "xyz.openbmc_project.Memory.MemoryECC",
        [asyncResp](const boost::system::error_code& ec,
                    const dbus::utility::DBusPropertiesMap& properties) {
            if (ec)
            {
                // Non-fatal error - MemoryECC interface may not exist
                // for non-ECC GPUs or CPUs
                BMCWEB_LOG_DEBUG("Failed to get MemoryECC properties: {}",
                                 ec.message());
                return;
            }

            // Parse properties
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

            // Populate CacheMetricsTotal.LifeTime ECC error counts
            nlohmann::json& cacheMetrics =
                asyncResp->res.jsonValue["CacheMetricsTotal"];
            nlohmann::json& lifeTime = cacheMetrics["LifeTime"];

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
 * @brief Populate ProcessorMetrics with ECC data from the processor's
 *        service map
 *
 * @param[in,out]   asyncResp   Async HTTP response.
 * @param[in]       objectPath  D-Bus object path of the processor.
 * @param[in]       serviceMap  Map of services and their interfaces.
 */
inline void getProcessorMetricsData(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& objectPath,
    const dbus::utility::MapperServiceMap& serviceMap)
{
    // Find service that provides MemoryECC interface
    for (const auto& [serviceName, interfaceList] : serviceMap)
    {
        for (const auto& interface : interfaceList)
        {
            if (interface == "xyz.openbmc_project.Memory.MemoryECC")
            {
                getProcessorMetricsECCData(asyncResp, serviceName, objectPath);
                return;
            }
        }
    }
    // No MemoryECC interface found - this is OK, ProcessorMetrics
    // will exist but without ECC data
    BMCWEB_LOG_DEBUG("No MemoryECC interface found for {}", objectPath);
}

/**
 * @brief Main handler for ProcessorMetrics GET request
 *
 * @param[in,out]   asyncResp       Async HTTP response.
 * @param[in]       systemName      System name.
 * @param[in]       processorId     Processor ID.
 */
inline void doProcessorMetricsGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& processorId)
{
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/ProcessorMetrics/ProcessorMetrics.json>; rel=describedby");
    asyncResp->res.jsonValue["@odata.type"] =
        "#ProcessorMetrics.v1_7_0.ProcessorMetrics";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/{}/Processors/{}/ProcessorMetrics", systemName,
        processorId);
    asyncResp->res.jsonValue["Id"] = "ProcessorMetrics";
    asyncResp->res.jsonValue["Name"] = "Processor Metrics";

    // Reuse getProcessorObject from processor.hpp to find the processor
    // and get its service map, then query for ECC data
    getProcessorObject(
        asyncResp, processorId,
        [asyncResp](const std::string& objectPath,
                    const dbus::utility::MapperServiceMap& serviceMap) {
            getProcessorMetricsData(asyncResp, objectPath, serviceMap);
        });
}

inline void handleProcessorMetricsHead(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& /*processorId*/)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        // Option currently returns no systems. TBD
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
        "</redfish/v1/JsonSchemas/ProcessorMetrics/ProcessorMetrics.json>; rel=describedby");
}

inline void handleProcessorMetricsGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& processorId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        // Option currently returns no systems. TBD
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

    doProcessorMetricsGet(asyncResp, systemName, processorId);
}

inline void requestRoutesProcessorMetrics(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/<str>/Processors/<str>/ProcessorMetrics/")
        .privileges(redfish::privileges::headProcessorMetrics)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleProcessorMetricsHead, std::ref(app)));

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/<str>/Processors/<str>/ProcessorMetrics/")
        .privileges(redfish::privileges::getProcessorMetrics)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleProcessorMetricsGet, std::ref(app)));
}

} // namespace redfish
