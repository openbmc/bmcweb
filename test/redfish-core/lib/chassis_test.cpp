#include "app.hpp"
#include "async_resp.hpp"
#include "chassis.hpp"
#include "dbus_utility.hpp"
#include "generated/enums/chassis.hpp"
#include "http_request.hpp"
#include "http_response.hpp"

#include <boost/beast/core/string_type.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
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

} // namespace
} // namespace redfish
