#pragma once

#include "app.hpp"
#include "dbus_utility.hpp"
#include "generated/enums/metric_report_definition.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "sensors.hpp"
#include "utils/collection.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/telemetry_utils.hpp"
#include "utils/time_utils.hpp"

#include <boost/container/flat_map.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <map>
#include <string_view>
#include <tuple>
#include <variant>

namespace redfish
{

namespace telemetry
{

using ReadingParameters = std::vector<std::tuple<
    std::vector<std::tuple<sdbusplus::message::object_path, std::string>>,
    std::string, std::string, uint64_t>>;

enum class AddReportType
{
    create,
    replace
};

inline bool verifyCommonErrors(crow::Response& res, const std::string& id,
                               const boost::system::error_code ec)
{
    if (ec.value() == EBADR || ec == boost::system::errc::host_unreachable)
    {
        messages::resourceNotFound(res, "MetricReportDefinition", id);
        return false;
    }

    if (ec == boost::system::errc::file_exists)
    {
        messages::resourceAlreadyExists(res, "MetricReportDefinition", "Id",
                                        id);
        return false;
    }

    if (ec == boost::system::errc::too_many_files_open)
    {
        messages::createLimitReachedForResource(res);
        return false;
    }

    if (ec)
    {
        BMCWEB_LOG_ERROR << "DBUS response error " << ec;
        messages::internalError(res);
        return false;
    }

    return true;
}

inline metric_report_definition::ReportActionsEnum
    toRedfishReportAction(std::string_view dbusValue)
{

    if (dbusValue ==
        "xyz.openbmc_project.Telemetry.Report.ReportActions.EmitsReadingsUpdate")
    {
        return metric_report_definition::ReportActionsEnum::RedfishEvent;
    }
    if (dbusValue ==
        "xyz.openbmc_project.Telemetry.Report.ReportActions.LogToMetricReportsCollection")
    {
        return metric_report_definition::ReportActionsEnum::
            LogToMetricReportsCollection;
    }
    return metric_report_definition::ReportActionsEnum::Invalid;
}

inline std::string
    toDbusReportAction(metric_report_definition::ReportActionsEnum redfishValue)
{
    if (redfishValue ==
        metric_report_definition::ReportActionsEnum::RedfishEvent)
    {
        return "xyz.openbmc_project.Telemetry.Report.ReportActions.EmitsReadingsUpdate";
    }
    if (redfishValue == metric_report_definition::ReportActionsEnum::
                            LogToMetricReportsCollection)
    {
        return "xyz.openbmc_project.Telemetry.Report.ReportActions.LogToMetricReportsCollection";
    }
    return "";
}

inline metric_report_definition::MetricReportDefinitionType
    toRedfishReportingType(std::string_view dbusValue)
{
    if (dbusValue ==
        "xyz.openbmc_project.Telemetry.Report.ReportingType.OnChange")
    {
        return metric_report_definition::MetricReportDefinitionType::OnChange;
    }
    if (dbusValue ==
        "xyz.openbmc_project.Telemetry.Report.ReportingType.OnRequest")
    {
        return metric_report_definition::MetricReportDefinitionType::OnRequest;
    }
    if (dbusValue ==
        "xyz.openbmc_project.Telemetry.Report.ReportingType.Periodic")
    {
        return metric_report_definition::MetricReportDefinitionType::Periodic;
    }
    return metric_report_definition::MetricReportDefinitionType::Invalid;
}

inline std::string toDbusReportingType(
    metric_report_definition::MetricReportDefinitionType redfishValue)
{
    if (redfishValue ==
        metric_report_definition::MetricReportDefinitionType::OnChange)
    {
        return "xyz.openbmc_project.Telemetry.Report.ReportingType.OnChange";
    }
    if (redfishValue ==
        metric_report_definition::MetricReportDefinitionType::OnRequest)
    {
        return "xyz.openbmc_project.Telemetry.Report.ReportingType.OnRequest";
    }
    if (redfishValue ==
        metric_report_definition::MetricReportDefinitionType::Periodic)
    {
        return "xyz.openbmc_project.Telemetry.Report.ReportingType.Periodic";
    }
    return "";
}

inline metric_report_definition::CollectionTimeScope
    toRedfishCollectionTimeScope(std::string_view dbusValue)
{
    if (dbusValue ==
        "xyz.openbmc_project.Telemetry.Report.CollectionTimescope.Point")
    {
        return metric_report_definition::CollectionTimeScope::Point;
    }
    if (dbusValue ==
        "xyz.openbmc_project.Telemetry.Report.CollectionTimescope.Interval")
    {
        return metric_report_definition::CollectionTimeScope::Interval;
    }
    if (dbusValue ==
        "xyz.openbmc_project.Telemetry.Report.CollectionTimescope.StartupInterval")
    {
        return metric_report_definition::CollectionTimeScope::StartupInterval;
    }
    return metric_report_definition::CollectionTimeScope::Invalid;
}

inline std::string toDbusCollectionTimeScope(
    metric_report_definition::CollectionTimeScope redfishValue)
{
    if (redfishValue == metric_report_definition::CollectionTimeScope::Point)
    {
        return "xyz.openbmc_project.Telemetry.Report.CollectionTimescope.Point";
    }
    if (redfishValue == metric_report_definition::CollectionTimeScope::Interval)
    {
        return "xyz.openbmc_project.Telemetry.Report.CollectionTimescope.Interval";
    }
    if (redfishValue ==
        metric_report_definition::CollectionTimeScope::StartupInterval)
    {
        return "xyz.openbmc_project.Telemetry.Report.CollectionTimescope.StartupInterval";
    }
    return "";
}

inline metric_report_definition::ReportUpdatesEnum
    toRedfishReportUpdates(std::string_view dbusValue)
{
    if (dbusValue ==
        "xyz.openbmc_project.Telemetry.Report.ReportUpdates.Overwrite")
    {
        return metric_report_definition::ReportUpdatesEnum::Overwrite;
    }
    if (dbusValue ==
        "xyz.openbmc_project.Telemetry.Report.ReportUpdates.AppendWrapsWhenFull")
    {
        return metric_report_definition::ReportUpdatesEnum::AppendWrapsWhenFull;
    }
    if (dbusValue ==
        "xyz.openbmc_project.Telemetry.Report.ReportUpdates.AppendStopsWhenFull")
    {
        return metric_report_definition::ReportUpdatesEnum::AppendStopsWhenFull;
    }
    return metric_report_definition::ReportUpdatesEnum::Invalid;
}

inline std::string toDbusReportUpdates(
    metric_report_definition::ReportUpdatesEnum redfishValue)
{
    if (redfishValue == metric_report_definition::ReportUpdatesEnum::Overwrite)
    {
        return "xyz.openbmc_project.Telemetry.Report.ReportUpdates.Overwrite";
    }
    if (redfishValue ==
        metric_report_definition::ReportUpdatesEnum::AppendWrapsWhenFull)
    {
        return "xyz.openbmc_project.Telemetry.Report.ReportUpdates.AppendWrapsWhenFull";
    }
    if (redfishValue ==
        metric_report_definition::ReportUpdatesEnum::AppendStopsWhenFull)
    {
        return "xyz.openbmc_project.Telemetry.Report.ReportUpdates.AppendStopsWhenFull";
    }
    return "";
}

inline std::optional<nlohmann::json> getLinkedTriggers(
    std::span<const sdbusplus::message::object_path> triggerPaths)
{
    nlohmann::json triggers = nlohmann::json::array();

    for (const sdbusplus::message::object_path& path : triggerPaths)
    {
        if (path.parent_path() !=
            "/xyz/openbmc_project/Telemetry/Triggers/TelemetryService")
        {
            BMCWEB_LOG_ERROR << "Property Triggers contains invalid value: "
                             << path.str;
            return std::nullopt;
        }

        std::string id = path.filename();
        if (id.empty())
        {
            BMCWEB_LOG_ERROR << "Property Triggers contains invalid value: "
                             << path.str;
            return std::nullopt;
        }

        triggers.push_back({
            {"@odata.id",
             crow::utility::urlFromPieces("redfish", "v1", "TelemetryService",
                                          "Triggers", id)},
        });
    }

    return std::make_optional(triggers);
}

inline void
    fillReportDefinition(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& id,
                         const dbus::utility::DBusPropertiesMap& properties)
{
    std::vector<std::string> reportActions;
    ReadingParameters readingParams;
    std::string reportingType;
    std::string reportUpdates;
    std::string name;
    uint64_t appendLimit = 0;
    uint64_t interval = 0;
    bool enabled = false;
    std::vector<sdbusplus::message::object_path> triggers;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "ReportingType",
        reportingType, "Interval", interval, "ReportActions", reportActions,
        "ReportUpdates", reportUpdates, "AppendLimit", appendLimit,
        "ReadingParameters", readingParams, "Name", name, "Enabled", enabled,
        "Triggers", triggers);

