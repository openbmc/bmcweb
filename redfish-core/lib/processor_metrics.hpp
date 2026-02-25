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

#include <array>
#include <cmath>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace redfish
{

/**
 * @brief Callback handler for MemoryECC properties response
 *
 * @param[in,out]   asyncResp   Async HTTP response.
 * @param[in]       ec          Error code from D-Bus call.
 * @param[in]       properties  D-Bus properties map.
 */
inline void afterGetProcessorMetricsECCData(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& properties)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBUS response error: {}", ec.message());
        messages::internalError(asyncResp->res);
        return;
    }

    const int64_t* ceCount = nullptr;
    const int64_t* ueCount = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "ceCount", ceCount,
        "ueCount", ueCount);

    if (!success)
    {
        BMCWEB_LOG_ERROR("Failed to unpack MemoryECC properties");
        messages::internalError(asyncResp->res);
        return;
    }

    if (ceCount == nullptr || ueCount == nullptr)
    {
        BMCWEB_LOG_WARNING("MemoryECC properties not fully populated");
    }

    if (ceCount != nullptr)
    {
        asyncResp->res.jsonValue["CacheMetricsTotal"]["LifeTime"]
                                ["CorrectableECCErrorCount"] = *ceCount;
    }

    if (ueCount != nullptr)
    {
        asyncResp->res.jsonValue["CacheMetricsTotal"]["LifeTime"]
                                ["UncorrectableECCErrorCount"] = *ueCount;
    }
}

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

    dbus::utility::getAllProperties(
        service, objPath, "xyz.openbmc_project.Memory.MemoryECC",
        std::bind_front(afterGetProcessorMetricsECCData, asyncResp));
}

inline void afterGetOperatingFrequency(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec, double value)
{
    if (ec)
    {
        if (ec.value() != EBADR && ec != boost::system::errc::io_error)
        {
            BMCWEB_LOG_ERROR(
                "DBus error on GetProperty for OperatingFrequency: {}",
                ec.message());
            messages::internalError(asyncResp->res);
        }
        return;
    }

    if (!std::isfinite(value))
    {
        BMCWEB_LOG_DEBUG("Received non-finite value for OperatingSpeedMHz");
        asyncResp->res.jsonValue["OperatingSpeedMHz"] = nullptr;
        return;
    }

    constexpr double hzPerMhz = 1000000.0;
    asyncResp->res.jsonValue["OperatingSpeedMHz"] =
        static_cast<int64_t>(value / hzPerMhz);
}

inline void afterGetProcessorMetricsMetricPaths(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBus response error on GetAssociatedSubTree: {}",
                             ec);
            messages::internalError(asyncResp->res);
        }
        return;
    }

    for (const auto& [path, services] : subtree)
    {
        if (services.size() != 1)
        {
            continue;
        }

        sdbusplus::message::object_path objPath(path);
        const std::string metricName = objPath.filename();

        if (metricName != "OperatingFrequency")
        {
            continue;
        }

        const auto& serviceName = services.begin()->first;
        dbus::utility::getProperty<double>(
            serviceName, path, "xyz.openbmc_project.Metric.Value", "Value",
            std::bind_front(afterGetOperatingFrequency, asyncResp));
        return;
    }
}

inline void getProcessorMetricsOperatingSpeed(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& objectPath)
{
    const sdbusplus::message::object_path assocPath =
        sdbusplus::message::object_path(objectPath) / "measured_by";
    dbus::utility::getAssociatedSubTree(
        assocPath,
        sdbusplus::message::object_path("/xyz/openbmc_project/metric"), 0,
        std::array<std::string_view, 1>{"xyz.openbmc_project.Metric.Value"},
        std::bind_front(afterGetProcessorMetricsMetricPaths, asyncResp));
}

/**
 * @brief Populate ProcessorMetrics from the processor's service map
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
    for (const auto& [serviceName, interfaceList] : serviceMap)
    {
        for (const auto& interface : interfaceList)
        {
            if (interface == "xyz.openbmc_project.Memory.MemoryECC")
            {
                getProcessorMetricsECCData(asyncResp, serviceName, objectPath);
                break;
            }
        }
    }

    getProcessorMetricsOperatingSpeed(asyncResp, objectPath);
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

    getProcessorObject(asyncResp, processorId,
                       std::bind_front(getProcessorMetricsData, asyncResp));
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
