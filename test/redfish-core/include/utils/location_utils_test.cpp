#include "utils/location_utils.hpp"

#include <optional>

#include "gtest/gtest.h"

namespace redfish::location_util
{
namespace
{

TEST(LocationUtility, ValidLocationType)
{
    EXPECT_EQ(getLocationType("xyz.openbmc_project.Inventory.Connector.Slot"),
              "Slot");
    EXPECT_EQ(
        getLocationType("xyz.openbmc_project.Inventory.Connector.Embedded"),
        "Embedded");
}

TEST(LocationUtility, InvalidLocationType)
{
    EXPECT_EQ(getLocationType(""), std::nullopt);
    EXPECT_EQ(getLocationType("xyz.openbmc_project.Inventory.Connector.RANDOM"),
              std::nullopt);
    EXPECT_EQ(getLocationType("xyz.openbmc_project.Inventory.Connector"),
              std::nullopt);
    EXPECT_EQ(getLocationType("xyz.openbmc_project.Inventory."), std::nullopt);
    EXPECT_EQ(getLocationType("xyz.openbmc_project.Item.Connector"),
              std::nullopt);
    EXPECT_EQ(getLocationType("xyz.openbmc_project.Inventory.Connector.BAY"),
              std::nullopt);
    EXPECT_EQ(getLocationType("xyz.openbmc_project.Inventory.Connector.Bay2"),
              std::nullopt);
}

} // namespace
} // namespace redfish::location_util
