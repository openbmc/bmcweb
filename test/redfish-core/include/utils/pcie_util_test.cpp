// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "generated/enums/pcie_device.hpp"
#include "generated/enums/pcie_slots.hpp"
#include "utils/pcie_util.hpp"

#include <optional>

#include <gtest/gtest.h>

namespace redfish::pcie_util
{
namespace
{

TEST(DbusSlotTypeToRf, MapsEveryKnownSlotType)
{
    EXPECT_EQ(
        dbusSlotTypeToRf(
            "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.FullLength"),
        pcie_slots::SlotTypes::FullLength);
    EXPECT_EQ(
        dbusSlotTypeToRf(
            "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.HalfLength"),
        pcie_slots::SlotTypes::HalfLength);
    EXPECT_EQ(
        dbusSlotTypeToRf(
            "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.LowProfile"),
        pcie_slots::SlotTypes::LowProfile);
    EXPECT_EQ(dbusSlotTypeToRf(
                  "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.Mini"),
              pcie_slots::SlotTypes::Mini);
    EXPECT_EQ(dbusSlotTypeToRf(
                  "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.M_2"),
              pcie_slots::SlotTypes::M2);
    EXPECT_EQ(dbusSlotTypeToRf(
                  "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.OEM"),
              pcie_slots::SlotTypes::OEM);
    EXPECT_EQ(
        dbusSlotTypeToRf(
            "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.OCP3Small"),
        pcie_slots::SlotTypes::OCP3Small);
    EXPECT_EQ(
        dbusSlotTypeToRf(
            "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.OCP3Large"),
        pcie_slots::SlotTypes::OCP3Large);
    EXPECT_EQ(dbusSlotTypeToRf(
                  "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.U_2"),
              pcie_slots::SlotTypes::U2);
}

TEST(DbusSlotTypeToRf, UnknownIsNullopt)
{
    EXPECT_EQ(
        dbusSlotTypeToRf(
            "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.Unknown"),
        std::nullopt);
}

TEST(DbusSlotTypeToRf, UnrecognizedIsInvalid)
{
    EXPECT_EQ(dbusSlotTypeToRf("NotASlotType"), pcie_slots::SlotTypes::Invalid);
    EXPECT_EQ(dbusSlotTypeToRf(""), pcie_slots::SlotTypes::Invalid);
}

TEST(RedfishPcieGenerationFromDbus, MapsEveryKnownGeneration)
{
    EXPECT_EQ(
        redfishPcieGenerationFromDbus(
            "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Gen1"),
        pcie_device::PCIeTypes::Gen1);
    EXPECT_EQ(
        redfishPcieGenerationFromDbus(
            "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Gen2"),
        pcie_device::PCIeTypes::Gen2);
    EXPECT_EQ(
        redfishPcieGenerationFromDbus(
            "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Gen3"),
        pcie_device::PCIeTypes::Gen3);
    EXPECT_EQ(
        redfishPcieGenerationFromDbus(
            "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Gen4"),
        pcie_device::PCIeTypes::Gen4);
    EXPECT_EQ(
        redfishPcieGenerationFromDbus(
            "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Gen5"),
        pcie_device::PCIeTypes::Gen5);
    EXPECT_EQ(
        redfishPcieGenerationFromDbus(
            "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Gen6"),
        pcie_device::PCIeTypes::Gen6);
}

TEST(RedfishPcieGenerationFromDbus, EmptyAndUnknownAreNullopt)
{
    EXPECT_EQ(redfishPcieGenerationFromDbus(""), std::nullopt);
    EXPECT_EQ(
        redfishPcieGenerationFromDbus(
            "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Unknown"),
        std::nullopt);
}

TEST(RedfishPcieGenerationFromDbus, UnrecognizedIsInvalid)
{
    EXPECT_EQ(redfishPcieGenerationFromDbus("Gen99"),
              pcie_device::PCIeTypes::Invalid);
}

TEST(RedfishPcieDeviceTypeFromDbus, MapsEveryKnownDeviceType)
{
    EXPECT_EQ(
        redfishPcieDeviceTypeFromDbus(
            "xyz.openbmc_project.Inventory.Item.PCIeDevice.DeviceTypes.SingleFunction"),
        pcie_device::DeviceType::SingleFunction);
    EXPECT_EQ(
        redfishPcieDeviceTypeFromDbus(
            "xyz.openbmc_project.Inventory.Item.PCIeDevice.DeviceTypes.MultiFunction"),
        pcie_device::DeviceType::MultiFunction);
    EXPECT_EQ(
        redfishPcieDeviceTypeFromDbus(
            "xyz.openbmc_project.Inventory.Item.PCIeDevice.DeviceTypes.Simulated"),
        pcie_device::DeviceType::Simulated);
    EXPECT_EQ(
        redfishPcieDeviceTypeFromDbus(
            "xyz.openbmc_project.Inventory.Item.PCIeDevice.DeviceTypes.Retimer"),
        pcie_device::DeviceType::Retimer);
}

TEST(RedfishPcieDeviceTypeFromDbus, EmptyAndUnknownAreNullopt)
{
    EXPECT_EQ(redfishPcieDeviceTypeFromDbus(""), std::nullopt);
    EXPECT_EQ(
        redfishPcieDeviceTypeFromDbus(
            "xyz.openbmc_project.Inventory.Item.PCIeDevice.DeviceTypes.Unknown"),
        std::nullopt);
}

TEST(RedfishPcieDeviceTypeFromDbus, UnrecognizedIsInvalid)
{
    EXPECT_EQ(redfishPcieDeviceTypeFromDbus("NotADeviceType"),
              pcie_device::DeviceType::Invalid);
}

} // namespace
} // namespace redfish::pcie_util
