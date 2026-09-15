// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "app.hpp"
#include "async_resp.hpp"
#include "http_body.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "sensors.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/errc.hpp>
#include <boost/system/error_code.hpp>
#include <sdbusplus/message/native_types.hpp>

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
    EXPECT_EQ(
        errorMessage(response),
        "The property Thresholds/LowerCaution/Activation is a read-only property and cannot be assigned a value.");
}

TEST(HandleSensorPatch, AThresholdWithoutReadingIsRefused)
{
    crow::App app;
    auto response = std::make_shared<bmcweb::AsyncResp>();
    crow::Request request = makePatch(R"({"Thresholds":{"LowerCaution":{}}})");

    handleSensorPatch(app, request, response, chassisId, sensorId);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::bad_request);
    EXPECT_EQ(
        errorMessage(response),
        "The property Reading is a required property and must be included in the request.");
}

TEST(HandleSensorPatch, AThresholdTheSensorDoesNotServeIsRefused)
{
    crow::App app;
    auto response = std::make_shared<bmcweb::AsyncResp>();
    crow::Request request =
        makePatch(R"({"Thresholds":{"Bogus":{"Reading":1.0}}})");

    handleSensorPatch(app, request, response, chassisId, sensorId);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::bad_request);
    EXPECT_EQ(
        errorMessage(response),
        "The property Thresholds/Bogus is not in the list of valid properties for the resource.");
}

TEST(HandleSensorPatch, TheFatalThresholdsResolve)
{
    crow::App app;
    auto response = std::make_shared<bmcweb::AsyncResp>();
    crow::Request request = makePatch(
        R"({"Thresholds":{"UpperFatal":{"Reading":1.0},"ZBogus":{"Reading":1.0}}})");

    handleSensorPatch(app, request, response, chassisId, sensorId);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::bad_request);
    EXPECT_EQ(
        errorMessage(response),
        "The property Thresholds/ZBogus is not in the list of valid properties for the resource.");
}

TEST(HandleSensorPatch, AMalformedSensorIdIsNotFound)
{
    crow::App app;
    auto response = std::make_shared<bmcweb::AsyncResp>();
    crow::Request request =
        makePatch(R"({"Thresholds":{"LowerCaution":{"Reading":1.0}}})");

    handleSensorPatch(app, request, response, chassisId, "novoltage");

    EXPECT_EQ(response->res.result(), boost::beast::http::status::not_found);
    EXPECT_EQ(
        errorMessage(response),
        "The requested resource of type Sensor named 'novoltage' was not found.");
}

TEST(SetSensorThresholds, AnUnreachableSensorIsNotFound)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    std::vector<ThresholdWrite> thresholds;

    setSensorThresholds(response, sensorId, sdbusplus::object_path(sensorPath),
                        thresholds, ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::not_found);
    EXPECT_EQ(
        errorMessage(response),
        "The requested resource of type Sensor named 'voltage_Sensor0' was not found.");
}

TEST(SetSensorThresholds, ADbusErrorIsInternalFailure)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::errc::make_error_code(boost::system::errc::timed_out);
    std::vector<ThresholdWrite> thresholds;

    setSensorThresholds(response, sensorId, sdbusplus::object_path(sensorPath),
                        thresholds, ec, {});

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

} // namespace
} // namespace sensors
} // namespace redfish
