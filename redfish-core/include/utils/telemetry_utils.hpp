#pragma once

#include "dbus_utility.hpp"
#include "utility.hpp"

namespace redfish
{

namespace telemetry
{
constexpr const char* service = "xyz.openbmc_project.Telemetry";
constexpr const char* reportInterface = "xyz.openbmc_project.Telemetry.Report";
constexpr const char* metricReportDefinitionUri =
    "/redfish/v1/TelemetryService/MetricReportDefinitions";
constexpr const char* metricReportUri =
    "/redfish/v1/TelemetryService/MetricReports";

inline std::string getDbusReportPath(const std::string& id)
{
    sdbusplus::message::object_path reportsPath(
        "/xyz/openbmc_project/Telemetry/Reports/TelemetryService");
    return {reportsPath / id};
}

inline std::string getDbusTriggerPath(const std::string& id)
{
    sdbusplus::message::object_path triggersPath(
        "/xyz/openbmc_project/Telemetry/Triggers/TelemetryService");
    return {triggersPath / id};
}

inline std::optional<std::pair<std::string, size_t>> getChassisSensorNode(
    const std::vector<std::string>& uris,
    boost::container::flat_set<std::pair<std::string, std::string>>& matched)
{
    size_t uriIdx = 0;
    for (const std::string& uri : uris)
    {
        boost::urls::result<boost::urls::url_view> parsed =
            boost::urls::parse_relative_ref(uri);
        if (parsed)
        {
            const boost::urls::segments_view& segments = parsed->segments();
            std::string chassis, node;
            if (segments.is_absolute() &&
                crow::utility::readUrlSegmentsPartial(
                    segments.begin(), segments.end(), "redfish", "v1",
                    "Chassis", chassis, node))
            {
                matched.emplace(std::move(chassis), std::move(node));
                uriIdx++;
                continue;
            }
        }

        BMCWEB_LOG_ERROR << "Failed to get chassis and sensor Node "
                            "from "
                         << uri;
        return std::make_optional<std::pair<std::string, size_t>>(uri, uriIdx);
    }
    return std::nullopt;
}

} // namespace telemetry
} // namespace redfish