    if (!success)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    metric_report_definition::MetricReportDefinitionType redfishReportingType =
        toRedfishReportingType(reportingType);
    if (redfishReportingType ==
        metric_report_definition::MetricReportDefinitionType::Invalid)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    asyncResp->res.jsonValue["MetricReportDefinitionType"] =
        redfishReportingType;

    std::optional<nlohmann::json> linkedTriggers = getLinkedTriggers(triggers);
    if (!linkedTriggers)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    asyncResp->res.jsonValue["Links"]["Triggers"] = *linkedTriggers;

    nlohmann::json::array_t redfishReportActions;
    for (const std::string& action : reportActions)
    {
        metric_report_definition::ReportActionsEnum redfishAction =
            toRedfishReportAction(action);
        if (redfishAction ==
            metric_report_definition::ReportActionsEnum::Invalid)
        {
            BMCWEB_LOG_ERROR << "Unknown ReportActions element: " << action;
            messages::internalError(asyncResp->res);
            return;
        }

        redfishReportActions.emplace_back(std::move(redfishAction));
    }

    asyncResp->res.jsonValue["ReportActions"] = std::move(redfishReportActions);

    nlohmann::json::array_t metrics = nlohmann::json::array();
    for (const auto& [sensorData, collectionFunction, collectionTimeScope,
                      collectionDuration] : readingParams)
    {
        nlohmann::json::array_t metricProperties;

        for (const auto& [sensorPath, sensorMetadata] : sensorData)
        {
            metricProperties.emplace_back(sensorMetadata);
        }

        nlohmann::json::object_t metric;

        std::string redfishCollectionFunction =
            telemetry::toRedfishCollectionFunction(collectionFunction);
        if (redfishCollectionFunction.empty())
        {
            messages::internalError(asyncResp->res);
            return;
        }
        metric["CollectionFunction"] = redfishCollectionFunction;

        metric_report_definition::CollectionTimeScope
            redfishCollectionTimeScope =
                toRedfishCollectionTimeScope(collectionTimeScope);
        if (redfishCollectionTimeScope ==
            metric_report_definition::CollectionTimeScope::Invalid)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        metric["CollectionTimeScope"] = redfishCollectionTimeScope;

        metric["MetricProperties"] = std::move(metricProperties);
        metric["CollectionDuration"] = time_utils::toDurationString(
            std::chrono::milliseconds(collectionDuration));
        metrics.push_back(std::move(metric));
    }
    asyncResp->res.jsonValue["Metrics"] = std::move(metrics);

    if (enabled)
    {
        asyncResp->res.jsonValue["Status"]["State"] = "Enabled";
    }
    else
    {
        asyncResp->res.jsonValue["Status"]["State"] = "Disabled";
    }

    metric_report_definition::ReportUpdatesEnum redfishReportUpdates =
        toRedfishReportUpdates(reportUpdates);
    if (redfishReportUpdates ==
        metric_report_definition::ReportUpdatesEnum::Invalid)
    {
        messages::internalError(asyncResp->res);
        return;
    }
    asyncResp->res.jsonValue["ReportUpdates"] = redfishReportUpdates;

