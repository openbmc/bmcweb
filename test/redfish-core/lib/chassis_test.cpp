// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "app.hpp"
#include "async_resp.hpp"
#include "chassis.hpp"
#include "dbus_utility.hpp"
#include "generated/enums/chassis.hpp"
#include "generated/enums/resource.hpp"
#include "http_request.hpp"
#include "http_response.hpp"

#include <boost/beast/core/string_type.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/errc.hpp>
#include <boost/system/linux_error.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <system_error>
#include <utility>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

void assertChassisResetActionInfoGet(const std::string& chassisId,
                                     crow::Response& res)
{
    EXPECT_EQ(res.jsonValue["@odata.type"], "#ActionInfo.v1_1_2.ActionInfo");
    EXPECT_EQ(res.jsonValue["@odata.id"],
              "/redfish/v1/Chassis/" + chassisId + "/ResetActionInfo");
    EXPECT_EQ(res.jsonValue["Name"], "Reset Action Info");

    EXPECT_EQ(res.jsonValue["Id"], "ResetActionInfo");

    nlohmann::json::array_t parameters;
    nlohmann::json::object_t parameter;
    parameter["Name"] = "ResetType";
    parameter["Required"] = true;
    parameter["DataType"] = "String";
    nlohmann::json::array_t allowed;
    allowed.emplace_back("PowerCycle");
    parameter["AllowableValues"] = std::move(allowed);
    parameters.emplace_back(std::move(parameter));

    EXPECT_EQ(res.jsonValue["Parameters"], parameters);
}

TEST(HandleChassisResetActionInfoGet, StaticAttributesAreExpected)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    std::error_code err;
    crow::Request request{{boost::beast::http::verb::get, "/whatever", 11},
                          err};

    std::string fakeChassis = "fakeChassis";
    response->res.setCompleteRequestHandler(
        std::bind_front(assertChassisResetActionInfoGet, fakeChassis));

    crow::App app;
    handleChassisResetActionInfoGet(app, request, response, fakeChassis);
}

TEST(TranslateChassisTypeToRedfish, TranslationsAreExpected)
{
    ASSERT_EQ(
        chassis::ChassisType::Blade,
        translateChassisTypeToRedfish(
            "xyz.openbmc_project.Inventory.Item.Chassis.ChassisType.Blade"));
    ASSERT_EQ(
        chassis::ChassisType::Component,
        translateChassisTypeToRedfish(
            "xyz.openbmc_project.Inventory.Item.Chassis.ChassisType.Component"));
    ASSERT_EQ(
        chassis::ChassisType::Enclosure,
        translateChassisTypeToRedfish(
            "xyz.openbmc_project.Inventory.Item.Chassis.ChassisType.Enclosure"));
    ASSERT_EQ(
        chassis::ChassisType::Module,
        translateChassisTypeToRedfish(
            "xyz.openbmc_project.Inventory.Item.Chassis.ChassisType.Module"));
    ASSERT_EQ(
        chassis::ChassisType::RackMount,
        translateChassisTypeToRedfish(
            "xyz.openbmc_project.Inventory.Item.Chassis.ChassisType.RackMount"));
    ASSERT_EQ(
        chassis::ChassisType::StandAlone,
        translateChassisTypeToRedfish(
            "xyz.openbmc_project.Inventory.Item.Chassis.ChassisType.StandAlone"));
    ASSERT_EQ(
        chassis::ChassisType::StorageEnclosure,
        translateChassisTypeToRedfish(
            "xyz.openbmc_project.Inventory.Item.Chassis.ChassisType.StorageEnclosure"));
    ASSERT_EQ(
        chassis::ChassisType::Zone,
        translateChassisTypeToRedfish(
            "xyz.openbmc_project.Inventory.Item.Chassis.ChassisType.Zone"));
    ASSERT_EQ(
        chassis::ChassisType::Invalid,
        translateChassisTypeToRedfish(
            "xyz.openbmc_project.Inventory.Item.Chassis.ChassisType.Unknown"));
}

