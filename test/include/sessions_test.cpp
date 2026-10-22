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
    store.userConnections.clear();
    store.needWrite = false;
    return store;
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

TEST(SessionStore, RemoveSessionsByUsernameClosesUserConnections)
{
    SessionStore& store = emptyStore();
    std::shared_ptr<UserSession> session = makeSession("user1");
    std::shared_ptr<UserSession> other = makeSession("user2");
    ASSERT_NE(session, nullptr);
    ASSERT_NE(other, nullptr);

    auto conn = std::make_shared<int>(0);
    auto otherConn = std::make_shared<int>(0);
    int closed = 0;
    int otherClosed = 0;
    store.addUserConnection(*session, conn, [&closed]() { closed++; });
    store.addUserConnection(*other, otherConn, [&otherClosed]() {
        otherClosed++;
    });

    store.removeSessionsByUsername("user1");
    EXPECT_EQ(closed, 1);
    EXPECT_EQ(otherClosed, 0);

    // A closed connection is forgotten, so it is closed only once.
    store.removeSessionsByUsername("user1");
    EXPECT_EQ(closed, 1);
    ASSERT_EQ(store.userConnections.size(), 1U);
    EXPECT_EQ(store.userConnections[0].username, "user2");
}

TEST(SessionStore, RemoveSessionsByUsernameClosesConnectionAfterLogout)
{
    // A connection outlives the session that opened it, so it is still
    // closed when the account loses access after that session ended.
    SessionStore& store = emptyStore();
    std::shared_ptr<UserSession> session = makeSession("user1");
    ASSERT_NE(session, nullptr);
    auto conn = std::make_shared<int>(0);
    int closed = 0;
    store.addUserConnection(*session, conn, [&closed]() { closed++; });

    store.removeSession(session);
    EXPECT_EQ(closed, 0);

    store.removeSessionsByUsername("user1");
    EXPECT_EQ(closed, 1);
}

TEST(SessionStore, RemoveSessionsByUsernameExceptSessionKeepsThatSession)
{
    SessionStore& store = emptyStore();
    std::shared_ptr<UserSession> kept = makeSession("user1");
    std::shared_ptr<UserSession> removed = makeSession("user1");
    ASSERT_NE(kept, nullptr);
    ASSERT_NE(removed, nullptr);

    auto keptConn = std::make_shared<int>(0);
    auto removedConn = std::make_shared<int>(0);
    int keptClosed = 0;
    int removedClosed = 0;
    store.addUserConnection(*kept, keptConn, [&keptClosed]() { keptClosed++; });
    store.addUserConnection(*removed, removedConn, [&removedClosed]() {
        removedClosed++;
    });

    store.removeSessionsByUsernameExceptSession("user1", kept);

    ASSERT_EQ(store.authTokens.size(), 1U);
    EXPECT_EQ(store.authTokens.begin()->second, kept);
    EXPECT_EQ(keptClosed, 0);
    EXPECT_EQ(removedClosed, 1);
}

TEST(SessionStore, AddUserConnectionForgetsDestroyedConnections)
{
    SessionStore& store = emptyStore();
    std::shared_ptr<UserSession> session = makeSession("user1");
    ASSERT_NE(session, nullptr);

    bool called = false;
    auto gone = std::make_shared<int>(0);
    store.addUserConnection(*session, gone, [&called]() { called = true; });
    gone.reset();

    auto live = std::make_shared<int>(0);
    store.addUserConnection(*session, live, []() {});
    EXPECT_EQ(store.userConnections.size(), 1U);

    store.removeSessionsByUsername("user1");
    EXPECT_FALSE(called);
}
} // namespace
