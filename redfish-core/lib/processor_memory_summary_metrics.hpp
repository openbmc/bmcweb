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
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace redfish
{

/**
 * @brief Process DRAM ECC properties and combine with SRAM ECC.
 *
 * @param[in,out] asyncResp      Async HTTP response.
 * @param[in]     sramCeCount    SRAM correctable error count.
 * @param[in]     sramUeCount    SRAM uncorrectable error count.
 * @param[in]     ec             Error code from D-Bus call.
 * @param[in]     properties     D-Bus properties map.
 */
inline void afterGetDramEccProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, int64_t sramCeCount,
    int64_t sramUeCount, const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& properties)
{
    int64_t dramCeCount = 0;
    int64_t dramUeCount = 0;

    if (!ec)
    {
        const int64_t* ceCount = nullptr;
        const int64_t* ueCount = nullptr;

        const bool success = sdbusplus::unpackPropertiesNoThrow(
            dbus_utils::UnpackErrorPrinter(), properties, "ceCount", ceCount,
            "ueCount", ueCount);

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
    asyncResp->res.jsonValue["LifeTime"]["CorrectableECCErrorCount"] =
        sramCeCount + dramCeCount;
    asyncResp->res.jsonValue["LifeTime"]["UncorrectableECCErrorCount"] =
        sramUeCount + dramUeCount;
}

/**
 * @brief Get DRAM ECC data from Memory object and combine with SRAM ECC.
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

    dbus::utility::getAllProperties(
        service, memoryPath, "xyz.openbmc_project.Memory.MemoryECC",
        std::bind_front(afterGetDramEccProperties, asyncResp, sramCeCount,
                        sramUeCount));
}

/**
 * @brief Set SRAM-only ECC counts in response.
 *
 * @param[in,out] asyncResp      Async HTTP response.
 * @param[in]     sramCeCount    SRAM correctable error count.
 * @param[in]     sramUeCount    SRAM uncorrectable error count.
 */
inline void setSramOnlyEccCounts(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, int64_t sramCeCount,
    int64_t sramUeCount)
{
    asyncResp->res.jsonValue["LifeTime"]["CorrectableECCErrorCount"] =
        sramCeCount;
    asyncResp->res.jsonValue["LifeTime"]["UncorrectableECCErrorCount"] =
        sramUeCount;
}

/**
 * @brief Process Association endpoints and get DRAM ECC.
 *
 * @param[in,out] asyncResp      Async HTTP response.
 * @param[in]     service        D-Bus service name.
 * @param[in]     sramCeCount    SRAM correctable error count.
 * @param[in]     sramUeCount    SRAM uncorrectable error count.
 * @param[in]     ec             Error code from D-Bus call.
 * @param[in]     endpoints      Association endpoints.
 */
inline void afterGetAssociationEndpoints(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& service, int64_t sramCeCount, int64_t sramUeCount,
    const boost::system::error_code& ec,
    const dbus::utility::MapperEndPoints& endpoints)
{
    if (ec)
    {
        BMCWEB_LOG_DEBUG("No all_memory association found: {}", ec.message());
        setSramOnlyEccCounts(asyncResp, sramCeCount, sramUeCount);
        return;
    }

    if (endpoints.empty())
    {
        BMCWEB_LOG_DEBUG("No Memory objects in association");
        setSramOnlyEccCounts(asyncResp, sramCeCount, sramUeCount);
        return;
    }

    // Use the first associated Memory object for DRAM ECC
    const std::string& memoryPath = endpoints.front();
    BMCWEB_LOG_DEBUG("Found associated Memory: {}", memoryPath);

    getProcessorMemorySummaryDramEcc(asyncResp, service, memoryPath,
                                     sramCeCount, sramUeCount);
}

/**
 * @brief Find Memory object associated with processor via Association.
 *
 * @param[in,out] asyncResp       Async HTTP response.
 * @param[in]     service         D-Bus service name.
 * @param[in]     processorPath   D-Bus path of Processor object.
 * @param[in]     sramCeCount     SRAM correctable error count.
 * @param[in]     sramUeCount     SRAM uncorrectable error count.
 */
inline void findProcessorMemoryObject(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& service, const std::string& processorPath,
    int64_t sramCeCount, int64_t sramUeCount)
{
    BMCWEB_LOG_DEBUG("Find Memory objects via Association for {}",
                     processorPath);

    dbus::utility::getAssociationEndPoints(
        processorPath + "/all_memory",
        std::bind_front(afterGetAssociationEndpoints, asyncResp, service,
                        sramCeCount, sramUeCount));
}

/**
 * @brief Process SRAM ECC properties and find DRAM.
 *
 * @param[in,out] asyncResp      Async HTTP response.
 * @param[in]     service        D-Bus service name.
 * @param[in]     processorPath  D-Bus path of Processor object.
 * @param[in]     ec             Error code from D-Bus call.
 * @param[in]     properties     D-Bus properties map.
 */
inline void afterGetSramEccProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& service, const std::string& processorPath,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& properties)
{
    int64_t sramCeCount = 0;
    int64_t sramUeCount = 0;

    if (!ec)
    {
        const int64_t* ceCount = nullptr;
        const int64_t* ueCount = nullptr;

        const bool success = sdbusplus::unpackPropertiesNoThrow(
            dbus_utils::UnpackErrorPrinter(), properties, "ceCount", ceCount,
            "ueCount", ueCount);

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

    // Find Memory object via Association and get DRAM ECC
    findProcessorMemoryObject(asyncResp, service, processorPath, sramCeCount,
                              sramUeCount);
}

/**
 * @brief Get SRAM ECC data from processor object.
 *
 * @param[in,out] asyncResp      Async HTTP response.
 * @param[in]     service        D-Bus service name.
 * @param[in]     processorPath  D-Bus path of Processor object.
 */
inline void getProcessorMemorySummarySramEcc(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& service, const std::string& processorPath)
{
    BMCWEB_LOG_DEBUG("Get SRAM ECC data from {}", processorPath);

    dbus::utility::getAllProperties(
        service, processorPath, "xyz.openbmc_project.Memory.MemoryECC",
        std::bind_front(afterGetSramEccProperties, asyncResp, service,
                        processorPath));
}

/**
 * @brief Process GetSubTree response and get ECC data.
 *
 * @param[in,out] asyncResp      Async HTTP response.
 * @param[in]     processorId    Processor ID from URL.
 * @param[in]     ec             Error code from D-Bus call.
 * @param[in]     subtree        GetSubTree response.
 */
inline void afterGetProcessorSubTree(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& processorId, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
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
            getProcessorMemorySummarySramEcc(asyncResp, service, path);
            return;
        }
    }

    // Processor not found
    messages::resourceNotFound(asyncResp->res, "Processor", processorId);
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
        std::bind_front(afterGetProcessorSubTree, asyncResp, processorId));
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
        .methods(boost::beast::http::verb::head)(std::bind_front(
            handleProcessorMemorySummaryMetricsHead, std::ref(app)));

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/<str>/Processors/<str>/MemorySummary/MemoryMetrics/")
        .privileges(redfish::privileges::getMemoryMetrics)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleProcessorMemorySummaryMetricsGet, std::ref(app)));
}

} // namespace redfish
