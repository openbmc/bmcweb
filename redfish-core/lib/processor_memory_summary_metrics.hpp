// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "logging.hpp"
#include "memory_metrics.hpp"
#include "processor.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/dbus_utils.hpp"

#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdint>
#include <format>
#include <functional>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>

namespace redfish
{

inline bool isSoftDbusError(const boost::system::error_code& ec)
{
    // EBADR (interface absent) and io_error (transient) are treated as
    // "not present" rather than transport failures.
    return ec.value() == EBADR || ec == boost::system::errc::io_error;
}

inline bool validateMemoryMetricsSystem(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName)
{
    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return false;
    }
    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return false;
    }
    return true;
}

struct ProcessorMemorySummaryAccumulator
{
    std::shared_ptr<bmcweb::AsyncResp> asyncResp;
    bool hasAnyEcc = false;
    bool hasError = false;
    int64_t ceCount = 0;
    int64_t ueCount = 0;

    ~ProcessorMemorySummaryAccumulator()
    {
        if (hasError)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        if (hasAnyEcc)
        {
            asyncResp->res.jsonValue["LifeTime"]["CorrectableECCErrorCount"] =
                ceCount;
            asyncResp->res.jsonValue["LifeTime"]["UncorrectableECCErrorCount"] =
                ueCount;
        }
    }

    ProcessorMemorySummaryAccumulator(
        const std::shared_ptr<bmcweb::AsyncResp>& resp) : asyncResp(resp)
    {}
    ProcessorMemorySummaryAccumulator(
        const ProcessorMemorySummaryAccumulator&) = delete;
    ProcessorMemorySummaryAccumulator& operator=(
        const ProcessorMemorySummaryAccumulator&) = delete;
    ProcessorMemorySummaryAccumulator(ProcessorMemorySummaryAccumulator&&) =
        delete;
    ProcessorMemorySummaryAccumulator& operator=(
        ProcessorMemorySummaryAccumulator&&) = delete;
};

inline void afterGetDramEccProperties(
    const std::shared_ptr<ProcessorMemorySummaryAccumulator>& acc,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& properties)
{
    if (ec)
    {
        if (!isSoftDbusError(ec))
        {
            BMCWEB_LOG_ERROR("D-Bus response error for DRAM MemoryECC: {}",
                             ec.message());
            acc->hasError = true;
        }
        return;
    }

    const int64_t* ceCount = nullptr;
    const int64_t* ueCount = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "ceCount", ceCount,
        "ueCount", ueCount);

    if (!success || ceCount == nullptr || ueCount == nullptr)
    {
        acc->hasError = true;
        return;
    }

    acc->ceCount += *ceCount;
    acc->ueCount += *ueCount;
    acc->hasAnyEcc = true;
}

inline void afterGetDramAssociation(
    const std::shared_ptr<ProcessorMemorySummaryAccumulator>& acc,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        if (!isSoftDbusError(ec))
        {
            BMCWEB_LOG_ERROR("D-Bus error for containing association: {}",
                             ec.message());
            acc->hasError = true;
        }
        return;
    }

    if (subtree.empty())
    {
        return;
    }

    for (const auto& [dramPath, serviceMap] : subtree)
    {
        const auto svcIt = std::ranges::find_if(serviceMap, [](const auto& kv) {
            return std::ranges::find(kv.second,
                                     "xyz.openbmc_project.Memory.MemoryECC") !=
                   kv.second.end();
        });
        if (svcIt == serviceMap.end())
        {
            BMCWEB_LOG_ERROR("No MemoryECC service for DRAM path {}", dramPath);
            continue;
        }

        dbus::utility::getAllProperties(
            svcIt->first, dramPath, "xyz.openbmc_project.Memory.MemoryECC",
            std::bind_front(afterGetDramEccProperties, acc));
    }
}

inline void startDramAssociationProbe(
    const std::shared_ptr<ProcessorMemorySummaryAccumulator>& acc,
    const std::string& processorPath)
{
    constexpr std::array<std::string_view, 2> memoryInterfaces = {
        "xyz.openbmc_project.Inventory.Item.Dimm",
        "xyz.openbmc_project.Memory.MemoryECC"};

    dbus::utility::getAssociatedSubTree(
        sdbusplus::message::object_path(processorPath) / "containing",
        sdbusplus::message::object_path("/xyz/openbmc_project/inventory"), 0,
        memoryInterfaces, std::bind_front(afterGetDramAssociation, acc));
}

inline void afterGetSramEccProperties(
    const std::shared_ptr<ProcessorMemorySummaryAccumulator>& acc,
    const std::string& processorPath, const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& properties)
{
    if (ec)
    {
        if (!isSoftDbusError(ec))
        {
            BMCWEB_LOG_ERROR("D-Bus response error for SRAM MemoryECC: {}",
                             ec.message());
            acc->hasError = true;
            return;
        }
        // SRAM absent; continue to DRAM association for accumulator lifecycle.
        startDramAssociationProbe(acc, processorPath);
        return;
    }

    const int64_t* ceCount = nullptr;
    const int64_t* ueCount = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "ceCount", ceCount,
        "ueCount", ueCount);

    if (!success || ceCount == nullptr || ueCount == nullptr)
    {
        acc->hasError = true;
        return;
    }

    acc->ceCount += *ceCount;
    acc->ueCount += *ueCount;
    acc->hasAnyEcc = true;

    startDramAssociationProbe(acc, processorPath);
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
    if (!validateMemoryMetricsSystem(asyncResp, systemName))
    {
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
    if (!validateMemoryMetricsSystem(asyncResp, systemName))
    {
        return;
    }

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/MemoryMetrics/MemoryMetrics.json>; rel=describedby");

    asyncResp->res.jsonValue["@odata.type"] =
        "#MemoryMetrics.v1_7_0.MemoryMetrics";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/{}/Processors/{}/MemorySummary/MemoryMetrics",
        systemName, processorId);
    asyncResp->res.jsonValue["Id"] = "MemoryMetrics";
    asyncResp->res.jsonValue["Name"] =
        std::format("{} Memory Summary Metrics", processorId);

    getProcessorObject(
        asyncResp, processorId,
        [asyncResp,
         processorId](const std::string& processorPath,
                      const dbus::utility::MapperServiceMap& serviceMap) {
            if (serviceMap.empty())
            {
                messages::resourceNotFound(asyncResp->res, "Processor",
                                           processorId);
                return;
            }

            getMemoryCapacityUtilization(asyncResp, processorPath);

            auto acc =
                std::make_shared<ProcessorMemorySummaryAccumulator>(asyncResp);

            // Iterate to find the service that exposes MemoryECC. If none
            // does, fall through to the DRAM-only probe (processor present
            // but without SRAM ECC).
            const auto svcIt =
                std::ranges::find_if(serviceMap, [](const auto& kv) {
                    return std::ranges::find(
                               kv.second,
                               "xyz.openbmc_project.Memory.MemoryECC") !=
                           kv.second.end();
                });
            if (svcIt == serviceMap.end())
            {
                startDramAssociationProbe(acc, processorPath);
                return;
            }

            dbus::utility::getAllProperties(
                svcIt->first, processorPath,
                "xyz.openbmc_project.Memory.MemoryECC",
                std::bind_front(afterGetSramEccProperties, acc, processorPath));
        });
}

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
