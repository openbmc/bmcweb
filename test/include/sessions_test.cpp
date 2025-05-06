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
    jsonValue["SessionToken"] = true;
    jsonValue["MTLSCommonNameParseMode"] = 2;
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
    jsonValue["SessionToken"] = true;
    jsonValue["MTLSCommonNameParseMode"] = 4;
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
} // namespace
