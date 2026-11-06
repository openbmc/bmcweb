// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "error_message_utils.hpp"
#include "http_response.hpp"
#include "metric_report_definition.hpp"

#include <string>
#include <string_view>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(ToRedfishProperty, IdentityKeysMapToThemselves)
{
    EXPECT_EQ(telemetry::toRedfishProperty("Id"), "Id");
    EXPECT_EQ(telemetry::toRedfishProperty("Name"), "Name");
    EXPECT_EQ(telemetry::toRedfishProperty("AppendLimit"), "AppendLimit");
    EXPECT_EQ(telemetry::toRedfishProperty("ReportActions"), "ReportActions");
    EXPECT_EQ(telemetry::toRedfishProperty("ReportUpdates"), "ReportUpdates");
}

TEST(ToRedfishProperty, ReportingTypeMapsToMetricReportDefinitionType)
{
    EXPECT_EQ(telemetry::toRedfishProperty("ReportingType"),
              "MetricReportDefinitionType");
}

TEST(ToRedfishProperty, IntervalMapsToRecurrenceInterval)
{
    EXPECT_EQ(telemetry::toRedfishProperty("Interval"), "RecurrenceInterval");
}

TEST(ToRedfishProperty, ReadingParametersMapsToMetrics)
{
    EXPECT_EQ(telemetry::toRedfishProperty("ReadingParameters"), "Metrics");
}

TEST(ToRedfishProperty, UnknownKeyReturnsEmpty)
{
    EXPECT_TRUE(telemetry::toRedfishProperty("UnknownProperty").empty());
}

TEST(ToRedfishProperty, EmptyKeyReturnsEmpty)
{
    EXPECT_TRUE(telemetry::toRedfishProperty("").empty());
    EXPECT_TRUE(telemetry::toRedfishProperty(std::string_view{}).empty());
}

TEST(ToRedfishProperty, ComparisonIsCaseSensitive)
{
    EXPECT_TRUE(telemetry::toRedfishProperty("id").empty());
    EXPECT_TRUE(telemetry::toRedfishProperty("reportingType").empty());
}

TEST(ToRedfishProperty, MatchIsExactNotPrefix)
{
    EXPECT_TRUE(telemetry::toRedfishProperty("Id ").empty());
    EXPECT_TRUE(telemetry::toRedfishProperty("IdExtra").empty());
    EXPECT_TRUE(telemetry::toRedfishProperty("Report").empty());
}

TEST(ToRedfishProperty, RedfishNamesAreNotValidInputs)
{
    EXPECT_TRUE(telemetry::toRedfishProperty("Metrics").empty());
    EXPECT_TRUE(telemetry::toRedfishProperty("RecurrenceInterval").empty());
    EXPECT_TRUE(
        telemetry::toRedfishProperty("MetricReportDefinitionType").empty());
}

TEST(ToRedfishProperty, AllAddReportKeysMapToNonEmpty)
{
    for (const char* key :
         {"Id", "Name", "ReportingType", "AppendLimit", "ReportActions",
          "Interval", "ReportUpdates", "ReadingParameters"})
    {
        const std::string redfishProperty = telemetry::toRedfishProperty(key);
        EXPECT_FALSE(redfishProperty.empty()) << key;
    }
}

TEST(InvalidArgsMethodMatches, JsonMessageMatchesMethod)
{
    constexpr std::string_view jsonMessage =
        R"({"org.freedesktop.DBus.Error.InvalidArgs":{"METHOD":"Interval","DETAILS":"Interval is below minimum."}})";
    EXPECT_TRUE(telemetry::invalidArgsMethodMatches(jsonMessage, "Interval"));
    EXPECT_FALSE(
        telemetry::invalidArgsMethodMatches(jsonMessage, "ReportingType"));
}

TEST(InvalidArgsMethodMatches, LegacyPlainMessageDoesNotMatch)
{
    // Bare property names predate the JSON envelope; they are handled by
    // the legacy match in handleParamError, not here.
    EXPECT_FALSE(telemetry::invalidArgsMethodMatches("Interval", "Interval"));
}

TEST(InvalidArgsMethodMatches, MalformedInputDoesNotMatch)
{
    EXPECT_FALSE(telemetry::invalidArgsMethodMatches("", "Interval"));
    EXPECT_FALSE(telemetry::invalidArgsMethodMatches("{not json", "Interval"));
    EXPECT_FALSE(telemetry::invalidArgsMethodMatches(
        R"({"org.freedesktop.DBus.Error.InvalidArgs":{}})", "Interval"));
    EXPECT_FALSE(telemetry::invalidArgsMethodMatches(
        R"({"org.freedesktop.DBus.Error.Failed":{"METHOD":"Interval"}})",
        "Interval"));
}

