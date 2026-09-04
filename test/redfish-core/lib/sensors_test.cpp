// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "http_body.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "sensors.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <nlohmann/json.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace redfish
{
namespace sensors
{
namespace
{

constexpr const char* chassisId = "ChassisId";
constexpr const char* sensorId = "voltage_Sensor0";
constexpr const char* sensorPath =
    "/xyz/openbmc_project/sensors/voltage/Sensor0";

crow::Request makePatch(std::string_view body)
{
    boost::beast::http::request<bmcweb::HttpBody> request{
        boost::beast::http::verb::patch,
        "/redfish/v1/Chassis/ChassisId/Sensors/voltage_Sensor0", 11};
    request.set(boost::beast::http::field::content_type, "application/json");
    request.body().str() = body;
    std::error_code ec;
    return {std::move(request), ec};
}

std::string errorMessage(const std::shared_ptr<bmcweb::AsyncResp>& response)
{
    return response->res.jsonValue["error"]["message"].get<std::string>();
}

TEST(HandleSensorPatch, APropertyOtherThanReadingIsNotWritable)
{
    crow::App app;
    auto response = std::make_shared<bmcweb::AsyncResp>();
    crow::Request request = makePatch(
        R"({"Thresholds":{"LowerCaution":{"Activation":"Increasing"}}})");

    handleSensorPatch(app, request, response, chassisId, sensorId);

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::method_not_allowed);
    EXPECT_NE(errorMessage(response).find("Thresholds/LowerCaution/Activation"),
              std::string::npos);
}

TEST(HandleSensorPatch, AThresholdWithoutReadingIsRefused)
{
    crow::App app;
    auto response = std::make_shared<bmcweb::AsyncResp>();
    crow::Request request = makePatch(R"({"Thresholds":{"LowerCaution":{}}})");

    handleSensorPatch(app, request, response, chassisId, sensorId);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::bad_request);
    EXPECT_NE(errorMessage(response).find("Thresholds/LowerCaution/Reading"),
              std::string::npos);
}

TEST(HandleSensorPatch, AThresholdTheSensorDoesNotServeIsRefused)
{
    crow::App app;
    auto response = std::make_shared<bmcweb::AsyncResp>();
    crow::Request request =
        makePatch(R"({"Thresholds":{"Bogus":{"Reading":1.0}}})");

    handleSensorPatch(app, request, response, chassisId, sensorId);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::bad_request);
    EXPECT_NE(errorMessage(response).find("Thresholds/Bogus"),
              std::string::npos);
}

TEST(HandleSensorPatch, TheFatalThresholdsResolve)
{
    crow::App app;
    auto response = std::make_shared<bmcweb::AsyncResp>();
    crow::Request request = makePatch(
        R"({"Thresholds":{"UpperFatal":{"Reading":1.0},"ZBogus":{"Reading":1.0}}})");

    handleSensorPatch(app, request, response, chassisId, sensorId);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::bad_request);
    EXPECT_NE(errorMessage(response).find("Thresholds/ZBogus"),
              std::string::npos);
}

TEST(HandleSensorPatch, AMalformedSensorIdIsNotFound)
{
    crow::App app;
    auto response = std::make_shared<bmcweb::AsyncResp>();
    crow::Request request =
        makePatch(R"({"Thresholds":{"LowerCaution":{"Reading":1.0}}})");

    handleSensorPatch(app, request, response, chassisId, "novoltage");

    EXPECT_EQ(response->res.result(), boost::beast::http::status::not_found);
    EXPECT_NE(errorMessage(response).find("of type Sensor named 'novoltage'"),
              std::string::npos);
}

TEST(SetSensorThresholds, AnUnreachableSensorIsNotFound)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    std::vector<ThresholdWrite> thresholds;

    setSensorThresholds(response, sensorId, sensorPath, thresholds, ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::not_found);
    EXPECT_NE(errorMessage(response).find("of type Sensor named"),
              std::string::npos);
}

} // namespace
} // namespace sensors
} // namespace redfish
