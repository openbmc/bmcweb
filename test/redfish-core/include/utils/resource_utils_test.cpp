// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "generated/enums/resource.hpp"
#include "utils/resource_utils.hpp"

#include <asm-generic/errno.h>

#include <boost/beast/http/status.hpp>
#include <boost/system/error_code.hpp>
#include <nlohmann/json.hpp>

#include <memory>

#include <gtest/gtest.h>

namespace redfish::resource_utils
{
namespace
{

std::shared_ptr<bmcweb::AsyncResp> createAsyncResp()
{
    return std::make_shared<bmcweb::AsyncResp>();
}

// Tests for determineResourceState

TEST(DetermineResourceState, Absent)
{
    auto asyncResp = createAsyncResp();
    bool present = false;
    bool available = true;

    determineResourceState(asyncResp, present, available, true,
                           ""_json_pointer);

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["State"],
              resource::State::Absent);
}

TEST(DetermineResourceState, UnavailableOffline)
{
    auto asyncResp = createAsyncResp();
    bool present = true;
    bool available = false;

    determineResourceState(asyncResp, present, available, true,
                           ""_json_pointer);

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["State"],
              resource::State::UnavailableOffline);
}

TEST(DetermineResourceState, Enabled)
{
    auto asyncResp = createAsyncResp();
    bool present = true;
    bool available = true;

    determineResourceState(asyncResp, present, available, true,
                           ""_json_pointer);

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["State"],
              resource::State::Enabled);
}

TEST(DetermineResourceState, Disabled)
{
    auto asyncResp = createAsyncResp();
    bool present = true;
    bool available = true;

    determineResourceState(asyncResp, present, available, false,
                           ""_json_pointer);

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["State"],
              resource::State::Disabled);
}

TEST(DetermineResourceState, AbsentTakesPriorityOverUnavailable)
{
    auto asyncResp = createAsyncResp();
    bool present = false;
    bool available = false;

    determineResourceState(asyncResp, present, available, true,
                           ""_json_pointer);

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["State"],
              resource::State::Absent);
}

TEST(DetermineResourceState, AbsentTakesPriorityOverDisabled)
{
    auto asyncResp = createAsyncResp();

    determineResourceState(asyncResp, false, true, false, ""_json_pointer);

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["State"],
              resource::State::Absent);
}

TEST(DetermineResourceState, UnavailableTakesPriorityOverDisabled)
{
    auto asyncResp = createAsyncResp();

    determineResourceState(asyncResp, true, false, false, ""_json_pointer);

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["State"],
              resource::State::UnavailableOffline);
}

TEST(DetermineResourceState, WithJsonPointer)
{
    auto asyncResp = createAsyncResp();
    bool present = true;
    bool available = true;

    nlohmann::json::json_pointer ptr("/Assemblies/0");
    determineResourceState(asyncResp, present, available, true, ptr);

    EXPECT_EQ(asyncResp->res.jsonValue["Assemblies"][0]["Status"]["State"],
              resource::State::Enabled);
}

// Tests for determineResourceHealth

TEST(DetermineResourceHealth, OK)
{
    auto asyncResp = createAsyncResp();
    bool functional = true;

    determineResourceHealth(asyncResp, ""_json_pointer, functional);

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["Health"],
              resource::Health::OK);
}

TEST(DetermineResourceHealth, Critical)
{
    auto asyncResp = createAsyncResp();
    bool functional = false;

    determineResourceHealth(asyncResp, ""_json_pointer, functional);

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["Health"],
              resource::Health::Critical);
}

TEST(DetermineResourceHealth, WithJsonPointer)
{
    auto asyncResp = createAsyncResp();
    bool functional = false;
    nlohmann::json::json_pointer ptr("/Resource1");

    determineResourceHealth(asyncResp, ptr, functional);

    EXPECT_EQ(asyncResp->res.jsonValue["Resource1"]["Status"]["Health"],
              resource::Health::Critical);
}

// Integration-style tests

TEST(ResourceUtils, MultipleResourcesWithDifferentStates)
{
    auto asyncResp = createAsyncResp();

    // Resource 1: Absent
    determineResourceState(asyncResp, false, true, true,
                           "/Resource1"_json_pointer);

    // Resource 2: UnavailableOffline
    determineResourceState(asyncResp, true, false, true,
                           "/Resource2"_json_pointer);

    // Resource 3: Enabled
    determineResourceState(asyncResp, true, true, true,
                           "/Resource3"_json_pointer);

    // Resource 4: Disabled
    determineResourceState(asyncResp, true, true, false,
                           "/Resource4"_json_pointer);

    EXPECT_EQ(asyncResp->res.jsonValue["Resource1"]["Status"]["State"],
              resource::State::Absent);
    EXPECT_EQ(asyncResp->res.jsonValue["Resource2"]["Status"]["State"],
              resource::State::UnavailableOffline);
    EXPECT_EQ(asyncResp->res.jsonValue["Resource3"]["Status"]["State"],
              resource::State::Enabled);
    EXPECT_EQ(asyncResp->res.jsonValue["Resource4"]["Status"]["State"],
              resource::State::Disabled);
}