TEST(HandleParamError, JsonInvalidArgsYieldsPropertyValueError)
{
    crow::Response res;
    constexpr const char* jsonMessage =
        R"({"org.freedesktop.DBus.Error.InvalidArgs":{"METHOD":"Interval","DETAILS":"Interval is below minimum."}})";
    EXPECT_FALSE(telemetry::handleParamError(res, jsonMessage, "Interval"));

    const nlohmann::json& extendedInfo =
        res.jsonValue["error"][messages::messageAnnotation];
    ASSERT_EQ(extendedInfo.size(), 1);
    EXPECT_TRUE(extendedInfo[0]["MessageId"].get<std::string>().ends_with(
        "PropertyValueError"));
    EXPECT_EQ(extendedInfo[0]["MessageArgs"][0], "RecurrenceInterval");
}

TEST(HandleParamError, LegacyPlainMessageStillYieldsPropertyValueError)
{
    crow::Response res;
    EXPECT_FALSE(telemetry::handleParamError(res, "ReadingParameters.Foo",
                                             "ReadingParameters"));

    const nlohmann::json& extendedInfo =
        res.jsonValue["error"][messages::messageAnnotation];
    ASSERT_EQ(extendedInfo.size(), 1);
    EXPECT_EQ(extendedInfo[0]["MessageArgs"][0], "Metrics");
}

TEST(HandleParamError, NonMatchingKeyLeavesResponseUntouched)
{
    crow::Response res;
    constexpr const char* jsonMessage =
        R"({"org.freedesktop.DBus.Error.InvalidArgs":{"METHOD":"Interval","DETAILS":"Interval is below minimum."}})";
    EXPECT_TRUE(telemetry::handleParamError(res, jsonMessage, "Name"));
    EXPECT_TRUE(res.jsonValue.empty());
}

TEST(DbusPropertyMatches, ExactAndDottedChildMatch)
{
    EXPECT_TRUE(telemetry::dbusPropertyMatches("Interval", "Interval"));
    EXPECT_TRUE(telemetry::dbusPropertyMatches("ReadingParameters",
                                               "ReadingParameters"));
    EXPECT_TRUE(telemetry::dbusPropertyMatches(
        "ReadingParameters.CollectionDuration", "ReadingParameters"));
    EXPECT_TRUE(telemetry::dbusPropertyMatches("ReadingParameters.Foo.Bar",
                                               "ReadingParameters"));
}

TEST(DbusPropertyMatches, PrefixWithoutDotBoundaryDoesNotMatch)
{
    EXPECT_FALSE(telemetry::dbusPropertyMatches("IntervalFoo", "Interval"));
    EXPECT_FALSE(telemetry::dbusPropertyMatches("NameExtra", "Name"));
    EXPECT_FALSE(telemetry::dbusPropertyMatches("Report", "ReportActions"));
    EXPECT_FALSE(telemetry::dbusPropertyMatches("Interval", "IntervalFoo"));
    EXPECT_FALSE(telemetry::dbusPropertyMatches("", "Interval"));
}

TEST(InvalidArgsMethodMatches, JsonDottedChildMatchesParent)
{
    constexpr std::string_view jsonMessage =
        R"({"org.freedesktop.DBus.Error.InvalidArgs":{"METHOD":"ReadingParameters.CollectionDuration","DETAILS":"Bad duration."}})";
    EXPECT_TRUE(
        telemetry::invalidArgsMethodMatches(jsonMessage, "ReadingParameters"));
    EXPECT_FALSE(telemetry::invalidArgsMethodMatches(jsonMessage, "Interval"));

    constexpr std::string_view prefixMessage =
        R"({"org.freedesktop.DBus.Error.InvalidArgs":{"METHOD":"IntervalFoo","DETAILS":"Bad."}})";
    EXPECT_FALSE(
        telemetry::invalidArgsMethodMatches(prefixMessage, "Interval"));
}

TEST(HandleParamError, JsonDottedChildYieldsParentPropertyError)
{
    crow::Response res;
    constexpr const char* jsonMessage =
        R"({"org.freedesktop.DBus.Error.InvalidArgs":{"METHOD":"ReadingParameters.CollectionDuration","DETAILS":"Bad duration."}})";
    EXPECT_FALSE(
        telemetry::handleParamError(res, jsonMessage, "ReadingParameters"));

    const nlohmann::json& extendedInfo =
        res.jsonValue["error"][messages::messageAnnotation];
    ASSERT_EQ(extendedInfo.size(), 1);
    EXPECT_EQ(extendedInfo[0]["MessageArgs"][0], "Metrics");
}

TEST(HandleParamError, PrefixWithoutDotBoundaryLeavesResponseUntouched)
{
    crow::Response res;
    EXPECT_TRUE(telemetry::handleParamError(res, "IntervalFoo", "Interval"));
    EXPECT_TRUE(res.jsonValue.empty());

    constexpr const char* jsonMessage =
        R"({"org.freedesktop.DBus.Error.InvalidArgs":{"METHOD":"IntervalFoo","DETAILS":"Bad."}})";
    EXPECT_TRUE(telemetry::handleParamError(res, jsonMessage, "Interval"));
    EXPECT_TRUE(res.jsonValue.empty());
}

} // namespace
} // namespace redfish
