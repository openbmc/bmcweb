#include "utils/location_utils.hpp"

#include <gtest/gtest.h>

namespace redfish::location_utils
{
namespace
{

TEST(LocationUtilsTest, GetLocationTypeTest)
{
    // Test Embedded type
    EXPECT_EQ(location_utils::getLocationType(
                  "xyz.openbmc_project.Inventory.Connector.Embedded"),
              resource::LocationType::Embedded);

    // Test Slot type
    EXPECT_EQ(location_utils::getLocationType(
                  "xyz.openbmc_project.Inventory.Connector.Slot"),
              resource::LocationType::Slot);

    // Test invalid type
    EXPECT_EQ(location_utils::getLocationType(
                  "xyz.openbmc_project.Inventory.Connector.Port"),
              resource::LocationType::Invalid);

    // Test empty string
    EXPECT_EQ(location_utils::getLocationType(""),
              resource::LocationType::Invalid);
}

} // namespace
} // namespace redfish::location_utils
