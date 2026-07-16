// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "bmcweb_config.h"

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/pcie_device.hpp"
#include "generated/enums/pcie_slots.hpp"
#include "generated/enums/port.hpp"
#include "generated/enums/protocol.hpp"
#include "logging.hpp"
#include "utils/collection.hpp"
#include "utils/dbus_utils.hpp"

#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <boost/url/url.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace redfish
{
namespace pcie_util
{

/**
 * @brief Populate the PCIe Device list from a GetSubTreePaths search of
 *        inventory
 *
 * @param[i,o] asyncResp  Async response object
 * @param[i]   Name   Key to store the list of PCIe devices in asyncResp
 *
 * @return void
 */

inline void getPCIeDeviceList(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const nlohmann::json::json_pointer& jsonKeyName)
{
    static constexpr std::array<std::string_view, 1> pcieDeviceInterface = {
        "xyz.openbmc_project.Inventory.Item.PCIeDevice"};
    const boost::urls::url pcieDeviceUrl = boost::urls::format(
        "/redfish/v1/Systems/{}/PCIeDevices", BMCWEB_REDFISH_SYSTEM_URI_NAME);

    collection_util::getCollectionToKey(
        asyncResp, pcieDeviceUrl, pcieDeviceInterface,
        "/xyz/openbmc_project/inventory", jsonKeyName);
}

inline std::optional<pcie_slots::SlotTypes> dbusSlotTypeToRf(
    const std::string& slotType)
{
    if (slotType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.FullLength")
    {
        return pcie_slots::SlotTypes::FullLength;
    }
    if (slotType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.HalfLength")
    {
        return pcie_slots::SlotTypes::HalfLength;
    }
    if (slotType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.LowProfile")
    {
        return pcie_slots::SlotTypes::LowProfile;
    }
    if (slotType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.Mini")
    {
        return pcie_slots::SlotTypes::Mini;
    }
    if (slotType == "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.M_2")
    {
        return pcie_slots::SlotTypes::M2;
    }
    if (slotType == "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.OEM")
    {
        return pcie_slots::SlotTypes::OEM;
    }
    if (slotType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.OCP3Small")
    {
        return pcie_slots::SlotTypes::OCP3Small;
    }
    if (slotType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.OCP3Large")
    {
        return pcie_slots::SlotTypes::OCP3Large;
    }
    if (slotType == "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.U_2")
    {
        return pcie_slots::SlotTypes::U2;
    }
    if (slotType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.Unknown")
    {
        return std::nullopt;
    }

    return pcie_slots::SlotTypes::Invalid;
}

inline std::optional<pcie_device::PCIeTypes> redfishPcieGenerationFromDbus(
    const std::string& generationInUse)
{
    if (generationInUse ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Gen1")
    {
        return pcie_device::PCIeTypes::Gen1;
    }
    if (generationInUse ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Gen2")
    {
        return pcie_device::PCIeTypes::Gen2;
    }
    if (generationInUse ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Gen3")
    {
        return pcie_device::PCIeTypes::Gen3;
    }
    if (generationInUse ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Gen4")
    {
        return pcie_device::PCIeTypes::Gen4;
    }
    if (generationInUse ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Gen5")
    {
        return pcie_device::PCIeTypes::Gen5;
    }
    if (generationInUse ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Gen6")
    {
        return pcie_device::PCIeTypes::Gen6;
    }
    if (generationInUse.empty() ||
        generationInUse ==
            "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Unknown")
    {
        return std::nullopt;
    }

    return pcie_device::PCIeTypes::Invalid;
}

inline std::optional<pcie_device::DeviceType> redfishPcieDeviceTypeFromDbus(
    const std::string& deviceType)
{
    if (deviceType ==
        "xyz.openbmc_project.Inventory.Item.PCIeDevice.DeviceTypes."
        "SingleFunction")
    {
        return pcie_device::DeviceType::SingleFunction;
    }
    if (deviceType ==
        "xyz.openbmc_project.Inventory.Item.PCIeDevice.DeviceTypes."
        "MultiFunction")
    {
        return pcie_device::DeviceType::MultiFunction;
    }
    if (deviceType ==
        "xyz.openbmc_project.Inventory.Item.PCIeDevice.DeviceTypes.Simulated")
    {
        return pcie_device::DeviceType::Simulated;
    }
    if (deviceType ==
        "xyz.openbmc_project.Inventory.Item.PCIeDevice.DeviceTypes.Retimer")
    {
        return pcie_device::DeviceType::Retimer;
    }
    if (deviceType.empty() ||
        deviceType ==
            "xyz.openbmc_project.Inventory.Item.PCIeDevice.DeviceTypes.Unknown")
    {
        return std::nullopt;
    }

    return pcie_device::DeviceType::Invalid;
}

/**
 * @brief Populate the common PCIe interface generation and lane properties
 *        (PCIeType, MaxPCIeType, LanesInUse, MaxLanes) under the given JSON
 *        pointer from xyz.openbmc_project.Inventory.Item.PCIeDevice properties.
 *
 * Shared by the PCIeDevice PCIeInterface and the Processor SystemInterface.
 *
 * @return false if a property value was invalid (internalError is set on the
 *         response and the caller should stop processing); true otherwise.
 */
inline bool addPcieInterfaceProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const nlohmann::json::json_pointer& jsonPtr,
    const dbus::utility::DBusPropertiesMap& properties)
{
    const std::string* generationInUse = nullptr;
    const std::string* generationSupported = nullptr;
    const size_t* lanesInUse = nullptr;
    const size_t* maxLanes = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "GenerationInUse",
        generationInUse, "GenerationSupported", generationSupported,
        "LanesInUse", lanesInUse, "MaxLanes", maxLanes);

    if (!success)
    {
        messages::internalError(asyncResp->res);
        return false;
    }

    if (generationInUse != nullptr)
    {
        std::optional<pcie_device::PCIeTypes> pcieType =
            redfishPcieGenerationFromDbus(*generationInUse);
        if (!pcieType)
        {
            BMCWEB_LOG_WARNING("Unknown PCIe Generation: {}", *generationInUse);
        }
        else if (*pcieType == pcie_device::PCIeTypes::Invalid)
        {
            BMCWEB_LOG_ERROR("Invalid PCIe Generation: {}", *generationInUse);
            messages::internalError(asyncResp->res);
            return false;
        }
        else
        {
            asyncResp->res.jsonValue[jsonPtr / "PCIeType"] = *pcieType;
        }
    }

    if (generationSupported != nullptr)
    {
        std::optional<pcie_device::PCIeTypes> maxPcieType =
            redfishPcieGenerationFromDbus(*generationSupported);
        if (!maxPcieType)
        {
            BMCWEB_LOG_WARNING("Unknown PCIe Generation: {}",
                               *generationSupported);
        }
        else if (*maxPcieType == pcie_device::PCIeTypes::Invalid)
        {
            BMCWEB_LOG_ERROR("Invalid PCIe Generation: {}",
                             *generationSupported);
            messages::internalError(asyncResp->res);
            return false;
        }
        else
        {
            asyncResp->res.jsonValue[jsonPtr / "MaxPCIeType"] = *maxPcieType;
        }
    }

    if (lanesInUse != nullptr)
    {
        if (*lanesInUse == std::numeric_limits<size_t>::max())
        {
            // The default value of LanesInUse is "maxint", and the field will
            // be null if it is a default value.
            asyncResp->res.jsonValue[jsonPtr / "LanesInUse"] = nullptr;
        }
        else
        {
            asyncResp->res.jsonValue[jsonPtr / "LanesInUse"] = *lanesInUse;
        }
    }

    // The default value of MaxLanes is 0, and the field will be left off if it
    // is a default value.
    if (maxLanes != nullptr && *maxLanes != 0)
    {
        asyncResp->res.jsonValue[jsonPtr / "MaxLanes"] = *maxLanes;
    }

    return true;
}

inline std::optional<protocol::Protocol> dbusPortProtocolToRf(
    const std::string& portProtocol)
{
    if (portProtocol ==
        "xyz.openbmc_project.Inventory.Connector.Port.PortProtocol.PCIe")
    {
        return protocol::Protocol::PCIe;
    }
    if (portProtocol ==
        "xyz.openbmc_project.Inventory.Connector.Port.PortProtocol.NVLink")
    {
        return protocol::Protocol::NVLink;
    }
    if (portProtocol ==
        "xyz.openbmc_project.Inventory.Connector.Port.PortProtocol.Unknown")
    {
        return std::nullopt;
    }
    return protocol::Protocol::Invalid;
}

inline std::optional<port::PortType> dbusPortTypeToRf(
    const std::string& portType)
{
    if (portType ==
        "xyz.openbmc_project.Inventory.Connector.Port.PortType.Upstream")
    {
        return port::PortType::UpstreamPort;
    }
    if (portType ==
        "xyz.openbmc_project.Inventory.Connector.Port.PortType.Downstream")
    {
        return port::PortType::DownstreamPort;
    }
    if (portType ==
        "xyz.openbmc_project.Inventory.Connector.Port.PortType.Bidirectional")
    {
        return port::PortType::BidirectionalPort;
    }
    if (portType ==
        "xyz.openbmc_project.Inventory.Connector.Port.PortType.Unknown")
    {
        return std::nullopt;
    }
    return port::PortType::Invalid;
}

inline std::optional<port::LinkState> dbusPortLinkStateToRf(
    const std::string& linkState)
{
    if (linkState ==
        "xyz.openbmc_project.Inventory.Connector.Port.LinkState.Enabled")
    {
        return port::LinkState::Enabled;
    }
    if (linkState ==
        "xyz.openbmc_project.Inventory.Connector.Port.LinkState.Disabled")
    {
        return port::LinkState::Disabled;
    }
    // Unknown / anything else: omit the property
    return std::nullopt;
}

inline std::optional<port::LinkStatus> dbusPortLinkStatusToRf(
    const std::string& linkStatus)
{
    if (linkStatus ==
        "xyz.openbmc_project.Inventory.Connector.Port.LinkStatus.LinkUp")
    {
        return port::LinkStatus::LinkUp;
    }
    if (linkStatus ==
        "xyz.openbmc_project.Inventory.Connector.Port.LinkStatus.LinkDown")
    {
        return port::LinkStatus::LinkDown;
    }
    if (linkStatus ==
        "xyz.openbmc_project.Inventory.Connector.Port.LinkStatus.NoLink")
    {
        return port::LinkStatus::NoLink;
    }
    if (linkStatus ==
        "xyz.openbmc_project.Inventory.Connector.Port.LinkStatus.Starting")
    {
        return port::LinkStatus::Starting;
    }
    if (linkStatus ==
        "xyz.openbmc_project.Inventory.Connector.Port.LinkStatus.Training")
    {
        return port::LinkStatus::Training;
    }
    // Unknown / anything else: omit the property
    return std::nullopt;
}

/**
 * @brief Populate common Redfish Port properties from the D-Bus
 *        xyz.openbmc_project.Inventory.Connector.Port interface.
 *
 * Shared by Processor Ports and Fabric Switch Ports. Intended to be used
 * directly as the getAllProperties callback via std::bind_front(fn, asyncResp).
 */
inline void afterGetPortProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& properties)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBus error getting port properties: {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }

    const size_t* width = nullptr;
    const uint64_t* speed = nullptr;
    const uint64_t* maxSpeed = nullptr;
    const std::string* portType = nullptr;
    const std::string* portProtocol = nullptr;
    const std::string* linkState = nullptr;
    const std::string* linkStatus = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "Width", width, "Speed",
        speed, "MaxSpeed", maxSpeed, "PortType", portType, "PortProtocol",
        portProtocol, "LinkState", linkState, "LinkStatus", linkStatus);

    if (!success)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    if (width != nullptr && *width != 0 &&
        *width != std::numeric_limits<size_t>::max())
    {
        asyncResp->res.jsonValue["Width"] = *width;
    }

    if (speed != nullptr && *speed != 0 &&
        *speed != std::numeric_limits<uint64_t>::max())
    {
        // D-Bus Speed is bits/s; Gbps is decimal (1 Gbit = 1e9 bits)
        static constexpr double bitsPerGbit = 1e9;
        asyncResp->res.jsonValue["CurrentSpeedGbps"] =
            static_cast<double>(*speed) / bitsPerGbit;
    }

    if (maxSpeed != nullptr && *maxSpeed != 0 &&
        *maxSpeed != std::numeric_limits<uint64_t>::max())
    {
        // D-Bus MaxSpeed is bits/s; Gbps is decimal (1 Gbit = 1e9 bits)
        static constexpr double bitsPerGbit = 1e9;
        asyncResp->res.jsonValue["MaxSpeedGbps"] =
            static_cast<double>(*maxSpeed) / bitsPerGbit;
    }

    if (portType != nullptr)
    {
        std::optional<port::PortType> portTypeEnum =
            dbusPortTypeToRf(*portType);
        if (!portTypeEnum)
        {
            BMCWEB_LOG_WARNING("Unknown Port PortType: {}", *portType);
        }
        else
        {
            if (*portTypeEnum == port::PortType::Invalid)
            {
                BMCWEB_LOG_ERROR("Invalid Port PortType: {}", *portType);
                messages::internalError(asyncResp->res);
                return;
            }
            asyncResp->res.jsonValue["PortType"] = *portTypeEnum;
        }
    }

    if (portProtocol != nullptr)
    {
        std::optional<protocol::Protocol> protocolEnum =
            dbusPortProtocolToRf(*portProtocol);
        if (!protocolEnum)
        {
            BMCWEB_LOG_WARNING("Unknown Port PortProtocol: {}", *portProtocol);
        }
        else
        {
            if (*protocolEnum == protocol::Protocol::Invalid)
            {
                BMCWEB_LOG_ERROR("Invalid Port PortProtocol: {}",
                                 *portProtocol);
                messages::internalError(asyncResp->res);
                return;
            }
            asyncResp->res.jsonValue["PortProtocol"] = *protocolEnum;
        }
    }

    if (linkState != nullptr)
    {
        std::optional<port::LinkState> linkStateEnum =
            dbusPortLinkStateToRf(*linkState);
        if (!linkStateEnum)
        {
            BMCWEB_LOG_WARNING("Unknown Port LinkState: {}", *linkState);
        }
        else
        {
            asyncResp->res.jsonValue["LinkState"] = *linkStateEnum;
        }
    }

    if (linkStatus != nullptr)
    {
        std::optional<port::LinkStatus> linkStatusEnum =
            dbusPortLinkStatusToRf(*linkStatus);
        if (!linkStatusEnum)
        {
            BMCWEB_LOG_WARNING("Unknown Port LinkStatus: {}", *linkStatus);
        }
        else
        {
            asyncResp->res.jsonValue["LinkStatus"] = *linkStatusEnum;
        }
    }
}

} // namespace pcie_util
} // namespace redfish
