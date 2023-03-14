#include "metric_store.hpp"
#include "redfish_aggregator.hpp"

#include <gtest/gtest.h> // IWYU pragma: keep

namespace redfish
{

void incrementAllMetrics(std::string_view entity)
{
    using RM = RawMetric;
    MetricStore::getMetrics(entity).incrementMetric(RM::LatencyMs, 90);
    MetricStore::getMetrics(entity).incrementMetric(RM::RequestsSucceeded, 1);
    MetricStore::getMetrics(entity).incrementMetric(RM::RequestsDropped, 1);
    MetricStore::getMetrics(entity).incrementMetric(RM::RequestsFailed, 1);
    MetricStore::getMetrics(entity).incrementMetric(RM::Responses1xx, 1);
    MetricStore::getMetrics(entity).incrementMetric(RM::Responses2xx, 1);
    MetricStore::getMetrics(entity).incrementMetric(RM::Responses3xx, 1);
    MetricStore::getMetrics(entity).incrementMetric(RM::Responses4xx, 1);
    MetricStore::getMetrics(entity).incrementMetric(RM::Responses5xx, 1);
}

TEST(incrementMetric, IncrementMetrics)
{
    using DM = DerivedMetric;
    const std::string entity("IncrementMetrics");

    // Make sure we handle divide by 0 edge case
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::AverageLatencyMs),
              0);

    incrementAllMetrics(entity);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::AverageLatencyMs),
              30);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::RequestsDropped),
              1);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::RequestsFailed), 1);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::RequestsSucceeded),
              1);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::ResponsesTotal), 3);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses1xx), 1);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses2xx), 1);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses3xx), 1);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses4xx), 1);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses5xx), 1);

    incrementAllMetrics(entity);
    incrementAllMetrics(entity);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::AverageLatencyMs),
              30);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::RequestsDropped),
              3);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::RequestsFailed), 3);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::RequestsSucceeded),
              3);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::ResponsesTotal), 9);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses1xx), 3);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses2xx), 3);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses3xx), 3);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses4xx), 3);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses5xx), 3);
}

void incrementAllRespCodes(std::string_view entity)
{
    for (uint i = 100; i <= 500; i += 100)
    {
        MetricStore::getMetrics(entity).incrementMetricRespCode(i);
    }
}

TEST(incrementMetricRespCode, IncrementResponseCodes)
{
    using DM = DerivedMetric;
    const std::string entity("IncrementResponseCodes");
    incrementAllRespCodes(entity);
    incrementAllRespCodes(entity);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::RequestsDropped),
              0);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::RequestsFailed), 0);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::RequestsSucceeded),
              10);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::ResponsesTotal),
              10);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses1xx), 2);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses2xx), 2);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses3xx), 2);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses4xx), 2);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses5xx), 2);

    // Now let's have 3 requests get dropped and 4 fail
    MetricStore::getMetrics(entity).incrementMetricRespCode(429);
    MetricStore::getMetrics(entity).incrementMetricRespCode(429);
    MetricStore::getMetrics(entity).incrementMetricRespCode(429);
    MetricStore::getMetrics(entity).incrementMetricRespCode(502);
    MetricStore::getMetrics(entity).incrementMetricRespCode(502);
    MetricStore::getMetrics(entity).incrementMetricRespCode(502);
    MetricStore::getMetrics(entity).incrementMetricRespCode(502);

    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::RequestsDropped),
              3);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::RequestsFailed), 4);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::RequestsSucceeded),
              10);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::ResponsesTotal),
              17);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses1xx), 2);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses2xx), 2);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses3xx), 2);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses4xx), 5);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses5xx), 6);
}

