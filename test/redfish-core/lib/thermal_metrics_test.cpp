// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "http_response.hpp"
#include "thermal_metrics.hpp"

#include <boost/asio/error.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/system/linux_error.hpp>
#include <nlohmann/json.hpp>

#include <memory>
#include <optional>
#include <string>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

std::shared_ptr<bmcweb::AsyncResp> makeRespWithReadingsArray()
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    asyncResp->res.jsonValue["TemperatureReadingsCelsius"] =
        nlohmann::json::array_t();
    return asyncResp;
}

TEST(AfterGetTemperatureValue, EbadrIsIgnored)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    afterGetTemperatureValue(
        asyncResp, "chassis0", "/xyz/openbmc_project/sensors/temperature/t0",
        boost::system::linux_error::bad_request_descriptor, {});

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(asyncResp->res.jsonValue.empty());
}

TEST(AfterGetTemperatureValue, OtherErrorSetsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    afterGetTemperatureValue(asyncResp, "chassis0",
                             "/xyz/openbmc_project/sensors/temperature/t0",
                             boost::asio::error::invalid_argument, {});

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetTemperatureValue, NonTemperatureSensorIsSkipped)
{
    auto asyncResp = makeRespWithReadingsArray();
    dbus::utility::DBusPropertiesMap properties = {{"Value", 12.0}};

    afterGetTemperatureValue(asyncResp, "chassis0",
                             "/xyz/openbmc_project/sensors/voltage/v0", {},
                             properties);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(asyncResp->res.jsonValue["TemperatureReadingsCelsius"].empty());
    EXPECT_FALSE(asyncResp->res.jsonValue.contains(
        "TemperatureReadingsCelsius@odata.count"));
}

TEST(AfterGetTemperatureValue, MissingReadingsArraySetsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::DBusPropertiesMap properties = {{"Value", 40.0}};

    afterGetTemperatureValue(asyncResp, "chassis0",
                             "/xyz/openbmc_project/sensors/temperature/t0", {},
                             properties);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetTemperatureValue, ReadingsAppendedSortedAndCounted)
{
    auto asyncResp = makeRespWithReadingsArray();
    dbus::utility::DBusPropertiesMap properties1 = {{"Value", 40.0}};
    dbus::utility::DBusPropertiesMap properties0 = {{"Value", 45.0}};

    afterGetTemperatureValue(asyncResp, "chassis0",
                             "/xyz/openbmc_project/sensors/temperature/t1", {},
                             properties1);
    afterGetTemperatureValue(asyncResp, "chassis0",
                             "/xyz/openbmc_project/sensors/temperature/t0", {},
                             properties0);

    const nlohmann::json& json = asyncResp->res.jsonValue;
    const nlohmann::json& readings = json["TemperatureReadingsCelsius"];
    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    ASSERT_EQ(readings.size(), 2);
    EXPECT_EQ(readings[0]["DataSourceUri"],
              "/redfish/v1/Chassis/chassis0/Sensors/temperature_t0");
    EXPECT_EQ(readings[0]["Reading"], 45.0);
    EXPECT_EQ(readings[1]["DataSourceUri"],
              "/redfish/v1/Chassis/chassis0/Sensors/temperature_t1");
    EXPECT_EQ(readings[1]["Reading"], 40.0);
    EXPECT_EQ(json["TemperatureReadingsCelsius@odata.count"], 2);
}

TEST(HandleTemperatureReadingsCelsius, EbadrIsIgnored)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    handleTemperatureReadingsCelsius(
        asyncResp, "chassis0",
        boost::system::linux_error::bad_request_descriptor, {});

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(
        asyncResp->res.jsonValue.contains("TemperatureReadingsCelsius"));
}

TEST(HandleTemperatureReadingsCelsius, OtherErrorSetsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    handleTemperatureReadingsCelsius(asyncResp, "chassis0",
                                     boost::asio::error::invalid_argument, {});

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(HandleTemperatureReadingsCelsius, NoSensorsGivesEmptyReadings)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    handleTemperatureReadingsCelsius(asyncResp, "chassis0", {}, {});

    const nlohmann::json& json = asyncResp->res.jsonValue;
    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(json["TemperatureReadingsCelsius"].is_array());
    EXPECT_TRUE(json["TemperatureReadingsCelsius"].empty());
    EXPECT_EQ(json["TemperatureReadingsCelsius@odata.count"], 0);
}

TEST(DoThermalMetrics, InvalidChassisReturnsNotFound)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    doThermalMetrics(asyncResp, "chassis0", std::nullopt);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::not_found);
    const nlohmann::json& info =
        asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"];
    EXPECT_EQ(info[0]["MessageArgs"],
              nlohmann::json::array({"Chassis", "chassis0"}));
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("@odata.id"));
    EXPECT_TRUE(
        asyncResp->res.getHeaderValue(boost::beast::http::field::link).empty());
}

TEST(DoThermalMetricsHead, InvalidChassisReturnsNotFound)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    doThermalMetricsHead(asyncResp, "chassis0", std::nullopt);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::not_found);
    const nlohmann::json& info =
        asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"];
    EXPECT_EQ(info[0]["MessageArgs"],
              nlohmann::json::array({"Chassis", "chassis0"}));
    EXPECT_TRUE(
        asyncResp->res.getHeaderValue(boost::beast::http::field::link).empty());
}

TEST(DoThermalMetricsHead, ValidChassisSetsLinkHeader)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    doThermalMetricsHead(asyncResp, "chassis0",
                         "/xyz/openbmc_project/inventory/system/chassis0");

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(
        asyncResp->res.getHeaderValue(boost::beast::http::field::link),
        "</redfish/v1/JsonSchemas/ThermalMetrics/ThermalMetrics.json>; rel=describedby");
    EXPECT_TRUE(asyncResp->res.jsonValue.empty());
}

TEST(FillThermalMetricsResponse, PopulatesResource)
{
    crow::Response res;

    fillThermalMetricsResponse(res, "chassis0");

    EXPECT_EQ(
        res.getHeaderValue(boost::beast::http::field::link),
        "</redfish/v1/JsonSchemas/ThermalMetrics/ThermalMetrics.json>; rel=describedby");
    const nlohmann::json& json = res.jsonValue;
    EXPECT_EQ(json["@odata.type"], "#ThermalMetrics.v1_0_1.ThermalMetrics");
    EXPECT_EQ(json["@odata.id"],
              "/redfish/v1/Chassis/chassis0/ThermalSubsystem/ThermalMetrics");
    EXPECT_EQ(json["Id"], "ThermalMetrics");
    EXPECT_EQ(json["Name"], "Thermal Metrics");
}

} // namespace
} // namespace redfish
