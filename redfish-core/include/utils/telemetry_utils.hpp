#pragma once

#include "dbus_utility.hpp"

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

inline bool getChassisSensorNode(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::vector<std::string>& uris,
    boost::container::flat_set<std::pair<std::string, std::string>>& matched)
{
    size_t uriIdx = 0;
    for (const std::string& uri : uris)
    {
        std::string chassis;
        std::string node;

        if (!uri.starts_with("/redfish/v1/Chassis/") ||
            !dbus::utility::getNthStringFromPath(uri, 3, chassis) ||
            !dbus::utility::getNthStringFromPath(uri, 4, node))
        {
            BMCWEB_LOG_ERROR << "Failed to get chassis and sensor Node "
                                "from "
                             << uri;
            messages::propertyValueIncorrect(asyncResp->res, uri,
                                             "MetricProperties/" +
                                                 std::to_string(uriIdx));
            return false;
        }

        if (node.ends_with('#'))
        {
            node.pop_back();
        }

        matched.emplace(std::move(chassis), std::move(node));
        uriIdx++;
    }
    return true;
}

} // namespace telemetry
} // namespace redfish
