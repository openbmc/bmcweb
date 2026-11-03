// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/port.hpp"
#include "generated/enums/protocol.hpp"
#include "generated/enums/resource.hpp"
#include "http_request.hpp"
#include "human_sort.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "switch_port.hpp"
#include "utils/chassis_utils.hpp"
#include "utils/dbus_utils.hpp"

#include <boost/beast/http/verb.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

static constexpr std::array<std::string_view, 1> networkAdapterInterface = {
    "xyz.openbmc_project.Inventory.Item.NetworkAdapter"};

namespace redfish
{

static constexpr std::array<std::pair<std::string_view, std::string_view>, 32>
    nicPortMetrics = {{
        {"rx_bytes", "/RXBytes"},
        {"tx_bytes", "/TXBytes"},
        {"rx_multicast_frames", "/Networking/RXMulticastFrames"},
        {"tx_multicast_frames", "/Networking/TXMulticastFrames"},
        {"rx_broadcast_frames", "/Networking/RXBroadcastFrames"},
        {"tx_broadcast_frames", "/Networking/TXBroadcastFrames"},
        {"rx_unicast_frames", "/Networking/RXUnicastFrames"},
        {"tx_unicast_frames", "/Networking/TXUnicastFrames"},
        {"rx_fcs_errors", "/Networking/RXFCSErrors"},
        {"rx_frame_alignment_errors", "/Networking/RXFrameAlignmentErrors"},
        {"rx_false_carrier_errors", "/Networking/RXFalseCarrierErrors"},
        {"rx_undersize_frames", "/Networking/RXUndersizeFrames"},
        {"rx_oversize_frames", "/Networking/RXOversizeFrames"},
        {"rx_pause_xon_frames", "/Networking/RXPauseXONFrames"},
        {"rx_pause_xoff_frames", "/Networking/RXPauseXOFFFrames"},
        {"tx_pause_xon_frames", "/Networking/TXPauseXONFrames"},
        {"tx_pause_xoff_frames", "/Networking/TXPauseXOFFFrames"},
        {"tx_single_collisions", "/Networking/TXSingleCollisions"},
        {"tx_multiple_collisions", "/Networking/TXMultipleCollisions"},
        {"tx_late_collisions", "/Networking/TXLateCollisions"},
        {"tx_excessive_collisions", "/Networking/TXExcessiveCollisions"},
        {"rx_errors", "/RXErrors"},
        {"tx_errors", "/TXErrors"},
        {"rx_discards", "/Networking/RXDiscards"},
        {"tx_discards", "/Networking/TXDiscards"},
        {"rx_frames", "/Networking/RXFrames"},
        {"tx_frames", "/Networking/TXFrames"},
        {"rx_pfc_frames", "/Networking/RXPFCFrames"},
        {"tx_pfc_frames", "/Networking/TXPFCFrames"},
        {"rdma_tx_read_requests", "/Networking/RDMATXReadRequests"},
        {"rdma_tx_send_requests", "/Networking/RDMATXSendRequests"},
        {"rdma_tx_write_requests", "/Networking/RDMATXWriteRequests"},
    }};

inline void afterGetTransceiverMetricValue(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const nlohmann::json::json_pointer& jsonPtr, double scale,
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
        asyncResp->res.jsonValue[jsonPtr] = nullptr;
        return;
    }

    asyncResp->res.jsonValue[jsonPtr] = value * scale;
}

inline void getTransceiverMetric(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& serviceName, const sdbusplus::object_path& objectPath,
    const nlohmann::json::json_pointer& jsonPtr, double scale)
{
    dbus::utility::getProperty<double>(
        serviceName, objectPath, "xyz.openbmc_project.Metric.Value", "Value",
        std::bind_front(afterGetTransceiverMetricValue, asyncResp, jsonPtr,
                        scale));
}

// Metric.Value names a lane by appending its number to the path segment.
inline std::optional<size_t> getLaneFromMetricName(std::string_view name,
                                                   std::string_view prefix)
{
    if (!name.starts_with(prefix))
    {
        return std::nullopt;
    }
    name.remove_prefix(prefix.size());

    size_t lane = 0;
    const auto [end, ec] = std::from_chars(name.begin(), name.end(), lane);
    if (ec != std::errc() || end != name.end())
    {
        return std::nullopt;
    }
    return lane;
}

