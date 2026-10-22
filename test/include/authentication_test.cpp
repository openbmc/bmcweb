// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "authentication.hpp"
#include "http_response.hpp"
#include "sessions.hpp"

#include <boost/asio/ip/address.hpp>

#include <memory>
#include <optional>

#include <gtest/gtest.h>

namespace crow::authentication
{
namespace
{

using persistent_data::SessionStore;
using persistent_data::SessionType;
using persistent_data::UserSession;

std::shared_ptr<UserSession> makeTlsSession()
{
    return SessionStore::getInstance().generateUserSession(
        "user1", boost::asio::ip::make_address("127.0.0.1"), std::nullopt,
        SessionType::MutualTLS);
}

TEST(PerformTLSAuth, NoSession)
{
    Response res;
    EXPECT_EQ(performTLSAuth(res, nullptr), nullptr);
}

TEST(PerformTLSAuth, KeepsSessionInUseFromTimingOut)
{
    SessionStore& store = SessionStore::getInstance();
    std::shared_ptr<UserSession> session = makeTlsSession();
    ASSERT_NE(session, nullptr);
    session->lastUpdated -= store.timeoutInSeconds;

    Response res;
    EXPECT_EQ(performTLSAuth(res, session), session);

    store.lastTimeoutUpdate = {};
    store.applySessionTimeouts();
    EXPECT_TRUE(store.authTokens.contains(session->sessionToken));

    store.removeSession(session);
}

} // namespace
} // namespace crow::authentication
