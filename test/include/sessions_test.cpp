#include "bmcweb_config.h"

#include "sessions.hpp"

#include <boost/asio/ip/address.hpp>
#include <nlohmann/json.hpp>

#include <chrono>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>

#include <gtest/gtest.h>

namespace
{
TEST(SessionStore, PasswordSessionRevocation)
{
    auto& store = persistent_data::SessionStore::getInstance();
    for (auto type : {persistent_data::SessionType::Session,
                      persistent_data::SessionType::Basic})
    {
        auto session = store.generateUserSession(
            "user", boost::asio::ip::address{}, std::nullopt, type);
        ASSERT_NE(session, nullptr);

        // Both cookie and token authentication resolve the same session.
        for (bool cookieAuth : {false, true})
        {
            session->cookieAuth = cookieAuth;
            store.authTokens[session->sessionToken] = session;
            store.needWrite = false;
            auto previousUpdate = session->lastUpdated;

            auto authenticated =
                store.loginSessionByToken(session->sessionToken);
            if constexpr (BMCWEB_PASSWORD_AUTH)
            {
                EXPECT_EQ(authenticated, session);
                EXPECT_EQ(store.getSessionByUid(session->uniqueId), session);
            }
            else
            {
                EXPECT_EQ(authenticated, nullptr);
                EXPECT_EQ(store.getSessionByUid(session->uniqueId), nullptr);
                EXPECT_TRUE(store.needsWrite());
                EXPECT_EQ(session->lastUpdated, previousUpdate);
                EXPECT_EQ(store.loginSessionByToken(session->sessionToken),
                          nullptr);
            }
        }
        store.removeSession(session);
    }
}

TEST(SessionStore, RestoredPasswordSessionRevocation)
{
    auto& store = persistent_data::SessionStore::getInstance();
    nlohmann::json::object_t jsonValue;
    jsonValue["unique_id"] = "restored-password-session";
    jsonValue["username"] = "user";
    jsonValue["session_token"] =
        std::string(persistent_data::sessionTokenSize, 'a');
    jsonValue["csrf_token"] =
        std::string(persistent_data::sessionTokenSize, 'b');
    auto session = persistent_data::UserSession::fromJson(jsonValue);
    ASSERT_NE(session, nullptr);
    store.authTokens[session->sessionToken] = session;
    store.needWrite = false;

    auto authenticated = store.loginSessionByToken(session->sessionToken);
    if constexpr (BMCWEB_PASSWORD_AUTH)
    {
        EXPECT_EQ(authenticated, session);
    }
    else
    {
        EXPECT_EQ(authenticated, nullptr);
        EXPECT_EQ(store.getSessionByUid(session->uniqueId), nullptr);
        EXPECT_TRUE(store.needsWrite());
    }
    store.removeSession(session);
}

TEST(SessionStore, CertificateSessionRemainsValid)
{
    auto& store = persistent_data::SessionStore::getInstance();
    auto session = store.generateUserSession(
        "user", boost::asio::ip::address{}, std::nullopt,
        persistent_data::SessionType::MutualTLS);
    ASSERT_NE(session, nullptr);
    store.needWrite = false;

    EXPECT_EQ(store.loginSessionByToken(session->sessionToken), session);
    EXPECT_EQ(store.getSessionByUid(session->uniqueId), session);
    EXPECT_FALSE(store.needsWrite());
    store.removeSession(session);
}

TEST(AuthConfigMethods, FromJsonDisabledTLS)
{
    persistent_data::AuthConfigMethods methods;
    nlohmann::json::object_t jsonValue;
    jsonValue["TLS"] = false;
    jsonValue["TLSStrict"] = false;

    methods.fromJson(jsonValue);

    EXPECT_EQ(methods.tls, !BMCWEB_PASSWORD_AUTH && BMCWEB_MUTUAL_TLS_AUTH);
    EXPECT_FALSE(methods.tlsStrict);
}

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

    EXPECT_EQ(methods.basic, BMCWEB_BASIC_AUTH && BMCWEB_PASSWORD_AUTH);
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

    EXPECT_EQ(methods.basic, BMCWEB_BASIC_AUTH && BMCWEB_PASSWORD_AUTH);
    EXPECT_EQ(methods.cookie, true);
    EXPECT_EQ(methods.sessionToken, true);
    EXPECT_EQ(methods.tls, true);
    EXPECT_EQ(methods.tlsStrict, false);
    EXPECT_EQ(methods.xtoken, true);
    EXPECT_EQ(methods.mTLSCommonNameParsingMode, prevValue);
}
} // namespace
