#pragma once

#include "dbus_utility.hpp"
#include "utility.hpp"

namespace redfish
{

namespace telemetry
{
constexpr const char* service = "xyz.openbmc_project.Telemetry";
constexpr const char* reportInterface = "xyz.openbmc_project.Telemetry.Report";

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

struct IncorrectMetricProperty
{
    std::string metricProperty;
    size_t index;
};

inline std::optional<IncorrectMetricProperty> getChassisSensorNode(
    const std::vector<std::string>& metricProperties,
    boost::container::flat_set<std::pair<std::string, std::string>>& matched)
{
    size_t uriIdx = 0;
    for (const std::string& metricProperty : metricProperties)
    {
        boost::urls::result<boost::urls::url_view> parsed =
            boost::urls::parse_relative_ref(metricProperty);

        if (!parsed)
        {
            BMCWEB_LOG_ERROR << "Failed to get chassis and sensor Node "
                                "from "
                             << metricProperty;
            return std::make_optional<IncorrectMetricProperty>(
                {metricProperty, uriIdx});
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

        // Those 2 segments cannot be validated here, as we don't know which
        // sensors exist at the moment of parsing.
        std::string ignoredSenorId;

        if (crow::utility::readUrlSegments(*parsed, "redfish", "v1", "Chassis",
                                           std::ref(chassis), "Sensors",
                                           std::ref(ignoredSenorId)))
        {
            matched.emplace(std::move(chassis), "Sensors");
            uriIdx++;
            continue;
        }

        BMCWEB_LOG_ERROR << "Failed to get chassis and sensor Node "
                            "from "
                         << metricProperty;
        return std::make_optional<IncorrectMetricProperty>(
            {metricProperty, uriIdx});
    }
    return std::nullopt;
}

} // namespace telemetry
} // namespace redfish
