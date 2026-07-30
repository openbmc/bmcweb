#include "sessions.hpp"

#include <nlohmann/json.hpp>

#include <gtest/gtest.h>

namespace
{
TEST(AuthConfigMethods, FromJsonHappyPath)
{
    persistent_data::AuthConfigMethods methods;
    nlohmann::json::object_t jsonValue;
    jsonValue["BasicAuth"] = true;
    jsonValue["CookieAuth"] = true;
    jsonValue["MTLSCommonNameParseMode"] = 2;
    jsonValue["SessionToken"] = true;
    jsonValue["TLS"] = true;
    jsonValue["TLSStrict"] = false;
    jsonValue["XToken"] = true;

    methods.fromJson(jsonValue);

    EXPECT_EQ(methods.basic, true);
    EXPECT_EQ(methods.cookie, true);
    EXPECT_EQ(methods.sessionToken, true);
    EXPECT_EQ(methods.tls, true);
    EXPECT_EQ(methods.tlsStrict, false);
    EXPECT_EQ(methods.xtoken, true);
    EXPECT_EQ(methods.mTLSCommonNameParsingMode,
              static_cast<persistent_data::MTLSCommonNameParseMode>(2));
}

TEST(AuthConfigMethods, FromJsonMTLSCommonNameParseModeOutOfRange)
{
    persistent_data::AuthConfigMethods methods;
    persistent_data::MTLSCommonNameParseMode prevValue =
        methods.mTLSCommonNameParsingMode;
    nlohmann::json::object_t jsonValue;
    jsonValue["BasicAuth"] = true;
    jsonValue["CookieAuth"] = true;
    jsonValue["MTLSCommonNameParseMode"] = 4;
    jsonValue["SessionToken"] = true;
    jsonValue["TLS"] = true;
    jsonValue["TLSStrict"] = false;
    jsonValue["XToken"] = true;

    methods.fromJson(jsonValue);

    EXPECT_EQ(methods.basic, true);
    EXPECT_EQ(methods.cookie, true);
    EXPECT_EQ(methods.sessionToken, true);
    EXPECT_EQ(methods.tls, true);
    EXPECT_EQ(methods.tlsStrict, false);
    EXPECT_EQ(methods.xtoken, true);
    EXPECT_EQ(methods.mTLSCommonNameParsingMode, prevValue);
}

TEST(AuthConfigMethods, BasicAuthDefaultsToDisabled)
{
    persistent_data::AuthConfigMethods methods;

    EXPECT_FALSE(methods.basic);
    EXPECT_EQ(methods.sessionToken, BMCWEB_SESSION_AUTH);
    EXPECT_EQ(methods.xtoken, BMCWEB_XTOKEN_AUTH);
    EXPECT_EQ(methods.cookie, BMCWEB_COOKIE_AUTH);
}

TEST(AuthConfigMethods, FromJsonRestoresEnabledBasicAuth)
{
    persistent_data::AuthConfigMethods methods;
    nlohmann::json::object_t jsonValue;
    jsonValue["BasicAuth"] = true;

    methods.fromJson(jsonValue);

    EXPECT_TRUE(methods.basic);
}
} // namespace
