#include <boost/algorithm/string.hpp>
#include <boost/container/flat_set.hpp>
#include <dbus_singleton.hpp>
#include <dbus_utility.hpp>

#include "gmock/gmock.h"

TEST(DbusUtility, getNthStringFromPathGoodTest)
{
    std::string path("/0th/1st/2nd/3rd");
    std::string result;
    EXPECT_TRUE(dbus::utility::getNthStringFromPath(path, 0, result));
    EXPECT_EQ(result, "0th");
    EXPECT_TRUE(dbus::utility::getNthStringFromPath(path, 1, result));
    EXPECT_EQ(result, "1st");
    EXPECT_TRUE(dbus::utility::getNthStringFromPath(path, 2, result));
    EXPECT_EQ(result, "2nd");
    EXPECT_TRUE(dbus::utility::getNthStringFromPath(path, 3, result));
    EXPECT_EQ(result, "3rd");
    EXPECT_FALSE(dbus::utility::getNthStringFromPath(path, 4, result));
}

TEST(DbusUtility, getNthStringFromPathBadTest)
{
    std::string path("////0th///1st//\2nd///3rd?/");
    std::string result;
    EXPECT_TRUE(dbus::utility::getNthStringFromPath(path, 0, result));
    EXPECT_EQ(result, "0th");
    EXPECT_TRUE(dbus::utility::getNthStringFromPath(path, 1, result));
    EXPECT_EQ(result, "1st");
    EXPECT_TRUE(dbus::utility::getNthStringFromPath(path, 2, result));
    EXPECT_EQ(result, "\2nd");
    EXPECT_TRUE(dbus::utility::getNthStringFromPath(path, 3, result));
    EXPECT_EQ(result, "3rd?");
    EXPECT_FALSE(dbus::utility::getNthStringFromPath(path, -1, result));
}

TEST(DbusUtility, getResourceName)
{
    EXPECT_EQ(dbus::utility::getResourceName("hello_world"), "hello_world");
    EXPECT_EQ(dbus::utility::getResourceName("hello_world_123c"),
              "hello_world_123c");
    EXPECT_EQ(dbus::utility::getResourceName("hello_world_1a23"),
              "hello_world_1a23");
    EXPECT_EQ(dbus::utility::getResourceName("hello_world_"), "hello_world_");
    EXPECT_EQ(dbus::utility::getResourceName("hello_world_123"), "hello_world");
}
