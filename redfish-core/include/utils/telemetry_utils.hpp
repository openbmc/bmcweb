#pragma once

#include "dbus_utility.hpp"
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
        boost::urls::result<boost::urls::url_view> parsed =
            boost::urls::parse_relative_ref(uri);

        if (!parsed)
        {
            BMCWEB_LOG_ERROR << "Failed to get chassis and sensor Node "
                                "from "
                             << uri;
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

        BMCWEB_LOG_ERROR << "Failed to get chassis and sensor Node "
                            "from "
                         << uri;
        return std::make_optional<IncorrectMetricUri>({uri, uriIdx});
    }
    return std::nullopt;
}

inline std::string toRedfishCollectionFunction(std::string_view dbusValue)
{
    if (dbusValue ==
        "xyz.openbmc_project.Telemetry.Report.OperationType.Maximum")
    {
        return "Maximum";
    }
    if (dbusValue ==
        "xyz.openbmc_project.Telemetry.Report.OperationType.Minimum")
    {
        return "Minimum";
    }
    if (dbusValue ==
        "xyz.openbmc_project.Telemetry.Report.OperationType.Average")
    {
        return "Average";
    }
    if (dbusValue ==
        "xyz.openbmc_project.Telemetry.Report.OperationType.Summation")
    {
        return "Summation";
    }
    return "";
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

inline std::optional<std::vector<std::string>>
    toRedfishCollectionFunctions(std::span<const std::string> dbusEnums)
{
    std::vector<std::string> redfishEnums;
    redfishEnums.reserve(dbusEnums.size());

    for (const auto& dbusValue : dbusEnums)
    {
        std::string redfishValue = toRedfishCollectionFunction(dbusValue);

        if (redfishValue.empty())
        {
            return std::nullopt;
        }

        redfishEnums.emplace_back(redfishValue);
    }
    return redfishEnums;
}

} // namespace telemetry
} // namespace redfish