struct TransceiverLaneMetric
{
    std::string_view prefix;
    std::string_view property;
};

// Metric.Value publishes base SI units where Redfish asks for milli.
constexpr double milliPerUnit = 1e3;

constexpr std::array<TransceiverLaneMetric, 3> transceiverLaneMetrics{{
    {"tx_output_power_lane_", "TXOutputPowerMilliWatts"},
    {"rx_input_power_lane_", "RXInputPowerMilliWatts"},
    {"tx_bias_current_lane_", "TXBiasCurrentMilliAmps"},
}};

// Module-wide metrics, published in the unit Redfish reports them in.
constexpr std::array<std::pair<std::string_view, std::string_view>, 1>
    transceiverMetrics{{
        {"supply_voltage", "SupplyVoltage"},
    }};

constexpr std::array<std::string_view, 1> transceiverInterface = {
    "xyz.openbmc_project.Inventory.Item.Transceiver"};

// ByLane is an array, so lanes pack in order and LaneId keeps each number.
inline std::vector<size_t> getTransceiverLanes(
    const dbus::utility::MapperGetSubTreeResponse& object)
{
    std::vector<size_t> lanes;
    for (const auto& [path, service] : object)
    {
        const std::string metricName = sdbusplus::object_path(path).filename();
        for (const TransceiverLaneMetric& metric : transceiverLaneMetrics)
        {
            const std::optional<size_t> lane =
                getLaneFromMetricName(metricName, metric.prefix);
            if (lane)
            {
                lanes.push_back(*lane);
            }
        }
    }

    std::ranges::sort(lanes);
    lanes.erase(std::ranges::unique(lanes).begin(), lanes.end());
    return lanes;
}

inline void addTransceiverMetric(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const nlohmann::json::json_pointer& transceiverPtr,
    const std::string& serviceName, const sdbusplus::object_path& objectPath,
    std::span<const size_t> lanes)
{
    const std::string metricName = objectPath.filename();
    for (const auto& [name, property] : transceiverMetrics)
    {
        if (name == metricName)
        {
            getTransceiverMetric(asyncResp, serviceName, objectPath,
                                 transceiverPtr / std::string(property), 1.0);
            return;
        }
    }

    for (const TransceiverLaneMetric& metric : transceiverLaneMetrics)
    {
        const std::optional<size_t> lane =
            getLaneFromMetricName(metricName, metric.prefix);
        if (!lane)
        {
            continue;
        }

        const auto index =
            std::distance(lanes.begin(), std::ranges::find(lanes, *lane));
        const nlohmann::json::json_pointer lanePtr =
            transceiverPtr / "ByLane" / static_cast<size_t>(index);
        asyncResp->res.jsonValue[lanePtr / "LaneId"] = *lane;
        getTransceiverMetric(asyncResp, serviceName, objectPath,
                             lanePtr / std::string(metric.property),
                             milliPerUnit);
        return;
    }
}

