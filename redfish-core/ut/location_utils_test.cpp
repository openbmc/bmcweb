#include "utils/location_utils.hpp"

#include "gtest/gtest.h"

using redfish::location_util::getLocationType;
using redfish::location_util::isConnector;

TEST(LocationUtility, getLocationTypeValid)
{
    EXPECT_EQ(*getLocationType("xyz.openbmc_project.Inventory.Connector.Slot"),
              "Slot");
    EXPECT_EQ(
        *getLocationType("xyz.openbmc_project.Inventory.Connector.Embedded"),
        "Embedded");
}

TEST(LocationUtility, getLocationTypeInvalid)
{
    EXPECT_EQ(getLocationType(""), std::nullopt);
    EXPECT_EQ(getLocationType("xyz.openbmc_project.Inventory.Connector.RANDOM"),
              std::nullopt);
    EXPECT_EQ(getLocationType("xyz.openbmc_project.Inventory.Connector"),
              std::nullopt);
    EXPECT_EQ(getLocationType("xyz.openbmc_project.Inventory."), std::nullopt);
    EXPECT_EQ(getLocationType("xyz.openbmc_project.Item.Connector"),
              std::nullopt);
}

TEST(LocationUtility, isConnector)
{
    EXPECT_TRUE(isConnector("xyz.openbmc_project.Inventory.Connector.Slot"));
    EXPECT_TRUE(
        isConnector("xyz.openbmc_project.Inventory.Connector.Embedded"));

    EXPECT_FALSE(isConnector(""));
    EXPECT_FALSE(isConnector("xyz.openbmc_project.Inventory.Connector.RANDOM"));
    EXPECT_FALSE(isConnector("xyz.openbmc_project.Inventory.Connector"));
    EXPECT_FALSE(isConnector("xyz.openbmc_project.Inventory."));
    EXPECT_FALSE(isConnector("xyz.openbmc_project.Item.Connector"));
}
