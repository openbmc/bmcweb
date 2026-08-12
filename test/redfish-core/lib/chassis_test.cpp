// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "chassis.hpp"
#include "dbus_utility.hpp"
#include "generated/enums/chassis.hpp"
#include "generated/enums/resource.hpp"
#include "http_request.hpp"
#include "http_response.hpp"

#include <asm-generic/errno.h>

#include <boost/beast/core/string_type.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/errc.hpp>
#include <nlohmann/json.hpp>

#include <functional>
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
    handleChassisPowerState(response,
                            "/xyz/openbmc_project/inventory/system/chassis", ec,
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
    handleChassisPowerState(response,
                            "/xyz/openbmc_project/inventory/system/chassis", ec,
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
        response, "/xyz/openbmc_project/inventory/system/chassis", ec,
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
        response, "/xyz/openbmc_project/inventory/system/chassis", ec,
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
    handleChassisPowerState(
        response, "/xyz/openbmc_project/inventory/system/chassis", ec, "");
    EXPECT_FALSE(response->res.jsonValue.contains("PowerState"));
    EXPECT_FALSE(response->res.jsonValue.contains("Status"));
    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
}

TEST(HandleChassisPowerState, GenericErrorSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::errc::make_error_code(boost::system::errc::io_error);
    handleChassisPowerState(
        response, "/xyz/openbmc_project/inventory/system/chassis", ec, "");
    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

void assertFallback(bool& callbackCalled, const boost::system::error_code& cbEc,
                    const std::string& statePath, const std::string& service)
{
    callbackCalled = true;
    EXPECT_EQ(cbEc, boost::system::errc::make_error_code(
                        boost::system::errc::success));
    if constexpr (BMCWEB_REDFISH_CHASSIS_STATE_OBJECT_FALLBACK)
    {
        EXPECT_EQ(statePath, "/xyz/openbmc_project/state/chassis0");
        EXPECT_EQ(service, "xyz.openbmc_project.State.Chassis");
    }
    else
    {
        EXPECT_EQ(statePath, "");
        EXPECT_EQ(service, "");
    }
}

void assertEmptySubtreeFallback(
    bool& callbackCalled, const boost::system::error_code& cbEc,
    const std::string& statePath, const std::string& service)
{
    callbackCalled = true;
    EXPECT_EQ(cbEc, boost::system::error_code{});
    if constexpr (BMCWEB_REDFISH_CHASSIS_STATE_OBJECT_FALLBACK)
    {
        EXPECT_EQ(statePath, "/xyz/openbmc_project/state/chassis0");
        EXPECT_EQ(service, "xyz.openbmc_project.State.Chassis");
    }
    else
    {
        EXPECT_EQ(statePath, "");
        EXPECT_EQ(service, "");
    }
}

TEST(AfterGetChassisStatePath, SuccessSingleEntry)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperGetSubTreeResponse subtree = {
        {"/xyz/openbmc_project/state/chassis_custom",
         {{"xyz.openbmc_project.State.ChassisCustom",
           {"xyz.openbmc_project.State.Chassis"}}}}};

    bool callbackCalled = false;
    auto callback = [&callbackCalled](const boost::system::error_code& cbEc,
                                      const std::string& statePath,
                                      const std::string& service) {
        callbackCalled = true;
        EXPECT_EQ(cbEc, boost::system::error_code{});
        EXPECT_EQ(statePath, "/xyz/openbmc_project/state/chassis_custom");
        EXPECT_EQ(service, "xyz.openbmc_project.State.ChassisCustom");
    };

    afterGetChassisStatePath(response,
                             "/xyz/openbmc_project/inventory/system/chassis",
                             callback, ec, subtree);

    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
}

TEST(AfterGetChassisStatePath, AssociationMissingEbadr)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec(EBADR, boost::system::generic_category());
    dbus::utility::MapperGetSubTreeResponse subtree;

    bool callbackCalled = false;
    afterGetChassisStatePath(
        response, "/xyz/openbmc_project/inventory/system/chassis",
        std::bind_front(assertFallback, std::ref(callbackCalled)), ec, subtree);

    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
}

TEST(AfterGetChassisStatePath, HostUnreachableFallback)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::system::errc::make_error_code(
        boost::system::errc::host_unreachable);
    dbus::utility::MapperGetSubTreeResponse subtree;

    bool callbackCalled = false;
    afterGetChassisStatePath(
        response, "/xyz/openbmc_project/inventory/system/chassis",
        std::bind_front(assertFallback, std::ref(callbackCalled)), ec, subtree);

    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
}

TEST(AfterGetChassisStatePath, OtherErrorPassesThrough)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::errc::make_error_code(boost::system::errc::io_error);
    dbus::utility::MapperGetSubTreeResponse subtree;

    bool callbackCalled = false;
    auto callback = [&callbackCalled, ec](const boost::system::error_code& cbEc,
                                          const std::string& statePath,
                                          const std::string& service) {
        callbackCalled = true;
        EXPECT_EQ(cbEc, ec);
        EXPECT_EQ(statePath, "");
        EXPECT_EQ(service, "");
    };

    afterGetChassisStatePath(response,
                             "/xyz/openbmc_project/inventory/system/chassis",
                             callback, ec, subtree);

    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
}

TEST(AfterGetChassisStatePath, EmptySubtreeUsesFallback)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperGetSubTreeResponse subtree;

    bool callbackCalled = false;
    afterGetChassisStatePath(
        response, "/xyz/openbmc_project/inventory/system/chassis",
        std::bind_front(assertEmptySubtreeFallback, std::ref(callbackCalled)),
        ec, subtree);

    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
}

TEST(AfterGetChassisStatePath, MultipleSubtreeObjectsSetInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperGetSubTreeResponse subtree = {
        {"/xyz/openbmc_project/state/chassis0",
         {{"xyz.openbmc_project.State.Chassis0",
           {"xyz.openbmc_project.State.Chassis"}}}},
        {"/xyz/openbmc_project/state/chassis1",
         {{"xyz.openbmc_project.State.Chassis1",
           {"xyz.openbmc_project.State.Chassis"}}}}};

    bool callbackCalled = false;
    auto callback = [&callbackCalled](const boost::system::error_code&,
                                      const std::string&, const std::string&) {
        callbackCalled = true;
    };

    afterGetChassisStatePath(response,
                             "/xyz/openbmc_project/inventory/system/chassis",
                             callback, ec, subtree);

    EXPECT_FALSE(callbackCalled);
    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetChassisStatePath, EmptyServiceMapSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperGetSubTreeResponse subtree = {
        {"/xyz/openbmc_project/state/chassis0", {}}};

    bool callbackCalled = false;
    auto callback = [&callbackCalled](const boost::system::error_code&,
                                      const std::string&, const std::string&) {
        callbackCalled = true;
    };

    afterGetChassisStatePath(response,
                             "/xyz/openbmc_project/inventory/system/chassis",
                             callback, ec, subtree);

    EXPECT_FALSE(callbackCalled);
    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

} // namespace
} // namespace redfish