// We'll increment metrics as part of processing aggregated responses
TEST(processResponse, IncrementResponseCode)
{
    using DM = DerivedMetric;
    nlohmann::json jsonResp;
    jsonResp["@odata.id"] = "/redfish/v1/Chassis/TestChassis";
    jsonResp["Name"] = "Test";

    crow::Response resp;
    resp.body() =
        jsonResp.dump(2, ' ', true, nlohmann::json::error_handler_t::replace);
    resp.addHeader("Content-Type", "application/json");
    resp.addHeader("Allow", "GET");
    resp.addHeader("Location", "/redfish/v1/Chassis/TestChassis");
    resp.addHeader("Link", "</redfish/v1/Test.json>; rel=describedby");
    resp.addHeader("Retry-After", "120");
    resp.result(200);

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    const std::string entity("processResponseIncCode");

    RedfishAggregator::processResponse(entity, asyncResp, resp);
    RedfishAggregator::processResponse(entity, asyncResp, resp);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::RequestsDropped),
              0);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::RequestsFailed), 0);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::RequestsSucceeded),
              2);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::ResponsesTotal), 2);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses1xx), 0);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses2xx), 2);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses3xx), 0);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses4xx), 0);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses5xx), 0);

    resp.result(429);
    RedfishAggregator::processResponse(entity, asyncResp, resp);
    RedfishAggregator::processResponse(entity, asyncResp, resp);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::RequestsDropped),
              2);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::RequestsFailed), 0);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::RequestsSucceeded),
              2);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::ResponsesTotal), 4);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses1xx), 0);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses2xx), 2);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses3xx), 0);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses4xx), 2);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses5xx), 0);

    resp.result(502);
    RedfishAggregator::processResponse(entity, asyncResp, resp);
    RedfishAggregator::processResponse(entity, asyncResp, resp);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::RequestsDropped),
              2);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::RequestsFailed), 2);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::RequestsSucceeded),
              2);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::ResponsesTotal), 6);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses1xx), 0);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses2xx), 2);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses3xx), 0);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses4xx), 2);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses5xx), 2);
}

TEST(processCollectionResponse, IncrementResponseCode)
{
    using DM = DerivedMetric;
    nlohmann::json jsonResp;
    jsonResp["@odata.id"] = "/redfish/v1/Chassis/TestChassis";
    jsonResp["Name"] = "Test";

    crow::Response resp;
    resp.body() =
        jsonResp.dump(2, ' ', true, nlohmann::json::error_handler_t::replace);
    resp.addHeader("Content-Type", "application/json");
    resp.addHeader("Allow", "GET");
    resp.addHeader("Location", "/redfish/v1/Chassis/TestChassis");
    resp.addHeader("Link", "</redfish/v1/Test.json>; rel=describedby");
    resp.addHeader("Retry-After", "120");
    resp.result(200);

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    const std::string entity("processCollectionResponseIncCode");

    RedfishAggregator::processCollectionResponse(entity, asyncResp, resp);
    RedfishAggregator::processCollectionResponse(entity, asyncResp, resp);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::RequestsDropped),
              0);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::RequestsFailed), 0);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::RequestsSucceeded),
              2);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::ResponsesTotal), 2);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses1xx), 0);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses2xx), 2);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses3xx), 0);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses4xx), 0);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses5xx), 0);

    resp.result(429);
    RedfishAggregator::processCollectionResponse(entity, asyncResp, resp);
    RedfishAggregator::processCollectionResponse(entity, asyncResp, resp);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::RequestsDropped),
              2);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::RequestsFailed), 0);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::RequestsSucceeded),
              2);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::ResponsesTotal), 4);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses1xx), 0);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses2xx), 2);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses3xx), 0);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses4xx), 2);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses5xx), 0);

    resp.result(502);
    RedfishAggregator::processCollectionResponse(entity, asyncResp, resp);
    RedfishAggregator::processCollectionResponse(entity, asyncResp, resp);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::RequestsDropped),
              2);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::RequestsFailed), 2);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::RequestsSucceeded),
              2);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::ResponsesTotal), 6);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses1xx), 0);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses2xx), 2);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses3xx), 0);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses4xx), 2);
    EXPECT_EQ(MetricStore::getMetrics(entity).getMetric(DM::Responses5xx), 2);
}
} // namespace redfish
