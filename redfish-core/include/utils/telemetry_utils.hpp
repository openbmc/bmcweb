#pragma once

#include "dbus_utility.hpp"
#include "generated/enums/metric_report_definition.hpp"
#include "http/utility.hpp"
#include "logging.hpp"
#include "utility.hpp"

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <string>

namespace redfish
{

namespace telemetry
{
constexpr const char* service = "xyz.openbmc_project.Telemetry";
constexpr const char* reportInterface = "xyz.openbmc_project.Telemetry.Report";

constexpr const char* metricDefinitionUri =
    "/redfish/v1/TelemetryService/MetricDefinitions/";
inline std::string getDbusReportPath(std::string_view id)
{
    sdbusplus::message::object_path reportsPath(
        "/xyz/openbmc_project/Telemetry/Reports/TelemetryService");
    return {reportsPath / id};
}

inline std::string getDbusTriggerPath(std::string_view id)
{
    sdbusplus::message::object_path triggersPath(
        "/xyz/openbmc_project/Telemetry/Triggers/TelemetryService");
    return {triggersPath / id};
}

inline std::optional<std::string>
    getTriggerIdFromDbusPath(const std::string& dbusPath)
{
    sdbusplus::message::object_path converted(dbusPath);

    if (converted.parent_path() !=
        "/xyz/openbmc_project/Telemetry/Triggers/TelemetryService")
    {
        return std::nullopt;
    }

    const std::string& id = converted.filename();
    if (id.empty())
    {
        return std::nullopt;
    }
    return id;
}

struct IncorrectMetricUri
{
    std::string uri;
    size_t index;
};

inline std::optional<IncorrectMetricUri> getChassisSensorNode(
    std::span<const std::string> uris,
    boost::container::flat_set<std::pair<std::string, std::string>>& matched)
{
    size_t uriIdx = 0;
    for (const std::string& uri : uris)
    {
        boost::system::result<boost::urls::url_view> parsed =
            boost::urls::parse_relative_ref(uri);

        if (!parsed)
        {
            BMCWEB_LOG_ERROR("Failed to get chassis and sensor Node "
                             "from {}",
                             uri);
            return std::make_optional<IncorrectMetricUri>({uri, uriIdx});
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

        BMCWEB_LOG_ERROR("Failed to get chassis and sensor Node "
                         "from {}",
                         uri);
        return std::make_optional<IncorrectMetricUri>({uri, uriIdx});
    }
    return std::nullopt;
}

inline metric_report_definition::CalculationAlgorithmEnum
    toRedfishCollectionFunction(std::string_view dbusValue)
{
    if (dbusValue ==
        "xyz.openbmc_project.Telemetry.Report.OperationType.Maximum")
    {
        return metric_report_definition::CalculationAlgorithmEnum::Maximum;
    }
    if (dbusValue ==
        "xyz.openbmc_project.Telemetry.Report.OperationType.Minimum")
    {
        return metric_report_definition::CalculationAlgorithmEnum::Minimum;
    }
    if (dbusValue ==
        "xyz.openbmc_project.Telemetry.Report.OperationType.Average")
    {
        return metric_report_definition::CalculationAlgorithmEnum::Average;
    }
    if (dbusValue ==
        "xyz.openbmc_project.Telemetry.Report.OperationType.Summation")
    {
        return metric_report_definition::CalculationAlgorithmEnum::Summation;
    }
    return metric_report_definition::CalculationAlgorithmEnum::Invalid;
}

inline std::string toDbusCollectionFunction(std::string_view redfishValue)
{
    if (redfishValue == "Maximum")
    {
        return "xyz.openbmc_project.Telemetry.Report.OperationType.Maximum";
    }
    if (redfishValue == "Minimum")
    {
        return "xyz.openbmc_project.Telemetry.Report.OperationType.Minimum";
    }
    if (redfishValue == "Average")
    {
        return "xyz.openbmc_project.Telemetry.Report.OperationType.Average";
    }
    if (redfishValue == "Summation")
    {
        return "xyz.openbmc_project.Telemetry.Report.OperationType.Summation";
    }
    return "";
}

inline std::optional<nlohmann::json::array_t>
    toRedfishCollectionFunctions(std::span<const std::string> dbusEnums)
{
    nlohmann::json::array_t redfishEnums;
    redfishEnums.reserve(dbusEnums.size());

    for (const auto& dbusValue : dbusEnums)
    {
        metric_report_definition::CalculationAlgorithmEnum redfishValue =
            toRedfishCollectionFunction(dbusValue);

        if (redfishValue ==
            metric_report_definition::CalculationAlgorithmEnum::Invalid)
        {
            return std::nullopt;
        }

        redfishEnums.emplace_back(redfishValue);
    }
    return redfishEnums;
}

} // namespace telemetry
} // namespace redfish