    asyncResp->res.jsonValue["MetricReportDefinitionEnabled"] = enabled;
    asyncResp->res.jsonValue["AppendLimit"] = appendLimit;
    asyncResp->res.jsonValue["Name"] = name;
    asyncResp->res.jsonValue["Schedule"]["RecurrenceInterval"] =
        time_utils::toDurationString(std::chrono::milliseconds(interval));
    asyncResp->res.jsonValue["@odata.type"] =
        "#MetricReportDefinition.v1_3_0.MetricReportDefinition";
    asyncResp->res.jsonValue["@odata.id"] = crow::utility::urlFromPieces(
        "redfish", "v1", "TelemetryService", "MetricReportDefinitions", id);
    asyncResp->res.jsonValue["Id"] = id;
    asyncResp->res.jsonValue["MetricReport"]["@odata.id"] =
        crow::utility::urlFromPieces("redfish", "v1", "TelemetryService",
                                     "MetricReports", id);
}

struct AddReportArgs
{
    struct MetricArgs
    {
        std::vector<std::string> uris;
        std::string collectionFunction;
        std::string collectionTimeScope;
        uint64_t collectionDuration = 0;
    };

    std::string id;
    std::string name;
    std::string reportingType;
    std::string reportUpdates;
    uint64_t appendLimit = std::numeric_limits<uint64_t>::max();
    std::vector<std::string> reportActions;
    uint64_t interval = std::numeric_limits<uint64_t>::max();
    std::vector<MetricArgs> metrics;
    bool metricReportDefinitionEnabled = true;
};

class ErrorVerificator
{
  public:
    ErrorVerificator(
        crow::Response& resIn, boost::system::error_code ecIn,
        const sdbusplus::message_t& msgIn = sdbusplus::message_t()) :
        res(resIn),
        ec(ecIn), msg(msgIn)
    {}

    bool verifyId(const std::optional<std::string>& id)
    {
        if (id && ec == boost::system::errc::invalid_argument &&
            getError() == "Id")
        {
            messages::propertyValueIncorrect(res, *id, "Id");
            return false;
        }

        return true;
    }

    bool verifyName(const std::optional<std::string>& name)
    {
        if (name && ec == boost::system::errc::invalid_argument &&
            getError() == "Name")
        {
            messages::propertyValueIncorrect(res, *name, "Name");
            return false;
        }

        return true;
    }

    bool verifyReportingType(
        const std::optional<
            metric_report_definition::MetricReportDefinitionType>&
            reportingType)
    {
        if (reportingType && ec == boost::system::errc::invalid_argument &&
            getError() == "ReportingType")
        {
            messages::propertyValueIncorrect(
                res, nlohmann::json(*reportingType).dump(),
                "MetricReportDefinitionType");
            return false;
        }

        return true;
    }

    bool verifyReportingType(const std::optional<std::string>& reportingType)
    {
        return verifyReportingType(
            json_util::toEnum<
                metric_report_definition::MetricReportDefinitionType>(
                reportingType));
    }

    bool verifyAppendLimit(const std::optional<uint64_t>& appendLimit)
    {
        if (appendLimit && ec == boost::system::errc::invalid_argument &&
            getError() == "AppendLimit")
        {
            messages::propertyValueIncorrect(res, std::to_string(*appendLimit),
                                             "AppendLimit");
            return false;
        }

        return true;
    }

    bool verifyReportActions(
        const std::optional<
            std::vector<metric_report_definition::ReportActionsEnum>>&
            reportActions)
    {
        if (reportActions && ec == boost::system::errc::invalid_argument &&
            getError() == "ReportActions")
        {
            nlohmann::json tmp = nlohmann::json::array();
            tmp = *reportActions;
            messages::propertyValueIncorrect(res, tmp.dump(), "ReportActions");
            return false;
        }

        return true;
    }

    bool verifyReportActions(
        const std::optional<std::vector<std::string>>& reportActions)
    {
        return verifyReportActions(
            json_util::toEnum<metric_report_definition::ReportActionsEnum>(
                reportActions));
    }

    bool verifyRecurrenceInterval(
        const std::optional<std::string>& recurrenceIntervalStr)
    {
        if (recurrenceIntervalStr &&
            ec == boost::system::errc::invalid_argument &&
            getError() == "Interval")
        {
            messages::propertyValueIncorrect(res, *recurrenceIntervalStr,
                                             "RecurrenceInterval");
            return false;
        }

        return true;
    }

    bool verifyReportUpdates(
        const std::optional<metric_report_definition::ReportUpdatesEnum>&
            reportUpdates)
    {
        if (reportUpdates && ec == boost::system::errc::invalid_argument &&
            getError() == "ReportUpdates")
        {
            messages::propertyValueIncorrect(
                res, nlohmann::json(*reportUpdates).dump(), "ReportUpdates");
            return false;
        }

        return true;
    }

    bool verifyReportUpdates(const std::optional<std::string>& reportUpdates)
    {
        return verifyReportUpdates(
            json_util::toEnum<metric_report_definition::ReportUpdatesEnum>(
                reportUpdates));
    }

    bool verifyReadingParameters(
        const std::optional<std::vector<nlohmann::json>>& redfishMetrics)
    {
        if (redfishMetrics && ec == boost::system::errc::invalid_argument &&
            getError().starts_with("ReadingParameters"))
        {
            nlohmann::json readingParameters = nlohmann::json::array();
            readingParameters = *redfishMetrics;

            messages::propertyValueIncorrect(res, readingParameters.dump(),
                                             "MetricProperties");
            return false;
        }

        return true;
    }

    bool verify(const std::optional<std::string>& id)
    {
        if ((ec.value() == EBADR ||
             ec == boost::system::errc::host_unreachable))
        {
            messages::resourceNotFound(res, "MetricReportDefinition",
                                       id.value_or(""));
            return false;
        }

        if (ec == boost::system::errc::file_exists)
        {
            messages::resourceAlreadyExists(res, "MetricReportDefinition", "Id",
                                            id.value_or(""));
            return false;
        }

        if (ec == boost::system::errc::too_many_files_open)
        {
            messages::createLimitReachedForResource(res);
            return false;
        }

        if (ec)
        {
            messages::internalError(res);
            BMCWEB_LOG_ERROR << "respHandler DBus error " << ec;
            return false;
        }

        return true;
    }

  private:
    std::string_view getError() const
    {
        const sd_bus_error* dbusError = msg.get_error();
        if (dbusError == nullptr)
        {
            return "";
        }

        return dbusError->message;
    }

