// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "redfish_sessions.hpp"

#include "sessions.hpp"

#include <boost/asio/ip/address.hpp>

#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <string>

namespace redfish
{
namespace
{

// The collection route is only Login-privileged, so a ReadOnly caller must
// not learn about other users' sessions from it.  ConfigureUsers keeps the
// full view (used by the web UI and by administrators).
TEST(SessionCollection, FiltersByOwner)
{
    persistent_data::SessionStore& store =
        persistent_data::SessionStore::getInstance();
    boost::asio::ip::address ip = boost::asio::ip::make_address("10.0.0.1");

    std::shared_ptr<persistent_data::UserSession> admin =
        store.generateUserSession("admin", ip, std::nullopt,
                                  persistent_data::SessionType::Basic);
    ASSERT_NE(admin, nullptr);
    admin->userRole = "priv-admin";

    std::shared_ptr<persistent_data::UserSession> op =
        store.generateUserSession("op", ip, std::nullopt,
                                  persistent_data::SessionType::Basic);
    ASSERT_NE(op, nullptr);
    op->userRole = "priv-operator";

    std::shared_ptr<persistent_data::UserSession> ro1 =
        store.generateUserSession("ro", ip, std::nullopt,
                                  persistent_data::SessionType::Basic);
    ASSERT_NE(ro1, nullptr);
    ro1->userRole = "priv-user";

    std::shared_ptr<persistent_data::UserSession> ro2 =
        store.generateUserSession("ro", ip, std::nullopt,
                                  persistent_data::SessionType::Basic);
    ASSERT_NE(ro2, nullptr);
    ro2->userRole = "priv-user";

    crow::Request req;

    // ReadOnly caller with two sessions sees both of its own sessions and
    // neither of the other users' sessions.
    req.session = ro1;
    nlohmann::json roMembers = getSessionCollectionMembers(req);
    EXPECT_EQ(roMembers.size(), 2U);

    // Operator does not hold ConfigureUsers, so it sees only its own session.
    req.session = op;
    nlohmann::json opMembers = getSessionCollectionMembers(req);
    ASSERT_EQ(opMembers.size(), 1U);
    EXPECT_EQ(opMembers[0]["@odata.id"],
              "/redfish/v1/SessionService/Sessions/" + op->uniqueId);

    // A ConfigureUsers caller still sees every session.
    req.session = admin;
    nlohmann::json all = getSessionCollectionMembers(req);
    EXPECT_EQ(all.size(), 4U);

    store.removeSession(ro2);
    store.removeSession(ro1);
    store.removeSession(op);
    store.removeSession(admin);
}

} // namespace
} // namespace redfish
