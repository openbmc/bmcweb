#include "utils/location_utils.hpp"

#include "gtest/gtest.h"

using redfish::location_util::getLocationType;
using redfish::location_util::isConnector;

TEST(LocationUtility, getLocationTypeValid)
{
    EXPECT_EQ(
        getLocationType("xyz.openbmc_project.Inventory.Connector.Backplane"),
        "Backplane");
    EXPECT_EQ(getLocationType("xyz.openbmc_project.Inventory.Connector.Bay"),
              "Bay");
    EXPECT_EQ(
        getLocationType("xyz.openbmc_project.Inventory.Connector.Connector"),
        "Connector");
    EXPECT_EQ(getLocationType("xyz.openbmc_project.Inventory.Connector.Slot"),
              "Slot");
    EXPECT_EQ(
        getLocationType("xyz.openbmc_project.Inventory.Connector.Embedded"),
        "Embedded");
    EXPECT_EQ(getLocationType("xyz.openbmc_project.Inventory.Connector.Socket"),
              "Socket");
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
    EXPECT_EQ(getLocationType("xyz.openbmc_project.Inventory.Connector.BAY"),
              std::nullopt);
    EXPECT_EQ(getLocationType("xyz.openbmc_project.Inventory.Connector.Bay2"),
              std::nullopt);
}

TEST(LocationUtility, isConnector)
{
    EXPECT_TRUE(
        isConnector("xyz.openbmc_project.Inventory.Connector.Backplane"));
    EXPECT_TRUE(isConnector("xyz.openbmc_project.Inventory.Connector.Bay"));
    EXPECT_TRUE(
        isConnector("xyz.openbmc_project.Inventory.Connector.Connector"));
    EXPECT_TRUE(isConnector("xyz.openbmc_project.Inventory.Connector.Slot"));
    EXPECT_TRUE(
        isConnector("xyz.openbmc_project.Inventory.Connector.Embedded"));
    EXPECT_TRUE(isConnector("xyz.openbmc_project.Inventory.Connector.Socket"));

    EXPECT_FALSE(isConnector(""));
    EXPECT_FALSE(isConnector("xyz.openbmc_project.Inventory.Connector.RANDOM"));
    EXPECT_FALSE(isConnector("xyz.openbmc_project.Inventory.Connector"));
    EXPECT_FALSE(isConnector("xyz.openbmc_project.Inventory."));
    EXPECT_FALSE(isConnector("xyz.openbmc_project.Item.Connector"));
}
