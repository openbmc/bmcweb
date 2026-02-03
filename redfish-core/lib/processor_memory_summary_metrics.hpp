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
 * @brief Get DRAM ECC data from Memory object and combine with SRAM ECC.
 *
 * Reads ceCount and ueCount from the Memory object's MemoryECC interface
 * and adds them to the SRAM counts to produce the MemorySummary totals.
 *
 * @param[in,out] asyncResp      Async HTTP response.
 * @param[in]     service        D-Bus service name.
 * @param[in]     memoryPath     D-Bus path of Memory object.
 * @param[in]     sramCeCount    SRAM correctable error count.
 * @param[in]     sramUeCount    SRAM uncorrectable error count.
 */
inline void getProcessorMemorySummaryDramEcc(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& service, const std::string& memoryPath,
    int64_t sramCeCount, int64_t sramUeCount)
{
    BMCWEB_LOG_DEBUG("Get DRAM ECC data from {}", memoryPath);

    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, service, memoryPath,
        "xyz.openbmc_project.Memory.MemoryECC",
        [asyncResp, sramCeCount,
         sramUeCount](const boost::system::error_code& ec,
                      const dbus::utility::DBusPropertiesMap& properties) {
            int64_t dramCeCount = 0;
            int64_t dramUeCount = 0;

            if (!ec)
            {
                const int64_t* ceCount = nullptr;
                const int64_t* ueCount = nullptr;

                const bool success = sdbusplus::unpackPropertiesNoThrow(
                    dbus_utils::UnpackErrorPrinter(), properties, "ceCount",
                    ceCount, "ueCount", ueCount);

                if (success)
                {
                    if (ceCount != nullptr)
                    {
                        dramCeCount = *ceCount;
                    }
                    if (ueCount != nullptr)
                    {
                        dramUeCount = *ueCount;
                    }
                }
            }

            // Combine SRAM and DRAM counts for MemorySummary
            nlohmann::json& lifeTime = asyncResp->res.jsonValue["LifeTime"];
            lifeTime["CorrectableECCErrorCount"] = sramCeCount + dramCeCount;
            lifeTime["UncorrectableECCErrorCount"] = sramUeCount + dramUeCount;
        });
}

/**
 * @brief Find Memory object associated with processor and get DRAM ECC.
 *
 * Searches for Memory objects (Item.Dimm) that match the processor's
 * DRAM naming convention ({processorId}_DRAM_0).
 *
 * @param[in,out] asyncResp      Async HTTP response.
 * @param[in]     processorId    Processor ID from URL.
 * @param[in]     sramCeCount    SRAM correctable error count.
 * @param[in]     sramUeCount    SRAM uncorrectable error count.
 */
inline void findProcessorMemoryObject(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& processorId, int64_t sramCeCount, int64_t sramUeCount)
{
    BMCWEB_LOG_DEBUG("Find Memory object for processor {}", processorId);

    // Expected DRAM object name: {processorId}_DRAM_0
    std::string expectedDramId = processorId + "_DRAM_0";

    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Dimm"};

    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [asyncResp, expectedDramId, sramCeCount, sramUeCount](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreeResponse& subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG("No Memory objects found: {}", ec.message());
                // Still return SRAM counts even if no DRAM found
                nlohmann::json& lifeTime =
                    asyncResp->res.jsonValue["LifeTime"];
                lifeTime["CorrectableECCErrorCount"] = sramCeCount;
                lifeTime["UncorrectableECCErrorCount"] = sramUeCount;
                return;
            }

            for (const auto& [path, serviceMap] : subtree)
            {
                sdbusplus::message::object_path objPath(path);
                if (objPath.filename() != expectedDramId)
                {
                    continue;
                }

                // Found matching DRAM object
                for (const auto& [service, ifaces] : serviceMap)
                {
                    getProcessorMemorySummaryDramEcc(asyncResp, service, path,
                                                     sramCeCount, sramUeCount);
                    return;
                }
            }

            // DRAM not found - return SRAM counts only
            BMCWEB_LOG_DEBUG("DRAM object {} not found", expectedDramId);
            nlohmann::json& lifeTime = asyncResp->res.jsonValue["LifeTime"];
            lifeTime["CorrectableECCErrorCount"] = sramCeCount;
            lifeTime["UncorrectableECCErrorCount"] = sramUeCount;
        });
}

