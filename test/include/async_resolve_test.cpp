// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resolve.hpp"

#include <boost/asio/ip/tcp.hpp>

#include <gtest/gtest.h>

TEST(AsyncResolve, ipv4Positive)
{
    boost::asio::ip::tcp::endpoint ep;
    ASSERT_TRUE(async_resolve::endpointFromResolveTuple({1, 2, 3, 4}, ep));
    EXPECT_TRUE(ep.address().is_v4());
    EXPECT_FALSE(ep.address().is_v6());

    EXPECT_EQ(ep.address().to_string(), "1.2.3.4");
}

TEST(AsyncResolve, ipv6Positive)
{
    boost::asio::ip::tcp::endpoint ep;
    ASSERT_TRUE(async_resolve::endpointFromResolveTuple(
        {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}, ep));
    EXPECT_FALSE(ep.address().is_v4());
    EXPECT_TRUE(ep.address().is_v6());
    EXPECT_EQ(ep.address().to_string(), "102:304:506:708:90a:b0c:d0e:f10");
}
