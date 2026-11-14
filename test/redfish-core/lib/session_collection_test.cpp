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

    std::shared_ptr<persistent_data::UserSession> ro =
        store.generateUserSession("ro", ip, std::nullopt,
                                  persistent_data::SessionType::Basic);
    ASSERT_NE(ro, nullptr);
    ro->userRole = "priv-user";

    crow::Request req;
    req.session = ro;

    nlohmann::json members = getSessionCollectionMembers(req);
    ASSERT_EQ(members.size(), 1U);
    EXPECT_EQ(members[0]["@odata.id"],
              "/redfish/v1/SessionService/Sessions/" + ro->uniqueId);

    // A ConfigureUsers caller still sees every session.
    req.session = admin;
    nlohmann::json all = getSessionCollectionMembers(req);
    EXPECT_EQ(all.size(), 2U);
}

} // namespace
} // namespace redfish
