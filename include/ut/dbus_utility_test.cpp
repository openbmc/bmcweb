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
} // namespace
} // namespace dbus::utility