#include "privileges.hpp"

#include <boost/beast/http/verb.hpp>
#include <nlohmann/json.hpp>

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

TEST(PrivilegeTest, PrivilegeCheckForSingleCaseSuccess)
{
    Privileges userPrivileges{"Login"};

    std::vector<Privileges> requiredPrivileges{{"Login"}};

    EXPECT_TRUE(
        isOperationAllowedWithPrivileges(requiredPrivileges, userPrivileges));
}

TEST(PrivilegeTest, PrivilegeCheckForNoPrivilegesRequired)
{
    Privileges userPrivileges{"Login"};
    std::vector<Privileges> requiredPrivileges{};

    EXPECT_TRUE(
        isOperationAllowedWithPrivileges(requiredPrivileges, userPrivileges));
}

TEST(PrivilegeTest, PrivilegeCheckForSingleCaseFailure)
{
    Privileges userPrivileges{"Login"};
    std::vector<Privileges> requiredPrivileges{{"ConfigureManager"}};

    EXPECT_FALSE(
        isOperationAllowedWithPrivileges(requiredPrivileges, userPrivileges));
}

TEST(PrivilegeTest, PrivilegeCheckForANDCaseSuccess)
{
    Privileges userPrivileges{"Login", "ConfigureManager", "ConfigureSelf"};
    std::vector<Privileges> requiredPrivileges{
        {"Login", "ConfigureManager", "ConfigureSelf"}};

    EXPECT_TRUE(
        isOperationAllowedWithPrivileges(requiredPrivileges, userPrivileges));
}

TEST(PrivilegeTest, PrivilegeCheckForANDCaseFailure)
{
    Privileges userPrivileges{"Login", "ConfigureManager"};
    std::vector<Privileges> requiredPrivileges{
        {"Login", "ConfigureManager", "ConfigureSelf"}};

    EXPECT_FALSE(
        isOperationAllowedWithPrivileges(requiredPrivileges, userPrivileges));
}

TEST(PrivilegeTest, PrivilegeCheckForORCaseSuccess)
{
    Privileges userPrivileges{"ConfigureManager"};
    std::vector<Privileges> requiredPrivileges{{"Login"}, {"ConfigureManager"}};

    EXPECT_TRUE(
        isOperationAllowedWithPrivileges(requiredPrivileges, userPrivileges));
}

TEST(PrivilegeTest, PrivilegeCheckForORCaseFailure)
{
    Privileges userPrivileges{"ConfigureComponents"};
    std::vector<Privileges> requiredPrivileges{{"Login"}, {"ConfigureManager"}};

    EXPECT_FALSE(
        isOperationAllowedWithPrivileges(requiredPrivileges, userPrivileges));
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