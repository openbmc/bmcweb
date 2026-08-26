// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/port.hpp"
#include "generated/enums/resource.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "switch_port.hpp"
#include "utils/chassis_utils.hpp"
#include "utils/dbus_utils.hpp"

#include <boost/beast/http/verb.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

static constexpr std::array<std::string_view, 1> networkAdapterInterface = {
    "xyz.openbmc_project.Inventory.Item.NetworkAdapter"};

static constexpr std::array<std::string_view, 1> lldpConfigurationInterface = {
    "xyz.openbmc_project.Network.LLDP.Configuration"};

static constexpr std::array<std::string_view, 1> lldpTlvsInterface = {
    "xyz.openbmc_project.Network.LLDP.TLVs"};

namespace redfish
{

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

        if (metricType != "nic")
        {
            continue;
        }

        const auto& serviceName = service.begin()->first;

        if (metricName == "rx_bytes")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/RXBytes"_json_pointer);
        }
        else if (metricName == "tx_bytes")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/TXBytes"_json_pointer);
        }
        else if (metricName == "rx_multicast_frames")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/Networking/RXMulticastFrames"_json_pointer);
        }
        else if (metricName == "tx_multicast_frames")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/Networking/TXMulticastFrames"_json_pointer);
        }
        else if (metricName == "rx_broadcast_frames")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/Networking/RXBroadcastFrames"_json_pointer);
        }
        else if (metricName == "tx_broadcast_frames")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/Networking/TXBroadcastFrames"_json_pointer);
        }
        else if (metricName == "rx_unicast_frames")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/Networking/RXUnicastFrames"_json_pointer);
        }
        else if (metricName == "tx_unicast_frames")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/Networking/TXUnicastFrames"_json_pointer);
        }
        else if (metricName == "rx_fcs_errors")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/Networking/RXFCSErrors"_json_pointer);
        }
        else if (metricName == "rx_frame_alignment_errors")
        {
            getMetricProperty(
                asyncResp, serviceName, path,
                "/Networking/RXFrameAlignmentErrors"_json_pointer);
        }
        else if (metricName == "rx_false_carrier_errors")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/Networking/RXFalseCarrierErrors"_json_pointer);
        }
        else if (metricName == "rx_undersize_frames")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/Networking/RXUndersizeFrames"_json_pointer);
        }
        else if (metricName == "rx_oversize_frames")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/Networking/RXOversizeFrames"_json_pointer);
        }
        else if (metricName == "rx_pause_xon_frames")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/Networking/RXPauseXONFrames"_json_pointer);
        }
        else if (metricName == "rx_pause_xoff_frames")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/Networking/RXPauseXOFFFrames"_json_pointer);
        }
        else if (metricName == "tx_pause_xon_frames")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/Networking/TXPauseXONFrames"_json_pointer);
        }
        else if (metricName == "tx_pause_xoff_frames")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/Networking/TXPauseXOFFFrames"_json_pointer);
        }
        else if (metricName == "tx_single_collisions")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/Networking/TXSingleCollisions"_json_pointer);
        }
        else if (metricName == "tx_multiple_collisions")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/Networking/TXMultipleCollisions"_json_pointer);
        }
        else if (metricName == "tx_late_collisions")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/Networking/TXLateCollisions"_json_pointer);
        }
        else if (metricName == "tx_excessive_collisions")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/Networking/TXExcessiveCollisions"_json_pointer);
        }
    }
}

inline void handleNetworkAdapterPortPathPortMetricsGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& networkAdapterId,
    const std::string& portId, const sdbusplus::object_path& portPath,
    [[maybe_unused]] const std::string& serviceName)
{
    asyncResp->res.jsonValue["@odata.type"] = "#PortMetrics.v1_7_0.PortMetrics";
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
}

// The agent of a network device is described by a mode per direction, while
// Redfish asks only whether it is on. It is on when it is doing something in
// either direction: a port that only announces itself still speaks the
// protocol.
inline bool lldpEnabledFrom(const std::string& transmitMode,
                            const std::string& receiveMode)
{
    constexpr std::string_view disabled =
        "xyz.openbmc_project.Network.LLDP.Configuration.Mode.Disabled";
    return transmitMode != disabled || receiveMode != disabled;
}

inline void afterGetLldpConfiguration(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const nlohmann::json::json_pointer& target,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& properties)
{
    // What the agent is set to decorates the resource; it is not the resource.
    // A device that went away between being found and being read leaves the
    // rest of what was gathered standing rather than turning it into an error.
    if (ec)
    {
        BMCWEB_LOG_WARNING("Reading the LLDP configuration failed: {}", ec);
        return;
    }

    std::optional<std::string> transmitMode;
    std::optional<std::string> receiveMode;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "TransmitMode",
        transmitMode, "ReceiveMode", receiveMode);

    if (!success)
    {
        BMCWEB_LOG_WARNING("The LLDP configuration is not what was expected");
        return;
    }

    if (transmitMode && receiveMode)
    {
        asyncResp->res.jsonValue[target] =
            lldpEnabledFrom(*transmitMode, *receiveMode);
    }
}

inline void afterGetLldpConfigurationPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const nlohmann::json::json_pointer& target,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& object)
{
    // A device whose agent cannot be configured has no such object, which is
    // not an error: the resource simply says nothing about it.
    if (ec || object.empty() || object.front().second.empty())
    {
        return;
    }

    if (object.size() > 1)
    {
        BMCWEB_LOG_WARNING(
            "An adapter has more than one LLDP configuration; reporting {}",
            object.front().first);
    }

    dbus::utility::getAllProperties(
        object.front().second.front().first, object.front().first,
        "xyz.openbmc_project.Network.LLDP.Configuration",
        std::bind_front(afterGetLldpConfiguration, asyncResp, target));
}

// The object that configures the agent is not part of the inventory, so it is
// reached from the adapter it controls rather than found beneath it.
inline void getLldpEnabled(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& adapterPath,
                           const nlohmann::json::json_pointer& target)
{
    dbus::utility::getAssociatedSubTree(
        sdbusplus::object_path{adapterPath} / "controlled_by",
        sdbusplus::object_path{"/xyz/openbmc_project/network/lldp"}, 0,
        lldpConfigurationInterface,
        std::bind_front(afterGetLldpConfigurationPath, asyncResp, target));
}

// The interface names a subtype with the same words IEEE 802.1AB uses, and so
// does Redfish, but a name is matched rather than trimmed so that a value
// either side stops recognising is reported as such.
inline port::IEEE802IdSubtype toIdSubtype(const std::string& value)
{
    constexpr std::string_view prefix =
        "xyz.openbmc_project.Network.LLDP.TLVs.IdSubtype.";
    if (!value.starts_with(prefix))
    {
        return port::IEEE802IdSubtype::Invalid;
    }
    // The generated enum knows its own names, and a name it does not know
    // deserialises to Invalid, which is what an unrecognised value should be.
    return nlohmann::json(value.substr(prefix.size()))
        .get<port::IEEE802IdSubtype>();
}

inline port::LLDPSystemCapabilities toSystemCapability(const std::string& value)
{
    constexpr std::string_view prefix =
        "xyz.openbmc_project.Network.LLDP.TLVs.SystemCapability.";
    if (!value.starts_with(prefix))
    {
        return port::LLDPSystemCapabilities::Invalid;
    }
    return nlohmann::json(value.substr(prefix.size()))
        .get<port::LLDPSystemCapabilities>();
}

// A subtype only means something next to the identifier it describes, so one
// the frame did not carry is reported as absent rather than guessed. A subtype
// this build has no name for is reported the same way: the enumeration has no
// member to stand for it, and inventing one would put a value in the resource
// that the schema does not define.
inline void reportSubtype(nlohmann::json& frame, const char* name,
                          const std::optional<std::string>& value)
{
    if (!value)
    {
        return;
    }
    const port::IEEE802IdSubtype subtype = toIdSubtype(*value);
    frame[name] = (subtype == port::IEEE802IdSubtype::NotTransmitted ||
                   subtype == port::IEEE802IdSubtype::Invalid)
                      ? nlohmann::json(nullptr)
                      : nlohmann::json(subtype);
}

