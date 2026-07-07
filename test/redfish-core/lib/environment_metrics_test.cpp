// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "environment_metrics.hpp"
#include "generated/enums/sensor.hpp"
#include "http_response.hpp"

#include <boost/asio/error.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/system/linux_error.hpp>
#include <nlohmann/json.hpp>

#include <memory>
#include <string>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(PopulateEnergykWhFromJoules, MissingReadingOmitsEnergykWh)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    nlohmann::json item = nlohmann::json::object();

    populateEnergykWhFromJoules(response, item);

    EXPECT_FALSE(response->res.jsonValue.contains("EnergykWh"));
}

TEST(PopulateEnergykWhFromJoules, NonNumericReadingOmitsEnergykWh)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    nlohmann::json item;
    item["Reading"] = nullptr;

    populateEnergykWhFromJoules(response, item);

    EXPECT_FALSE(response->res.jsonValue.contains("EnergykWh"));
}

TEST(PopulateEnergykWhFromJoules, SuccessComputesKilowattHours)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    nlohmann::json item;
    item["Reading"] = 7200000.0;

    populateEnergykWhFromJoules(response, item);

    EXPECT_DOUBLE_EQ(
        response->res.jsonValue["EnergykWh"]["Reading"].get<double>(), 2.0);
}

TEST(AfterGetProcessorSensorProperties, EbadrOmitsProperty)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::linux_error::bad_request_descriptor;

    afterGetProcessorSensorProperties(
        response, "chassis0", "/xyz/openbmc_project/sensors/power/Power_0",
        "PowerWatts", sensor::ReadingType::Power, ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("PowerWatts"));
}

TEST(AfterGetProcessorSensorProperties, ErrorSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;

    afterGetProcessorSensorProperties(
        response, "chassis0", "/xyz/openbmc_project/sensors/power/Power_0",
        "PowerWatts", sensor::ReadingType::Power, ec, {});

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetProcessorSensorProperties, MismatchedReadingTypeOmitsProperty)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::DBusPropertiesMap propertiesDict = {{"Value", 29.5}};

    afterGetProcessorSensorProperties(
        response, "chassis0", "/xyz/openbmc_project/sensors/power/Power_0",
        "TemperatureCelsius", sensor::ReadingType::Temperature, ec,
        propertiesDict);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("TemperatureCelsius"));
}

TEST(AfterGetProcessorSensorProperties, SuccessSetsReadingAndDataSourceUri)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::DBusPropertiesMap propertiesDict = {{"Value", 29.5}};

    afterGetProcessorSensorProperties(
        response, "chassis0", "/xyz/openbmc_project/sensors/power/Power_0",
        "PowerWatts", sensor::ReadingType::Power, ec, propertiesDict);

    EXPECT_DOUBLE_EQ(
        response->res.jsonValue["PowerWatts"]["Reading"].get<double>(), 29.5);
    EXPECT_EQ(response->res.jsonValue["PowerWatts"]["DataSourceUri"],
              "/redfish/v1/Chassis/chassis0/Sensors/power_Power_0");
}

TEST(AfterGetProcessorSensorProperties, EnergyJoulesAlsoPopulatesEnergykWh)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::DBusPropertiesMap propertiesDict = {{"Value", 7200000.0}};

    afterGetProcessorSensorProperties(
        response, "chassis0", "/xyz/openbmc_project/sensors/energy/Energy_0",
        "EnergyJoules", sensor::ReadingType::EnergyJoules, ec, propertiesDict);

    EXPECT_DOUBLE_EQ(
        response->res.jsonValue["EnergyJoules"]["Reading"].get<double>(),
        7200000.0);
    EXPECT_DOUBLE_EQ(
        response->res.jsonValue["EnergykWh"]["Reading"].get<double>(), 2.0);
    EXPECT_FALSE(
        response->res.jsonValue["EnergykWh"].contains("DataSourceUri"));
}

TEST(AfterGetProcessorSensorChassis, EbadrOmitsProperty)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::linux_error::bad_request_descriptor;

    afterGetProcessorSensorChassis(
        response, "xyz.openbmc_project.Service0",
        "/xyz/openbmc_project/sensors/power/Power_0", "PowerWatts",
        sensor::ReadingType::Power, ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("PowerWatts"));
}

TEST(AfterGetProcessorSensorChassis, ErrorSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;

    afterGetProcessorSensorChassis(
        response, "xyz.openbmc_project.Service0",
        "/xyz/openbmc_project/sensors/power/Power_0", "PowerWatts",
        sensor::ReadingType::Power, ec, {});

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetProcessorSensorChassis, EmptyEndpointsOmitsProperty)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterGetProcessorSensorChassis(
        response, "xyz.openbmc_project.Service0",
        "/xyz/openbmc_project/sensors/power/Power_0", "PowerWatts",
        sensor::ReadingType::Power, ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("PowerWatts"));
}

TEST(AfterGetProcessorSensorChassis, MultipleEndpointsSetInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperEndPoints endpoints = {
        "/xyz/openbmc_project/inventory/chassis0",
        "/xyz/openbmc_project/inventory/chassis1"};

    afterGetProcessorSensorChassis(
        response, "xyz.openbmc_project.Service0",
        "/xyz/openbmc_project/sensors/power/Power_0", "PowerWatts",
        sensor::ReadingType::Power, ec, endpoints);

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetProcessorSensorExcerpt, EbadrOmitsProperty)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::linux_error::bad_request_descriptor;

    afterGetProcessorSensorExcerpt(response, "PowerWatts",
                                   sensor::ReadingType::Power, ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("PowerWatts"));
}

TEST(AfterGetProcessorSensorExcerpt, ErrorSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;

    afterGetProcessorSensorExcerpt(response, "PowerWatts",
                                   sensor::ReadingType::Power, ec, {});

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetProcessorSensorExcerpt, EmptySubTreeOmitsProperty)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterGetProcessorSensorExcerpt(response, "PowerWatts",
                                   sensor::ReadingType::Power, ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("PowerWatts"));
}

TEST(AfterGetProcessorSensorExcerpt, EmptyServiceMapSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperGetSubTreeResponse subtree = {
        {"/xyz/openbmc_project/sensors/power/Power_0", {}}};

    afterGetProcessorSensorExcerpt(response, "PowerWatts",
                                   sensor::ReadingType::Power, ec, subtree);

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetProcessorSensorExcerpt, MultipleSensorsSetInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperGetSubTreeResponse subtree = {
        {"/xyz/openbmc_project/sensors/power/Power_0",
         {{"xyz.openbmc_project.Service0",
           {"xyz.openbmc_project.Sensor.Value"}}}},
        {"/xyz/openbmc_project/sensors/power/Power_1",
         {{"xyz.openbmc_project.Service0",
           {"xyz.openbmc_project.Sensor.Value"}}}}};

    afterGetProcessorSensorExcerpt(response, "PowerWatts",
                                   sensor::ReadingType::Power, ec, subtree);

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

} // namespace
} // namespace redfish
