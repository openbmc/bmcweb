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
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>

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

inline void getMappedMetricProperty(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& serviceName, const sdbusplus::object_path& objectPath,
    std::string_view metricName,
    std::span<const std::pair<std::string_view, std::string_view>> table)
{
    for (const auto& [name, pointer] : table)
    {
        if (name != metricName)
        {
            continue;
        }
        getMetricProperty(asyncResp, serviceName, objectPath,
                          nlohmann::json::json_pointer(std::string(pointer)));
        return;
    }
}

static constexpr std::array<std::pair<std::string_view, std::string_view>, 9>
    pciePortMetrics = {{
        {"correctable_error_count", "/PCIeErrors/CorrectableErrorCount"},
        {"non_fatal_error_count", "/PCIeErrors/NonFatalErrorCount"},
        {"fatal_error_count", "/PCIeErrors/FatalErrorCount"},
        {"l0_to_recovery_count", "/PCIeErrors/L0ToRecoveryCount"},
        {"replay_count", "/PCIeErrors/ReplayCount"},
        {"replay_rollover_count", "/PCIeErrors/ReplayRolloverCount"},
        {"nak_sent_count", "/PCIeErrors/NAKSentCount"},
        {"nak_received_count", "/PCIeErrors/NAKReceivedCount"},
        {"unsupported_request_count", "/PCIeErrors/UnsupportedRequestCount"},
    }};

inline void afterGetPortPCIeMetrics(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& object)
{
    if (ec)
    {
        if (ec.value() == EBADR || ec == boost::system::errc::io_error)
        {
            BMCWEB_LOG_DEBUG("No measured_by association for port");
            return;
        }
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

        getMappedMetricProperty(asyncResp, service.begin()->first, path,
                                metricName, pciePortMetrics);
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