TEST(ResourceUtils, StateAndHealthSeparately)
{
    auto asyncResp = createAsyncResp();

    determineResourceState(asyncResp, true, true, true, ""_json_pointer);
    determineResourceHealth(asyncResp, ""_json_pointer, false);

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["State"],
              resource::State::Enabled);
    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["Health"],
              resource::Health::Critical);
}

TEST(ResourceUtils, StateAndHealthWithJsonPointer)
{
    auto asyncResp = createAsyncResp();
    nlohmann::json::json_pointer ptr("/Component");

    determineResourceState(asyncResp, true, false, true, ptr);
    determineResourceHealth(asyncResp, ptr, true);

    EXPECT_EQ(asyncResp->res.jsonValue["Component"]["Status"]["State"],
              resource::State::UnavailableOffline);
    EXPECT_EQ(asyncResp->res.jsonValue["Component"]["Status"]["Health"],
              resource::Health::OK);
}

// Tests for getStatusEnabledState

TEST(GetStatusEnabledState, MissingInterfaceDefaultsToEnabled)
{
    auto asyncResp = createAsyncResp();

    boost::system::error_code ec(EBADR, boost::system::generic_category());

    // EBADR should default enabled=true, yielding Enabled state
    getStatusEnabledState(asyncResp, ""_json_pointer, true, true, ec, false);

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["State"],
              resource::State::Enabled);
}

TEST(GetStatusEnabledState, MissingInterfacePreservesAbsent)
{
    auto asyncResp = createAsyncResp();

    boost::system::error_code ec(EBADR, boost::system::generic_category());

    // present=false takes priority even when enabled defaults to true
    getStatusEnabledState(asyncResp, ""_json_pointer, false, true, ec, false);

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["State"],
              resource::State::Absent);
}

TEST(GetStatusEnabledState, NonMissingInterfaceError)
{
    auto asyncResp = createAsyncResp();

    boost::system::error_code ec(ENONET, boost::system::generic_category());

    getStatusEnabledState(asyncResp, ""_json_pointer, false, true, ec, true);
    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_FALSE(asyncResp->res.jsonValue["Status"].contains("State"));
}

TEST(GetStatusEnabledState, EnabledFalseYieldsDisabled)
{
    auto asyncResp = createAsyncResp();

    boost::system::error_code ec;

    getStatusEnabledState(asyncResp, ""_json_pointer, true, true, ec, false);

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["State"],
              resource::State::Disabled);
}

// Tests for getStatusAvailableState

TEST(GetStatusAvailableState, NonMissingInterfaceError)
{
    auto asyncResp = createAsyncResp();

    boost::system::error_code ec(ENONET, boost::system::generic_category());

    getStatusAvailableState(asyncResp, "Service", "Path", ""_json_pointer,
                            false, ec, true);
    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_FALSE(asyncResp->res.jsonValue["Status"].contains("State"));
}

TEST(GetStatusPresentState, NonMissingInterfaceError)
{
    auto asyncResp = createAsyncResp();

    boost::system::error_code ec(ENONET, boost::system::generic_category());

    getStatusPresentState(asyncResp, "Service", "Path", ""_json_pointer, ec,
                          true);
    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_FALSE(asyncResp->res.jsonValue["Status"].contains("State"));
}

TEST(AfterGetResourceHealth, MissingInterfaceDefaultsToOK)
{
    auto asyncResp = createAsyncResp();

    boost::system::error_code ec(EBADR, boost::system::generic_category());

    // The bool value supplied on error is false, but on EBADR result in OK
    afterGetResourceHealth(asyncResp, ""_json_pointer, ec, false);

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["Health"],
              resource::Health::OK);
}

TEST(AfterGetResourceHealth, NonMissingInterfaceError)
{
    auto asyncResp = createAsyncResp();

    boost::system::error_code ec(ENONET, boost::system::generic_category());

    afterGetResourceHealth(asyncResp, ""_json_pointer, ec, true);
    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_FALSE(asyncResp->res.jsonValue["Status"].contains("Health"));
}

} // namespace
} // namespace redfish::resource_utils
