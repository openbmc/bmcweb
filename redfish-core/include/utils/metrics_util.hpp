// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "logging.hpp"

#include <boost/system/error_code.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace redfish
{
namespace metrics_util
{

inline void populateMetricsProperty(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const nlohmann::json::json_pointer& jsonPtr,
    const boost::system::error_code& ec, double value)
{
    if (ec)
    {
        BMCWEB_LOG_DEBUG(
            "DBus response error on GetProperty {} for property {}", ec,
            jsonPtr.to_string());
        return;
    }

    if (!std::isfinite(value))
    {
        BMCWEB_LOG_DEBUG("Received non-finite value for property {}",
                         jsonPtr.to_string());
        asyncResp->res.jsonValue[jsonPtr] = nullptr;
    }
    else
    {
        asyncResp->res.jsonValue[jsonPtr] = static_cast<int64_t>(value);
    }
}

inline void getMetricProperty(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& serviceName, const sdbusplus::object_path& objectPath,
    const nlohmann::json::json_pointer& jsonPtr)
{
    dbus::utility::getProperty<double>(
        serviceName, objectPath, "xyz.openbmc_project.Metric.Value", "Value",
        std::bind_front(populateMetricsProperty, asyncResp, jsonPtr));
}

inline void afterGetPortPCIeMetrics(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& object)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBus response error on GetAssociatedSubTree{}", ec);
        messages::internalError(asyncResp->res);
        return;
    }

    for (const auto& [path, service] : object)
    {
        if (service.size() != 1)
        {
            continue;
        }

        sdbusplus::object_path objectPath(path);
        const std::string metricType = objectPath.parent_path().filename();
        const std::string metricName = objectPath.filename();

        if (metricType != "pcie")
        {
            continue;
        }

        const auto& serviceName = service.begin()->first;

        if (metricName == "correctable_error_count")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/PCIeErrors/CorrectableErrorCount"_json_pointer);
        }
        else if (metricName == "non_fatal_error_count")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/PCIeErrors/NonFatalErrorCount"_json_pointer);
        }
        else if (metricName == "fatal_error_count")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/PCIeErrors/FatalErrorCount"_json_pointer);
        }
        else if (metricName == "l0_to_recovery_count")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/PCIeErrors/L0ToRecoveryCount"_json_pointer);
        }
        else if (metricName == "replay_count")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/PCIeErrors/ReplayCount"_json_pointer);
        }
        else if (metricName == "replay_rollover_count")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/PCIeErrors/ReplayRolloverCount"_json_pointer);
        }
        else if (metricName == "nak_sent_count")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/PCIeErrors/NAKSentCount"_json_pointer);
        }
        else if (metricName == "nak_received_count")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/PCIeErrors/NAKReceivedCount"_json_pointer);
        }
        else if (metricName == "unsupported_request_count")
        {
            getMetricProperty(
                asyncResp, serviceName, path,
                "/PCIeErrors/UnsupportedRequestCount"_json_pointer);
        }
    }
}

inline void getPortPCIeMetrics(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& portPath)
{
    const sdbusplus::object_path associationPath =
        sdbusplus::object_path(portPath) / "measured_by";
    dbus::utility::getAssociatedSubTree(
        associationPath, sdbusplus::object_path("/xyz/openbmc_project/metric"),
        0, std::array<std::string_view, 1>{"xyz.openbmc_project.Metric.Value"},
        std::bind_front(afterGetPortPCIeMetrics, asyncResp));
}

} // namespace metrics_util
} // namespace redfish
