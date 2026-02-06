// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "telemetry_service.hpp"

#include <nlohmann/json.hpp>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(TelemetryServiceTest, TelemetryServiceStaticAttributesAreExpected)
{
    nlohmann::json json;

    setTelemetryServiceStaticAttributes(json);

    EXPECT_EQ(json.size(), 7);
    EXPECT_EQ(json.at("@odata.type"),
              "#TelemetryService.v1_2_1.TelemetryService");
    EXPECT_EQ(json.at("@odata.id"), "/redfish/v1/TelemetryService");
    EXPECT_EQ(json.at("Id"), "TelemetryService");
    EXPECT_EQ(json.at("Name"), "Telemetry Service");

    const nlohmann::json& metricReportDefinitions =
        json.at("MetricReportDefinitions");
    EXPECT_EQ(metricReportDefinitions.size(), 1);
    EXPECT_EQ(metricReportDefinitions.at("@odata.id"),
              "/redfish/v1/TelemetryService/MetricReportDefinitions");

    const nlohmann::json& metricReports = json.at("MetricReports");
    EXPECT_EQ(metricReports.size(), 1);
    EXPECT_EQ(metricReports.at("@odata.id"),
              "/redfish/v1/TelemetryService/MetricReports");

    const nlohmann::json& triggers = json.at("Triggers");
    EXPECT_EQ(triggers.size(), 1);
    EXPECT_EQ(triggers.at("@odata.id"),
              "/redfish/v1/TelemetryService/Triggers");
}

} // namespace
} // namespace redfish
