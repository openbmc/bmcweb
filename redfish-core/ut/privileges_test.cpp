#include "nlohmann/json.hpp"
#include "privileges.hpp"

#include <fstream>
#include <string>

#include "gmock/gmock.h"

TEST(PrivilegeTest, PrivilegeConstructor)
{
    redfish::Privileges privileges{"Login", "ConfigureManager"};

    EXPECT_THAT(
        privileges.getActivePrivilegeNames(redfish::PrivilegeType::BASE),
        ::testing::UnorderedElementsAre("Login", "ConfigureManager"));
}

TEST(PrivilegeTest, PrivilegeCheckForNoredfish::PrivilegesRequired)
{
    redfish::Privileges userredfish::Privileges{"Login"};

    redfish::OperationMap entityredfish::Privileges{
        {boost::beast::http::verb::get, {{"Login"}}}};

    EXPECT_TRUE(isMethodAllowedWithredfish::Privileges(
        boost::beast::http::verb::get, entityredfish::Privileges,
        userredfish::Privileges));
}

TEST(PrivilegeTest, PrivilegeCheckForSingleCaseSuccess)
{
    auto userredfish::Privileges = redfish::Privileges{"Login"};
    redfish::OperationMap entityredfish::Privileges{
        {boost::beast::http::verb::get, {}}};

    EXPECT_TRUE(isMethodAllowedWithredfish::Privileges(
        boost::beast::http::verb::get, entityredfish::Privileges,
        userredfish::Privileges));
}

TEST(PrivilegeTest, PrivilegeCheckForSingleCaseFailure)
{
    auto userredfish::Privileges = redfish::Privileges{"Login"};
    redfish::OperationMap entityredfish::Privileges{
        {boost::beast::http::verb::get, {{"ConfigureManager"}}}};

    EXPECT_FALSE(isMethodAllowedWithredfish::Privileges(
        boost::beast::http::verb::get, entityredfish::Privileges,
        userredfish::Privileges));
}

TEST(PrivilegeTest, PrivilegeCheckForANDCaseSuccess)
{
    auto userredfish::Privileges =
        redfish::Privileges{"Login", "ConfigureManager", "ConfigureSelf"};
    redfish::OperationMap entityredfish::Privileges{
        {boost::beast::http::verb::get,
         {{"Login", "ConfigureManager", "ConfigureSelf"}}}};

    EXPECT_TRUE(isMethodAllowedWithredfish::Privileges(
        boost::beast::http::verb::get, entityredfish::Privileges,
        userredfish::Privileges));
}

TEST(PrivilegeTest, PrivilegeCheckForANDCaseFailure)
{
    auto userredfish::Privileges =
        redfish::Privileges{"Login", "ConfigureManager"};
    redfish::OperationMap entityredfish::Privileges{
        {boost::beast::http::verb::get,
         {{"Login", "ConfigureManager", "ConfigureSelf"}}}};

    EXPECT_FALSE(isMethodAllowedWithredfish::Privileges(
        boost::beast::http::verb::get, entityredfish::Privileges,
        userredfish::Privileges));
}

TEST(PrivilegeTest, PrivilegeCheckForORCaseSuccess)
{
    auto userredfish::Privileges = redfish::Privileges{"ConfigureManager"};
    redfish::OperationMap entityredfish::Privileges{
        {boost::beast::http::verb::get, {{"Login"}, {"ConfigureManager"}}}};

    EXPECT_TRUE(isMethodAllowedWithredfish::Privileges(
        boost::beast::http::verb::get, entityredfish::Privileges,
        userredfish::Privileges));
}

TEST(PrivilegeTest, PrivilegeCheckForORCaseFailure)
{
    auto userredfish::Privileges = redfish::Privileges{"ConfigureComponents"};
    redfish::OperationMap entityredfish::Privileges = redfish::OperationMap(
        {{boost::beast::http::verb::get, {{"Login"}, {"ConfigureManager"}}}});

    EXPECT_FALSE(isMethodAllowedWithredfish::Privileges(
        boost::beast::http::verb::get, entityredfish::Privileges,
        userredfish::Privileges));
}

TEST(PrivilegeTest, DefaultPrivilegeBitsetsAreEmpty)
{
    redfish::Privileges privileges;

    EXPECT_THAT(
        privileges.getActivePrivilegeNames(redfish::PrivilegeType::BASE),
        ::testing::IsEmpty());

    EXPECT_THAT(privileges.getActivePrivilegeNames(redfish::PrivilegeType::OEM),
                ::testing::IsEmpty());
}

TEST(PrivilegeTest, GetActivePrivilegeNames)
{
    redfish::Privileges privileges;

    EXPECT_THAT(
        privileges.getActivePrivilegeNames(redfish::PrivilegeType::BASE),
        ::testing::IsEmpty());

    std::array<const char*, 5> expectedredfish::Privileges{
        "Login", "ConfigureManager", "ConfigureUsers", "ConfigureComponents",
        "ConfigureSelf"};

    for (const auto& privilege : expectedredfish::Privileges)
    {
        EXPECT_TRUE(privileges.setSinglePrivilege(privilege));
    }

    EXPECT_THAT(
        privileges.getActivePrivilegeNames(redfish::PrivilegeType::BASE),
        ::testing::UnorderedElementsAre(
            expectedredfish::Privileges[0], expectedredfish::Privileges[1],
            expectedredfish::Privileges[2], expectedredfish::Privileges[3],
            expectedredfish::Privileges[4]));
}
