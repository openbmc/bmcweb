#pragma once

#include "error_messages.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>

namespace redfish
{

// TODO: Would it be better to forego enums and just have APIs for interacting
// with each raw and derived metric?

// There should be a one to one match between each enum and each private
// variable metric in MetricStore
enum class RawMetric
{
    LatencyMs,
    RequestsDropped, // HTTP Client's request buffer was full
    RequestsFailed,  // Send message flow failed
    RequestsSucceeded,
    Responses1xx,
    Responses2xx,
    Responses3xx,
    Responses4xx,
    Responses5xx
};

// These are the metrics actually returned by the MetricStore.  They are derived
// from the raw metrics when needed instead of being regularly updated
enum class DerivedMetric
{
    AverageLatencyMs,
    RequestsDropped, // HTTP Client's request buffer was full
    RequestsFailed,  // Send message flow failed
    RequestsSucceeded,
    Responses1xx,
    Responses2xx,
    Responses3xx,
    Responses4xx,
    Responses5xx,
    ResponsesTotal
};

// This class is used to handle metrics which are specific to BMCWeb.  That
// mostly entails the processing of requests as well as aggregation operations.
class Metrics
{
  private:
    std::unordered_map<RawMetric, uint64_t> rawMetrics;

  public:
    void incrementMetric(const RawMetric metric, const uint64_t amount)
    {
        rawMetrics[metric] += amount;
    }

    // Helper function for incrementing response code related metrics
    void incrementMetricRespCode(const uint code)
    {
        // Increment our counts for the different levels of responses
        if ((code >= 100) && (code <= 199))
        {
            incrementMetric(RawMetric::Responses1xx, 1);
        }
        else if ((code >= 200) && (code <= 299))
        {
            incrementMetric(RawMetric::Responses2xx, 1);
        }
        else if ((code >= 300) && (code <= 399))
        {
            incrementMetric(RawMetric::Responses3xx, 1);
        }
        else if ((code >= 400) && (code <= 499))
        {
            incrementMetric(RawMetric::Responses4xx, 1);
        }
        else if ((code >= 500) && (code <= 599))
        {
            incrementMetric(RawMetric::Responses5xx, 1);
        }

        if (code == 429)
        {
            // We generate a dummy 429 response to indicate that the request was
            // dropped because the HTTP client's buffer was already full
            incrementMetric(RawMetric::RequestsDropped, 1);
        }
        else if (code == 502)
        {
            // We return a 502 response when the retry policy is exhausted.
            // That occurs when one of the connect -> send -> receive steps fail
            incrementMetric(RawMetric::RequestsFailed, 1);
        }
        else
        {
            // We successfully received a reponse
            incrementMetric(RawMetric::RequestsSucceeded, 1);
        }
    }

    uint64_t getMetric(const DerivedMetric metric)
    {
        using DM = DerivedMetric;
        using RM = RawMetric;
        if (metric == DM::AverageLatencyMs)
        {
            uint64_t totalResp = rawMetrics[RM::RequestsSucceeded] +
                                 rawMetrics[RM::RequestsDropped] +
                                 rawMetrics[RM::RequestsFailed];

            return (totalResp > 0) ? rawMetrics[RM::LatencyMs] / totalResp : 0;
        }
        if (metric == DM::RequestsDropped)
        {
            return rawMetrics[RM::RequestsDropped];
        }
        if (metric == DM::RequestsFailed)
        {
            return rawMetrics[RM::RequestsFailed];
        }
        if (metric == DM::RequestsSucceeded)
        {
            return rawMetrics[RM::RequestsSucceeded];
        }
        if (metric == DM::ResponsesTotal)
        {
            return rawMetrics[RM::RequestsSucceeded] +
                   rawMetrics[RM::RequestsDropped] +
                   rawMetrics[RM::RequestsFailed];
        }
        if (metric == DM::Responses1xx)
        {
            return rawMetrics[RM::Responses1xx];
        }
        if (metric == DM::Responses2xx)
        {
            return rawMetrics[RM::Responses2xx];
        }
        if (metric == DM::Responses3xx)
        {
            return rawMetrics[RM::Responses3xx];
        }
        if (metric == DM::Responses4xx)
        {
            return rawMetrics[RM::Responses4xx];
        }
        if (metric == DM::Responses5xx)
        {
            return rawMetrics[RM::Responses5xx];
        }

        // This should never be reached since all of the derived metrics are
        // already accounted for
        BMCWEB_LOG_ERROR << "Unable to retrieve requested metric!";
        return 0;
    }

    // TODO: Add API to add metrics to Oem.Metrics of a json response
    // void addMetricsToJson(nlohmann::json& respJson)
    // {
    // }
};

// We need to separately track metrics for entities like aggregated satellite
// BMCs.  The MetricStore provides a way to retrieve all metrics for a given
// entity
class MetricStore
{
  private:
    MetricStore() = default;
    std::unordered_map<std::string, Metrics> metricsMap;

  public:
    MetricStore(const MetricStore&) = delete;
    MetricStore& operator=(const MetricStore&) = delete;
    MetricStore(MetricStore&&) = delete;
    MetricStore& operator=(MetricStore&&) = delete;
    ~MetricStore() = default;

    static Metrics& getMetrics(std::string_view entity)
    {
        static MetricStore m;
        return m.metricsMap[std::string(entity)];
    }
};

} // namespace redfish