inline void afterGetTransceiverMetrics(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const nlohmann::json::json_pointer& transceiverPtr,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& object)
{
    if (ec)
    {
        if (ec.value() == EBADR)
        {
            BMCWEB_LOG_DEBUG("No metrics for transceiver {}",
                             transceiverPtr.to_string());
            return;
        }
        BMCWEB_LOG_ERROR("DBus response error on GetAssociatedSubTree {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }

    const std::vector<size_t> lanes = getTransceiverLanes(object);
    for (const auto& [path, service] : object)
    {
        if (service.size() != 1)
        {
            continue;
        }
        addTransceiverMetric(asyncResp, transceiverPtr, service.begin()->first,
                             sdbusplus::object_path(path), lanes);
    }
}

inline void afterGetPortTransceiverPaths(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& paths)
{
    if (ec)
    {
        if (ec.value() == EBADR)
        {
            BMCWEB_LOG_DEBUG("No transceiver associated with the port");
            return;
        }
        BMCWEB_LOG_ERROR("DBus response error on GetAssociatedSubTreePaths {}",
                         ec);
        messages::internalError(asyncResp->res);
        return;
    }

    if (paths.empty())
    {
        return;
    }

    std::vector<std::string> sorted = paths;
    std::ranges::sort(sorted, AlphanumLess<std::string>());
    // Create every entry first, so one without metrics leaves no null.
    asyncResp->res.jsonValue["Transceivers"] =
        nlohmann::json::array_t(sorted.size(), nlohmann::json::object_t());
    for (size_t index = 0; index < sorted.size(); index++)
    {
        dbus::utility::getAssociatedSubTree(
            sdbusplus::object_path(sorted[index]) / "measured_by",
            sdbusplus::object_path("/xyz/openbmc_project/metric"), 0,
            std::array<std::string_view, 1>{"xyz.openbmc_project.Metric.Value"},
            std::bind_front(afterGetTransceiverMetrics, asyncResp,
                            "/Transceivers"_json_pointer / index));
    }
}

inline void handleNetworkAdapterPortMetricsPathsPortMetricsGet(
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

        sdbusplus::object_path objectPah(path);

        const std::string metricType = objectPah.parent_path().filename();
        const std::string metricName = objectPah.filename();
        const auto& serviceName = service.begin()->first;

        if (metricType == "pcie")
        {
            getMappedMetricProperty(asyncResp, serviceName, path, metricName,
                                    pciePortMetrics);
            continue;
        }

        if (metricType != "nic")
        {
            continue;
        }

        getMappedMetricProperty(asyncResp, serviceName, path, metricName,
                                nicPortMetrics);
    }
}

inline void handleNetworkAdapterPortPathPortMetricsGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& networkAdapterId,
    const std::string& portId, const sdbusplus::object_path& portPath,
    [[maybe_unused]] const std::string& serviceName)
{
    asyncResp->res.jsonValue["@odata.type"] = "#PortMetrics.v1_9_0.PortMetrics";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/NetworkAdapters/{}/Ports/{}/Metrics", chassisId,
        networkAdapterId, portId);
    asyncResp->res.jsonValue["Id"] = "Metrics";
    asyncResp->res.jsonValue["Name"] =
        std::format("{} {} Port Metrics", networkAdapterId, portId);

    const std::string associationPath = portPath / "measured_by";
    dbus::utility::getAssociatedSubTree(
        associationPath, sdbusplus::object_path("/xyz/openbmc_project/metric"),
        0, std::array<std::string_view, 1>{"xyz.openbmc_project.Metric.Value"},
        std::bind_front(handleNetworkAdapterPortMetricsPathsPortMetricsGet,
                        asyncResp));
    dbus::utility::getAssociatedSubTreePaths(
        portPath / "containing",
        sdbusplus::object_path("/xyz/openbmc_project/inventory"), 0,
        transceiverInterface,
        std::bind_front(afterGetPortTransceiverPaths, asyncResp));
}