    crow::Response& res;
    boost::system::error_code ec;
    const sdbusplus::message_t& msg;
};

inline bool toDbusReportActions(
    crow::Response& res,
    std::span<const metric_report_definition::ReportActionsEnum> actions,
    std::vector<std::string>& outReportActions)
{
    outReportActions.clear();

    size_t index = 0;
    for (const auto& action : actions)
    {
        std::string dbusReportAction = toDbusReportAction(action);

        if (dbusReportAction.empty())
        {
            messages::propertyValueNotInList(res, nlohmann::json(action).dump(),
                                             "ReportActions/" +
                                                 std::to_string(index));
            return false;
        }

        outReportActions.emplace_back(std::move(dbusReportAction));
        index++;
    }
    return true;
}

inline bool getUserMetric(crow::Response& res, nlohmann::json& metric,
                          AddReportArgs::MetricArgs& metricArgs)
{
    std::optional<std::vector<std::string>> uris;
    std::optional<std::string> collectionDurationStr;
    std::optional<std::string> collectionFunction;
    std::optional<std::string> collectionTimeScopeStr;

    if (!json_util::readJson(metric, res, "MetricProperties", uris,
                             "CollectionFunction", collectionFunction,
                             "CollectionTimeScope", collectionTimeScopeStr,
                             "CollectionDuration", collectionDurationStr))
    {
        return false;
    }

    std::optional<metric_report_definition::CollectionTimeScope>
        collectionTimeScope =
            json_util::toEnum<metric_report_definition::CollectionTimeScope>(
                collectionTimeScopeStr);

    if (uris)
    {
        metricArgs.uris = std::move(*uris);
    }

    if (collectionFunction)
    {
        std::string dbusCollectionFunction =
            telemetry::toDbusCollectionFunction(*collectionFunction);
        if (dbusCollectionFunction.empty())
        {
            messages::propertyValueIncorrect(res, "CollectionFunction",
                                             *collectionFunction);
            return false;
        }
        metricArgs.collectionFunction = dbusCollectionFunction;
    }

    if (collectionTimeScope)
    {
        std::string dbusCollectionTimeScope =
            toDbusCollectionTimeScope(*collectionTimeScope);
        if (dbusCollectionTimeScope.empty())
        {
            messages::propertyValueIncorrect(
                res, "CollectionTimeScope",
                nlohmann::json(*collectionTimeScope).dump());
            return false;
        }
        metricArgs.collectionTimeScope = dbusCollectionTimeScope;
    }

    if (collectionDurationStr)
    {
        std::optional<std::chrono::milliseconds> duration =
            time_utils::fromDurationString(*collectionDurationStr);

        if (!duration || duration->count() < 0)
        {
            messages::propertyValueIncorrect(res, "CollectionDuration",
                                             *collectionDurationStr);
            return false;
        }

        metricArgs.collectionDuration =
            static_cast<uint64_t>(duration->count());
    }

    return true;
}

inline bool getUserMetrics(crow::Response& res,
                           std::span<nlohmann::json> metrics,
                           std::vector<AddReportArgs::MetricArgs>& result)
{
    result.reserve(metrics.size());

    for (nlohmann::json& m : metrics)
    {
        AddReportArgs::MetricArgs metricArgs;

        if (!getUserMetric(res, m, metricArgs))
        {
            return false;
        }

        result.emplace_back(std::move(metricArgs));
    }

    return true;
}

class ReportUserArgs
{
  public:
    bool getUserParameters(crow::Response& res, const crow::Request& req,
                           AddReportArgs& args)
    {
        if (!json_util::readJsonPatch(
                req, res, "Id", id, "Name", name, "Metrics", metrics,
                "MetricReportDefinitionType", reportingTypeStr, "ReportUpdates",
                reportUpdatesStr, "AppendLimit", appendLimit, "ReportActions",
                reportActionsStr, "Schedule", schedule,
                "MetricReportDefinitionEnabled", metricReportDefinitionEnabled))
        {
            return false;
        }

        std::optional<metric_report_definition::MetricReportDefinitionType>
            reportingType = json_util::toEnum<
                metric_report_definition::MetricReportDefinitionType>(
                reportingTypeStr);
        std::optional<metric_report_definition::ReportUpdatesEnum>
            reportUpdates =
                json_util::toEnum<metric_report_definition::ReportUpdatesEnum>(
                    reportUpdatesStr);
        std::optional<std::vector<metric_report_definition::ReportActionsEnum>>
            reportActions =
                json_util::toEnum<metric_report_definition::ReportActionsEnum>(
                    reportActionsStr);

        if (id)
        {
            constexpr const char* allowedCharactersInId =
                "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
            if (id->empty() || id->find_first_not_of(allowedCharactersInId) !=
                                   std::string::npos)
            {
                messages::propertyValueIncorrect(res, "Id", *id);
                return false;
            }
            args.id = *id;
        }

        if (name)
        {
            args.name = *name;
        }

        if (reportingType)
        {
            std::string dbusReportingType = toDbusReportingType(*reportingType);
            if (dbusReportingType.empty())
            {
                messages::propertyValueNotInList(
                    res, nlohmann::json(*reportingType).dump(),
                    "MetricReportDefinitionType");
                return false;
            }
            args.reportingType = dbusReportingType;
        }

        if (reportUpdates)
        {
            std::string dbusReportUpdates = toDbusReportUpdates(*reportUpdates);
            if (dbusReportUpdates.empty())
            {
                messages::propertyValueNotInList(
                    res, nlohmann::json(*reportUpdates).dump(),
                    "ReportUpdates");
                return false;
            }
            args.reportUpdates = dbusReportUpdates;
        }

        if (appendLimit)
        {
            args.appendLimit = *appendLimit;
        }

        if (metricReportDefinitionEnabled)
        {
            args.metricReportDefinitionEnabled = *metricReportDefinitionEnabled;
        }

        if (reportActions &&
            !toDbusReportActions(res, *reportActions, args.reportActions))
        {
            return false;
        }

        if (reportingType ==
            metric_report_definition::MetricReportDefinitionType::Periodic)
        {
            if (!schedule)
            {
                messages::createFailedMissingReqProperties(res, "Schedule");
                return false;
            }

            if (!json_util::readJson(*schedule, res, "RecurrenceInterval",
                                     durationStr) ||
                !durationStr)
            {
                return false;
            }

            std::optional<std::chrono::milliseconds> durationNum =
                time_utils::fromDurationString(*durationStr);
            if (!durationNum || durationNum->count() < 0)
            {
                messages::propertyValueIncorrect(res, "RecurrenceInterval",
                                                 *durationStr);
                return false;
            }
            args.interval = static_cast<uint64_t>(durationNum->count());
        }

        if (metrics)
        {
            if (!getUserMetrics(res, *metrics, args.metrics))
            {
                return false;
            }
        }

        return true;
    }

