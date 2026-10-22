#include "sessions.hpp"

#include <boost/asio/ip/address.hpp>
#include <nlohmann/json.hpp>

#include <memory>
#include <optional>
#include <string>

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

using persistent_data::SessionStore;
using persistent_data::SessionType;
using persistent_data::UserSession;

std::shared_ptr<UserSession> makeSession(const std::string& username)
{
    return SessionStore::getInstance().generateUserSession(
        username, boost::asio::ip::make_address("127.0.0.1"), std::nullopt,
        SessionType::Session);
}

// SessionStore is a singleton; start every test from an empty one.
SessionStore& emptyStore()
{
    SessionStore& store = SessionStore::getInstance();
    store.authTokens.clear();
    store.needWrite = false;
    return store;
}

TEST(SessionStore, RemoveSessionRevokesSession)
{
    SessionStore& store = emptyStore();
    std::shared_ptr<UserSession> session = makeSession("user1");
    ASSERT_NE(session, nullptr);
    EXPECT_FALSE(session->revoked);

    store.removeSession(session);

    EXPECT_TRUE(session->revoked);
    EXPECT_TRUE(store.authTokens.empty());
}

TEST(SessionStore, ApplySessionTimeoutsRevokesSession)
{
    SessionStore& store = emptyStore();
    std::shared_ptr<UserSession> session = makeSession("user1");
    ASSERT_NE(session, nullptr);
    session->lastUpdated -= store.timeoutInSeconds;
    store.lastTimeoutUpdate = {};

    store.applySessionTimeouts();

    EXPECT_TRUE(session->revoked);
    EXPECT_TRUE(store.authTokens.empty());
}

TEST(SessionStore, RemoveSessionsByUsernameRemovesOnlyThatUser)
{
    SessionStore& store = emptyStore();
    std::shared_ptr<UserSession> first = makeSession("user1");
    std::shared_ptr<UserSession> second = makeSession("user1");
    std::shared_ptr<UserSession> other = makeSession("user2");
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    ASSERT_NE(other, nullptr);
    store.needWrite = false;

    store.removeSessionsByUsername("user1");

    EXPECT_TRUE(first->revoked);
    EXPECT_TRUE(second->revoked);
    EXPECT_FALSE(other->revoked);
    ASSERT_EQ(store.authTokens.size(), 1U);
    EXPECT_EQ(store.authTokens.begin()->second, other);
    EXPECT_TRUE(store.needsWrite());
}

TEST(SessionStore, GenerateUserSessionKeepsPendingWrite)
{
    SessionStore& store = emptyStore();
    std::shared_ptr<UserSession> session = makeSession("user1");
    ASSERT_NE(session, nullptr);
    store.needWrite = false;
    store.removeSessionsByUsername("user1");
    ASSERT_TRUE(store.needsWrite());

    // A login that is not persisted must not drop the pending write.
    EXPECT_NE(store.generateUserSession(
                  "user2", boost::asio::ip::make_address("127.0.0.1"),
                  std::nullopt, SessionType::Basic),
              nullptr);
    EXPECT_TRUE(store.needsWrite());
}

TEST(SessionStore, RemoveSessionsByUsernameExceptSessionKeepsThatSession)
{
    SessionStore& store = emptyStore();
    std::shared_ptr<UserSession> kept = makeSession("user1");
    std::shared_ptr<UserSession> removed = makeSession("user1");
    ASSERT_NE(kept, nullptr);
    ASSERT_NE(removed, nullptr);

    store.removeSessionsByUsernameExceptSession("user1", kept);

    EXPECT_FALSE(kept->revoked);
    EXPECT_TRUE(removed->revoked);
    ASSERT_EQ(store.authTokens.size(), 1U);
    EXPECT_EQ(store.authTokens.begin()->second, kept);
}
} // namespace
