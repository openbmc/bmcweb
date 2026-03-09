// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "bmcweb_config.h"

#include "async_resp.hpp"
#include "generated/enums/pcie_device.hpp"
#include "generated/enums/pcie_function.hpp"
#include "generated/enums/pcie_slots.hpp"
#include "utils/collection.hpp"

#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <boost/url/url.hpp>
#include <nlohmann/json.hpp>

#include <array>
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

inline std::optional<pcie_function::DeviceClass> dbusDeviceClassToRf(
    const std::string& deviceClass)
{
    if (deviceClass ==
        "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses.Bridge")
    {
        return pcie_function::DeviceClass::Bridge;
    }
    if (deviceClass ==
        "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
        ".CommunicationController")
    {
        return pcie_function::DeviceClass::CommunicationController;
    }
    if (deviceClass ==
        "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
        ".Coprocessor")
    {
        return pcie_function::DeviceClass::Coprocessor;
    }
    if (deviceClass ==
        "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
        ".DisplayController")
    {
        return pcie_function::DeviceClass::DisplayController;
    }
    if (deviceClass ==
        "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
        ".DockingStation")
    {
        return pcie_function::DeviceClass::DockingStation;
    }
    if (deviceClass ==
        "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
        ".EncryptionController")
    {
        return pcie_function::DeviceClass::EncryptionController;
    }
    if (deviceClass ==
        "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
        ".GenericSystemPeripheral")
    {
        return pcie_function::DeviceClass::GenericSystemPeripheral;
    }
    if (deviceClass ==
        "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
        ".InputDeviceController")
    {
        return pcie_function::DeviceClass::InputDeviceController;
    }
    if (deviceClass ==
        "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
        ".IntelligentController")
    {
        return pcie_function::DeviceClass::IntelligentController;
    }
    if (deviceClass ==
        "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
        ".MassStorageController")
    {
        return pcie_function::DeviceClass::MassStorageController;
    }
    if (deviceClass ==
        "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
        ".MemoryController")
    {
        return pcie_function::DeviceClass::MemoryController;
    }
    if (deviceClass ==
        "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
        ".MultimediaController")
    {
        return pcie_function::DeviceClass::MultimediaController;
    }
    if (deviceClass ==
        "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
        ".NetworkController")
    {
        return pcie_function::DeviceClass::NetworkController;
    }
    if (deviceClass ==
        "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
        ".NonEssentialInstrumentation")
    {
        return pcie_function::DeviceClass::NonEssentialInstrumentation;
    }
    if (deviceClass ==
        "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses.Other")
    {
        return pcie_function::DeviceClass::Other;
    }
    if (deviceClass ==
        "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
        ".ProcessingAccelerators")
    {
        return pcie_function::DeviceClass::ProcessingAccelerators;
    }
    if (deviceClass ==
        "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
        ".Processor")
    {
        return pcie_function::DeviceClass::Processor;
    }
    if (deviceClass ==
        "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
        ".SatelliteCommunicationsController")
    {
        return pcie_function::DeviceClass::SatelliteCommunicationsController;
    }
    if (deviceClass ==
        "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
        ".SerialBusController")
    {
        return pcie_function::DeviceClass::SerialBusController;
    }
    if (deviceClass ==
        "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
        ".SignalProcessingController")
    {
        return pcie_function::DeviceClass::SignalProcessingController;
    }
    if (deviceClass ==
        "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
        ".UnassignedClass")
    {
        return pcie_function::DeviceClass::UnassignedClass;
    }
    if (deviceClass ==
        "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
        ".UnclassifiedDevice")
    {
        return pcie_function::DeviceClass::UnclassifiedDevice;
    }
    if (deviceClass ==
        "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
        ".WirelessController")
    {
        return pcie_function::DeviceClass::WirelessController;
    }
    if (deviceClass ==
        "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
        ".Unknown")
    {
        return std::nullopt;
    }
    return pcie_function::DeviceClass::Invalid;
}

inline std::optional<pcie_function::FunctionType> dbusFunctionTypeToRf(
    const std::string& functionType)
{
    if (functionType ==
        "xyz.openbmc_project.Inventory.Item.PCIeFunction.FunctionTypes"
        ".Physical")
    {
        return pcie_function::FunctionType::Physical;
    }
    if (functionType ==
        "xyz.openbmc_project.Inventory.Item.PCIeFunction.FunctionTypes"
        ".Virtual")
    {
        return pcie_function::FunctionType::Virtual;
    }
    if (functionType ==
        "xyz.openbmc_project.Inventory.Item.PCIeFunction.FunctionTypes"
        ".Unknown")
    {
        return std::nullopt;
    }
    return pcie_function::FunctionType::Invalid;
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
    if (generationInUse.empty() ||
        generationInUse ==
            "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Unknown")
    {
        return std::nullopt;
    }

    return pcie_device::PCIeTypes::Invalid;
}

} // namespace pcie_util
} // namespace redfish
