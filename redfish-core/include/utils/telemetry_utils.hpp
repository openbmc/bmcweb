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

struct ChassisSensor
{
    std::string uri;
    size_t index;
};

inline std::optional<ChassisSensor> getChassisSensorNode(
    const std::vector<std::string>& uris,
    boost::container::flat_set<std::pair<std::string, std::string>>& matched)
{
    size_t uriIdx = 0;
    for (const std::string& uri : uris)
    {
        boost::urls::result<boost::urls::url_view> parsed =
            boost::urls::parse_relative_ref(uri);

        if (!parsed)
        {
            BMCWEB_LOG_ERROR << "Failed to get chassis and sensor Node "
                                "from "
                             << uri;
            return std::make_optional<ChassisSensor>({uri, uriIdx});
        }

        std::string chassis;
        std::string node;

        if (crow::utility::readUrlSegments(*parsed, "redfish", "v1", "Chassis",
                                           std::ref(chassis), std::ref(node)))
        {
            matched.emplace(std::move(chassis), std::move(node));
            uriIdx++;
            continue;
        }

        if (crow::utility::readUrlSegments(*parsed, "redfish", "v1", "Chassis",
                                           std::ref(chassis), "Sensors",
                                           crow::utility::anySegment,
                                           crow::utility::anySegment))
        {
            matched.emplace(std::move(chassis), "Sensors");
            uriIdx++;
            continue;
        }

        BMCWEB_LOG_ERROR << "Failed to get chassis and sensor Node "
                            "from "
                         << uri;
        return std::make_optional<ChassisSensor>({uri, uriIdx});
    }
    return std::nullopt;
}

} // namespace telemetry
} // namespace redfish