inline void afterGetSfpPresence(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec, bool present)
{
    if (ec)
    {
        if (ec.value() == EBADR)
        {
            BMCWEB_LOG_DEBUG("Transceiver has no Present property");
            return;
        }
        BMCWEB_LOG_ERROR("DBus response error for Present {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }

    nlohmann::json& status = asyncResp->res.jsonValue["SFP"]["Status"];
    if (!present)
    {
        status["State"] = resource::State::Absent;
        return;
    }

    status["State"] = resource::State::Enabled;
    // afterGetPortFunctional may already have reported the module Critical.
    if (!status.contains("Health"))
    {
        status["Health"] = resource::Health::OK;
    }
}

inline void afterGetPortFunctional(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, bool hasSfp,
    const boost::system::error_code& ec, bool functional)
{
    if (ec)
    {
        if (ec.value() == EBADR)
        {
            BMCWEB_LOG_DEBUG("Port has no Functional property");
            return;
        }
        BMCWEB_LOG_ERROR("DBus response error for Functional {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }

    if (functional)
    {
        return;
    }

    nlohmann::json& status = asyncResp->res.jsonValue["Status"];
    status["Health"] = resource::Health::Critical;
    status["HealthRollup"] = resource::Health::Critical;
    if (hasSfp)
    {
        asyncResp->res.jsonValue["SFP"]["Status"]["Health"] =
            resource::Health::Critical;
    }
}

// Empty if Redfish has no name for the value; suffix completes port types.
template <typename T>
inline std::optional<T> dbusToRedfishEnum(const std::string* value,
                                          std::string_view suffix = {})
{
    if (value == nullptr)
    {
        return std::nullopt;
    }

    const T mapped = nlohmann::json(value->substr(value->rfind('.') + 1) +
                                    std::string(suffix))
                         .get<T>();
    if (mapped == T::Invalid)
    {
        return std::nullopt;
    }
    return mapped;
}

inline void afterGetSfpAsset(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& properties)
{
    if (ec)
    {
        if (ec.value() == EBADR)
        {
            BMCWEB_LOG_DEBUG("Transceiver has no Asset interface");
            return;
        }
        BMCWEB_LOG_ERROR("DBus response error for Asset {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }

    const std::string* manufacturer = nullptr;
    const std::string* partNumber = nullptr;
    const std::string* serialNumber = nullptr;

    if (!sdbusplus::unpackPropertiesNoThrow(
            dbus_utils::UnpackErrorPrinter(), properties, "Manufacturer",
            manufacturer, "PartNumber", partNumber, "SerialNumber",
            serialNumber))
    {
        messages::internalError(asyncResp->res);
        return;
    }

    nlohmann::json& sfp = asyncResp->res.jsonValue["SFP"];
    if (manufacturer != nullptr)
    {
        sfp["Manufacturer"] = *manufacturer;
    }
    if (partNumber != nullptr)
    {
        sfp["PartNumber"] = *partNumber;
    }
    if (serialNumber != nullptr)
    {
        sfp["SerialNumber"] = *serialNumber;
    }
}

inline void afterGetSfpDescription(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& properties)
{
    if (ec)
    {
        if (ec.value() == EBADR)
        {
            BMCWEB_LOG_DEBUG("Transceiver has no Item.Transceiver interface");
            return;
        }
        BMCWEB_LOG_ERROR("DBus response error for Transceiver {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }

    const std::string* type = nullptr;
    const std::string* mediumType = nullptr;
    const std::string* managementInterfaceType = nullptr;

    if (!sdbusplus::unpackPropertiesNoThrow(
            dbus_utils::UnpackErrorPrinter(), properties, "Type", type,
            "MediumType", mediumType, "ManagementInterfaceType",
            managementInterfaceType))
    {
        messages::internalError(asyncResp->res);
        return;
    }

    nlohmann::json& sfp = asyncResp->res.jsonValue["SFP"];
    if (const auto sfpType = dbusToRedfishEnum<port::SFPType>(type))
    {
        sfp["Type"] = *sfpType;
    }
    if (const auto medium = dbusToRedfishEnum<port::MediumType>(mediumType))
    {
        sfp["MediumType"] = *medium;
    }
    if (const auto management =
            dbusToRedfishEnum<port::TransceiverManagementInterfaceType>(
                managementInterfaceType))
    {
        sfp["ManagementInterfaceType"] = *management;
    }
}

inline void afterGetPortLink(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& serviceName, const std::string& portPath,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& properties)
{
    if (ec)
    {
        if (ec.value() == EBADR)
        {
            BMCWEB_LOG_DEBUG("Port {} has no Connector.Port interface",
                             portPath);
            return;
        }
        BMCWEB_LOG_ERROR("DBus response error for Port {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }

    const std::string* portProtocol = nullptr;
    const std::string* portType = nullptr;
    const std::string* linkState = nullptr;
    const std::string* linkStatus = nullptr;
    const uint64_t* speed = nullptr;
    // The interface declares Width as "size", not uint64.
    const size_t* width = nullptr;

    if (!sdbusplus::unpackPropertiesNoThrow(
            dbus_utils::UnpackErrorPrinter(), properties, "PortProtocol",
            portProtocol, "PortType", portType, "LinkState", linkState,
            "LinkStatus", linkStatus, "Speed", speed, "Width", width))
    {
        messages::internalError(asyncResp->res);
        return;
    }

    nlohmann::json& portJson = asyncResp->res.jsonValue;
    const std::optional<protocol::Protocol> redfishProtocol =
        dbusToRedfishEnum<protocol::Protocol>(portProtocol);
    if (redfishProtocol)
    {
        portJson["PortProtocol"] = *redfishProtocol;
    }
    if (const auto type = dbusToRedfishEnum<port::PortType>(portType, "Port"))
    {
        portJson["PortType"] = *type;
    }
    if (const auto state = dbusToRedfishEnum<port::LinkState>(linkState))
    {
        portJson["LinkState"] = *state;
    }
    if (const auto status = dbusToRedfishEnum<port::LinkStatus>(linkStatus))
    {
        portJson["LinkStatus"] = *status;
    }

    // A PCIe port has no SFP, so its health is the port's alone.
    const bool hasSfp = redfishProtocol != protocol::Protocol::PCIe;
    dbus::utility::getProperty<bool>(
        serviceName, portPath,
        "xyz.openbmc_project.State.Decorator.OperationalStatus", "Functional",
        std::bind_front(afterGetPortFunctional, asyncResp, hasSfp));

    // The maximum means the port doesn't support the property.
    constexpr uint64_t unsupportedSpeed = std::numeric_limits<uint64_t>::max();
    constexpr size_t unsupportedWidth = std::numeric_limits<size_t>::max();
    // Speed is in bits per second.
    constexpr double bitsPerGigabit = 1e9;

    // 0 means the value isn't known yet, such as while the port is inactive.
    if (width != nullptr && *width != unsupportedWidth && *width != 0)
    {
        portJson["Width"] = *width;
    }

    if (speed != nullptr && *speed != unsupportedSpeed && *speed != 0)
    {
        portJson["CurrentSpeedGbps"] =
            static_cast<double>(*speed) / bitsPerGigabit;
    }
}

inline void afterGetPortTransceiver(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& object)
{
    if (ec)
    {
        if (ec.value() == EBADR)
        {
            BMCWEB_LOG_DEBUG("No transceiver associated with the port");
            return;
        }
        BMCWEB_LOG_ERROR("DBus response error on GetAssociatedSubTree {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }

    if (object.empty())
    {
        return;
    }

    const auto& [path, services] = *std::ranges::min_element(
        object, {}, [](const auto& entry) { return entry.first; });
    if (services.size() != 1)
    {
        return;
    }
    const std::string& service = services.front().first;

    dbus::utility::getAllProperties(
        service, path, "xyz.openbmc_project.Inventory.Decorator.Asset",
        std::bind_front(afterGetSfpAsset, asyncResp));
    dbus::utility::getProperty<bool>(
        service, path, "xyz.openbmc_project.Inventory.Item", "Present",
        std::bind_front(afterGetSfpPresence, asyncResp));
    dbus::utility::getAllProperties(
        service, path, "xyz.openbmc_project.Inventory.Item.Transceiver",
        std::bind_front(afterGetSfpDescription, asyncResp));
}

inline void handleNetworkAdapterPathPortGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassiId, const std::string& networkAdapterId,
    const std::string& portId, const std::string& portPath,
    const std::string& serviceName)
{
    asyncResp->res.jsonValue["@odata.type"] = "#Port.v1_19_0.Port";
    asyncResp->res.jsonValue["@odata.id"] =
        std::format("/redfish/v1/Chassis/{}/NetworkAdapters/{}/Ports/{}",
                    chassiId, networkAdapterId, portId);
    asyncResp->res.jsonValue["Id"] = portId;
    asyncResp->res.jsonValue["Name"] =
        std::format("{} {} Port", networkAdapterId, portId);

    nlohmann::json& status = asyncResp->res.jsonValue["Status"];
    status["Health"] = resource::Health::OK;
    status["HealthRollup"] = resource::Health::OK;
    status["State"] = resource::State::Enabled;

    asyncResp->res.jsonValue["Metrics"]["@odata.id"] = std::format(
        "/redfish/v1/Chassis/{}/NetworkAdapters/{}/Ports/{}/Metrics", chassiId,
        networkAdapterId, portId);

    dbus::utility::getAssociatedSubTree(
        sdbusplus::object_path(portPath) / "containing",
        sdbusplus::object_path("/xyz/openbmc_project/inventory"), 0,
        transceiverInterface,
        std::bind_front(afterGetPortTransceiver, asyncResp));
    dbus::utility::getAllProperties(
        serviceName, portPath, "xyz.openbmc_project.Inventory.Connector.Port",
        std::bind_front(afterGetPortLink, asyncResp, serviceName, portPath));
}

inline void afterNetworkAdapterPortPaths(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& portId,
    const std::function<void(const std::string& portPath,
                             const std::string& serviceName)>& callback,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& object)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBus response error on GetAssociatedSubTreeById {}",
                         ec);
        messages::internalError(asyncResp->res);
        return;
    }

    std::string portPath;
    std::string serviceName;
    for (const auto& [path, service] : object)
    {
        std::string portName = sdbusplus::object_path(path).filename();
        if (portName == portId)
        {
            portPath = path;
            if (service.size() != 1)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            serviceName = service.begin()->first;
            break;
        }
    }

    if (portPath.empty())
    {
        messages::resourceNotFound(asyncResp->res, "Port", portId);
        return;
    }

    callback(portPath, serviceName);
}

inline void getNetworkAdapterPortPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& portId,
    std::function<void(const std::string& portPath,
                       const std::string& serviceName)>&& callback,
    const std::string& path, [[maybe_unused]] const std::string& serviceName)
{
    std::string associationPath = path + "/connecting";
    dbus::utility::getAssociatedSubTree(
        associationPath,
        sdbusplus::object_path{"/xyz/openbmc_project/inventory"}, 0,
        std::array<std::string_view, 1>{
            "xyz.openbmc_project.Inventory.Connector.Port"},
        std::bind_front(afterNetworkAdapterPortPaths, asyncResp, portId,
                        std::move(callback)));
}

inline void handleNetworkAdapterPortPathPortCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& networkAdapterId,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& object)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBus response error on GetSubTreePaths {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }

    asyncResp->res.jsonValue["@odata.type"] = "#PortCollection.PortCollection";
    asyncResp->res.jsonValue["@odata.id"] =
        std::format("/redfish/v1/Chassis/{}/NetworkAdapters/{}/Ports",
                    chassisId, networkAdapterId);
    asyncResp->res.jsonValue["Name"] = networkAdapterId + " Port Collection";

    asyncResp->res.jsonValue["Members@odata.count"] = object.size();

    nlohmann::json::array_t members;
    for (const std::string& path : object)
    {
        std::string name = sdbusplus::object_path(path).filename();
        nlohmann::json::object_t member;
        member["@odata.id"] =
            std::format("/redfish/v1/Chassis/{}/NetworkAdapters/{}/Ports/{}",
                        chassisId, networkAdapterId, name);
        members.emplace_back(std::move(member));
    }

    asyncResp->res.jsonValue["Members"] = std::move(members);
}

inline void getNetworkAdapterPortPaths(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& networkAdapterId,
    const std::string& networkAdapterPath)
{
    std::string associationPath = networkAdapterPath + "/connecting";
    dbus::utility::getAssociatedSubTreePaths(
        associationPath,
        sdbusplus::object_path{"/xyz/openbmc_project/inventory"}, 0,
        std::array<std::string_view, 1>{
            "xyz.openbmc_project.Inventory.Connector.Port"},
        std::bind_front(handleNetworkAdapterPortPathPortCollection, asyncResp,
                        chassisId, networkAdapterId));
}

inline void handleNetworkAdapterPathNetworkAdapterGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& networkAdapterId,
    [[maybe_unused]] const std::string& path)
{
    asyncResp->res.jsonValue["@odata.type"] =
        "#NetworkAdapter.v1_11_0.NetworkAdapter";
    asyncResp->res.jsonValue["@odata.id"] =
        std::format("/redfish/v1/Chassis/{}/NetworkAdapters/{}", chassisId,
                    networkAdapterId);
    asyncResp->res.jsonValue["Id"] = networkAdapterId;
    asyncResp->res.jsonValue["Name"] = networkAdapterId + " Network Adapter";

    auto& status = asyncResp->res.jsonValue["Status"];
    status["Health"] = resource::Health::OK;
    status["HealthRollup"] = resource::Health::OK;
    status["State"] = resource::State::Enabled;

    asyncResp->res.jsonValue["Ports"]["@odata.id"] =
        std::format("/redfish/v1/Chassis/{}/NetworkAdapters/{}/Ports",
                    chassisId, networkAdapterId);
}

inline void handleNetworkAdapterPaths(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& networkAdapterId,
    const std::function<void(const std::string& path)>& callback,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& object)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBus response error on GetAssociatedSubTreeById {}",
                         ec);
        messages::internalError(asyncResp->res);
        return;
    }

    for (const auto& path : object)
    {
        std::string name = sdbusplus::object_path(path).filename();
        if (name == networkAdapterId)
        {
            callback(path);
            return;
        }
    }

    messages::resourceNotFound(asyncResp->res, "NetworkAdapter",
                               networkAdapterId);
}

inline void getNetworkAdapterPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& networkAdapterId,
    std::function<void(const std::string& path)>&& callback)
{
    dbus::utility::getAssociatedSubTreePathsById(
        chassisId, "/xyz/openbmc_project/inventory", chassisInterfaces,
        "containing", networkAdapterInterface,
        std::bind_front(handleNetworkAdapterPaths, asyncResp, networkAdapterId,
                        std::move(callback)));
}

