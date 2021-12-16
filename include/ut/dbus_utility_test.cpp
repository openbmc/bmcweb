#include "dbus_utility.hpp"

#include <string>

#include <gtest/gtest.h>

namespace dbus::utility
{
namespace
{

TEST(DbusUtility, getNthStringFromPathGoodTest)
{
    std::string path("/0th/1st/2nd/3rd");
    std::string result;
    EXPECT_TRUE(getNthStringFromPath(path, 0, result));
    EXPECT_EQ(result, "0th");
    EXPECT_TRUE(getNthStringFromPath(path, 1, result));
    EXPECT_EQ(result, "1st");
    EXPECT_TRUE(getNthStringFromPath(path, 2, result));
    EXPECT_EQ(result, "2nd");
    EXPECT_TRUE(getNthStringFromPath(path, 3, result));
    EXPECT_EQ(result, "3rd");
    EXPECT_FALSE(getNthStringFromPath(path, 4, result));
}

TEST(DbusUtility, getNthStringFromPathBadTest)
{
    std::string path("////0th///1st//\2nd///3rd?/");
    std::string result;
    EXPECT_TRUE(getNthStringFromPath(path, 0, result));
    EXPECT_EQ(result, "0th");
    EXPECT_TRUE(getNthStringFromPath(path, 1, result));
    EXPECT_EQ(result, "1st");
    EXPECT_TRUE(getNthStringFromPath(path, 2, result));
    EXPECT_EQ(result, "\2nd");
    EXPECT_TRUE(getNthStringFromPath(path, 3, result));
    EXPECT_EQ(result, "3rd?");
    EXPECT_FALSE(getNthStringFromPath(path, -1, result));
}

TEST(DbusUtility, getResourceName)
{
    EXPECT_EQ(getResourceName("hello_world"), "hello_world");
    EXPECT_EQ(getResourceName("hello_world_123c"), "hello_world_123c");
    EXPECT_EQ(getResourceName("hello_world_1a23"), "hello_world_1a23");
    EXPECT_EQ(getResourceName("hello_world_"), "hello_world_");
    EXPECT_EQ(getResourceName("hello_world_123"), "hello_world");
    EXPECT_EQ(getResourceName("456_123"), "456");
    EXPECT_EQ(getResourceName("_123"), "");
    EXPECT_EQ(getResourceName(""), "");
}
} // namespace
} // namespace dbus::utility
