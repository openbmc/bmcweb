// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "logging.hpp"
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

#include <array>
#include <cerrno>
#include <cstdint>
#include <format>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace redfish
{

struct ProcessorMemorySummaryAccumulator
{
    std::shared_ptr<bmcweb::AsyncResp> asyncResp;
    bool sramPresent = false;
    bool dramEndpointsFound = false;
    bool dramAllSuccess = true;
    bool hasError = false;
    int64_t sramCe = 0;
    int64_t sramUe = 0;
    int64_t dramCe = 0;
    int64_t dramUe = 0;

    ~ProcessorMemorySummaryAccumulator()
    {
        if (hasError)
        {
            asyncResp->res.jsonValue = nlohmann::json::object();
            messages::internalError(asyncResp->res);
            return;
        }
        const bool dramPresent = dramEndpointsFound && dramAllSuccess;
        if (sramPresent && dramPresent)
        {
            asyncResp->res.jsonValue["LifeTime"]["CorrectableECCErrorCount"] =
                sramCe + dramCe;
            asyncResp->res.jsonValue["LifeTime"]["UncorrectableECCErrorCount"] =
                sramUe + dramUe;
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
        if (ec.value() != EBADR && ec != boost::system::errc::io_error)
        {
            BMCWEB_LOG_ERROR("D-Bus response error for DRAM MemoryECC: {}",
                             ec.message());
            acc->hasError = true;
            return;
        }
        acc->dramAllSuccess = false;
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

    acc->dramCe += *ceCount;
    acc->dramUe += *ueCount;
}

inline void afterGetDramEccService(
    const std::shared_ptr<ProcessorMemorySummaryAccumulator>& acc,
    const std::string& dramPath, const boost::system::error_code& ec,
    const dbus::utility::MapperGetObject& object)
{
    if (ec)
    {
        if (ec.value() != EBADR && ec != boost::system::errc::io_error)
        {
            BMCWEB_LOG_ERROR("D-Bus error for DRAM MemoryECC service: {}",
                             ec.message());
            acc->hasError = true;
            return;
        }
        acc->dramAllSuccess = false;
        return;
    }

    if (object.size() != 1)
    {
        BMCWEB_LOG_ERROR("Expected exactly one service for DRAM {}, got {}",
                         dramPath, object.size());
        acc->hasError = true;
        return;
    }

    const std::string& service = object.begin()->first;
    dbus::utility::getAllProperties(
        service, dramPath, "xyz.openbmc_project.Memory.MemoryECC",
        std::bind_front(afterGetDramEccProperties, acc));
}

inline void afterGetDramAssociation(
    const std::shared_ptr<ProcessorMemorySummaryAccumulator>& acc,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& endpoints)
{
    if (ec)
    {
        if (ec.value() != EBADR && ec != boost::system::errc::io_error)
        {
            BMCWEB_LOG_ERROR("D-Bus error for containing association: {}",
                             ec.message());
            acc->hasError = true;
            return;
        }
        return;
    }

    if (endpoints.empty())
    {
        return;
    }

    acc->dramEndpointsFound = true;
    constexpr std::array<std::string_view, 1> eccInterface = {
        "xyz.openbmc_project.Memory.MemoryECC"};

    for (const std::string& dramPath : endpoints)
    {
        dbus::utility::getDbusObject(
            dramPath, eccInterface,
            std::bind_front(afterGetDramEccService, acc, dramPath));
    }
}

inline void afterGetSramEccProperties(
    const std::shared_ptr<ProcessorMemorySummaryAccumulator>& acc,
    const std::string& processorPath, const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& properties)
{
    if (ec)
    {
        if (ec.value() != EBADR && ec != boost::system::errc::io_error)
        {
            BMCWEB_LOG_ERROR("D-Bus response error for SRAM MemoryECC: {}",
                             ec.message());
            acc->hasError = true;
            return;
        }
        // SRAM absent; continue to DRAM association for accumulator lifecycle.
    }
    else
    {
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

        acc->sramCe = *ceCount;
        acc->sramUe = *ueCount;
        acc->sramPresent = true;
    }

    constexpr std::array<std::string_view, 2> memoryInterfaces = {
        "xyz.openbmc_project.Inventory.Item.Dimm",
        "xyz.openbmc_project.Inventory.Item.Memory"};

    dbus::utility::getAssociatedSubTreePaths(
        sdbusplus::message::object_path(processorPath) / "containing",
        sdbusplus::message::object_path("/xyz/openbmc_project/inventory"), 0,
        memoryInterfaces, std::bind_front(afterGetDramAssociation, acc));
}

inline void afterGetSramEccService(
    const std::shared_ptr<ProcessorMemorySummaryAccumulator>& acc,
    const std::string& processorPath, const boost::system::error_code& ec,
    const dbus::utility::MapperGetObject& object)
{
    if (ec)
    {
        if (ec.value() != EBADR && ec != boost::system::errc::io_error)
        {
            BMCWEB_LOG_ERROR("D-Bus error for SRAM MemoryECC service: {}",
                             ec.message());
            acc->hasError = true;
            return;
        }
        // SRAM MemoryECC not present; still probe DRAM via association.
        afterGetSramEccProperties(acc, processorPath, ec, {});
        return;
    }

    if (object.size() != 1)
    {
        BMCWEB_LOG_ERROR("Expected exactly one service for SRAM on {}, got {}",
                         processorPath, object.size());
        acc->hasError = true;
        return;
    }

    const std::string& service = object.begin()->first;
    dbus::utility::getAllProperties(
        service, processorPath, "xyz.openbmc_project.Memory.MemoryECC",
        std::bind_front(afterGetSramEccProperties, acc, processorPath));
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
        [asyncResp](const std::string& processorPath,
                    const dbus::utility::MapperServiceMap& /*serviceMap*/) {
            auto acc =
                std::make_shared<ProcessorMemorySummaryAccumulator>(asyncResp);

            constexpr std::array<std::string_view, 1> eccInterface = {
                "xyz.openbmc_project.Memory.MemoryECC"};
            dbus::utility::getDbusObject(
                processorPath, eccInterface,
                std::bind_front(afterGetSramEccService, acc, processorPath));
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