inline void handleNetworkAdapterPathsNetworkAdapterCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& object)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBus response error on GetSubTreePaths {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }

    asyncResp->res.jsonValue["@odata.id"] =
        std::format("/redfish/v1/Chassis/{}/NetworkAdapters", chassisId);
    asyncResp->res.jsonValue["@odata.type"] =
        "#NetworkAdapterCollection.NetworkAdapterCollection";
    asyncResp->res.jsonValue["Name"] =
        chassisId + " Network Adapter Collection";

    asyncResp->res.jsonValue["Members@odata.count"] = object.size();

    nlohmann::json& members = asyncResp->res.jsonValue["Members"];
    members = nlohmann::json::array();
    for (const std::string& path : object)
    {
        std::string name = sdbusplus::object_path(path).filename();
        nlohmann::json member;
        member["@odata.id"] = std::format(
            "/redfish/v1/Chassis/{}/NetworkAdapters/{}", chassisId, name);
        members.push_back(std::move(member));
    }
}

inline void handleNetworkAdapterGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& networkAdapterId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    getNetworkAdapterPath(
        asyncResp, chassisId, networkAdapterId,
        std::bind_front(handleNetworkAdapterPathNetworkAdapterGet, asyncResp,
                        chassisId, networkAdapterId));
}

inline void handleNetworkAdapterCollectionGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    dbus::utility::getAssociatedSubTreePathsById(
        chassisId, "/xyz/openbmc_project/inventory", chassisInterfaces,
        "containing", networkAdapterInterface,
        std::bind_front(handleNetworkAdapterPathsNetworkAdapterCollection,
                        asyncResp, chassisId));
}

inline void handleNetworkAdapterPortMetricsGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& networkAdapterId,
    const std::string& portId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    getNetworkAdapterPath(
        asyncResp, chassisId, networkAdapterId,
        std::bind_front(
            getAssociatedPortPath, asyncResp, portId,
            std::bind_front(handleNetworkAdapterPortPathPortMetricsGet,
                            asyncResp, chassisId, networkAdapterId, portId)));
}

inline void handleNetworkAdapterPortGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& networkAdapterId,
    const std::string& portId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    getNetworkAdapterPath(
        asyncResp, chassisId, networkAdapterId,
        std::bind_front(
            getAssociatedPortPath, asyncResp, portId,
            std::bind_front(handleNetworkAdapterPathPortGet, asyncResp,
                            chassisId, networkAdapterId, portId)));
}

inline void handleNetworkAdapterPortCollectionGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& networkAdapterId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    getNetworkAdapterPath(asyncResp, chassisId, networkAdapterId,
                          std::bind_front(getNetworkAdapterPortPaths, asyncResp,
                                          chassisId, networkAdapterId));
}

inline void requestRoutesChassisNetworkAdapter(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/NetworkAdapters/")
        .privileges(redfish::privileges::getNetworkAdapterCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleNetworkAdapterCollectionGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/NetworkAdapters/<str>/")
        .privileges(redfish::privileges::getNetworkAdapter)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleNetworkAdapterGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/NetworkAdapters/<str>/Ports/")
        .privileges(redfish::privileges::getPortCollection)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleNetworkAdapterPortCollectionGet, std::ref(app)));

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Chassis/<str>/NetworkAdapters/<str>/Ports/<str>/")
        .privileges(redfish::privileges::getPort)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleNetworkAdapterPortGet, std::ref(app)));

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Chassis/<str>/NetworkAdapters/<str>/Ports/<str>/Metrics/")
        .privileges(redfish::privileges::getPortMetrics)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleNetworkAdapterPortMetricsGet, std::ref(app)));
}
} // namespace redfish
