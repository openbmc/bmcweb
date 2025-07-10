// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "dbus_privileges.hpp"

#include <memory>

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
} // namespace
} // namespace crow
