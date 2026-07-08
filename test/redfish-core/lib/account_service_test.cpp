#include "account_service.hpp"

#include <boost/beast/http/status.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <optional>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::Eq;
using ::testing::Optional;

namespace redfish
{
namespace
{

using json = nlohmann::json;

TEST(Conversion, PositiveToUint64)
{
    std::optional<uint64_t> passwordExpiration;

    passwordExpiration = passwordExpirationToUint64(nullptr);
    EXPECT_THAT(passwordExpiration, Optional(Eq(0)));

    passwordExpiration = passwordExpirationToUint64("1970-01-01T00:00:00");
    EXPECT_THAT(passwordExpiration, Eq(std::nullopt));

    passwordExpiration = passwordExpirationToUint64("2024-10-17T18:12:04");
    EXPECT_THAT(passwordExpiration, Optional(Eq(1729188724)));

    passwordExpiration = passwordExpirationToUint64("2024-10-17T18:12:04Z");
    EXPECT_THAT(passwordExpiration, Optional(Eq(1729188724)));

    passwordExpiration =
        passwordExpirationToUint64("2024-10-17T18:12:04+03:00");
    EXPECT_THAT(passwordExpiration, Optional(Eq(1729177924)));
}

TEST(Conversion, NegativeToUint64)
{
    EXPECT_EQ(passwordExpirationToUint64("01-02-03"), std::nullopt);

    EXPECT_EQ(passwordExpirationToUint64("1912-02-03"), std::nullopt);

    EXPECT_EQ(passwordExpirationToUint64("2024-10-17T00:00"), std::nullopt);

    EXPECT_EQ(passwordExpirationToUint64("ABC"), std::nullopt);
}

TEST(Conversion, PositiveToJson)
{
    constexpr uint64_t unexpiringPasswordExpiration = 0;

    json value = passwordExpirationToJson(unexpiringPasswordExpiration);
    EXPECT_TRUE(value.is_null());

    value = passwordExpirationToJson(1729188724);
    EXPECT_TRUE(value.is_string());
    EXPECT_EQ(value, R"("2024-10-17T18:12:04+00:00")"_json);
}

TEST(RoleMapping, PrivilegeToRoleId)
{
    EXPECT_EQ(getRoleIdFromPrivilege("priv-admin"), "Administrator");
    EXPECT_EQ(getRoleIdFromPrivilege("priv-user"), "ReadOnly");
    EXPECT_EQ(getRoleIdFromPrivilege("priv-operator"), "Operator");
    EXPECT_EQ(getRoleIdFromPrivilege("priv-unknown"), "");
}

TEST(RoleMapping, RoleIdToPrivilege)
{
    EXPECT_EQ(getPrivilegeFromRoleId("Administrator"), "priv-admin");
    EXPECT_EQ(getPrivilegeFromRoleId("ReadOnly"), "priv-user");
    EXPECT_EQ(getPrivilegeFromRoleId("Operator"), "priv-operator");
    EXPECT_EQ(getPrivilegeFromRoleId("NoSuchRole"), "");
}

TEST(TranslateUserGroup, ValidGroupsPopulateExpectedAccountTypes)
{
    crow::Response res;
    std::vector<std::string> userGroups = {"redfish", "ipmi", "ssh",
                                           "hostconsole", "web"};

    EXPECT_TRUE(translateUserGroup(userGroups, res));

    const nlohmann::json& accountTypes = res.jsonValue["AccountTypes"];
    ASSERT_TRUE(accountTypes.is_array());
    EXPECT_EQ(accountTypes.size(), 5);
    EXPECT_EQ(accountTypes[0], "Redfish");
    EXPECT_EQ(accountTypes[1], "WebUI");
    EXPECT_EQ(accountTypes[2], "IPMI");
    EXPECT_EQ(accountTypes[3], "ManagerConsole");
    EXPECT_EQ(accountTypes[4], "HostConsole");
}

TEST(TranslateUserGroup, InvalidGroupReturnsFalse)
{
    crow::Response res;
    std::vector<std::string> userGroups = {"redfish", "invalid-group"};

    EXPECT_FALSE(translateUserGroup(userGroups, res));
    EXPECT_FALSE(res.jsonValue.contains("AccountTypes"));
}

TEST(GetUserGroupFromAccountType, ValidAccountTypesPopulateExpectedGroups)
{
    crow::Response res;
    std::vector<std::string> accountTypes = {"Redfish", "WebUI", "IPMI",
                                             "ManagerConsole", "HostConsole"};
    std::vector<std::string> userGroups;

    EXPECT_TRUE(getUserGroupFromAccountType(res, accountTypes, userGroups));
    EXPECT_EQ(userGroups.size(), 4);
    EXPECT_EQ(userGroups[0], "ipmi");
    EXPECT_EQ(userGroups[1], "ssh");
    EXPECT_EQ(userGroups[2], "hostconsole");
    EXPECT_EQ(userGroups[3], "redfish");
    EXPECT_FALSE(res.jsonValue.contains("error"));
}

TEST(GetUserGroupFromAccountType,
     UnpairedRedfishWebUiReturnsStrictAccountTypesError)
{
    const nlohmann::json expectedMessage =
        messages::strictAccountTypes("AccountTypes");
    {
        crow::Response res;
        std::vector<std::string> accountTypes = {"Redfish"};
        std::vector<std::string> userGroups;

        EXPECT_FALSE(
            getUserGroupFromAccountType(res, accountTypes, userGroups));
        EXPECT_EQ(userGroups.size(), 0);
        EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
        EXPECT_EQ(res.jsonValue["error"]["code"], expectedMessage["MessageId"]);
        EXPECT_EQ(res.jsonValue["error"]["message"],
                  expectedMessage["Message"]);
        const nlohmann::json& extendedInfo =
            res.jsonValue["error"]["@Message.ExtendedInfo"];
        ASSERT_TRUE(extendedInfo.is_array());
        EXPECT_EQ(extendedInfo.size(), 1);
        EXPECT_EQ(extendedInfo[0], expectedMessage);
    }

    {
        crow::Response res;
        std::vector<std::string> accountTypes = {"WebUI"};
        std::vector<std::string> userGroups;

        EXPECT_FALSE(
            getUserGroupFromAccountType(res, accountTypes, userGroups));
        EXPECT_EQ(userGroups.size(), 0);
        EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
        EXPECT_EQ(res.jsonValue["error"]["code"], expectedMessage["MessageId"]);
        EXPECT_EQ(res.jsonValue["error"]["message"],
                  expectedMessage["Message"]);
        const nlohmann::json& extendedInfo =
            res.jsonValue["error"]["@Message.ExtendedInfo"];
        ASSERT_TRUE(extendedInfo.is_array());
        EXPECT_EQ(extendedInfo.size(), 1);
        EXPECT_EQ(extendedInfo[0], expectedMessage);
    }
}

TEST(GetUserGroupFromAccountType, InvalidAccountTypeReturnsPropertyValueError)
{
    const nlohmann::json expectedMessage =
        messages::propertyValueNotInList("InvalidAccountType", "AccountTypes");
    crow::Response res;
    std::vector<std::string> accountTypes = {"InvalidAccountType"};
    std::vector<std::string> userGroups;

    EXPECT_FALSE(getUserGroupFromAccountType(res, accountTypes, userGroups));
    EXPECT_EQ(userGroups.size(), 0);
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_EQ(res.jsonValue["error"]["code"], expectedMessage["MessageId"]);
    EXPECT_EQ(res.jsonValue["error"]["message"], expectedMessage["Message"]);
    const nlohmann::json& extendedInfo =
        res.jsonValue["error"]["@Message.ExtendedInfo"];
    ASSERT_TRUE(extendedInfo.is_array());
    EXPECT_EQ(extendedInfo.size(), 1);
    EXPECT_EQ(extendedInfo[0], expectedMessage);
}

TEST(CertificateMapping, ParseModeMapsToCertificateMappingAttribute)
{
    EXPECT_EQ(getCertificateMapping(MTLSCommonNameParseMode::CommonName),
              CertificateMappingAttribute::CommonName);
    EXPECT_EQ(getCertificateMapping(MTLSCommonNameParseMode::Whole),
              CertificateMappingAttribute::Whole);
    EXPECT_EQ(getCertificateMapping(MTLSCommonNameParseMode::UserPrincipalName),
              CertificateMappingAttribute::UserPrincipalName);
    EXPECT_EQ(getCertificateMapping(MTLSCommonNameParseMode::Invalid),
              CertificateMappingAttribute::Invalid);
}

} // namespace
} // namespace redfish
