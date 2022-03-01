#pragma once

#include "dbus_utility.hpp"
#include "utility.hpp"

namespace redfish
{

namespace telemetry
{
constexpr const char* service = "xyz.openbmc_project.Telemetry";
constexpr const char* reportInterface = "xyz.openbmc_project.Telemetry.Report";

inline std::string getMetricReportDefinitionUri(const std::string& id)
{
    boost::urls::url url = crow::utility::urlFromPieces(
        "redfish", "v1", "TelemetryService", "MetricReportDefinitions", id);
    return {url.data(), url.size()};
}

inline std::string getMetricReportUri(const std::string& id)
{
    boost::urls::url url = crow::utility::urlFromPieces(
        "redfish", "v1", "TelemetryService", "MetricReports", id);
    return {url.data(), url.size()};
}

inline std::string getTriggerUri(const std::string& id)
{
    boost::urls::url url = crow::utility::urlFromPieces(
        "redfish", "v1", "TelemetryService", "Triggers", id);
    return {url.data(), url.size()};
}

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

} // namespace telemetry
} // namespace redfish
