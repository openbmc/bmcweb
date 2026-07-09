// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "fabric.hpp"
#include "generated/enums/resource.hpp"
#include "http_response.hpp"

#include <boost/asio/error.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/system/errc.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/linux_error.hpp>
#include <nlohmann/json.hpp>

#include <array>
#include <memory>
#include <optional>
#include <utility>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

constexpr const char* switchPath =
    "/xyz/openbmc_project/inventory/system/fabric0/switch0";

TEST(DbusToRfPowerState, MapsKnownTokens)
{
    EXPECT_EQ(dbusToRfPowerState(
                  "xyz.openbmc_project.State.Decorator.PowerState.State.On"),
              resource::PowerState::On);
    EXPECT_EQ(dbusToRfPowerState(
                  "xyz.openbmc_project.State.Decorator.PowerState.State.Off"),
              resource::PowerState::Off);
    EXPECT_EQ(
        dbusToRfPowerState(
            "xyz.openbmc_project.State.Decorator.PowerState.State.PoweringOn"),
        resource::PowerState::PoweringOn);
    EXPECT_EQ(dbusToRfPowerState(
                  "xyz.openbmc_project.State.Decorator.PowerState."
                  "State.PoweringOff"),
              resource::PowerState::PoweringOff);
}

TEST(DbusToRfPowerState, UnmappedTokenReturnsNullopt)
{
    EXPECT_EQ(
        dbusToRfPowerState(
            "xyz.openbmc_project.State.Decorator.PowerState.State.Unknown"),
        std::nullopt);
    EXPECT_EQ(dbusToRfPowerState("On"), std::nullopt);
    EXPECT_EQ(dbusToRfPowerState(""), std::nullopt);
}

TEST(AfterGetSwitchPowerState, GenericErrorReportsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;

    afterGetSwitchPowerState(response, switchPath, ec, "");

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetSwitchPowerState, AbsentPropertyOmitsPowerState)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::linux_error::bad_request_descriptor;

    afterGetSwitchPowerState(response, switchPath, ec, "");

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("PowerState"));
}

TEST(AfterGetSwitchPowerState, UnmappedTokenOmitsPowerState)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterGetSwitchPowerState(
        response, switchPath, ec,
        "xyz.openbmc_project.State.Decorator.PowerState.State.Unknown");

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("PowerState"));
}

TEST(AfterGetSwitchPowerState, MappedTokensSetPowerState)
{
    constexpr std::array<std::pair<const char*, const char*>, 4> cases{
        {{"xyz.openbmc_project.State.Decorator.PowerState.State.On", "On"},
         {"xyz.openbmc_project.State.Decorator.PowerState.State.Off", "Off"},
         {"xyz.openbmc_project.State.Decorator.PowerState.State.PoweringOn",
          "PoweringOn"},
         {"xyz.openbmc_project.State.Decorator.PowerState.State.PoweringOff",
          "PoweringOff"}}};

    for (const auto& [dbusToken, expected] : cases)
    {
        auto response = std::make_shared<bmcweb::AsyncResp>();
        boost::system::error_code ec;

        afterGetSwitchPowerState(response, switchPath, ec, dbusToken);

        EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
        ASSERT_TRUE(response->res.jsonValue.contains("PowerState"))
            << "token: " << dbusToken;
        EXPECT_EQ(response->res.jsonValue["PowerState"], expected)
            << "token: " << dbusToken;
    }
}

TEST(AfterGetSwitchPowerStateService, EbadrOmitsPowerState)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::linux_error::bad_request_descriptor;

    afterGetSwitchPowerStateService(response, switchPath, ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("PowerState"));
}

TEST(AfterGetSwitchPowerStateService, IoErrorOmitsPowerState)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::errc::make_error_code(boost::system::errc::io_error);

    afterGetSwitchPowerStateService(response, switchPath, ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("PowerState"));
}

TEST(AfterGetSwitchPowerStateService, ErrorSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;

    afterGetSwitchPowerStateService(response, switchPath, ec, {});

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetSwitchPowerStateService, EmptyObjectOmitsPowerState)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterGetSwitchPowerStateService(response, switchPath, ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("PowerState"));
}

} // namespace
} // namespace redfish
