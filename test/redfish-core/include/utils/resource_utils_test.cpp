// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "generated/enums/resource.hpp"
#include "utils/resource_utils.hpp"

#include <boost/asio/io_context.hpp>
#include <nlohmann/json.hpp>

#include <memory>
#include <optional>
#include <string>

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
    auto status = std::make_shared<ResourceStatus>();
    status->present = false;
    status->available = true;

    determineResourceState(asyncResp, status, ""_json_pointer);

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["State"],
              resource::State::Absent);
}

TEST(DetermineResourceState, UnavailableOffline)
{
    auto asyncResp = createAsyncResp();
    auto status = std::make_shared<ResourceStatus>();
    status->present = true;
    status->available = false;

    determineResourceState(asyncResp, status, ""_json_pointer);

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["State"],
              resource::State::UnavailableOffline);
}

TEST(DetermineResourceState, Enabled)
{
    auto asyncResp = createAsyncResp();
    auto status = std::make_shared<ResourceStatus>();
    status->present = true;
    status->available = true;

    determineResourceState(asyncResp, status, ""_json_pointer);

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["State"],
              resource::State::Enabled);
}

TEST(DetermineResourceState, AbsentTakesPriorityOverUnavailable)
{
    auto asyncResp = createAsyncResp();
    auto status = std::make_shared<ResourceStatus>();
    status->present = false;
    status->available = false;

    determineResourceState(asyncResp, status, ""_json_pointer);

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["State"],
              resource::State::Absent);
}

TEST(DetermineResourceState, AbsentTakesPriorityOverUnavailableAvailableTrue)
{
    auto asyncResp = createAsyncResp();
    auto status = std::make_shared<ResourceStatus>();
    status->present = false;
    status->available = true;

    determineResourceState(asyncResp, status, ""_json_pointer);

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["State"],
              resource::State::Absent);
}

TEST(DetermineResourceState, WithJsonPointer)
{
    auto asyncResp = createAsyncResp();
    auto status = std::make_shared<ResourceStatus>();
    status->present = true;
    status->available = true;

    nlohmann::json::json_pointer ptr("/Assemblies/0");
    determineResourceState(asyncResp, status, ptr);

    EXPECT_EQ(asyncResp->res.jsonValue["Assemblies"][0]["Status"]["State"],
              resource::State::Enabled);
}

TEST(DetermineResourceState, OptionalNotSetReturnsEarly)
{
    auto asyncResp = createAsyncResp();
    auto status = std::make_shared<ResourceStatus>();

    determineResourceState(asyncResp, status, ""_json_pointer);

    EXPECT_FALSE(asyncResp->res.jsonValue.contains("Status"));
}

TEST(DetermineResourceState, PresentNotSetAvailableFalse)
{
    auto asyncResp = createAsyncResp();
    auto status = std::make_shared<ResourceStatus>();
    // present is std::nullopt - early return expected
    status->available = false;

    determineResourceState(asyncResp, status, ""_json_pointer);

    EXPECT_FALSE(asyncResp->res.jsonValue.contains("Status"));
}

TEST(DetermineResourceState, PresentFalseAvailableNotSet)
{
    auto asyncResp = createAsyncResp();
    auto status = std::make_shared<ResourceStatus>();
    status->present = false;

    determineResourceState(asyncResp, status, ""_json_pointer);

    EXPECT_FALSE(asyncResp->res.jsonValue.contains("Status"));
}

// Tests for determineResourceHealth

TEST(DetermineResourceHealth, OK)
{
    auto asyncResp = createAsyncResp();
    std::optional<bool> available = true;

    determineResourceHealth(asyncResp, ""_json_pointer, available);

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["Health"],
              resource::Health::OK);
}

TEST(DetermineResourceHealth, Critical)
{
    auto asyncResp = createAsyncResp();
    std::optional<bool> available = false;

    determineResourceHealth(asyncResp, ""_json_pointer, available);

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["Health"],
              resource::Health::Critical);
}

TEST(DetermineResourceHealth, NotSetDefaultsToOK)
{
    auto asyncResp = createAsyncResp();
    std::optional<bool> available = std::nullopt;

    determineResourceHealth(asyncResp, ""_json_pointer, available);

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["Health"],
              resource::Health::OK);
}

TEST(DetermineResourceHealth, WithJsonPointer)
{
    auto asyncResp = createAsyncResp();
    std::optional<bool> available = false;
    nlohmann::json::json_pointer ptr("/Resource1");

    determineResourceHealth(asyncResp, ptr, available);

    EXPECT_EQ(asyncResp->res.jsonValue["Resource1"]["Status"]["Health"],
              resource::Health::Critical);
}

// Tests for ResourceStatus struct

TEST(ResourceStatus, DefaultValues)
{
    ResourceStatus status;

    EXPECT_FALSE(status.present.has_value());
    EXPECT_FALSE(status.available.has_value());
}

TEST(ResourceStatus, SetValues)
{
    ResourceStatus status;
    status.present = true;
    status.available = false;

    EXPECT_TRUE(status.present.has_value());
    EXPECT_TRUE(status.available.has_value());
    EXPECT_TRUE(status.present.value());
    EXPECT_FALSE(status.available.value());
}

// Integration-style tests

TEST(ResourceUtils, MultipleResourcesWithDifferentStates)
{
    auto asyncResp = createAsyncResp();

    // Resource 1: Absent
    auto status1 = std::make_shared<ResourceStatus>();
    status1->present = false;
    status1->available = true;
    determineResourceState(asyncResp, status1, "/Resource1"_json_pointer);

    // Resource 2: UnavailableOffline
    auto status2 = std::make_shared<ResourceStatus>();
    status2->present = true;
    status2->available = false;
    determineResourceState(asyncResp, status2, "/Resource2"_json_pointer);

    // Resource 3: Enabled
    auto status3 = std::make_shared<ResourceStatus>();
    status3->present = true;
    status3->available = true;
    determineResourceState(asyncResp, status3, "/Resource3"_json_pointer);

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

    auto status = std::make_shared<ResourceStatus>();
    status->present = true;
    status->available = true;
    determineResourceState(asyncResp, status, ""_json_pointer);

    std::optional<bool> available = false;
    determineResourceHealth(asyncResp, ""_json_pointer, available);

    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["State"],
              resource::State::Enabled);
    EXPECT_EQ(asyncResp->res.jsonValue["Status"]["Health"],
              resource::Health::Critical);
}

} // namespace
} // namespace redfish::resource_utils