inline void afterGetLldpFrame(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& properties)
{
    // As above: a frame that could not be read leaves the port described by
    // everything else that was gathered.
    if (ec)
    {
        BMCWEB_LOG_WARNING("Reading an LLDP frame failed: {}", ec);
        return;
    }

    std::optional<std::string> chassisId;
    std::optional<std::string> portId;
    std::optional<std::string> systemName;
    std::optional<std::string> systemDescription;
    std::optional<std::string> managementAddressIPv4;
    std::optional<std::string> managementAddressIPv6;
    std::optional<std::string> managementAddressMAC;
    std::optional<uint64_t> managementVlanId;
    std::optional<std::string> direction;
    std::optional<std::string> chassisIdSubtype;
    std::optional<std::string> portIdSubtype;
    std::optional<std::vector<std::string>> systemCapabilities;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "ChassisId", chassisId,
        "PortId", portId, "SystemName", systemName, "SystemDescription",
        systemDescription, "ManagementAddressIPv4", managementAddressIPv4,
        "ManagementAddressIPv6", managementAddressIPv6, "ManagementAddressMAC",
        managementAddressMAC, "ManagementVlanId", managementVlanId,
        "ChassisIdSubtype", chassisIdSubtype, "PortIdSubtype", portIdSubtype,
        "SystemCapabilities", systemCapabilities, "Direction", direction);

    if (!success)
    {
        BMCWEB_LOG_WARNING("An LLDP frame is not what was expected");
        return;
    }

    // Which of a port's two frames this is comes from the frame itself. The
    // interface says so in as many words, because the names its objects are
    // given are not part of what it promises.
    constexpr std::string_view receivedDirection =
        "xyz.openbmc_project.Network.LLDP.TLVs.Direction.Received";
    constexpr std::string_view transmittedDirection =
        "xyz.openbmc_project.Network.LLDP.TLVs.Direction.Transmitted";

    if (!direction)
    {
        BMCWEB_LOG_WARNING("An LLDP frame does not say which way it went");
        return;
    }

    nlohmann::json::json_pointer target;
    if (*direction == receivedDirection)
    {
        target = nlohmann::json::json_pointer("/Ethernet/LLDPReceive");
    }
    else if (*direction == transmittedDirection)
    {
        target = nlohmann::json::json_pointer("/Ethernet/LLDPTransmit");
    }
    else
    {
        BMCWEB_LOG_WARNING("An LLDP frame went a way this does not know of");
        return;
    }

    nlohmann::json& frame = asyncResp->res.jsonValue[target];

    // A field the frame did not carry is reported as null rather than as an
    // empty string, which would claim the peer sent an empty value.
    auto reportText =
        [&frame](const char* name, const std::optional<std::string>& value) {
            if (value)
            {
                frame[name] = value->empty() ? nlohmann::json(nullptr)
                                             : nlohmann::json(*value);
            }
        };

    reportText("ChassisId", chassisId);
    reportText("PortId", portId);
    reportText("SystemName", systemName);
    reportText("SystemDescription", systemDescription);
    reportText("ManagementAddressIPv4", managementAddressIPv4);
    reportText("ManagementAddressIPv6", managementAddressIPv6);
    reportText("ManagementAddressMAC", managementAddressMAC);

    // The schema allows the twelve bits a VLAN identifier is, so anything
    // wider is either the value that means there is none or a peer saying
    // something a VLAN identifier cannot say.
    constexpr uint64_t highestVlanId = 4095;
    if (managementVlanId)
    {
        frame["ManagementVlanId"] = *managementVlanId > highestVlanId
                                        ? nlohmann::json(nullptr)
                                        : nlohmann::json(*managementVlanId);
    }

    reportSubtype(frame, "ChassisIdSubtype", chassisIdSubtype);
    reportSubtype(frame, "PortIdSubtype", portIdSubtype);

    // The schema says this property is absent when the peer sent no
    // capabilities, and that it never holds None. A peer that sent the field
    // but claimed nothing in it therefore reads the same as one that sent no
    // field at all, and a name this build cannot place is dropped rather than
    // reported as something the schema does not define.
    if (systemCapabilities)
    {
        nlohmann::json::array_t capabilities;
        for (const std::string& capability : *systemCapabilities)
        {
            const port::LLDPSystemCapabilities named =
                toSystemCapability(capability);
            if (named == port::LLDPSystemCapabilities::None ||
                named == port::LLDPSystemCapabilities::Invalid)
            {
                continue;
            }
            capabilities.emplace_back(named);
        }
        if (!capabilities.empty())
        {
            frame["SystemCapabilities"] = std::move(capabilities);
        }
    }
}

inline void afterGetLldpFramePaths(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& object)
{
    // A port whose device does not report what it discovers has no such
    // object; the resource says nothing about it rather than failing.
    if (ec)
    {
        return;
    }

    for (const auto& [path, services] : object)
    {
        if (services.empty())
        {
            continue;
        }

        dbus::utility::getAllProperties(
            services.front().first, path,
            "xyz.openbmc_project.Network.LLDP.TLVs",
            std::bind_front(afterGetLldpFrame, asyncResp));
    }
}

// What a port discovers is not part of the inventory either, so it is reached
// from the port it describes.
inline void getPortLldpFrames(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& portPath)
{
    dbus::utility::getAssociatedSubTree(
        sdbusplus::object_path{portPath} / "monitored_by",
        sdbusplus::object_path{"/xyz/openbmc_project/network/lldp"}, 0,
        lldpTlvsInterface, std::bind_front(afterGetLldpFramePaths, asyncResp));
}

inline void handleNetworkAdapterPathPortGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassiId, const std::string& networkAdapterId,
    const std::string& portId, const std::string& adapterPath,
    const std::string& portPath,
    [[maybe_unused]] const std::string& serviceName)
{
    asyncResp->res.jsonValue["@odata.type"] = "#Port.v1_9_0.Port";
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

    // The device runs one agent for all of its ports, so every port of an
    // adapter reports the same answer here.
    getLldpEnabled(asyncResp, adapterPath,
                   nlohmann::json::json_pointer("/Ethernet/LLDPEnabled"));
    getPortLldpFrames(asyncResp, portPath);
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
    const std::string& path)
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

    getLldpEnabled(asyncResp, path,
                   nlohmann::json::json_pointer("/LLDPEnabled"));
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
        [asyncResp, chassisId, networkAdapterId,
         portId](const std::string& adapterPath) {
            getAssociatedPortPath(
                asyncResp, portId,
                std::bind_front(handleNetworkAdapterPathPortGet, asyncResp,
                                chassisId, networkAdapterId, portId,
                                adapterPath),
                adapterPath);
        });
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
