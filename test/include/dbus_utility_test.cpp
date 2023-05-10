#include "dbus_utility.hpp"

#include <string>

#include <gtest/gtest.h> // IWYU pragma: keep

// IWYU pragma: no_include <gtest/gtest-message.h>
// IWYU pragma: no_include <gtest/gtest-test-part.h>
// IWYU pragma: no_include "gtest/gtest_pred_impl.h"

namespace dbus::utility
{
namespace
{

TEST(GetNthStringFromPath, ParsingSucceedsAndReturnsNthArg)
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

    path = "////0th///1st//\2nd///3rd?/";
    EXPECT_TRUE(getNthStringFromPath(path, 0, result));
    EXPECT_EQ(result, "0th");
    EXPECT_TRUE(getNthStringFromPath(path, 1, result));
    EXPECT_EQ(result, "1st");
    EXPECT_TRUE(getNthStringFromPath(path, 2, result));
    EXPECT_EQ(result, "\2nd");
    EXPECT_TRUE(getNthStringFromPath(path, 3, result));
    EXPECT_EQ(result, "3rd?");
}

TEST(GetNthStringFromPath, InvalidIndexReturnsFalse)
{
    std::string path("////0th///1st//\2nd///3rd?/");
    std::string result;
    EXPECT_FALSE(getNthStringFromPath(path, -1, result));
}
} // namespace
} // namespace dbus::utility
