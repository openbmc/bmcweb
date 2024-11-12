
#include "property_tree.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::ElementsAre;

TEST(PropertyTree, Doubles)
{
    dbus::PropertyTree tree;
    tree.setDoubleValue("/xyz/openbmc_project/sensors/foo", "Value", 1.0);
    EXPECT_EQ(tree.getDoubleValue("/xyz/openbmc_project/sensors/foo", "Value"),
              1.0);
    EXPECT_EQ(
        tree.getDoubleValue("/xyz/openbmc_project/sensors/foo", "NoExist"),
        std::nullopt);
    tree.setDoubleValue("/a/b", "Value", 1.0);
    EXPECT_EQ(tree.getDoubleValue("/a/b", "Value"), 1.0);
}

TEST(PropertyTree, Strings)
{
    dbus::PropertyTree tree;
    tree.setStringValue("/xyz/openbmc_project/sensors/foo", "Value", "MyValue");
    EXPECT_EQ(tree.getStringValue("/xyz/openbmc_project/sensors/foo", "Value"),
              "MyValue");
    EXPECT_EQ(
        tree.getStringValue("/xyz/openbmc_project/sensors/foo", "NoExist"),
        std::nullopt);
    tree.setStringValue("/a/b", "Value", "MyValue");
    EXPECT_EQ(tree.getStringValue("/a/b", "Value"), "MyValue");
}
