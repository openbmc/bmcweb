#pragma once

#include "utils/time_utils.hpp"

#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

namespace telemetry
{

using Readings = std::vector<std::tuple<std::string, double, uint64_t>>;
using TimestampReadings = std::tuple<uint64_t, Readings>;

inline nlohmann::json toMetricValues(const Readings& readings)
{
    nlohmann::json metricValues = nlohmann::json::array_t();

    for (const auto& [metadata, sensorValue, timestamp] : readings)
    {
        nlohmann::json::object_t metricReport;
        metricReport["MetricProperty"] = metadata;
        metricReport["MetricValue"] = std::to_string(sensorValue);
        metricReport["Timestamp"] =
            redfish::time_utils::getDateTimeUintMs(timestamp);
        metricValues.emplace_back(std::move(metricReport));
    }

    return metricValues;
}

inline bool fillReport(nlohmann::json& json, const std::string& id,
                       const TimestampReadings& timestampReadings)
{
    json["@odata.type"] = "#MetricReport.v1_3_0.MetricReport";
    json["@odata.id"] = boost::urls::format(
        "/redfish/v1/TelemetryService/MetricReports/{}", id);
    json["Id"] = id;
    json["Name"] = id;
    json["MetricReportDefinition"]["@odata.id"] = boost::urls::format(
        "/redfish/v1/TelemetryService/MetricReportDefinitions/{}", id);

    const auto& [timestamp, readings] = timestampReadings;
    json["Timestamp"] = redfish::time_utils::getDateTimeUintMs(timestamp);
    json["MetricValues"] = toMetricValues(readings);
    return true;
}

} // namespace telemetry