/**
 * @brief Get SRAM ECC data from processor object.
 *
 * Reads ceCount and ueCount from the Processor object's MemoryECC interface
 * (SRAM ECC), then finds the associated Memory object for DRAM ECC.
 *
 * @param[in,out] asyncResp      Async HTTP response.
 * @param[in]     service        D-Bus service name.
 * @param[in]     processorPath  D-Bus path of Processor object.
 * @param[in]     processorId    Processor ID from URL.
 */
inline void getProcessorMemorySummarySramEcc(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& service, const std::string& processorPath,
    const std::string& processorId)
{
    BMCWEB_LOG_DEBUG("Get SRAM ECC data from {}", processorPath);

    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, service, processorPath,
        "xyz.openbmc_project.Memory.MemoryECC",
        [asyncResp, processorId](
            const boost::system::error_code& ec,
            const dbus::utility::DBusPropertiesMap& properties) {
            int64_t sramCeCount = 0;
            int64_t sramUeCount = 0;

            if (!ec)
            {
                const int64_t* ceCount = nullptr;
                const int64_t* ueCount = nullptr;

                const bool success = sdbusplus::unpackPropertiesNoThrow(
                    dbus_utils::UnpackErrorPrinter(), properties, "ceCount",
                    ceCount, "ueCount", ueCount);

                if (success)
                {
                    if (ceCount != nullptr)
                    {
                        sramCeCount = *ceCount;
                    }
                    if (ueCount != nullptr)
                    {
                        sramUeCount = *ueCount;
                    }
                }
            }

            // Now find Memory object and get DRAM ECC
            findProcessorMemoryObject(asyncResp, processorId, sramCeCount,
                                      sramUeCount);
        });
}

/**
 * @brief Find processor object and initiate ECC data collection.
 *
 * @param[in,out] asyncResp      Async HTTP response.
 * @param[in]     processorId    Processor ID from URL.
 */
inline void getProcessorMemorySummaryMetricsData(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& processorId)
{
    BMCWEB_LOG_DEBUG("Get MemorySummary Metrics for processor {}", processorId);

    constexpr std::array<std::string_view, 2> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Accelerator",
        "xyz.openbmc_project.Inventory.Item.Cpu"};

    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [asyncResp,
         processorId](const boost::system::error_code& ec,
                      const dbus::utility::MapperGetSubTreeResponse& subtree) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("DBUS response error: {}", ec.message());
                messages::internalError(asyncResp->res);
                return;
            }

            for (const auto& [path, serviceMap] : subtree)
            {
                sdbusplus::message::object_path objPath(path);
                if (objPath.filename() != processorId)
                {
                    continue;
                }

                // Found matching processor
                for (const auto& [service, ifaces] : serviceMap)
                {
                    getProcessorMemorySummarySramEcc(asyncResp, service, path,
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
 * @brief Handle MemorySummary/MemoryMetrics GET request.
 *
 * @param[in,out] asyncResp      Async HTTP response.
 * @param[in]     systemName     System name from URL.
 * @param[in]     processorId    Processor ID from URL.
 */
inline void doProcessorMemorySummaryMetricsGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& processorId)
{
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/MemoryMetrics/MemoryMetrics.json>; rel=describedby");

    asyncResp->res.jsonValue["@odata.type"] =
        "#MemoryMetrics.v1_7_0.MemoryMetrics";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/{}/Processors/{}/MemorySummary/MemoryMetrics",
        systemName, processorId);
    asyncResp->res.jsonValue["Id"] = "MemoryMetrics";
    asyncResp->res.jsonValue["Name"] = processorId + " Memory Summary Metrics";

    getProcessorMemorySummaryMetricsData(asyncResp, processorId);
}

inline void handleProcessorMemorySummaryMetricsHead(
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

inline void handleProcessorMemorySummaryMetricsGet(
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

    doProcessorMemorySummaryMetricsGet(asyncResp, systemName, processorId);
}

/**
 * @brief Register MemorySummary/MemoryMetrics Redfish routes.
 *
 * Route: /redfish/v1/Systems/<str>/Processors/<str>/MemorySummary/MemoryMetrics
 */
inline void requestRoutesProcessorMemorySummaryMetrics(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/<str>/Processors/<str>/MemorySummary/MemoryMetrics/")
        .privileges(redfish::privileges::headMemoryMetrics)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleProcessorMemorySummaryMetricsHead,
                            std::ref(app)));

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/<str>/Processors/<str>/MemorySummary/MemoryMetrics/")
        .privileges(redfish::privileges::getMemoryMetrics)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleProcessorMemorySummaryMetricsGet,
                            std::ref(app)));
}

} // namespace redfish
