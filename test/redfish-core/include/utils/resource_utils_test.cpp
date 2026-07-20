// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "generated/enums/resource.hpp"
#include "utils/resource_utils.hpp"

#include <nlohmann/json.hpp>

#include <memory>
#include <optional>

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
    std::optional<bool> present = false;
    std::optional<bool> available = true;

    determineResourceState(asyncResp, present, available, ""_json_pointer);

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["State"],
              resource::State::Absent);
}

TEST(DetermineResourceState, UnavailableOffline)
{
    auto asyncResp = createAsyncResp();
    std::optional<bool> present = true;
    std::optional<bool> available = false;

    determineResourceState(asyncResp, present, available, ""_json_pointer);

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["State"],
              resource::State::UnavailableOffline);
}

TEST(DetermineResourceState, Enabled)
{
    auto asyncResp = createAsyncResp();
    std::optional<bool> present = true;
    std::optional<bool> available = true;

    determineResourceState(asyncResp, present, available, ""_json_pointer);

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["State"],
              resource::State::Enabled);
}

TEST(DetermineResourceState, AbsentTakesPriorityOverUnavailable)
{
    auto asyncResp = createAsyncResp();
    std::optional<bool> present = false;
    std::optional<bool> available = false;

    determineResourceState(asyncResp, present, available, ""_json_pointer);

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["State"],
              resource::State::Absent);
}

TEST(DetermineResourceState, AbsentTakesPriorityOverUnavailableAvailableTrue)
{
    auto asyncResp = createAsyncResp();
    std::optional<bool> present = false;
    std::optional<bool> available = true;

    determineResourceState(asyncResp, present, available, ""_json_pointer);

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["State"],
              resource::State::Absent);
}

TEST(DetermineResourceState, WithJsonPointer)
{
    auto asyncResp = createAsyncResp();
    std::optional<bool> present = true;
    std::optional<bool> available = true;

    nlohmann::json::json_pointer ptr("/Assemblies/0");
    determineResourceState(asyncResp, present, available, ptr);

    EXPECT_EQ(asyncResp->res.jsonValue["Assemblies"][0]["Status"]["State"],
              resource::State::Enabled);
}

TEST(DetermineResourceState, PresentNotSetDefaultsToTrue)
{
    auto asyncResp = createAsyncResp();
    std::optional<bool> present = std::nullopt;
    std::optional<bool> available = true;

    determineResourceState(asyncResp, present, available, ""_json_pointer);

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["State"],
              resource::State::Enabled);
}

TEST(DetermineResourceState, AvailableNotSetDefaultsToTrue)
{
    auto asyncResp = createAsyncResp();
    std::optional<bool> present = true;
    std::optional<bool> available = std::nullopt;

    determineResourceState(asyncResp, present, available, ""_json_pointer);

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["State"],
              resource::State::Enabled);
}

TEST(DetermineResourceState, BothNotSetDefaultsToEnabled)
{
    auto asyncResp = createAsyncResp();
    std::optional<bool> present = std::nullopt;
    std::optional<bool> available = std::nullopt;

    determineResourceState(asyncResp, present, available, ""_json_pointer);

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["State"],
              resource::State::Enabled);
}

TEST(DetermineResourceState, PresentNotSetAvailableFalse)
{
    auto asyncResp = createAsyncResp();
    std::optional<bool> present = std::nullopt;
    std::optional<bool> available = false;

    determineResourceState(asyncResp, present, available, ""_json_pointer);

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["State"],
              resource::State::UnavailableOffline);
}

// Tests for determineResourceHealth

TEST(DetermineResourceHealth, OK)
{
    auto asyncResp = createAsyncResp();
    std::optional<bool> functional = true;

    determineResourceHealth(asyncResp, ""_json_pointer, functional);

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["Health"],
              resource::Health::OK);
}

TEST(DetermineResourceHealth, Critical)
{
    auto asyncResp = createAsyncResp();
    std::optional<bool> functional = false;

    determineResourceHealth(asyncResp, ""_json_pointer, functional);

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["Health"],
              resource::Health::Critical);
}

TEST(DetermineResourceHealth, NotSetDefaultsToOK)
{
    auto asyncResp = createAsyncResp();
    std::optional<bool> functional = std::nullopt;

    determineResourceHealth(asyncResp, ""_json_pointer, functional);

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["Health"],
              resource::Health::OK);
}

TEST(DetermineResourceHealth, WithJsonPointer)
{
    auto asyncResp = createAsyncResp();
    std::optional<bool> functional = false;
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
    determineResourceState(asyncResp, false, true, "/Resource1"_json_pointer);

    // Resource 2: UnavailableOffline
    determineResourceState(asyncResp, true, false, "/Resource2"_json_pointer);

    // Resource 3: Enabled
    determineResourceState(asyncResp, true, true, "/Resource3"_json_pointer);

    EXPECT_EQ(asyncResp->res.jsonValue["Resource1"]["Status"]["State"],
              resource::State::Absent);
    EXPECT_EQ(asyncResp->res.jsonValue["Resource2"]["Status"]["State"],
              resource::State::UnavailableOffline);
    EXPECT_EQ(asyncResp->res.jsonValue["Resource3"]["Status"]["State"],
              resource::State::Enabled);
}

TEST(ResourceUtils, StateAndHealthSeparately)
{
    auto asyncResp = createAsyncResp();

    determineResourceState(asyncResp, true, true, ""_json_pointer);
    determineResourceHealth(asyncResp, ""_json_pointer, false);

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["State"],
              resource::State::Enabled);
    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["Health"],
              resource::Health::Critical);
}

} // namespace
} // namespace redfish::resource_utils