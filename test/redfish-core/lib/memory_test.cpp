// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "generated/enums/memory.hpp"
#include "memory.hpp"

#include <string>
#include <string_view>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

std::string dimmDeviceType(std::string_view suffix)
{
    return std::string("xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.") +
           std::string(suffix);
}

TEST(TranslateMemoryTypeToRedfish, DdrDeviceTypesMap)
{
    EXPECT_EQ(translateMemoryTypeToRedfish(dimmDeviceType("DDR")),
              memory::MemoryDeviceType::DDR);
    EXPECT_EQ(translateMemoryTypeToRedfish(dimmDeviceType("DDR2")),
              memory::MemoryDeviceType::DDR2);
    EXPECT_EQ(translateMemoryTypeToRedfish(dimmDeviceType("DDR3")),
              memory::MemoryDeviceType::DDR3);
    EXPECT_EQ(translateMemoryTypeToRedfish(dimmDeviceType("DDR4")),
              memory::MemoryDeviceType::DDR4);
    EXPECT_EQ(translateMemoryTypeToRedfish(dimmDeviceType("DDR5")),
              memory::MemoryDeviceType::DDR5);
    EXPECT_EQ(translateMemoryTypeToRedfish(dimmDeviceType("DDR4E_SDRAM")),
              memory::MemoryDeviceType::DDR4E_SDRAM);
    EXPECT_EQ(translateMemoryTypeToRedfish(dimmDeviceType("DDR_SGRAM")),
              memory::MemoryDeviceType::DDR_SGRAM);
}

TEST(TranslateMemoryTypeToRedfish, LowPowerAndFbDimmDeviceTypesMap)
{
    EXPECT_EQ(translateMemoryTypeToRedfish(dimmDeviceType("LPDDR3_SDRAM")),
              memory::MemoryDeviceType::LPDDR3_SDRAM);
    EXPECT_EQ(translateMemoryTypeToRedfish(dimmDeviceType("LPDDR4_SDRAM")),
              memory::MemoryDeviceType::LPDDR4_SDRAM);
    EXPECT_EQ(
        translateMemoryTypeToRedfish(dimmDeviceType("DDR2_SDRAM_FB_DIMM")),
        memory::MemoryDeviceType::DDR2_SDRAM_FB_DIMM);
}

TEST(TranslateMemoryTypeToRedfish, FbDimmProbMapsToProbeEnumerator)
{
    EXPECT_EQ(
        translateMemoryTypeToRedfish(dimmDeviceType("DDR2_SDRAM_FB_DIMM_PROB")),
        memory::MemoryDeviceType::DDR2_SDRAM_FB_DIMM_PROBE);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  dimmDeviceType("DDR2_SDRAM_FB_DIMM_PROBE")),
              memory::MemoryDeviceType::Invalid);
}

TEST(TranslateMemoryTypeToRedfish, LegacyDeviceTypesMap)
{
    EXPECT_EQ(translateMemoryTypeToRedfish(dimmDeviceType("ROM")),
              memory::MemoryDeviceType::ROM);
    EXPECT_EQ(translateMemoryTypeToRedfish(dimmDeviceType("SDRAM")),
              memory::MemoryDeviceType::SDRAM);
    EXPECT_EQ(translateMemoryTypeToRedfish(dimmDeviceType("EDO")),
              memory::MemoryDeviceType::EDO);
    EXPECT_EQ(translateMemoryTypeToRedfish(dimmDeviceType("FastPageMode")),
              memory::MemoryDeviceType::FastPageMode);
    EXPECT_EQ(translateMemoryTypeToRedfish(dimmDeviceType("PipelinedNibble")),
              memory::MemoryDeviceType::PipelinedNibble);
    EXPECT_EQ(translateMemoryTypeToRedfish(dimmDeviceType("Logical")),
              memory::MemoryDeviceType::Logical);
}

TEST(TranslateMemoryTypeToRedfish, HbmDeviceTypesMap)
{
    EXPECT_EQ(translateMemoryTypeToRedfish(dimmDeviceType("HBM")),
              memory::MemoryDeviceType::HBM);
    EXPECT_EQ(translateMemoryTypeToRedfish(dimmDeviceType("HBM2")),
              memory::MemoryDeviceType::HBM2);
    EXPECT_EQ(translateMemoryTypeToRedfish(dimmDeviceType("HBM3")),
              memory::MemoryDeviceType::HBM3);
}

TEST(TranslateMemoryTypeToRedfish, RedfishNamesWithoutABranchReturnInvalid)
{
    EXPECT_EQ(translateMemoryTypeToRedfish(dimmDeviceType("HBM")),
              memory::MemoryDeviceType::HBM);

    for (std::string_view suffix :
         {"DDR_SDRAM", "DDR2_SDRAM", "DDR3_SDRAM", "DDR4_SDRAM", "LPDDR5_SDRAM",
          "DDR5_MRDIMM", "HBM2E", "HBM3E", "HBM4", "GDDR", "GDDR2", "GDDR3",
          "GDDR4", "GDDR5", "GDDR5X", "GDDR6", "GDDR7", "OEM"})
    {
        EXPECT_EQ(translateMemoryTypeToRedfish(dimmDeviceType(suffix)),
                  memory::MemoryDeviceType::Invalid)
            << "suffix " << suffix;
    }
}

TEST(TranslateMemoryTypeToRedfish, UnhandledDbusDeviceTypesReturnInvalid)
{
    EXPECT_EQ(translateMemoryTypeToRedfish(dimmDeviceType("DDR")),
              memory::MemoryDeviceType::DDR);

    for (std::string_view suffix :
         {"Unknown", "Other", "DRAM", "EDRAM", "VRAM", "SRAM", "RAM", "FLASH",
          "EEPROM", "FEPROM", "EPROM", "CDRAM", "ThreeDRAM", "RDRAM", "FBD2",
          "LPDDR_SDRAM", "LPDDR2_SDRAM"})
    {
        EXPECT_EQ(translateMemoryTypeToRedfish(dimmDeviceType(suffix)),
                  memory::MemoryDeviceType::Invalid)
            << "suffix " << suffix;
    }
}

TEST(TranslateMemoryTypeToRedfish, MatchIsExactWholeString)
{
    EXPECT_EQ(translateMemoryTypeToRedfish(dimmDeviceType("DDR4")),
              memory::MemoryDeviceType::DDR4);

    EXPECT_EQ(translateMemoryTypeToRedfish(""),
              memory::MemoryDeviceType::Invalid);
    EXPECT_EQ(translateMemoryTypeToRedfish("DDR4"),
              memory::MemoryDeviceType::Invalid);
    EXPECT_EQ(translateMemoryTypeToRedfish(dimmDeviceType("")),
              memory::MemoryDeviceType::Invalid);
    EXPECT_EQ(translateMemoryTypeToRedfish(dimmDeviceType("ddr4")),
              memory::MemoryDeviceType::Invalid);
    EXPECT_EQ(translateMemoryTypeToRedfish(dimmDeviceType("DDR4 ")),
              memory::MemoryDeviceType::Invalid);
    EXPECT_EQ(translateMemoryTypeToRedfish(dimmDeviceType(" DDR4")),
              memory::MemoryDeviceType::Invalid);
    EXPECT_EQ(translateMemoryTypeToRedfish(dimmDeviceType("DDR4X")),
              memory::MemoryDeviceType::Invalid);
}

} // namespace
} // namespace redfish
