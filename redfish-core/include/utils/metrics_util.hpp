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

#include <algorithm>
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

// The tables below are looked up with a binary search, so each one has to
// stay sorted by the D-Bus metric name it keys on.
constexpr auto byMetricName =
    &std::pair<std::string_view, std::string_view>::first;

inline void getMappedMetricProperty(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& serviceName, const sdbusplus::object_path& objectPath,
    std::string_view metricName,
    std::span<const std::pair<std::string_view, std::string_view>> table)
{
    const auto entry =
        std::ranges::lower_bound(table, metricName, {}, byMetricName);
    if (entry == table.end() || entry->first != metricName)
    {
        return;
    }
    getMetricProperty(asyncResp, serviceName, objectPath,
                      nlohmann::json::json_pointer(std::string(entry->second)));
}

static constexpr auto nicPortMetrics =
    std::to_array<std::pair<std::string_view, std::string_view>>({
        {"rx_broadcast_frames", "/Networking/RXBroadcastFrames"},
        {"rx_bytes", "/RXBytes"},
        {"rx_errors", "/RXErrors"},
        {"rx_false_carrier_errors", "/Networking/RXFalseCarrierErrors"},
        {"rx_fcs_errors", "/Networking/RXFCSErrors"},
        {"rx_frame_alignment_errors", "/Networking/RXFrameAlignmentErrors"},
        {"rx_frames", "/Networking/RXFrames"},
        {"rx_multicast_frames", "/Networking/RXMulticastFrames"},
        {"rx_oversize_frames", "/Networking/RXOversizeFrames"},
        {"rx_pause_xoff_frames", "/Networking/RXPauseXOFFFrames"},
        {"rx_pause_xon_frames", "/Networking/RXPauseXONFrames"},
        {"rx_undersize_frames", "/Networking/RXUndersizeFrames"},
        {"rx_unicast_frames", "/Networking/RXUnicastFrames"},
        {"tx_broadcast_frames", "/Networking/TXBroadcastFrames"},
        {"tx_bytes", "/TXBytes"},
        {"tx_discards", "/Networking/TXDiscards"},
        {"tx_excessive_collisions", "/Networking/TXExcessiveCollisions"},
        {"tx_frames", "/Networking/TXFrames"},
        {"tx_late_collisions", "/Networking/TXLateCollisions"},
        {"tx_multicast_frames", "/Networking/TXMulticastFrames"},
        {"tx_multiple_collisions", "/Networking/TXMultipleCollisions"},
        {"tx_pause_xoff_frames", "/Networking/TXPauseXOFFFrames"},
        {"tx_pause_xon_frames", "/Networking/TXPauseXONFrames"},
        {"tx_single_collisions", "/Networking/TXSingleCollisions"},
        {"tx_unicast_frames", "/Networking/TXUnicastFrames"},
    });
static_assert(std::ranges::is_sorted(nicPortMetrics, {}, byMetricName));

static constexpr auto pciePortMetrics =
    std::to_array<std::pair<std::string_view, std::string_view>>({
        {"correctable_error_count", "/PCIeErrors/CorrectableErrorCount"},
        {"fatal_error_count", "/PCIeErrors/FatalErrorCount"},
        {"l0_to_recovery_count", "/PCIeErrors/L0ToRecoveryCount"},
        {"nak_received_count", "/PCIeErrors/NAKReceivedCount"},
        {"nak_sent_count", "/PCIeErrors/NAKSentCount"},
        {"non_fatal_error_count", "/PCIeErrors/NonFatalErrorCount"},
        {"replay_count", "/PCIeErrors/ReplayCount"},
        {"replay_rollover_count", "/PCIeErrors/ReplayRolloverCount"},
        {"unsupported_request_count", "/PCIeErrors/UnsupportedRequestCount"},
    });
static_assert(std::ranges::is_sorted(pciePortMetrics, {}, byMetricName));

inline void afterGetPortMetrics(
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

        if (metricType == "nic")
        {
            getMappedMetricProperty(asyncResp, service.begin()->first, path,
                                    metricName, nicPortMetrics);
        }
        else if (metricType == "pcie")
        {
            getMappedMetricProperty(asyncResp, service.begin()->first, path,
                                    metricName, pciePortMetrics);
        }
    }
}

inline void getPortMetrics(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& portPath)
{
    const sdbusplus::object_path associationPath =
        sdbusplus::object_path(portPath) / "measured_by";
    dbus::utility::getAssociatedSubTree(
        associationPath, sdbusplus::object_path("/xyz/openbmc_project/metric"),
        0, std::array<std::string_view, 1>{"xyz.openbmc_project.Metric.Value"},
        std::bind_front(afterGetPortMetrics, asyncResp));
}

} // namespace metrics_util
} // namespace redfish