    std::optional<std::string> id;
    std::optional<std::string> name;
    std::optional<std::string> reportingTypeStr;
    std::optional<std::string> reportUpdatesStr;
    std::optional<uint64_t> appendLimit;
    std::optional<bool> metricReportDefinitionEnabled;
    std::optional<std::vector<nlohmann::json>> metrics;
    std::optional<std::vector<std::string>> reportActionsStr;
    std::optional<nlohmann::json> schedule;
    std::optional<std::string> durationStr;
};

inline bool getChassisSensorNodeFromMetrics(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    std::span<const AddReportArgs::MetricArgs> metrics,
    boost::container::flat_set<std::pair<std::string, std::string>>& matched)
{
    for (const auto& metric : metrics)
    {
        std::optional<IncorrectMetricProperty> error =
            getChassisSensorNode(metric.uris, matched);
        if (error)
        {
            messages::propertyValueIncorrect(
                asyncResp->res, error->metricProperty,
                "MetricProperties/" + std::to_string(error->index));
            return false;
        }
    }
    return true;
}

class AddReport
{
  public:
    AddReport(AddReportArgs argsIn,
              const std::shared_ptr<bmcweb::AsyncResp>& asyncRespIn,
              AddReportType typeIn, ReportUserArgs userArgsIn) :
        asyncResp(asyncRespIn),
        args(std::move(argsIn)), userArgs(std::move(userArgsIn)), type(typeIn)
    {}

    ~AddReport()
    {
        boost::asio::post(crow::connections::systemBus->get_io_context(),
                          std::bind_front(&performAddReport, asyncResp, args,
                                          userArgs, std::move(uriToDbus),
                                          type));
    }

    static void performAddReport(
        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
        const AddReportArgs& args, const ReportUserArgs& userArgs,
        const boost::container::flat_map<std::string, std::string>& uriToDbus,
        AddReportType type)
    {
        if (asyncResp->res.result() != boost::beast::http::status::ok)
        {
            return;
        }

        telemetry::ReadingParameters readingParams;
        readingParams.reserve(args.metrics.size());

        for (const auto& metric : args.metrics)
        {
            std::vector<
                std::tuple<sdbusplus::message::object_path, std::string>>
                sensorParams;
            sensorParams.reserve(metric.uris.size());

            for (size_t i = 0; i < metric.uris.size(); i++)
            {
                const std::string& uri = metric.uris[i];
                auto el = uriToDbus.find(uri);
                if (el == uriToDbus.end())
                {
                    BMCWEB_LOG_ERROR
                        << "Failed to find DBus sensor corresponding to URI "
                        << uri;
                    messages::propertyValueNotInList(asyncResp->res, uri,
                                                     "MetricProperties/" +
                                                         std::to_string(i));
                    return;
                }

                const std::string& dbusPath = el->second;
                sensorParams.emplace_back(dbusPath, uri);
            }

            readingParams.emplace_back(
                std::move(sensorParams), metric.collectionFunction,
                metric.collectionTimeScope, metric.collectionDuration);
        }

        crow::connections::systemBus->async_method_call(
            [asyncResp, id = args.id, userArgs, uriToDbus,
             type](const boost::system::error_code& ec,
                   const sdbusplus::message_t& msg, const std::string&) {
            ErrorVerificator ev(asyncResp->res, ec, msg);
            if (!ev.verifyId(userArgs.id) || !ev.verifyName(userArgs.name) ||
                !ev.verifyReportingType(userArgs.reportingTypeStr) ||
                !ev.verifyAppendLimit(userArgs.appendLimit) ||
                !ev.verifyReportActions(userArgs.reportActionsStr) ||
                !ev.verifyRecurrenceInterval(userArgs.durationStr) ||
                !ev.verifyReportUpdates(userArgs.reportUpdatesStr) ||
                !ev.verifyReadingParameters(userArgs.metrics) ||
                !ev.verify(userArgs.id))
            {
                return;
            }

            if (type == AddReportType::create)
            {
                messages::created(asyncResp->res);
            }
            else
            {
                messages::success(asyncResp->res);
            }
            },
            telemetry::service, "/xyz/openbmc_project/Telemetry/Reports",
            "xyz.openbmc_project.Telemetry.ReportManager", "AddReport",
            "TelemetryService/" + args.id, args.name, args.reportingType,
            args.reportUpdates, args.appendLimit, args.reportActions,
            args.interval, readingParams, args.metricReportDefinitionEnabled);
    }

    AddReport(const AddReport&) = delete;
    AddReport(AddReport&&) = delete;
    AddReport& operator=(const AddReport&) = delete;
    AddReport& operator=(AddReport&&) = delete;

    void insert(const std::map<std::string, std::string>& el)
    {
        uriToDbus.insert(el.begin(), el.end());
    }

  private:
    std::shared_ptr<bmcweb::AsyncResp> asyncResp;
    AddReportArgs args;
    ReportUserArgs userArgs;
    boost::container::flat_map<std::string, std::string> uriToDbus{};
    AddReportType type;
};

void retrieveMetricPropertyToDbusPaths(
    boost::container::flat_set<std::pair<std::string, std::string>>
        chassisSensors,
    std::function<void(const std::map<std::string, std::string>& uriToDbus)>
        callback)
{
    for (const auto& [chassis, sensorType] : chassisSensors)
    {
        retrieveUriToDbusMap(
            chassis, sensorType,
            [callback](const boost::beast::http::status status,
                       const std::map<std::string, std::string>& uriToDbus) {
            if (status != boost::beast::http::status::ok)
            {
                BMCWEB_LOG_ERROR
                    << "Failed to retrieve URI to dbus sensors map with err "
                    << static_cast<unsigned>(status);

                callback({});
                return;
            }
            callback(uriToDbus);
            });
    }
}

class UpdateMetrics
{
  public:
    UpdateMetrics(std::string_view idIn,
                  const std::shared_ptr<bmcweb::AsyncResp>& asyncRespIn,
                  std::span<const nlohmann::json> redfishMetricsIn) :
        id(idIn),
        asyncResp(asyncRespIn),
        redfishMetrics(redfishMetricsIn.begin(), redfishMetricsIn.end())
    {}

