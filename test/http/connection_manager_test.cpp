// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "connection_manager.hpp"
#include "http/server_sent_event.hpp"
#include "sessions.hpp"

#include <boost/asio/ip/address.hpp>

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

namespace crow
{
namespace
{

struct CountingConnection : sse_socket::Connection
{
    void close(std::string_view /*msg*/) override
    {
        closed++;
    }
    void sendSseEvent(std::string_view /*id*/,
                      std::string_view /*msg*/) override
    {}

    int closed = 0;
};

std::shared_ptr<persistent_data::UserSession> makeSession(
    const std::string& username)
{
    return persistent_data::SessionStore::getInstance().generateUserSession(
        username, boost::asio::ip::make_address("127.0.0.1"), std::nullopt,
        persistent_data::SessionType::Session);
}

TEST(ConnectionManager, ClosesOnlyTheUsersConnections)
{
    ConnectionManager& manager = ConnectionManager::getInstance();
    std::shared_ptr<persistent_data::UserSession> session =
        makeSession("user1");
    std::shared_ptr<persistent_data::UserSession> other = makeSession("user2");
    ASSERT_NE(session, nullptr);
    ASSERT_NE(other, nullptr);
    auto conn = std::make_shared<CountingConnection>();
    auto otherConn = std::make_shared<CountingConnection>();
    manager.add(session, conn);
    manager.add(other, otherConn);

    manager.closeUserConnections("user1", "");
    EXPECT_EQ(conn->closed, 1);
    EXPECT_EQ(otherConn->closed, 0);

    // A closed connection is forgotten, so it is closed once.
    manager.closeUserConnections("user1", "");
    EXPECT_EQ(conn->closed, 1);
}

TEST(ConnectionManager, KeepsTheConnectionsOfTheKeptSession)
{
    ConnectionManager& manager = ConnectionManager::getInstance();
    std::shared_ptr<persistent_data::UserSession> kept = makeSession("user3");
    std::shared_ptr<persistent_data::UserSession> ended = makeSession("user3");
    ASSERT_NE(kept, nullptr);
    ASSERT_NE(ended, nullptr);
    auto keptConn = std::make_shared<CountingConnection>();
    auto endedConn = std::make_shared<CountingConnection>();
    manager.add(kept, keptConn);
    manager.add(ended, endedConn);

    manager.closeUserConnections("user3", kept->uniqueId);
    EXPECT_EQ(keptConn->closed, 0);
    EXPECT_EQ(endedConn->closed, 1);
}

TEST(ConnectionManager, ClosedOnlyWhenTheUserLosesAccess)
{
    persistent_data::SessionStore& store =
        persistent_data::SessionStore::getInstance();
    std::shared_ptr<persistent_data::UserSession> session =
        makeSession("user4");
    ASSERT_NE(session, nullptr);
    auto conn = std::make_shared<CountingConnection>();
    ConnectionManager::getInstance().add(session, conn);

    // A connection outlives the session that opened it.
    store.removeSession(session);
    EXPECT_EQ(conn->closed, 0);

    store.removeSessionsByUsername("user4");
    EXPECT_EQ(conn->closed, 1);
}

TEST(ConnectionManager, ClosedWhenThePasswordChangesInAnotherSession)
{
    persistent_data::SessionStore& store =
        persistent_data::SessionStore::getInstance();
    std::shared_ptr<persistent_data::UserSession> session =
        makeSession("user5");
    std::shared_ptr<persistent_data::UserSession> changing =
        makeSession("user5");
    ASSERT_NE(session, nullptr);
    ASSERT_NE(changing, nullptr);
    auto conn = std::make_shared<CountingConnection>();
    auto changingConn = std::make_shared<CountingConnection>();
    ConnectionManager::getInstance().add(session, conn);
    ConnectionManager::getInstance().add(changing, changingConn);

    store.removeSessionsByUsernameExceptSession("user5", changing);
    EXPECT_EQ(conn->closed, 1);
    EXPECT_EQ(changingConn->closed, 0);
}

TEST(ConnectionManager, KeepsNoConnectionWithoutSession)
{
    auto conn = std::make_shared<CountingConnection>();
    ConnectionManager::getInstance().add(nullptr, conn);
    ConnectionManager::getInstance().closeUserConnections("", "");
    EXPECT_EQ(conn->closed, 0);
}

} // namespace
} // namespace crow
