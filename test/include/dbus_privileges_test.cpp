// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "dbus_privileges.hpp"
#include "dbus_utility.hpp"
#include "http_request.hpp"
#include "routing/dynamicrule.hpp"
#include "sessions.hpp"

#include <boost/beast/http/verb.hpp>

#include <memory>
#include <string>
#include <system_error>
#include <vector>

#include <gtest/gtest.h>

namespace crow
{
namespace
{

TEST(HandleRequestUserInfo, NoError)
{
    const std::shared_ptr<bmcweb::AsyncResp> asyncResp =
        std::make_shared<bmcweb::AsyncResp>();
    const boost::system::error_code ec = {};
    bool called = false;

    handleRequestUserInfo(
        asyncResp, ec,
        [&called](const dbus::utility::DBusPropertiesMap& /* userInfoMap */) {
            called = true;
        },
        {});
    EXPECT_TRUE(called);
}

TEST(HandleRequestUserInfo, GenericError)
{
    const std::shared_ptr<bmcweb::AsyncResp> asyncResp =
        std::make_shared<bmcweb::AsyncResp>();
    const boost::system::error_code ec = {1, boost::system::system_category()};
    bool called = false;
    handleRequestUserInfo(
        asyncResp, ec,
        [&called](const dbus::utility::DBusPropertiesMap& /* userInfoMap */) {
            called = true;
        },
        {});
    EXPECT_FALSE(called);
    EXPECT_EQ(asyncResp->res.resultInt(), 500);
}

TEST(HandleRequestUserInfo, UserManagerUnreachableError)
{
    const std::shared_ptr<bmcweb::AsyncResp> asyncResp =
        std::make_shared<bmcweb::AsyncResp>();
    const boost::system::error_code ec = {boost::system::errc::host_unreachable,
                                          boost::system::system_category()};

    bool called = false;
    handleRequestUserInfo(
        asyncResp, ec,
        [&called](const dbus::utility::DBusPropertiesMap& /* userInfoMap */) {
            called = true;
        },
        {});
    EXPECT_FALSE(called);
    EXPECT_EQ(asyncResp->res.resultInt(), 500);
}

TEST(HandleRequestUserInfo, UnauthorizedError)
{
    const std::shared_ptr<bmcweb::AsyncResp> asyncResp =
        std::make_shared<bmcweb::AsyncResp>();
    const boost::system::error_code ec = {boost::system::errc::io_error,
                                          boost::system::system_category()};

    bool called = false;
    handleRequestUserInfo(
        asyncResp, ec,
        [&called](const dbus::utility::DBusPropertiesMap& /* userInfoMap */) {
            called = true;
        },
        {});
    EXPECT_FALSE(called);
    EXPECT_EQ(asyncResp->res.resultInt(), 401);
}

TEST(IsMtlsUserEnabled, LocalUserEnabled)
{
    const std::shared_ptr<bmcweb::AsyncResp> asyncResp =
        std::make_shared<bmcweb::AsyncResp>();
    persistent_data::UserSession session;
    session.sessionType = persistent_data::SessionType::MutualTLS;

    EXPECT_TRUE(isMtlsUserEnabled(
        session, asyncResp, {{"RemoteUser", false}, {"UserEnabled", true}}));
    EXPECT_EQ(asyncResp->res.resultInt(), 200);
}

TEST(IsMtlsUserEnabled, LocalUserDisabled)
{
    const std::shared_ptr<bmcweb::AsyncResp> asyncResp =
        std::make_shared<bmcweb::AsyncResp>();
    persistent_data::UserSession session;
    session.sessionType = persistent_data::SessionType::MutualTLS;

    EXPECT_FALSE(isMtlsUserEnabled(
        session, asyncResp, {{"RemoteUser", false}, {"UserEnabled", false}}));
    EXPECT_EQ(asyncResp->res.resultInt(), 401);
}

TEST(IsMtlsUserEnabled, RemoteUserWithoutUserEnabled)
{
    const std::shared_ptr<bmcweb::AsyncResp> asyncResp =
        std::make_shared<bmcweb::AsyncResp>();
    persistent_data::UserSession session;
    session.sessionType = persistent_data::SessionType::MutualTLS;

    EXPECT_TRUE(isMtlsUserEnabled(session, asyncResp, {{"RemoteUser", true}}));
    EXPECT_EQ(asyncResp->res.resultInt(), 200);
}

TEST(IsMtlsUserEnabled, UserEnabledWrongType)
{
    const std::shared_ptr<bmcweb::AsyncResp> asyncResp =
        std::make_shared<bmcweb::AsyncResp>();
    persistent_data::UserSession session;
    session.sessionType = persistent_data::SessionType::MutualTLS;

    EXPECT_FALSE(isMtlsUserEnabled(
        session, asyncResp,
        {{"RemoteUser", false}, {"UserEnabled", std::string("true")}}));
    EXPECT_EQ(asyncResp->res.resultInt(), 500);
}

TEST(IsMtlsUserEnabled, OtherSessionTypesNotChecked)
{
    const std::shared_ptr<bmcweb::AsyncResp> asyncResp =
        std::make_shared<bmcweb::AsyncResp>();
    persistent_data::UserSession session;
    session.sessionType = persistent_data::SessionType::Session;

    EXPECT_TRUE(isMtlsUserEnabled(
        session, asyncResp, {{"RemoteUser", false}, {"UserEnabled", false}}));
    EXPECT_EQ(asyncResp->res.resultInt(), 200);
}

TEST(AfterGetUserInfoValidate, MtlsUserDisabled)
{
    const std::shared_ptr<bmcweb::AsyncResp> asyncResp =
        std::make_shared<bmcweb::AsyncResp>();
    std::error_code ec;
    Request req{{boost::beast::http::verb::get, "/", 11}, ec};
    req.session = std::make_shared<persistent_data::UserSession>();
    req.session->sessionType = persistent_data::SessionType::MutualTLS;
    DynamicRule rule("/");

    EXPECT_FALSE(afterGetUserInfoValidate(
        req, asyncResp, rule,
        {{"UserPrivilege", std::string("priv-admin")},
         {"RemoteUser", false},
         {"UserPasswordExpired", false},
         {"UserGroups", std::vector<std::string>{"redfish"}},
         {"UserEnabled", false}}));
    EXPECT_EQ(asyncResp->res.resultInt(), 401);
}
} // namespace
} // namespace crow