    ~UpdateMetrics()
    {
        try
        {
            setReadingParams();
        }
        catch (const std::exception& e)
        {
            BMCWEB_LOG_ERROR << e.what();
        }
        catch (...)
        {
            BMCWEB_LOG_ERROR << "Unknown error";
        }
    }

    UpdateMetrics(const UpdateMetrics&) = delete;
    UpdateMetrics(UpdateMetrics&&) = delete;
    UpdateMetrics& operator=(const UpdateMetrics&) = delete;
    UpdateMetrics& operator=(UpdateMetrics&&) = delete;

    std::string id;
    std::map<std::string, std::string> metricPropertyToDbusPaths;

    void insert(const std::map<std::string, std::string>&
                    additionalMetricPropertyToDbusPaths)
    {
        metricPropertyToDbusPaths.insert(
            additionalMetricPropertyToDbusPaths.begin(),
            additionalMetricPropertyToDbusPaths.end());
    }

    void emplace(std::span<const std::tuple<sdbusplus::message::object_path,
                                            std::string>>
                     pathAndUri,
                 const AddReportArgs::MetricArgs metricArgs)
    {
        readingParamsUris.emplace_back(metricArgs.uris);
        readingParams.emplace_back(
            std::vector(pathAndUri.begin(), pathAndUri.end()),
            metricArgs.collectionFunction, metricArgs.collectionTimeScope,
            metricArgs.collectionDuration);
    }

    void setReadingParams()
    {
        if (asyncResp->res.result() != boost::beast::http::status::ok)
        {
            return;
        }

        for (size_t index = 0; index < readingParamsUris.size(); ++index)
        {
            std::span<const std::string> newUris = readingParamsUris[index];

            const std::optional<std::vector<
                std::tuple<sdbusplus::message::object_path, std::string>>>
                readingParam = sensorPathToUri(newUris);

            if (!readingParam)
            {
                return;
            }

            for (const std::tuple<sdbusplus::message::object_path, std::string>&
                     value : *readingParam)
            {
                std::get<0>(readingParams[index]).emplace_back(value);
            }
        }

        crow::connections::systemBus->async_method_call(
            [aResp = asyncResp, reportId = id,
             metrics = redfishMetrics](const boost::system::error_code ec,
                                       const sdbusplus::message_t& msg) {
            ErrorVerificator ev(aResp->res, ec, msg);
            if (!ev.verifyReadingParameters(metrics) || !ev.verify(reportId))
            {
                return;
            }
            },
            "xyz.openbmc_project.Telemetry", getDbusReportPath(id),
            "org.freedesktop.DBus.Properties", "Set",
            "xyz.openbmc_project.Telemetry.Report", "ReadingParameters",
            dbus::utility::DbusVariantType{readingParams});
    }

  private:
    std::optional<
        std::vector<std::tuple<sdbusplus::message::object_path, std::string>>>
        sensorPathToUri(std::span<const std::string> uris) const
    {
        std::vector<std::tuple<sdbusplus::message::object_path, std::string>>
            result;

        for (const std::string& uri : uris)
        {
            auto it = metricPropertyToDbusPaths.find(uri);
            if (it == metricPropertyToDbusPaths.end())
            {
                messages::propertyValueNotInList(asyncResp->res, uri,
                                                 "MetricProperties");
                return {};
            }
            result.emplace_back(it->second, uri);
        }

        return result;
    }

