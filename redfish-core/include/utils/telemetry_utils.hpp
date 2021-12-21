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
constexpr const char* triggerInterface =
    "xyz.openbmc_project.Telemetry.Trigger";
constexpr const char* triggerUri = "/redfish/v1/TelemetryService/Triggers";

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

inline std::optional<std::string>
    getReportNameFromReportDefinitionUri(const std::string& uri)
{
    constexpr const char* uriPattern =
        "/redfish/v1/TelemetryService/MetricReportDefinitions/";
    constexpr size_t idx = std::string_view(uriPattern).length();
    if (boost::starts_with(uri, uriPattern))
    {
        return uri.substr(idx);
    }
    return std::nullopt;
}

inline std::optional<std::string>
    getTriggerIdFromDbusPath(const std::string& dbusPath)
{
    constexpr const char* triggerTree =
        "/xyz/openbmc_project/Telemetry/Triggers/TelemetryService/";
    constexpr size_t idx = std::string_view(triggerTree).length();
    if (boost::starts_with(dbusPath, triggerTree))
    {
        return dbusPath.substr(idx);
    }
    return std::nullopt;
}

inline bool
    updateStringCollection(crow::Response& res,
                           const std::string& collectionName,
                           const std::vector<std::string>& originalCollection,
                           std::vector<nlohmann::json> patchData,
                           std::vector<std::string>& resultCollection)
{
    size_t idx = 0;

    resultCollection.reserve(static_cast<size_t>(
        std::count_if(patchData.begin(), patchData.end(),
                      [](nlohmann::json& json) { return !json.is_null(); })));

    for (nlohmann::json& newVal : patchData)
    {
        if (newVal.is_null())
        {
            continue;
        }

        if (newVal.is_object() && newVal.empty())
        {
            if (idx >= originalCollection.size())
            {
                messages::propertyValueIncorrect(
                    res, collectionName + "/" + std::to_string(idx),
                    newVal.dump());
                return false;
            }
            resultCollection.emplace_back(originalCollection.at(idx));
        }

        if (std::string* stringVal = newVal.get_ptr<std::string*>();
            stringVal != nullptr)
        {
            resultCollection.emplace_back(*stringVal);
        }
        messages::propertyValueTypeError(
            res, newVal.dump(), collectionName + "/" + std::to_string(idx));
        return false;
    }
    return true;
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

        if (!boost::starts_with(uri, "/redfish/v1/Chassis/") ||
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

        if (boost::ends_with(node, "#"))
        {
            node.pop_back();
        }

        matched.emplace(std::move(chassis), std::move(node));
        uriIdx++;
    }
    return true;
}

inline std::optional<std::string>
    redfishActionToDbusAction(const std::string& redfishAction)
{
    if (redfishAction == "RedfishMetricReport")
    {
        return "UpdateReport";
    }
    if (redfishAction == "RedfishEvent")
    {
        return "RedfishEvent";
    }
    if (redfishAction == "LogToLogService")
    {
        return "LogToLogService";
    }
    return std::nullopt;
}

} // namespace telemetry
} // namespace redfish