TEST(HandleChassisProperties, TypeFound)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    auto properties = dbus::utility::DBusPropertiesMap();
    properties.emplace_back(
        std::string("Type"),
        dbus::utility::DbusVariantType(
            "xyz.openbmc_project.Inventory.Item.Chassis.ChassisType.RackMount"));
    handleChassisProperties(response, properties);
    ASSERT_EQ("RackMount", response->res.jsonValue["ChassisType"]);

    response = std::make_shared<bmcweb::AsyncResp>();
    properties.clear();
    properties.emplace_back(
        std::string("Type"),
        dbus::utility::DbusVariantType(
            "xyz.openbmc_project.Inventory.Item.Chassis.ChassisType.StandAlone"));
    handleChassisProperties(response, properties);
    ASSERT_EQ("StandAlone", response->res.jsonValue["ChassisType"]);
}

TEST(HandleChassisProperties, BadTypeFound)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    auto properties = dbus::utility::DBusPropertiesMap();
    properties.emplace_back(
        std::string("Type"),
        dbus::utility::DbusVariantType(
            "xyz.openbmc_project.Inventory.Item.Chassis.ChassisType.Unknown"));
    handleChassisProperties(response, properties);
    // We fall back to RackMount
    ASSERT_EQ("RackMount", response->res.jsonValue["ChassisType"]);
}

TEST(HandleChassisProperties, FailToGetProperty)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    auto properties = dbus::utility::DBusPropertiesMap();
    properties.emplace_back(std::string("Type"),
                            dbus::utility::DbusVariantType(123));
    handleChassisProperties(response, properties);
    ASSERT_EQ(boost::beast::http::status::internal_server_error,
              response->res.result());
}

TEST(HandleChassisProperties, TypeNotFound)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    auto properties = dbus::utility::DBusPropertiesMap();
    handleChassisProperties(response, properties);
    ASSERT_EQ("RackMount", response->res.jsonValue["ChassisType"]);
}

TEST(HandleChassisPowerState, PowerStateOn)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    handleChassisPowerState(response, ec,
                            "xyz.openbmc_project.State.Chassis.PowerState.On");
    EXPECT_EQ(response->res.jsonValue["PowerState"], resource::PowerState::On);
    EXPECT_EQ(response->res.jsonValue["Status"]["State"],
              resource::State::Enabled);
    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
}

TEST(HandleChassisPowerState, PowerStateOff)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    handleChassisPowerState(response, ec,
                            "xyz.openbmc_project.State.Chassis.PowerState.Off");
    EXPECT_EQ(response->res.jsonValue["PowerState"], resource::PowerState::Off);
    EXPECT_EQ(response->res.jsonValue["Status"]["State"],
              resource::State::StandbyOffline);
    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
}

TEST(HandleChassisPowerState, PowerStateTransitioningToOff)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    handleChassisPowerState(
        response, ec,
        "xyz.openbmc_project.State.Chassis.PowerState.TransitioningToOff");
    EXPECT_EQ(response->res.jsonValue["PowerState"],
              resource::PowerState::PoweringOff);
    EXPECT_EQ(response->res.jsonValue["Status"]["State"],
              resource::State::StandbyOffline);
    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
}

TEST(HandleChassisPowerState, PowerStateTransitioningToOn)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    handleChassisPowerState(
        response, ec,
        "xyz.openbmc_project.State.Chassis.PowerState.TransitioningToOn");
    EXPECT_EQ(response->res.jsonValue["PowerState"],
              resource::PowerState::PoweringOn);
    EXPECT_EQ(response->res.jsonValue["Status"]["State"],
              resource::State::Starting);
    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
}

TEST(HandleChassisPowerState, HostUnreachableNoError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::system::errc::make_error_code(
        boost::system::errc::host_unreachable);
    handleChassisPowerState(response, ec, "");
    EXPECT_FALSE(response->res.jsonValue.contains("PowerState"));
    EXPECT_FALSE(response->res.jsonValue.contains("Status"));
    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
}

TEST(HandleChassisPowerState, GenericErrorSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::errc::make_error_code(boost::system::errc::io_error);
    handleChassisPowerState(response, ec, "");
    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetContainedProcessorUuid, UuidIsReported)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    afterGetContainedProcessorUuid(response, ec,
                                   "1115cd9d-8371-51c0-bf13-8a737577691d");
    EXPECT_EQ(response->res.jsonValue["UUID"],
              "1115cd9d-8371-51c0-bf13-8a737577691d");
    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
}

TEST(AfterGetContainedProcessorUuid, EmptyUuidIsOmitted)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    afterGetContainedProcessorUuid(response, ec, "");
    EXPECT_FALSE(response->res.jsonValue.contains("UUID"));
    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
}