    const std::shared_ptr<bmcweb::AsyncResp> asyncResp;
    std::vector<std::vector<std::string>> readingParamsUris;
    ReadingParameters readingParams{};
    std::vector<nlohmann::json> redfishMetrics;
};

inline void setReportEnabled(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                             std::string_view id, bool enabled)
{
    crow::connections::systemBus->async_method_call(
        [aResp, id = std::string(id)](const boost::system::error_code ec,
                                      const sdbusplus::message_t& msg) {
        ErrorVerificator ev(aResp->res, ec, msg);
        if (!ev.verify(id))
        {
            return;
        }
        },
        "xyz.openbmc_project.Telemetry", getDbusReportPath(id),
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Telemetry.Report", "Enabled",
        dbus::utility::DbusVariantType{enabled});
}

inline void setReportTypeAndInterval(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp, std::string_view id,
    const std::optional<metric_report_definition::MetricReportDefinitionType>&
        reportingType,
    std::optional<nlohmann::json>& schedule)
{
    if (!reportingType && !schedule)
    {
        return;
    }

    std::string dbusReportingType;
    if (reportingType)
    {
        dbusReportingType = toDbusReportingType(*reportingType);
        if (dbusReportingType.empty())
        {
            messages::propertyValueNotInList(
                aResp->res, nlohmann::json(*reportingType).dump(),
                "MetricReportDefinitionType");
            return;
        }
    }

    uint64_t recurrenceInterval = std::numeric_limits<uint64_t>::max();
    std::string durationStr;

    if (schedule)
    {
        if (!json_util::readJson(*schedule, aResp->res, "RecurrenceInterval",
                                 durationStr))
        {
            return;
        }

        std::optional<std::chrono::milliseconds> durationNum =
            time_utils::fromDurationString(durationStr);
        if (!durationNum || durationNum->count() < 0)
        {
            messages::propertyValueIncorrect(aResp->res, "RecurrenceInterval",
                                             durationStr);
            return;
        }

        recurrenceInterval = static_cast<uint64_t>(durationNum->count());
    }

    crow::connections::systemBus->async_method_call(
        [aResp, id = std::string(id), reportingType,
         durationStr](const boost::system::error_code ec,
                      const sdbusplus::message_t& msg) {
        ErrorVerificator ev(aResp->res, ec, msg);
        if (!ev.verifyReportingType(reportingType) ||
            !ev.verifyRecurrenceInterval(durationStr) || !ev.verify(id))
        {
            return;
        }
        },
        "xyz.openbmc_project.Telemetry", getDbusReportPath(id),
        "xyz.openbmc_project.Telemetry.Report", "SetReportingProperties",
        dbusReportingType, recurrenceInterval);
}

inline void setReportUpdates(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp, std::string_view id,
    const metric_report_definition::ReportUpdatesEnum reportUpdates)
{
    std::string dbusReportUpdates = toDbusReportUpdates(reportUpdates);
    if (dbusReportUpdates.empty())
    {
        messages::propertyValueNotInList(
            aResp->res, nlohmann::json(reportUpdates).dump(), "ReportUpdates");
        return;
    }

    crow::connections::systemBus->async_method_call(
        [aResp, id = std::string(id), redfishReportUpdates = reportUpdates](
            const boost::system::error_code ec,
            const sdbusplus::message_t& msg) {
        ErrorVerificator ev(aResp->res, ec, msg);
        if (!ev.verifyReportUpdates(redfishReportUpdates) || !ev.verify(id))
        {
            return;
        }
        },
        "xyz.openbmc_project.Telemetry", getDbusReportPath(id),
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Telemetry.Report", "ReportUpdates",
        dbus::utility::DbusVariantType{dbusReportUpdates});
}

inline void setReportActions(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp, std::string_view id,
    const std::vector<metric_report_definition::ReportActionsEnum>&
        reportActions)
{
    std::vector<std::string> dbusReportActions;
    if (!toDbusReportActions(aResp->res, reportActions, dbusReportActions))
    {
        return;
    }

    crow::connections::systemBus->async_method_call(
        [aResp, id = std::string(id), redfishReportActions = reportActions](
            const boost::system::error_code ec,
            const sdbusplus::message_t& msg) {
        ErrorVerificator ev(aResp->res, ec, msg);
        if (!ev.verifyReportActions(redfishReportActions) || !ev.verify(id))
        {
            return;
        }
        },
        "xyz.openbmc_project.Telemetry", getDbusReportPath(id),
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Telemetry.Report", "ReportActions",
        dbus::utility::DbusVariantType{dbusReportActions});
}

inline void
    setReportMetrics(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     std::string_view id, std::span<nlohmann::json> metrics)
{
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, telemetry::service,
        telemetry::getDbusReportPath(id), telemetry::reportInterface,
        [asyncResp, id = std::string(id),
         redfishMetrics =
             std::vector<nlohmann::json>(metrics.begin(), metrics.end())](
            boost::system::error_code ec,
            const dbus::utility::DBusPropertiesMap& properties) mutable {
        ErrorVerificator ev(asyncResp->res, ec);
        if (!ev.verify(id))
        {
            return;
        }

        ReadingParameters readingParams;

        const bool success = sdbusplus::unpackPropertiesNoThrow(
            dbus_utils::UnpackErrorPrinter(), properties, "ReadingParameters",
            readingParams);

        if (!success)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        std::shared_ptr<UpdateMetrics> updateMetricsReq =
            std::make_shared<UpdateMetrics>(id, asyncResp, redfishMetrics);

        boost::container::flat_set<std::pair<std::string, std::string>>
            chassisSensors;

        for (size_t index = 0;
             index < redfishMetrics.size() && index < redfishMetrics.size();
             ++index)
        {
            nlohmann::json& metric = redfishMetrics[index];

            if (metric.is_null())
            {
                continue;
            }

            AddReportArgs::MetricArgs metricArgs;
            std::vector<
                std::tuple<sdbusplus::message::object_path, std::string>>
                pathAndUri;

            if (index < readingParams.size())
            {
                const ReadingParameters::value_type& existing =
                    readingParams[index];

                if (metric.find("MetricProperties") == metric.end())
                {
                    pathAndUri = std::get<0>(existing);
                }
                metricArgs.collectionFunction = std::get<1>(existing);
                metricArgs.collectionTimeScope = std::get<2>(existing);
                metricArgs.collectionDuration = std::get<3>(existing);
            }

            if (!getUserMetric(asyncResp->res, metric, metricArgs))
            {
                return;
            }

            std::optional<IncorrectMetricProperty> error =
                getChassisSensorNode(metricArgs.uris, chassisSensors);

            if (error)
            {
                messages::propertyValueIncorrect(
                    asyncResp->res, error->metricProperty,
                    "MetricProperties/" + std::to_string(error->index));
                return;
            }

            updateMetricsReq->emplace(pathAndUri, metricArgs);
        }

        retrieveMetricPropertyToDbusPaths(
            chassisSensors, [asyncResp, updateMetricsReq](
                                const std::map<std::string, std::string>&
                                    metricPropertyToDbusPaths) {
                updateMetricsReq->insert(metricPropertyToDbusPaths);
            });
        });
}

inline void handleReportPatch(const crow::Request& req,
                              const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                              std::string_view id)
{
    std::optional<std::string> name;
    std::optional<std::string> reportingTypeStr;
    std::optional<std::string> reportUpdatesStr;
    std::optional<uint64_t> appendLimit;
    std::optional<bool> metricReportDefinitionEnabled;
    std::optional<std::vector<nlohmann::json>> metrics;
    std::optional<std::vector<std::string>> reportActionsStr;
    std::optional<nlohmann::json> schedule;

    if (!json_util::readJsonPatch(
            req, aResp->res, "Name", name, "Metrics", metrics,
            "MetricReportDefinitionType", reportingTypeStr, "ReportUpdates",
            reportUpdatesStr, "AppendLimit", appendLimit, "ReportActions",
            reportActionsStr, "Schedule", schedule,
            "MetricReportDefinitionEnabled", metricReportDefinitionEnabled))
    {
        return;
    }

    std::optional<metric_report_definition::MetricReportDefinitionType>
        reportingType = json_util::toEnum<
            metric_report_definition::MetricReportDefinitionType>(
            reportingTypeStr);
    std::optional<metric_report_definition::ReportUpdatesEnum> reportUpdates =
        json_util::toEnum<metric_report_definition::ReportUpdatesEnum>(
            reportUpdatesStr);
    std::optional<std::vector<metric_report_definition::ReportActionsEnum>>
        reportActions =
            json_util::toEnum<metric_report_definition::ReportActionsEnum>(
                reportActionsStr);

    if (metricReportDefinitionEnabled)
    {
        setReportEnabled(aResp, id, *metricReportDefinitionEnabled);
    }

    if (reportUpdates)
    {
        setReportUpdates(aResp, id, *reportUpdates);
    }

    if (reportActions)
    {
        setReportActions(aResp, id, *reportActions);
    }

    setReportTypeAndInterval(aResp, id, reportingType, schedule);

    if (metrics)
    {
        setReportMetrics(aResp, id, *metrics);
    }
}

