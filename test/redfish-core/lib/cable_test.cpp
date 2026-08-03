// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "async_resp.hpp"
#include "cable.hpp"
#include "generated/enums/resource.hpp"
#include "http_response.hpp"

#include <boost/beast/http/status.hpp>

#include <memory>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(CableStateAggregator, DefaultsToEnabled)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    {
        CableStateAggregator aggregator(asyncResp);
    }

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["State"],
              resource::State::Enabled);
}

TEST(CableStateAggregator, AbsentWhenNotPresent)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    {
        CableStateAggregator aggregator(asyncResp);
        aggregator.setPresent(false);
    }

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["State"],
              resource::State::Absent);
}

TEST(CableStateAggregator, StandbyOfflineWhenPoweredOff)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    {
        CableStateAggregator aggregator(asyncResp);
        aggregator.setPresent(true);
        aggregator.setPoweredState(resource::State::StandbyOffline);
    }

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["State"],
              resource::State::StandbyOffline);
}

TEST(CableStateAggregator, AbsenceTakesPrecedenceOverPowerState)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    {
        CableStateAggregator aggregator(asyncResp);
        aggregator.setPresent(false);
        aggregator.setPoweredState(resource::State::StandbyOffline);
    }

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["State"],
              resource::State::Absent);
}

TEST(CableStateAggregator, ResultIsIndependentOfCompletionOrder)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    {
        CableStateAggregator aggregator(asyncResp);
        // Reverse of the previous test.  The D-Bus replies have no defined
        // order, so the outcome must not depend on which arrives last.
        aggregator.setPoweredState(resource::State::StandbyOffline);
        aggregator.setPresent(false);
    }

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["State"],
              resource::State::Absent);
}

TEST(CableStateAggregator, WritesNothingAfterAnError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    {
        CableStateAggregator aggregator(asyncResp);
        aggregator.setPresent(false);
        asyncResp->res.result(
            boost::beast::http::status::internal_server_error);
    }

    EXPECT_FALSE(asyncResp->res.jsonValue.contains("Status"));
}

} // namespace
} // namespace redfish
