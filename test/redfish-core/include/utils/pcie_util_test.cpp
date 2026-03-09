// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "generated/enums/pcie_function.hpp"
#include "utils/pcie_util.hpp"

#include <optional>

#include <gtest/gtest.h>

namespace redfish::pcie_util
{
namespace
{

TEST(PcieUtil, DeviceClassFromDbus)
{
    EXPECT_EQ(
        dbusDeviceClassToRf(
            "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
            ".Bridge"),
        pcie_function::DeviceClass::Bridge);
    EXPECT_EQ(
        dbusDeviceClassToRf(
            "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
            ".CommunicationController"),
        pcie_function::DeviceClass::CommunicationController);
    EXPECT_EQ(
        dbusDeviceClassToRf(
            "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
            ".Coprocessor"),
        pcie_function::DeviceClass::Coprocessor);
    EXPECT_EQ(
        dbusDeviceClassToRf(
            "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
            ".DisplayController"),
        pcie_function::DeviceClass::DisplayController);
    EXPECT_EQ(
        dbusDeviceClassToRf(
            "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
            ".DockingStation"),
        pcie_function::DeviceClass::DockingStation);
    EXPECT_EQ(
        dbusDeviceClassToRf(
            "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
            ".EncryptionController"),
        pcie_function::DeviceClass::EncryptionController);
    EXPECT_EQ(
        dbusDeviceClassToRf(
            "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
            ".GenericSystemPeripheral"),
        pcie_function::DeviceClass::GenericSystemPeripheral);
    EXPECT_EQ(
        dbusDeviceClassToRf(
            "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
            ".InputDeviceController"),
        pcie_function::DeviceClass::InputDeviceController);
    EXPECT_EQ(
        dbusDeviceClassToRf(
            "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
            ".IntelligentController"),
        pcie_function::DeviceClass::IntelligentController);
    EXPECT_EQ(
        dbusDeviceClassToRf(
            "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
            ".MassStorageController"),
        pcie_function::DeviceClass::MassStorageController);
    EXPECT_EQ(
        dbusDeviceClassToRf(
            "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
            ".MemoryController"),
        pcie_function::DeviceClass::MemoryController);
    EXPECT_EQ(
        dbusDeviceClassToRf(
            "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
            ".MultimediaController"),
        pcie_function::DeviceClass::MultimediaController);
    EXPECT_EQ(
        dbusDeviceClassToRf(
            "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
            ".NetworkController"),
        pcie_function::DeviceClass::NetworkController);
    EXPECT_EQ(
        dbusDeviceClassToRf(
            "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
            ".NonEssentialInstrumentation"),
        pcie_function::DeviceClass::NonEssentialInstrumentation);
    EXPECT_EQ(
        dbusDeviceClassToRf(
            "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
            ".Other"),
        pcie_function::DeviceClass::Other);
    EXPECT_EQ(
        dbusDeviceClassToRf(
            "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
            ".ProcessingAccelerators"),
        pcie_function::DeviceClass::ProcessingAccelerators);
    EXPECT_EQ(
        dbusDeviceClassToRf(
            "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
            ".Processor"),
        pcie_function::DeviceClass::Processor);
    EXPECT_EQ(
        dbusDeviceClassToRf(
            "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
            ".SatelliteCommunicationsController"),
        pcie_function::DeviceClass::SatelliteCommunicationsController);
    EXPECT_EQ(
        dbusDeviceClassToRf(
            "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
            ".SerialBusController"),
        pcie_function::DeviceClass::SerialBusController);
    EXPECT_EQ(
        dbusDeviceClassToRf(
            "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
            ".SignalProcessingController"),
        pcie_function::DeviceClass::SignalProcessingController);
    EXPECT_EQ(
        dbusDeviceClassToRf(
            "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
            ".UnassignedClass"),
        pcie_function::DeviceClass::UnassignedClass);
    EXPECT_EQ(
        dbusDeviceClassToRf(
            "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
            ".UnclassifiedDevice"),
        pcie_function::DeviceClass::UnclassifiedDevice);
    EXPECT_EQ(
        dbusDeviceClassToRf(
            "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
            ".WirelessController"),
        pcie_function::DeviceClass::WirelessController);
    EXPECT_EQ(
        dbusDeviceClassToRf(
            "xyz.openbmc_project.Inventory.Item.PCIeFunction.DeviceClasses"
            ".Unknown"),
        std::nullopt);
    EXPECT_EQ(dbusDeviceClassToRf("garbage"),
              pcie_function::DeviceClass::Invalid);
}

TEST(PcieUtil, FunctionTypeFromDbus)
{
    EXPECT_EQ(
        dbusFunctionTypeToRf(
            "xyz.openbmc_project.Inventory.Item.PCIeFunction.FunctionTypes"
            ".Physical"),
        pcie_function::FunctionType::Physical);
    EXPECT_EQ(
        dbusFunctionTypeToRf(
            "xyz.openbmc_project.Inventory.Item.PCIeFunction.FunctionTypes"
            ".Virtual"),
        pcie_function::FunctionType::Virtual);
    EXPECT_EQ(
        dbusFunctionTypeToRf(
            "xyz.openbmc_project.Inventory.Item.PCIeFunction.FunctionTypes"
            ".Unknown"),
        std::nullopt);
    EXPECT_EQ(dbusFunctionTypeToRf("garbage"),
              pcie_function::FunctionType::Invalid);
}

} // namespace
} // namespace redfish::pcie_util