inline void handleReportPut(const crow::Request& req,
                            const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                            std::string_view id)
{
    AddReportArgs args;
    ReportUserArgs userArgs;

    if (!userArgs.getUserParameters(aResp->res, req, args))
    {
        return;
    }

    boost::container::flat_set<std::pair<std::string, std::string>>
        chassisSensors;
    if (!getChassisSensorNodeFromMetrics(aResp, args.metrics, chassisSensors))
    {
        return;
    }

    const std::string reportPath = getDbusReportPath(id);

    crow::connections::systemBus->async_method_call(
        [aResp, reportId = std::string(id), args = std::move(args),
         userArgs = std::move(userArgs),
         chassisSensors =
             std::move(chassisSensors)](const boost::system::error_code ec) {
        AddReportType addReportMode = AddReportType::replace;
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR << "respHandler DBus error " << ec;
                messages::internalError(aResp->res);
                return;
            }
            BMCWEB_LOG_INFO << "Report not found, creating new report: "
                            << reportId;
            addReportMode = AddReportType::create;
        }

        auto addReportReq = std::make_shared<AddReport>(
            std::move(args), aResp, addReportMode, std::move(userArgs));
        retrieveMetricPropertyToDbusPaths(
            chassisSensors,
            [aResp, addReportReq](const std::map<std::string, std::string>&
                                      metricPropertyToDbusPaths) {
            addReportReq->insert(metricPropertyToDbusPaths);
            });
        },
        service, reportPath, "xyz.openbmc_project.Object.Delete", "Delete");
}

inline void handleReportDelete(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                               std::string_view id)
{
    const std::string reportPath = getDbusReportPath(id);

    crow::connections::systemBus->async_method_call(
        [aResp, reportId = std::string(id)](const boost::system::error_code ec,
                                            const sdbusplus::message_t& msg) {
        ErrorVerificator ev(aResp->res, ec, msg);
        if (!ev.verify(reportId))
        {
            return;
        }
        aResp->res.result(boost::beast::http::status::no_content);
        },
        service, reportPath, "xyz.openbmc_project.Object.Delete", "Delete");
}
} // namespace telemetry

inline void requestRoutesMetricReportDefinitionCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/TelemetryService/MetricReportDefinitions/")
        .privileges(redfish::privileges::getMetricReportDefinitionCollection)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }

        asyncResp->res.jsonValue["@odata.type"] =
            "#MetricReportDefinitionCollection."
            "MetricReportDefinitionCollection";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/TelemetryService/MetricReportDefinitions";
        asyncResp->res.jsonValue["Name"] = "Metric Definition Collection";
        constexpr std::array<std::string_view, 1> interfaces{
            telemetry::reportInterface};
        collection_util::getCollectionMembers(
            asyncResp,
            boost::urls::url(
                "/redfish/v1/TelemetryService/MetricReportDefinitions"),
            interfaces,
            "/xyz/openbmc_project/Telemetry/Reports/TelemetryService");
        });

    BMCWEB_ROUTE(app, "/redfish/v1/TelemetryService/MetricReportDefinitions/")
        .privileges(redfish::privileges::postMetricReportDefinitionCollection)
        .methods(boost::beast::http::verb::post)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }

        telemetry::AddReportArgs args;
        telemetry::ReportUserArgs userArgs;
        if (!userArgs.getUserParameters(asyncResp->res, req, args))
        {
            return;
        }

        boost::container::flat_set<std::pair<std::string, std::string>>
            chassisSensors;
        if (!telemetry::getChassisSensorNodeFromMetrics(asyncResp, args.metrics,
                                                        chassisSensors))
        {
            return;
        }

        auto addReportReq = std::make_shared<telemetry::AddReport>(
            std::move(args), asyncResp, telemetry::AddReportType::create,
            std::move(userArgs));
        for (const auto& [chassis, sensorType] : chassisSensors)
        {
            retrieveUriToDbusMap(
                chassis, sensorType,
                [asyncResp, addReportReq](
                    const boost::beast::http::status status,
                    const std::map<std::string, std::string>& uriToDbus) {
                if (status != boost::beast::http::status::ok)
                {
                    BMCWEB_LOG_ERROR
                        << "Failed to retrieve URI to dbus sensors map with err "
                        << static_cast<unsigned>(status);
                    return;
                }
                addReportReq->insert(uriToDbus);
                });
        }
        });
}

inline void requestRoutesMetricReportDefinition(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/TelemetryService/MetricReportDefinitions/<str>/")
        .privileges(redfish::privileges::getMetricReportDefinition)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& id) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }

        sdbusplus::asio::getAllProperties(
            *crow::connections::systemBus, telemetry::service,
            telemetry::getDbusReportPath(id), telemetry::reportInterface,
            [asyncResp,
             id](const boost::system::error_code& ec,
                 const dbus::utility::DBusPropertiesMap& properties) {
            telemetry::ErrorVerificator ev(asyncResp->res, ec);
            if (!ev.verify(id))
            {
                return;
            }

            telemetry::fillReportDefinition(asyncResp, id, properties);
            });
        });

    BMCWEB_ROUTE(app,
                 "/redfish/v1/TelemetryService/MetricReportDefinitions/<str>/")
        .privileges(redfish::privileges::deleteMetricReportDefinition)
        .methods(boost::beast::http::verb::delete_)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& id) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }

        telemetry::handleReportDelete(asyncResp, id);
        });

    BMCWEB_ROUTE(app,
                 "/redfish/v1/TelemetryService/MetricReportDefinitions/<str>/")
        .privileges(redfish::privileges::putMetricReportDefinition)
        .methods(boost::beast::http::verb::put)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& id) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }

        telemetry::handleReportPut(req, asyncResp, id);
        });

    BMCWEB_ROUTE(app,
                 "/redfish/v1/TelemetryService/MetricReportDefinitions/<str>/")
        .privileges(redfish::privileges::patchMetricReportDefinition)
        .methods(boost::beast::http::verb::patch)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& id) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }

        redfish::telemetry::handleReportPatch(req, asyncResp, id);
        });
}
} // namespace redfish
