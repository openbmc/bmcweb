// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "dbus_privileges.hpp"

#include "async_resp.hpp"

#include <memory>

#include <gtest/gtest.h>

namespace crow
{
namespace
{
TEST(HandleRequestUserInfo, NoError)
{
    const std::shared_ptr<bmcweb::AsyncResp> asyncResp;
    const boost::system::error_code ec = {};

    bool success = handleRequestUserInfo(asyncResp, ec);
    EXPECT_TRUE(success);
}

TEST(HandleRequestUserInfo, UnknownError)
{
    const std::shared_ptr<bmcweb::AsyncResp> asyncResp;
    const boost::system::error_code ec = {1, boost::system::system_category()};

    bool success = handleRequestUserInfo(asyncResp, ec);
    EXPECT_FALSE(success);
    EXPECT_EQ(asyncResp->res.code, 500);
}

TEST(HandleRequestUserInfo, UserManagerUnreachableError)
{
    const std::shared_ptr<bmcweb::AsyncResp> asyncResp;
    const boost::system::error_code ec = {boost::system::errc::host_unreachable,
                                          boost::system::system_category()};

    bool success = handleRequestUserInfo(asyncResp, ec);
    EXPECT_FALSE(success);
    EXPECT_EQ(asyncResp->res.code, 500);
}

TEST(HandleRequestUserInfo, UnauthorizedError)
{
    const std::shared_ptr<bmcweb::AsyncResp> asyncResp;
    const boost::system::error_code ec = {boost::system::errc::io_error,
                                          boost::system::system_category()};

    bool success = handleRequestUserInfo(asyncResp, ec);
    EXPECT_FALSE(success);
    EXPECT_EQ(asyncResp->res.code, 401);
}
} // namespace
} // namespace crow
