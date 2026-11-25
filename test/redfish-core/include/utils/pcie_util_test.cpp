// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "generated/enums/pcie_device.hpp"
#include "generated/enums/pcie_slots.hpp"
#include "utils/pcie_util.hpp"

#include <optional>
#include <string>

#include <gtest/gtest.h>

namespace redfish::pcie_util
{
namespace
{

constexpr const char* slotPrefix =
    "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.";
constexpr const char* genPrefix =
    "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.";
constexpr const char* devPrefix =
    "xyz.openbmc_project.Inventory.Item.PCIeDevice.DeviceTypes.";

TEST(DbusSlotTypeToRf, MapsEveryKnownSlotType)
{
    const std::string p(slotPrefix);
    EXPECT_EQ(dbusSlotTypeToRf(p + "FullLength"),
              pcie_slots::SlotTypes::FullLength);
    EXPECT_EQ(dbusSlotTypeToRf(p + "HalfLength"),
              pcie_slots::SlotTypes::HalfLength);
    EXPECT_EQ(dbusSlotTypeToRf(p + "LowProfile"),
              pcie_slots::SlotTypes::LowProfile);
    EXPECT_EQ(dbusSlotTypeToRf(p + "Mini"), pcie_slots::SlotTypes::Mini);
    EXPECT_EQ(dbusSlotTypeToRf(p + "M_2"), pcie_slots::SlotTypes::M2);
    EXPECT_EQ(dbusSlotTypeToRf(p + "OEM"), pcie_slots::SlotTypes::OEM);
    EXPECT_EQ(dbusSlotTypeToRf(p + "OCP3Small"),
              pcie_slots::SlotTypes::OCP3Small);
    EXPECT_EQ(dbusSlotTypeToRf(p + "OCP3Large"),
              pcie_slots::SlotTypes::OCP3Large);
    EXPECT_EQ(dbusSlotTypeToRf(p + "U_2"), pcie_slots::SlotTypes::U2);
}

TEST(DbusSlotTypeToRf, UnknownIsNullopt)
{
    EXPECT_EQ(dbusSlotTypeToRf(std::string(slotPrefix) + "Unknown"),
              std::nullopt);
}

TEST(DbusSlotTypeToRf, UnrecognizedIsInvalid)
{
    EXPECT_EQ(dbusSlotTypeToRf("NotASlotType"), pcie_slots::SlotTypes::Invalid);
    EXPECT_EQ(dbusSlotTypeToRf(""), pcie_slots::SlotTypes::Invalid);
}

TEST(RedfishPcieGenerationFromDbus, MapsEveryKnownGeneration)
{
    const std::string p(genPrefix);
    EXPECT_EQ(redfishPcieGenerationFromDbus(p + "Gen1"),
              pcie_device::PCIeTypes::Gen1);
    EXPECT_EQ(redfishPcieGenerationFromDbus(p + "Gen2"),
              pcie_device::PCIeTypes::Gen2);
    EXPECT_EQ(redfishPcieGenerationFromDbus(p + "Gen3"),
              pcie_device::PCIeTypes::Gen3);
    EXPECT_EQ(redfishPcieGenerationFromDbus(p + "Gen4"),
              pcie_device::PCIeTypes::Gen4);
    EXPECT_EQ(redfishPcieGenerationFromDbus(p + "Gen5"),
              pcie_device::PCIeTypes::Gen5);
    EXPECT_EQ(redfishPcieGenerationFromDbus(p + "Gen6"),
              pcie_device::PCIeTypes::Gen6);
}

TEST(RedfishPcieGenerationFromDbus, EmptyAndUnknownAreNullopt)
{
    EXPECT_EQ(redfishPcieGenerationFromDbus(""), std::nullopt);
    EXPECT_EQ(redfishPcieGenerationFromDbus(std::string(genPrefix) + "Unknown"),
              std::nullopt);
}

TEST(RedfishPcieGenerationFromDbus, UnrecognizedIsInvalid)
{
    EXPECT_EQ(redfishPcieGenerationFromDbus("Gen99"),
              pcie_device::PCIeTypes::Invalid);
}

TEST(RedfishPcieDeviceTypeFromDbus, MapsEveryKnownDeviceType)
{
    const std::string p(devPrefix);
    EXPECT_EQ(redfishPcieDeviceTypeFromDbus(p + "SingleFunction"),
              pcie_device::DeviceType::SingleFunction);
    EXPECT_EQ(redfishPcieDeviceTypeFromDbus(p + "MultiFunction"),
              pcie_device::DeviceType::MultiFunction);
    EXPECT_EQ(redfishPcieDeviceTypeFromDbus(p + "Simulated"),
              pcie_device::DeviceType::Simulated);
    EXPECT_EQ(redfishPcieDeviceTypeFromDbus(p + "Retimer"),
              pcie_device::DeviceType::Retimer);
}

TEST(RedfishPcieDeviceTypeFromDbus, EmptyAndUnknownAreNullopt)
{
    EXPECT_EQ(redfishPcieDeviceTypeFromDbus(""), std::nullopt);
    EXPECT_EQ(redfishPcieDeviceTypeFromDbus(std::string(devPrefix) + "Unknown"),
              std::nullopt);
}

TEST(RedfishPcieDeviceTypeFromDbus, UnrecognizedIsInvalid)
{
    EXPECT_EQ(redfishPcieDeviceTypeFromDbus("NotADeviceType"),
              pcie_device::DeviceType::Invalid);
}

} // namespace
} // namespace redfish::pcie_util
