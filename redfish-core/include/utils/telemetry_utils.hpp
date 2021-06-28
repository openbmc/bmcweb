#pragma once

#include "dbus_utility.hpp"

namespace redfish
{

namespace telemetry
{

constexpr const char* service = "xyz.openbmc_project.Telemetry";
constexpr const char* reportInterface = "xyz.openbmc_project.Telemetry.Report";
constexpr const char* metricDefinitionUri =
    "/redfish/v1/TelemetryService/MetricDefinitions/";
constexpr const char* metricReportDefinitionUri =
    "/redfish/v1/TelemetryService/MetricReportDefinitions";
constexpr const char* metricReportUri =
    "/redfish/v1/TelemetryService/MetricReports";

inline std::optional<nlohmann::json>
    getMetadataJson(const std::string& metadataStr)
{
    std::optional<nlohmann::json> res =
        nlohmann::json::parse(metadataStr, nullptr, false);
    if (res->is_discarded())
    {
        BMCWEB_LOG_ERROR << "Malformed reading metatadata JSON provided by "
                            "telemetry service.";
        return std::nullopt;
    }
    return res;
}

inline std::optional<std::string>
    readStringFromMetadata(const nlohmann::json& metadataJson, const char* key)
{
    std::optional<std::string> res;
    if (auto it = metadataJson.find(key); it != metadataJson.end())
    {
        if (const std::string* value = it->get_ptr<const std::string*>())
        {
            res = *value;
        }
        else
        {
            BMCWEB_LOG_ERROR << "Incorrect reading metatadata JSON provided by "
                                "telemetry service. Missing key '"
                             << key << "'.";
        }
    }
    else
    {
        BMCWEB_LOG_ERROR << "Incorrect reading metatadata JSON provided by "
                            "telemetry service. Key '"
                         << key << "' has a wrong type.";
    }
    return res;
}

inline std::string getDbusReportPath(const std::string& id)
{
    std::string path =
        "/xyz/openbmc_project/Telemetry/Reports/TelemetryService/" + id;
    dbus::utility::escapePathForDbus(path);
    return path;
}

} // namespace telemetry
} // namespace redfish
