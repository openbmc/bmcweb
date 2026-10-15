// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "http_response.hpp"
#include "processor_operating_config.hpp"

#include <boost/asio/error.hpp>
#include <boost/beast/http/status.hpp>
#include <nlohmann/json.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(AfterGetOperatingConfigData, ErrorSetsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    afterGetOperatingConfigData(asyncResp, boost::asio::error::invalid_argument,
                                {});

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetOperatingConfigData, WrongPropertyTypeSetsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::DBusPropertiesMap properties = {
        {"BaseSpeed", std::string("fast")}};

    afterGetOperatingConfigData(asyncResp, {}, properties);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("BaseSpeedMHz"));
}

TEST(AfterGetOperatingConfigData, EmptyPropertiesLeaveResponseEmpty)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    afterGetOperatingConfigData(asyncResp, {}, {});

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(asyncResp->res.jsonValue.empty());
}

TEST(AfterGetOperatingConfigData, ScalarPropertiesMapped)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::DBusPropertiesMap properties = {
        {"AvailableCoreCount", static_cast<size_t>(8)},
        {"BaseSpeed", static_cast<uint32_t>(2000)},
        {"MaxJunctionTemperature", static_cast<uint32_t>(95)},
        {"MaxSpeed", static_cast<uint32_t>(3500)},
        {"PowerLimit", static_cast<uint32_t>(250)}};

    afterGetOperatingConfigData(asyncResp, {}, properties);

    const nlohmann::json& json = asyncResp->res.jsonValue;
    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(json.size(), 5);
    EXPECT_EQ(json["TotalAvailableCoreCount"], 8);
    EXPECT_EQ(json["BaseSpeedMHz"], 2000);
    EXPECT_EQ(json["MaxJunctionTemperatureCelsius"], 95);
    EXPECT_EQ(json["MaxSpeedMHz"], 3500);
    EXPECT_EQ(json["TDPWatts"], 250);
    EXPECT_FALSE(json.contains("TurboProfile"));
    EXPECT_FALSE(json.contains("BaseSpeedPrioritySettings"));
}

TEST(AfterGetOperatingConfigData, TurboProfileMapped)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    TurboProfileProperty turboProfile = {{3500, 2}, {3000, 8}};
    dbus::utility::DBusPropertiesMap properties = {
        {"TurboProfile", turboProfile}};

    afterGetOperatingConfigData(asyncResp, {}, properties);

    EXPECT_EQ(asyncResp->res.jsonValue.size(), 1);
    const nlohmann::json& turbo = asyncResp->res.jsonValue["TurboProfile"];
    ASSERT_EQ(turbo.size(), 2);
    EXPECT_EQ(turbo[0].size(), 2);
    EXPECT_EQ(turbo[0]["MaxSpeedMHz"], 3500);
    EXPECT_EQ(turbo[0]["ActiveCoreCount"], 2);
    EXPECT_EQ(turbo[1].size(), 2);
    EXPECT_EQ(turbo[1]["MaxSpeedMHz"], 3000);
    EXPECT_EQ(turbo[1]["ActiveCoreCount"], 8);
}

TEST(AfterGetOperatingConfigData, EmptyTurboProfileIsEmptyArray)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::DBusPropertiesMap properties = {
        {"TurboProfile", TurboProfileProperty{}}};

    afterGetOperatingConfigData(asyncResp, {}, properties);

    const nlohmann::json& turbo = asyncResp->res.jsonValue["TurboProfile"];
    EXPECT_TRUE(turbo.is_array());
    EXPECT_TRUE(turbo.empty());
}

TEST(AfterGetOperatingConfigData, BaseSpeedPrioritySettingsMapped)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    BaseSpeedPrioritySettingsProperty settings = {{2400, {0, 1, 2}},
                                                  {2000, {3}}};
    dbus::utility::DBusPropertiesMap properties = {
        {"BaseSpeedPrioritySettings", settings}};

    afterGetOperatingConfigData(asyncResp, {}, properties);

    EXPECT_EQ(asyncResp->res.jsonValue.size(), 1);
    const nlohmann::json& priority =
        asyncResp->res.jsonValue["BaseSpeedPrioritySettings"];
    ASSERT_EQ(priority.size(), 2);
    EXPECT_EQ(priority[0].size(), 3);
    EXPECT_EQ(priority[0]["BaseSpeedMHz"], 2400);
    EXPECT_EQ(priority[0]["CoreCount"], 3);
    EXPECT_EQ(priority[0]["CoreIDs"], nlohmann::json::array({0, 1, 2}));
    EXPECT_EQ(priority[1].size(), 3);
    EXPECT_EQ(priority[1]["BaseSpeedMHz"], 2000);
    EXPECT_EQ(priority[1]["CoreCount"], 1);
    EXPECT_EQ(priority[1]["CoreIDs"], nlohmann::json::array({3}));
}

TEST(AfterGetOperatingConfigCollection, ErrorSetsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    afterGetOperatingConfigCollection(asyncResp, "cpu0",
                                      boost::asio::error::invalid_argument, {});

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetOperatingConfigCollection, NoMatchingCpuLeavesResponseEmpty)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::MapperGetSubTreePathsResponse objects = {
        "/xyz/openbmc_project/inventory/system/cpu1"};

    afterGetOperatingConfigCollection(asyncResp, "cpu0", {}, objects);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("Members"));
}

TEST(AfterGetOperatingConfig, ErrorSetsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    afterGetOperatingConfig(asyncResp, "cpu0", "config0",
                            boost::asio::error::invalid_argument, {});

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetOperatingConfig, NoMatchingConfigReturnsNotFound)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::MapperGetSubTreeResponse subtree = {
        {"/xyz/openbmc_project/inventory/system/cpu0/config1",
         {{"xyz.openbmc_project.Example",
           {"xyz.openbmc_project.Inventory.Item.Cpu.OperatingConfig"}}}},
        {"/xyz/openbmc_project/inventory/system/cpu1/config0",
         {{"xyz.openbmc_project.Example",
           {"xyz.openbmc_project.Inventory.Item.Cpu.OperatingConfig"}}}}};

    afterGetOperatingConfig(asyncResp, "cpu0", "config0", {}, subtree);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::not_found);
    const nlohmann::json& info =
        asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"];
    EXPECT_EQ(info[0]["MessageArgs"],
              nlohmann::json::array({"OperatingConfig", "config0"}));
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("@odata.id"));
}

TEST(AfterGetOperatingConfig, MatchWithoutServiceReturnsNotFound)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::MapperGetSubTreeResponse subtree = {
        {"/xyz/openbmc_project/inventory/system/cpu0/config0", {}}};

    afterGetOperatingConfig(asyncResp, "cpu0", "config0", {}, subtree);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::not_found);
    const nlohmann::json& info =
        asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"];
    EXPECT_EQ(info[0]["MessageArgs"],
              nlohmann::json::array({"OperatingConfig", "config0"}));
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("@odata.id"));
}

} // namespace
} // namespace redfish
