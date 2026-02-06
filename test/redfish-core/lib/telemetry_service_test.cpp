// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "http_response.hpp"
#include "telemetry_service.hpp"

#include <nlohmann/json.hpp>

#include <memory>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

void assertTelemetryServiceGet(crow::Response& res)
{
    nlohmann::json& json = res.jsonValue;
    EXPECT_EQ(json["@odata.type"], "#TelemetryService.v1_2_1.TelemetryService");
    EXPECT_EQ(json["@odata.id"], "/redfish/v1/TelemetryService");
    EXPECT_EQ(json["Id"], "TelemetryService");
    EXPECT_EQ(json["Name"], "Telemetry Service");
    EXPECT_EQ(json["MetricReportDefinitions"]["@odata.id"],
              "/redfish/v1/TelemetryService/MetricReportDefinitions");
    EXPECT_EQ(json["MetricReports"]["@odata.id"],
              "/redfish/v1/TelemetryService/MetricReports");
    EXPECT_EQ(json["Triggers"]["@odata.id"],
              "/redfish/v1/TelemetryService/Triggers");
}

TEST(TelemetryServiceTest, TelemetryServiceStaticAttributesAreExpected)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    asyncResp->res.setCompleteRequestHandler(assertTelemetryServiceGet);

    setTelemetryServiceStaticAttributes(asyncResp);
}

} // namespace
} // namespace redfish
