// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <functional>
#include <memory>
#include <optional>
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
 * @brief Get processor object from inventory for ProcessorMetrics
 *
 * @param[in,out]   asyncResp       Async HTTP response.
 * @param[in]       processorId     Processor ID.
 * @param[in]       systemName      System name.
 */
inline void getProcessorObjectForMetrics(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& processorId, const std::string& systemName)
{
    BMCWEB_LOG_DEBUG("Get processor object for metrics: {}", processorId);

    // Use same interfaces as processor.hpp, plus MemoryECC for metrics
    constexpr std::array<std::string_view, 10> interfaces = {
        "xyz.openbmc_project.Common.UUID",
        "xyz.openbmc_project.Inventory.Decorator.Asset",
        "xyz.openbmc_project.Inventory.Decorator.Revision",
        "xyz.openbmc_project.Inventory.Item.Cpu",
        "xyz.openbmc_project.Inventory.Decorator.LocationCode",
        "xyz.openbmc_project.Inventory.Item.Accelerator",
        "xyz.openbmc_project.Control.Processor.CurrentOperatingConfig",
        "xyz.openbmc_project.Inventory.Decorator.UniqueIdentifier",
        "xyz.openbmc_project.Control.Power.Throttle",
        "xyz.openbmc_project.Memory.MemoryECC"};
    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [asyncResp, processorId,
         systemName](const boost::system::error_code& ec,
                     const dbus::utility::MapperGetSubTreeResponse& subtree) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("DBUS response error for getSubTree: {}",
                                 ec.message());
                messages::internalError(asyncResp->res);
                return;
            }

            // Find the processor object
            for (const auto& [objectPath, serviceMap] : subtree)
            {
                std::string objectName =
                    sdbusplus::message::object_path(objectPath).filename();
                if (objectName != processorId)
                {
                    continue;
                }

                // Check if this object has processor interfaces
                bool isProcessor = false;
                for (const auto& [serviceName, interfaceList] : serviceMap)
                {
                    for (const auto& interface : interfaceList)
                    {
                        if (interface ==
                                "xyz.openbmc_project.Inventory.Item.Cpu" ||
                            interface ==
                                "xyz.openbmc_project.Inventory.Item.Accelerator")
                        {
                            isProcessor = true;
                            break;
                        }
                    }
                    if (isProcessor)
                    {
                        break;
                    }
                }

                if (isProcessor)
                {
                    // Found the processor, now find service with MemoryECC
                    for (const auto& [serviceName, interfaceList] : serviceMap)
                    {
                        for (const auto& interface : interfaceList)
                        {
                            if (interface ==
                                "xyz.openbmc_project.Memory.MemoryECC")
                            {
                                // Found MemoryECC interface, get ECC data
                                getProcessorMetricsECCData(
                                    asyncResp, serviceName, objectPath);
                                return;
                            }
                        }
                    }
                    // Processor found but no ECC interface - this is OK
                    // ProcessorMetrics will exist but without ECC data
                    BMCWEB_LOG_DEBUG(
                        "Processor {} found but no MemoryECC interface",
                        processorId);
                    return;
                }
            }

            // Processor not found
            messages::resourceNotFound(asyncResp->res, "Processor",
                                       processorId);
        });
}

/**
 * @brief Main handler for ProcessorMetrics
 *
 * @param[in,out]   asyncResp       Async HTTP response.
 * @param[in]       systemName      System name.
 * @param[in]       processorId     Processor ID.
 */
inline void doProcessorMetrics(
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

    // Get ECC data
    getProcessorObjectForMetrics(asyncResp, processorId, systemName);
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

    doProcessorMetrics(asyncResp, systemName, processorId);
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
