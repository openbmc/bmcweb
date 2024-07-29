#include "account_service.hpp"

#include <string>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(AccountServiceTest, isGroupNameValid)
{
    EXPECT_TRUE(isGroupNameValid("abc"));
    EXPECT_TRUE(isGroupNameValid("123"));
    EXPECT_TRUE(isGroupNameValid(" abc"));
    EXPECT_TRUE(isGroupNameValid("abc "));
    EXPECT_TRUE(isGroupNameValid("a b c"));
    EXPECT_TRUE(isGroupNameValid("!@#"));

    EXPECT_FALSE(isGroupNameValid("   "));
    EXPECT_FALSE(isGroupNameValid("\t"));
    EXPECT_FALSE(isGroupNameValid("\n"));
    EXPECT_FALSE(isGroupNameValid(" \t\n"));
}

} // namespace
} // namespace redfish
