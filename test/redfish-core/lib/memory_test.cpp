// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "generated/enums/memory.hpp"
#include "memory.hpp"

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(Memory, TranslateMemoryTypeToRedfish)
{
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.DDR"),
              memory::MemoryDeviceType::DDR);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.DDR2"),
              memory::MemoryDeviceType::DDR2);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.DDR3"),
              memory::MemoryDeviceType::DDR3);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.DDR4"),
              memory::MemoryDeviceType::DDR4);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType."
                  "DDR4E_SDRAM"),
              memory::MemoryDeviceType::DDR4E_SDRAM);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.DDR5"),
              memory::MemoryDeviceType::DDR5);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType."
                  "LPDDR4_SDRAM"),
              memory::MemoryDeviceType::LPDDR4_SDRAM);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType."
                  "LPDDR3_SDRAM"),
              memory::MemoryDeviceType::LPDDR3_SDRAM);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType."
                  "DDR2_SDRAM_FB_DIMM"),
              memory::MemoryDeviceType::DDR2_SDRAM_FB_DIMM);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType."
                  "DDR2_SDRAM_FB_DIMM_PROB"),
              memory::MemoryDeviceType::DDR2_SDRAM_FB_DIMM_PROBE);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType."
                  "DDR_SGRAM"),
              memory::MemoryDeviceType::DDR_SGRAM);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.ROM"),
              memory::MemoryDeviceType::ROM);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.SDRAM"),
              memory::MemoryDeviceType::SDRAM);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.EDO"),
              memory::MemoryDeviceType::EDO);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType."
                  "FastPageMode"),
              memory::MemoryDeviceType::FastPageMode);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType."
                  "PipelinedNibble"),
              memory::MemoryDeviceType::PipelinedNibble);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType."
                  "Logical"),
              memory::MemoryDeviceType::Logical);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.HBM"),
              memory::MemoryDeviceType::HBM);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.HBM2"),
              memory::MemoryDeviceType::HBM2);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.HBM3"),
              memory::MemoryDeviceType::HBM3);
    EXPECT_EQ(translateMemoryTypeToRedfish("invalid"),
              memory::MemoryDeviceType::Invalid);
}

} // namespace
} // namespace redfish
