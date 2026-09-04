// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "http_response.hpp"
#include "leak_detection.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>
#include <nlohmann/json.hpp>

#include <memory>
#include <optional>
#include <string>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

constexpr const char* chassisId = "ChassisId";
constexpr const char* chassisPath = "ChassisPath";

TEST(DoLeakDetection, ValidChassisServesTheResource)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();

    doLeakDetection(response, chassisId,
                    std::make_optional<std::string>(chassisPath));

    nlohmann::json& json = response->res.jsonValue;
    EXPECT_EQ(json["@odata.type"], "#LeakDetection.v1_2_0.LeakDetection");
    EXPECT_EQ(json["@odata.id"],
              "/redfish/v1/Chassis/ChassisId/ThermalSubsystem/LeakDetection");
    EXPECT_EQ(json["Id"], "LeakDetection");
    EXPECT_EQ(json["Name"], "Leak Detection Systems");
    EXPECT_EQ(
        json["LeakDetectors"]["@odata.id"],
        "/redfish/v1/Chassis/ChassisId/ThermalSubsystem/LeakDetection/LeakDetectors");
    EXPECT_EQ(json["Status"]["State"], "Enabled");
    EXPECT_EQ(json["Status"]["Health"], "OK");
}

TEST(DoLeakDetection, UnknownChassisIsNotFound)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();

    doLeakDetection(response, chassisId, std::nullopt);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::not_found);
}

TEST(AfterLeakDetectionHead, ValidChassisNamesTheSchema)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();

    afterLeakDetectionHead(response, chassisId,
                           std::make_optional<std::string>(chassisPath));

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(
        response->res.getHeaderValue(boost::beast::http::field::link),
        "</redfish/v1/JsonSchemas/LeakDetection/LeakDetection.json>; rel=describedby");
}

TEST(AfterLeakDetectionHead, UnknownChassisIsNotFound)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();

    afterLeakDetectionHead(response, chassisId, std::nullopt);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::not_found);
    EXPECT_TRUE(
        response->res.getHeaderValue(boost::beast::http::field::link).empty());
}

} // namespace
} // namespace redfish
