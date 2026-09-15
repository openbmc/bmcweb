// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
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

} // namespace
} // namespace redfish
