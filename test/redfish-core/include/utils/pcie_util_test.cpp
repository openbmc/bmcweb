// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "generated/enums/pcie_slots.hpp"
#include "utils/pcie_util.hpp"

#include <optional>

#include <gtest/gtest.h>

namespace redfish::pcie_util
{
namespace
{

TEST(PcieUtil, SlotTypeFromDbus)
{
    EXPECT_EQ(dbusSlotTypeToRf(
                  "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes."
                  "FullLength"),
              pcie_slots::SlotTypes::FullLength);
    EXPECT_EQ(dbusSlotTypeToRf(
                  "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes."
                  "HalfLength"),
              pcie_slots::SlotTypes::HalfLength);
    EXPECT_EQ(dbusSlotTypeToRf(
                  "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes."
                  "LowProfile"),
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
    EXPECT_EQ(dbusSlotTypeToRf(
                  "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes."
                  "OCP3Small"),
              pcie_slots::SlotTypes::OCP3Small);
    EXPECT_EQ(dbusSlotTypeToRf(
                  "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes."
                  "OCP3Large"),
              pcie_slots::SlotTypes::OCP3Large);
    EXPECT_EQ(dbusSlotTypeToRf(
                  "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.U_2"),
              pcie_slots::SlotTypes::U2);
}

TEST(PcieUtil, UnsetSlotTypeIsOmitted)
{
    EXPECT_EQ(dbusSlotTypeToRf(""), std::nullopt);
    EXPECT_EQ(
        dbusSlotTypeToRf(
            "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.Unknown"),
        std::nullopt);
}

TEST(PcieUtil, UnrecognizedSlotTypeIsInvalid)
{
    EXPECT_EQ(
        dbusSlotTypeToRf(
            "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.RANDOM"),
        pcie_slots::SlotTypes::Invalid);
    EXPECT_EQ(dbusSlotTypeToRf("FullLength"), pcie_slots::SlotTypes::Invalid);
}

} // namespace
} // namespace redfish::pcie_util
