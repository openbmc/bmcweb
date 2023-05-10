#include "privileges.hpp"

#include <boost/beast/http/verb.hpp>

#include <array>

#include <gmock/gmock.h> // IWYU pragma: keep
#include <gtest/gtest.h> // IWYU pragma: keep

// IWYU pragma: no_include <gtest/gtest-message.h>
// IWYU pragma: no_include <gtest/gtest-test-part.h>
// IWYU pragma: no_include "gtest/gtest_pred_impl.h"
// IWYU pragma: no_include <gmock/gmock-matchers.h>
// IWYU pragma: no_include <gmock/gmock-more-matchers.h>

namespace redfish
{
namespace
{

using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

TEST(PrivilegeTest, PrivilegeConstructor)
{
    Privileges privileges{"Login", "ConfigureManager"};

    EXPECT_THAT(privileges.getActivePrivilegeNames(PrivilegeType::BASE),
                UnorderedElementsAre("Login", "ConfigureManager"));
}

TEST(PrivilegeTest, PrivilegeCheckForNoPrivilegesRequired)
{
    Privileges userPrivileges{"Login"};

    OperationMap entityPrivileges{{boost::beast::http::verb::get, {{"Login"}}}};

    EXPECT_TRUE(isMethodAllowedWithPrivileges(
        boost::beast::http::verb::get, entityPrivileges, userPrivileges));
}

TEST(PrivilegeTest, PrivilegeCheckForSingleCaseSuccess)
{
    auto userPrivileges = Privileges{"Login"};
    OperationMap entityPrivileges{{boost::beast::http::verb::get, {}}};

    EXPECT_TRUE(isMethodAllowedWithPrivileges(
        boost::beast::http::verb::get, entityPrivileges, userPrivileges));
}

TEST(PrivilegeTest, PrivilegeCheckForSingleCaseFailure)
{
    auto userPrivileges = Privileges{"Login"};
    OperationMap entityPrivileges{
        {boost::beast::http::verb::get, {{"ConfigureManager"}}}};

    EXPECT_FALSE(isMethodAllowedWithPrivileges(
        boost::beast::http::verb::get, entityPrivileges, userPrivileges));
}

TEST(PrivilegeTest, PrivilegeCheckForANDCaseSuccess)
{
    auto userPrivileges = Privileges{"Login", "ConfigureManager",
                                     "ConfigureSelf"};
    OperationMap entityPrivileges{
        {boost::beast::http::verb::get,
         {{"Login", "ConfigureManager", "ConfigureSelf"}}}};

    EXPECT_TRUE(isMethodAllowedWithPrivileges(
        boost::beast::http::verb::get, entityPrivileges, userPrivileges));
}

TEST(PrivilegeTest, PrivilegeCheckForANDCaseFailure)
{
    auto userPrivileges = Privileges{"Login", "ConfigureManager"};
    OperationMap entityPrivileges{
        {boost::beast::http::verb::get,
         {{"Login", "ConfigureManager", "ConfigureSelf"}}}};

    EXPECT_FALSE(isMethodAllowedWithPrivileges(
        boost::beast::http::verb::get, entityPrivileges, userPrivileges));
}

TEST(PrivilegeTest, PrivilegeCheckForORCaseSuccess)
{
    auto userPrivileges = Privileges{"ConfigureManager"};
    OperationMap entityPrivileges{
        {boost::beast::http::verb::get, {{"Login"}, {"ConfigureManager"}}}};

    EXPECT_TRUE(isMethodAllowedWithPrivileges(
        boost::beast::http::verb::get, entityPrivileges, userPrivileges));
}

TEST(PrivilegeTest, PrivilegeCheckForORCaseFailure)
{
    auto userPrivileges = Privileges{"ConfigureComponents"};
    OperationMap entityPrivileges = OperationMap(
        {{boost::beast::http::verb::get, {{"Login"}, {"ConfigureManager"}}}});

    EXPECT_FALSE(isMethodAllowedWithPrivileges(
        boost::beast::http::verb::get, entityPrivileges, userPrivileges));
}

TEST(PrivilegeTest, DefaultPrivilegeBitsetsAreEmpty)
{
    Privileges privileges;

    EXPECT_THAT(privileges.getActivePrivilegeNames(PrivilegeType::BASE),
                IsEmpty());

    EXPECT_THAT(privileges.getActivePrivilegeNames(PrivilegeType::OEM),
                IsEmpty());
}

TEST(PrivilegeTest, GetActivePrivilegeNames)
{
    Privileges privileges;

    EXPECT_THAT(privileges.getActivePrivilegeNames(PrivilegeType::BASE),
                IsEmpty());

    std::array<const char*, 5> expectedPrivileges{
        "Login", "ConfigureManager", "ConfigureUsers", "ConfigureComponents",
        "ConfigureSelf"};

    for (const auto& privilege : expectedPrivileges)
    {
        EXPECT_TRUE(privileges.setSinglePrivilege(privilege));
    }

    EXPECT_THAT(
        privileges.getActivePrivilegeNames(PrivilegeType::BASE),
        UnorderedElementsAre(expectedPrivileges[0], expectedPrivileges[1],
                             expectedPrivileges[2], expectedPrivileges[3],
                             expectedPrivileges[4]));
}
} // namespace
} // namespace redfish