TEST(AfterGetContainedProcessorUuid, ErrorSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::errc::make_error_code(boost::system::errc::io_error);
    afterGetContainedProcessorUuid(response, ec, "");
    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetPowerBounds, BoundsAreReported)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    const dbus::utility::DBusPropertiesMap properties = {
        {"MinPowerWatts",
         dbus::utility::DbusVariantType(static_cast<uint32_t>(300))},
        {"MaxPowerWatts",
         dbus::utility::DbusVariantType(static_cast<uint32_t>(600))}};
    boost::system::error_code ec;
    afterGetPowerBounds(response, ec, properties);
    EXPECT_EQ(response->res.jsonValue["MinPowerWatts"], 300);
    EXPECT_EQ(response->res.jsonValue["MaxPowerWatts"], 600);
    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
}

TEST(AfterGetPowerBounds, UnsupportedBoundIsOmitted)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    const dbus::utility::DBusPropertiesMap properties = {
        {"MinPowerWatts",
         dbus::utility::DbusVariantType(std::numeric_limits<uint32_t>::max())},
        {"MaxPowerWatts",
         dbus::utility::DbusVariantType(static_cast<uint32_t>(600))}};
    boost::system::error_code ec;
    afterGetPowerBounds(response, ec, properties);
    EXPECT_FALSE(response->res.jsonValue.contains("MinPowerWatts"));
    EXPECT_EQ(response->res.jsonValue["MaxPowerWatts"], 600);
    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
}

TEST(AfterGetPowerBounds, UnsupportedBoundsAreOmitted)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    const dbus::utility::DBusPropertiesMap properties = {
        {"MinPowerWatts",
         dbus::utility::DbusVariantType(std::numeric_limits<uint32_t>::max())},
        {"MaxPowerWatts",
         dbus::utility::DbusVariantType(std::numeric_limits<uint32_t>::max())}};
    boost::system::error_code ec;
    afterGetPowerBounds(response, ec, properties);
    EXPECT_FALSE(response->res.jsonValue.contains("MinPowerWatts"));
    EXPECT_FALSE(response->res.jsonValue.contains("MaxPowerWatts"));
    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
}

TEST(AfterGetPowerBounds, WrongTypeSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    const dbus::utility::DBusPropertiesMap properties = {
        {"MaxPowerWatts", dbus::utility::DbusVariantType("600")}};
    boost::system::error_code ec;
    afterGetPowerBounds(response, ec, properties);
    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetPowerBounds, ErrorSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::errc::make_error_code(boost::system::errc::io_error);
    afterGetPowerBounds(response, ec, dbus::utility::DBusPropertiesMap{});
    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetContainedProcessor, SeveralProcessorsReportNothing)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    const dbus::utility::MapperGetSubTreeResponse subtree = {
        {"/xyz/openbmc_project/inventory/GPU_2",
         {{"xyz.openbmc_project.GpuSensor",
           {"xyz.openbmc_project.Common.UUID",
            "xyz.openbmc_project.Inventory.Item.Accelerator"}}}},
        {"/xyz/openbmc_project/inventory/GPU_3",
         {{"xyz.openbmc_project.GpuSensor",
           {"xyz.openbmc_project.Common.UUID",
            "xyz.openbmc_project.Inventory.Item.Accelerator"}}}}};
    boost::system::error_code ec;
    afterGetContainedProcessor(response, false, false, false, ec, subtree);
    EXPECT_FALSE(response->res.jsonValue.contains("UUID"));
    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
}

TEST(AfterGetContainedProcessor, NoProcessorReportsNothing)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    afterGetContainedProcessor(response, false, false, false, ec,
                               dbus::utility::MapperGetSubTreeResponse{});
    EXPECT_FALSE(response->res.jsonValue.contains("UUID"));
    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
}

TEST(AfterGetContainedProcessor, NoAssociationIsNotAnError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::errc::make_error_code(boost::system::errc::io_error);
    afterGetContainedProcessor(response, false, false, false, ec,
                               dbus::utility::MapperGetSubTreeResponse{});
    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);

    ec = boost::system::linux_error::bad_request_descriptor;
    afterGetContainedProcessor(response, false, false, false, ec,
                               dbus::utility::MapperGetSubTreeResponse{});
    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
}

TEST(AfterGetContainedProcessor, GenericErrorSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::errc::make_error_code(boost::system::errc::timed_out);
    afterGetContainedProcessor(response, false, false, false, ec,
                               dbus::utility::MapperGetSubTreeResponse{});
    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

} // namespace
} // namespace redfish
