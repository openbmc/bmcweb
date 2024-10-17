#pragma once

#include "app.hpp"
#include "dbus_utility.hpp"
#include "generated/enums/metric_report_definition.hpp"
#include "generated/enums/resource.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "sensors.hpp"
#include "utils/collection.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"
#include "utils/telemetry_utils.hpp"
#include "utils/time_utils.hpp"

#include <boost/container/flat_map.hpp>
#include <boost/url/format.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

namespace redfish
{

namespace telemetry
{

using ReadingParameters = std::vector<std::tuple<
    std::vector<std::tuple<sdbusplus::message::object_path, std::string>>,
    std::string, std::string, uint64_t>>;

inline bool verifyCommonErrors(crow::Response& res, const std::string& id,
                               const boost::system::error_code& ec)
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
        BMCWEB_LOG_ERROR("DBUS response error {}", ec);
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

inline std::string toDbusReportAction(std::string_view redfishValue)
{
    if (redfishValue == "RedfishEvent")
    {
        return "xyz.openbmc_project.Telemetry.Report.ReportActions.EmitsReadingsUpdate";
    }
    if (redfishValue == "LogToMetricReportsCollection")
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

inline std::string toDbusReportingType(std::string_view redfishValue)
{
    if (redfishValue == "OnChange")
    {
        return "xyz.openbmc_project.Telemetry.Report.ReportingType.OnChange";
    }
    if (redfishValue == "OnRequest")
    {
        return "xyz.openbmc_project.Telemetry.Report.ReportingType.OnRequest";
    }
    if (redfishValue == "Periodic")
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

inline std::string toDbusCollectionTimeScope(std::string_view redfishValue)
{
    if (redfishValue == "Point")
    {
        return "xyz.openbmc_project.Telemetry.Report.CollectionTimescope.Point";
    }
    if (redfishValue == "Interval")
    {
        return "xyz.openbmc_project.Telemetry.Report.CollectionTimescope.Interval";
    }
    if (redfishValue == "StartupInterval")
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

inline std::string toDbusReportUpdates(std::string_view redfishValue)
{
    if (redfishValue == "Overwrite")
    {
        return "xyz.openbmc_project.Telemetry.Report.ReportUpdates.Overwrite";
    }
    if (redfishValue == "AppendWrapsWhenFull")
    {
        return "xyz.openbmc_project.Telemetry.Report.ReportUpdates.AppendWrapsWhenFull";
    }
    if (redfishValue == "AppendStopsWhenFull")
    {
        return "xyz.openbmc_project.Telemetry.Report.ReportUpdates.AppendStopsWhenFull";
    }
    return "";
}

inline std::optional<nlohmann::json::array_t> getLinkedTriggers(
    std::span<const sdbusplus::message::object_path> triggerPaths)
{
    nlohmann::json::array_t triggers;

    for (const sdbusplus::message::object_path& path : triggerPaths)
    {
        if (path.parent_path() !=
            "/xyz/openbmc_project/Telemetry/Triggers/TelemetryService")
        {
            BMCWEB_LOG_ERROR("Property Triggers contains invalid value: {}",
                             path.str);
            return std::nullopt;
        }

        std::string id = path.filename();
        if (id.empty())
        {
            BMCWEB_LOG_ERROR("Property Triggers contains invalid value: {}",
                             path.str);
            return std::nullopt;
        }
        nlohmann::json::object_t trigger;
        trigger["@odata.id"] =
            boost::urls::format("/redfish/v1/TelemetryService/Triggers/{}", id);
        triggers.emplace_back(std::move(trigger));
    }

    return triggers;
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

    std::optional<nlohmann::json::array_t> linkedTriggers =
        getLinkedTriggers(triggers);
    if (!linkedTriggers)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    asyncResp->res.jsonValue["Links"]["Triggers"] = std::move(*linkedTriggers);

    nlohmann::json::array_t redfishReportActions;
    for (const std::string& action : reportActions)
    {
        metric_report_definition::ReportActionsEnum redfishAction =
            toRedfishReportAction(action);
        if (redfishAction ==
            metric_report_definition::ReportActionsEnum::Invalid)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        redfishReportActions.emplace_back(redfishAction);
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

        metric_report_definition::CalculationAlgorithmEnum
            redfishCollectionFunction =
                telemetry::toRedfishCollectionFunction(collectionFunction);
        if (redfishCollectionFunction ==
            metric_report_definition::CalculationAlgorithmEnum::Invalid)
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
        metrics.emplace_back(std::move(metric));
    }
    asyncResp->res.jsonValue["Metrics"] = std::move(metrics);

    if (enabled)
    {
        asyncResp->res.jsonValue["Status"]["State"] = resource::State::Enabled;
    }
    else
    {
        asyncResp->res.jsonValue["Status"]["State"] = resource::State::Disabled;
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
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/TelemetryService/MetricReportDefinitions/{}", id);
    asyncResp->res.jsonValue["Id"] = id;
    asyncResp->res.jsonValue["MetricReport"]["@odata.id"] = boost::urls::format(
        "/redfish/v1/TelemetryService/MetricReports/{}", id);
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

inline bool toDbusReportActions(crow::Response& res,
                                const std::vector<std::string>& actions,
                                std::vector<std::string>& outReportActions)
{
    size_t index = 0;
    for (const std::string& action : actions)
    {
        std::string dbusReportAction = toDbusReportAction(action);
        if (dbusReportAction.empty())
        {
            messages::propertyValueNotInList(
                res, action, "ReportActions/" + std::to_string(index));
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
        metricArgs.collectionFunction = std::move(dbusCollectionFunction);
    }

    if (collectionTimeScopeStr)
    {
        std::string dbusCollectionTimeScope =
            toDbusCollectionTimeScope(*collectionTimeScopeStr);
        if (dbusCollectionTimeScope.empty())
        {
            messages::propertyValueIncorrect(res, "CollectionTimeScope",
                                             *collectionTimeScopeStr);
            return false;
        }
        metricArgs.collectionTimeScope = std::move(dbusCollectionTimeScope);
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

inline bool getUserParameters(crow::Response& res, const crow::Request& req,
                              AddReportArgs& args)
{
    std::optional<std::string> id;
    std::optional<std::string> name;
    std::optional<std::string> reportingTypeStr;
    std::optional<std::string> reportUpdatesStr;
    std::optional<uint64_t> appendLimit;
    std::optional<bool> metricReportDefinitionEnabled;
    std::optional<std::vector<nlohmann::json>> metrics;
    std::optional<std::vector<std::string>> reportActionsStr;
    std::optional<nlohmann::json> schedule;

    if (!json_util::readJsonPatch(
            req, res, "Id", id, "Name", name, "Metrics", metrics,
            "MetricReportDefinitionType", reportingTypeStr, "ReportUpdates",
            reportUpdatesStr, "AppendLimit", appendLimit, "ReportActions",
            reportActionsStr, "Schedule", schedule,
            "MetricReportDefinitionEnabled", metricReportDefinitionEnabled))
    {
        return false;
    }

    if (id)
    {
        constexpr const char* allowedCharactersInId =
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
        if (id->empty() ||
            id->find_first_not_of(allowedCharactersInId) != std::string::npos)
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

    if (reportingTypeStr)
    {
        std::string dbusReportingType = toDbusReportingType(*reportingTypeStr);
        if (dbusReportingType.empty())
        {
            messages::propertyValueNotInList(res, *reportingTypeStr,
                                             "MetricReportDefinitionType");
            return false;
        }
        args.reportingType = dbusReportingType;
    }

    if (reportUpdatesStr)
    {
        std::string dbusReportUpdates = toDbusReportUpdates(*reportUpdatesStr);
        if (dbusReportUpdates.empty())
        {
            messages::propertyValueNotInList(res, *reportUpdatesStr,
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

    if (reportActionsStr)
    {
        if (!toDbusReportActions(res, *reportActionsStr, args.reportActions))
        {
            return false;
        }
    }

    if (reportingTypeStr == "Periodic")
    {
        if (!schedule)
        {
            messages::createFailedMissingReqProperties(res, "Schedule");
            return false;
        }

        std::string durationStr;
        if (!json_util::readJson(*schedule, res, "RecurrenceInterval",
                                 durationStr))
        {
            return false;
        }

        std::optional<std::chrono::milliseconds> durationNum =
            time_utils::fromDurationString(durationStr);
        if (!durationNum || durationNum->count() < 0)
        {
            messages::propertyValueIncorrect(res, "RecurrenceInterval",
                                             durationStr);
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

inline bool getChassisSensorNodeFromMetrics(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    std::span<const AddReportArgs::MetricArgs> metrics,
    boost::container::flat_set<std::pair<std::string, std::string>>& matched)
{
    for (const auto& metric : metrics)
    {
        std::optional<IncorrectMetricUri> error =
            getChassisSensorNode(metric.uris, matched);
        if (error)
        {
            messages::propertyValueIncorrect(asyncResp->res, error->uri,
                                             "MetricProperties/" +
                                                 std::to_string(error->index));
            return false;
        }
    }
    return true;
}

class AddReport
{
  public:
    AddReport(AddReportArgs&& argsIn,
              const std::shared_ptr<bmcweb::AsyncResp>& asyncRespIn) :
        asyncResp(asyncRespIn), args(std::move(argsIn))
    {}

    ~AddReport()
    {
        boost::asio::post(crow::connections::systemBus->get_io_context(),
                          std::bind_front(&performAddReport, asyncResp, args,
                                          std::move(uriToDbus)));
    }

    static void performAddReport(
        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
        const AddReportArgs& args,
        const boost::container::flat_map<std::string, std::string>& uriToDbus)
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
                    BMCWEB_LOG_ERROR(
                        "Failed to find DBus sensor corresponding to URI {}",
                        uri);
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
            [asyncResp, id = args.id, uriToDbus](
                const boost::system::error_code& ec, const std::string&) {
            if (ec == boost::system::errc::file_exists)
            {
                messages::resourceAlreadyExists(
                    asyncResp->res, "MetricReportDefinition", "Id", id);
                return;
            }
            if (ec == boost::system::errc::too_many_files_open)
            {
                messages::createLimitReachedForResource(asyncResp->res);
                return;
            }
            if (ec == boost::system::errc::argument_list_too_long)
            {
                nlohmann::json metricProperties = nlohmann::json::array();
                for (const auto& [uri, _] : uriToDbus)
                {
                    metricProperties.emplace_back(uri);
                }
                messages::propertyValueIncorrect(
                    asyncResp->res, "MetricProperties", metricProperties);
                return;
            }
            if (ec)
            {
                messages::internalError(asyncResp->res);
                BMCWEB_LOG_ERROR("respHandler DBus error {}", ec);
                return;
            }

            messages::created(asyncResp->res);
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
    boost::container::flat_map<std::string, std::string> uriToDbus;
};

class UpdateMetrics
{
  public:
    UpdateMetrics(std::string_view idIn,
                  const std::shared_ptr<bmcweb::AsyncResp>& asyncRespIn) :
        id(idIn), asyncResp(asyncRespIn)
    {}

    ~UpdateMetrics()
    {
        try
        {
            setReadingParams();
        }
        catch (const std::exception& e)
        {
            BMCWEB_LOG_ERROR("{}", e.what());
        }
        catch (...)
        {
            BMCWEB_LOG_ERROR("Unknown error");
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
                 const AddReportArgs::MetricArgs& metricArgs)
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

            std::get<0>(readingParams[index]) = *readingParam;
        }

        crow::connections::systemBus->async_method_call(
            [asyncResp(this->asyncResp),
             reportId = id](const boost::system::error_code& ec) {
            if (!verifyCommonErrors(asyncResp->res, reportId, ec))
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
    ReadingParameters readingParams;
};

inline void
    setReportEnabled(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     std::string_view id, bool enabled)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, id = std::string(id)](const boost::system::error_code& ec) {
        if (!verifyCommonErrors(asyncResp->res, id, ec))
        {
            return;
        }
    }, "xyz.openbmc_project.Telemetry", getDbusReportPath(id),
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Telemetry.Report", "Enabled",
        dbus::utility::DbusVariantType{enabled});
}

inline void setReportTypeAndInterval(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, std::string_view id,
    const std::string& reportingType, uint64_t recurrenceInterval)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, id = std::string(id)](const boost::system::error_code& ec) {
        if (!verifyCommonErrors(asyncResp->res, id, ec))
        {
            return;
        }
    }, "xyz.openbmc_project.Telemetry", getDbusReportPath(id),
        "xyz.openbmc_project.Telemetry.Report", "SetReportingProperties",
        reportingType, recurrenceInterval);
}

inline void
    setReportUpdates(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     std::string_view id, const std::string& reportUpdates)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, id = std::string(id)](const boost::system::error_code& ec) {
        if (!verifyCommonErrors(asyncResp->res, id, ec))
        {
            return;
        }
    }, "xyz.openbmc_project.Telemetry", getDbusReportPath(id),
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Telemetry.Report", "ReportUpdates",
        dbus::utility::DbusVariantType{reportUpdates});
}

inline void
    setReportActions(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     std::string_view id,
                     const std::vector<std::string>& dbusReportActions)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, id = std::string(id)](const boost::system::error_code& ec) {
        if (!verifyCommonErrors(asyncResp->res, id, ec))
        {
            return;
        }
    }, "xyz.openbmc_project.Telemetry", getDbusReportPath(id),
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
         redfishMetrics = std::vector<nlohmann::json>(metrics.begin(),
                                                      metrics.end())](
            boost::system::error_code ec,
            const dbus::utility::DBusPropertiesMap& properties) mutable {
        if (!redfish::telemetry::verifyCommonErrors(asyncResp->res, id, ec))
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

        auto updateMetricsReq = std::make_shared<UpdateMetrics>(id, asyncResp);

        boost::container::flat_set<std::pair<std::string, std::string>>
            chassisSensors;

        size_t index = 0;
        for (nlohmann::json& metric : redfishMetrics)
        {
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

                pathAndUri = std::get<0>(existing);
                metricArgs.collectionFunction = std::get<1>(existing);
                metricArgs.collectionTimeScope = std::get<2>(existing);
                metricArgs.collectionDuration = std::get<3>(existing);
            }

            if (!getUserMetric(asyncResp->res, metric, metricArgs))
            {
                return;
            }

            std::optional<IncorrectMetricUri> error =
                getChassisSensorNode(metricArgs.uris, chassisSensors);

            if (error)
            {
                messages::propertyValueIncorrect(
                    asyncResp->res, error->uri,
                    "MetricProperties/" + std::to_string(error->index));
                return;
            }

            updateMetricsReq->emplace(pathAndUri, metricArgs);
            index++;
        }

        for (const auto& [chassis, sensorType] : chassisSensors)
        {
            retrieveUriToDbusMap(
                chassis, sensorType,
                [asyncResp, updateMetricsReq](
                    const boost::beast::http::status status,
                    const std::map<std::string, std::string>& uriToDbus) {
                if (status != boost::beast::http::status::ok)
                {
                    BMCWEB_LOG_ERROR(
                        "Failed to retrieve URI to dbus sensors map with err {}",
                        static_cast<unsigned>(status));
                    return;
                }
                updateMetricsReq->insert(uriToDbus);
            });
        }
    });
}

inline void handleMetricReportDefinitionCollectionHead(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/MetricReportDefinitionCollection/MetricReportDefinitionCollection.json>; rel=describedby");
}

inline void handleMetricReportDefinitionCollectionGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/MetricReportDefinition/MetricReportDefinition.json>; rel=describedby");

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
        interfaces, "/xyz/openbmc_project/Telemetry/Reports/TelemetryService");
}

inline void
    handleReportPatch(App& app, const crow::Request& req,
                      const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      std::string_view id)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    std::optional<std::string> reportingTypeStr;
    std::optional<std::string> reportUpdatesStr;
    std::optional<bool> metricReportDefinitionEnabled;
    std::optional<std::vector<nlohmann::json>> metrics;
    std::optional<std::vector<std::string>> reportActionsStr;
    std::optional<nlohmann::json> schedule;

    if (!json_util::readJsonPatch(
            req, asyncResp->res, "Metrics", metrics,
            "MetricReportDefinitionType", reportingTypeStr, "ReportUpdates",
            reportUpdatesStr, "ReportActions", reportActionsStr, "Schedule",
            schedule, "MetricReportDefinitionEnabled",
            metricReportDefinitionEnabled))
    {
        return;
    }

    if (metricReportDefinitionEnabled)
    {
        setReportEnabled(asyncResp, id, *metricReportDefinitionEnabled);
    }

    if (reportUpdatesStr)
    {
        std::string dbusReportUpdates = toDbusReportUpdates(*reportUpdatesStr);
        if (dbusReportUpdates.empty())
        {
            messages::propertyValueNotInList(asyncResp->res, *reportUpdatesStr,
                                             "ReportUpdates");
            return;
        }
        setReportUpdates(asyncResp, id, dbusReportUpdates);
    }

    if (reportActionsStr)
    {
        std::vector<std::string> dbusReportActions;
        if (!toDbusReportActions(asyncResp->res, *reportActionsStr,
                                 dbusReportActions))
        {
            return;
        }
        setReportActions(asyncResp, id, dbusReportActions);
    }

    if (reportingTypeStr || schedule)
    {
        std::string dbusReportingType;
        if (reportingTypeStr)
        {
            dbusReportingType = toDbusReportingType(*reportingTypeStr);
            if (dbusReportingType.empty())
            {
                messages::propertyValueNotInList(asyncResp->res,
                                                 *reportingTypeStr,
                                                 "MetricReportDefinitionType");
                return;
            }
        }

        uint64_t recurrenceInterval = std::numeric_limits<uint64_t>::max();
        if (schedule)
        {
            std::string durationStr;
            if (!json_util::readJson(*schedule, asyncResp->res,
                                     "RecurrenceInterval", durationStr))
            {
                return;
            }

            std::optional<std::chrono::milliseconds> durationNum =
                time_utils::fromDurationString(durationStr);
            if (!durationNum || durationNum->count() < 0)
            {
                messages::propertyValueIncorrect(
                    asyncResp->res, "RecurrenceInterval", durationStr);
                return;
            }

            recurrenceInterval = static_cast<uint64_t>(durationNum->count());
        }

        setReportTypeAndInterval(asyncResp, id, dbusReportingType,
                                 recurrenceInterval);
    }

    if (metrics)
    {
        setReportMetrics(asyncResp, id, *metrics);
    }
}

inline void
    handleReportDelete(App& app, const crow::Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       std::string_view id)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    const std::string reportPath = getDbusReportPath(id);

    crow::connections::systemBus->async_method_call(
        [asyncResp,
         reportId = std::string(id)](const boost::system::error_code& ec) {
        if (!verifyCommonErrors(asyncResp->res, reportId, ec))
        {
            return;
        }
        asyncResp->res.result(boost::beast::http::status::no_content);
    },
        service, reportPath, "xyz.openbmc_project.Object.Delete", "Delete");
}
} // namespace telemetry

inline void afterRetrieveUriToDbusMap(
    const std::shared_ptr<bmcweb::AsyncResp>& /*asyncResp*/,
    const std::shared_ptr<telemetry::AddReport>& addReportReq,
    const boost::beast::http::status status,
    const std::map<std::string, std::string>& uriToDbus)
{
    if (status != boost::beast::http::status::ok)
    {
        BMCWEB_LOG_ERROR(
            "Failed to retrieve URI to dbus sensors map with err {}",
            static_cast<unsigned>(status));
        return;
    }
    addReportReq->insert(uriToDbus);
}

inline void handleMetricReportDefinitionsPost(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    telemetry::AddReportArgs args;
    if (!telemetry::getUserParameters(asyncResp->res, req, args))
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

    auto addReportReq = std::make_shared<telemetry::AddReport>(std::move(args),
                                                               asyncResp);
    for (const auto& [chassis, sensorType] : chassisSensors)
    {
        retrieveUriToDbusMap(chassis, sensorType,
                             std::bind_front(afterRetrieveUriToDbusMap,
                                             asyncResp, addReportReq));
    }
}

inline void
    handleMetricReportHead(App& app, const crow::Request& req,
                           const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& /*id*/)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/MetricReport/MetricReport.json>; rel=describedby");
}

inline void
    handleMetricReportGet(App& app, const crow::Request& req,
                          const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& id)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/MetricReport/MetricReport.json>; rel=describedby");

    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, telemetry::service,
        telemetry::getDbusReportPath(id), telemetry::reportInterface,
        [asyncResp, id](const boost::system::error_code& ec,
                        const dbus::utility::DBusPropertiesMap& properties) {
        if (!redfish::telemetry::verifyCommonErrors(asyncResp->res, id, ec))
        {
            return;
        }

        telemetry::fillReportDefinition(asyncResp, id, properties);
    });
}

inline void handleMetricReportDelete(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, const std::string& id)

{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    const std::string reportPath = telemetry::getDbusReportPath(id);

    crow::connections::systemBus->async_method_call(
        [asyncResp, id](const boost::system::error_code& ec) {
        /*
         * boost::system::errc and std::errc are missing value
         * for EBADR error that is defined in Linux.
         */
        if (ec.value() == EBADR)
        {
            messages::resourceNotFound(asyncResp->res, "MetricReportDefinition",
                                       id);
            return;
        }

        if (ec)
        {
            BMCWEB_LOG_ERROR("respHandler DBus error {}", ec);
            messages::internalError(asyncResp->res);
            return;
        }

        asyncResp->res.result(boost::beast::http::status::no_content);
    }, telemetry::service, reportPath, "xyz.openbmc_project.Object.Delete",
        "Delete");
}

inline void requestRoutesMetricReportDefinitionCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/TelemetryService/MetricReportDefinitions/")
        .privileges(redfish::privileges::headMetricReportDefinitionCollection)
        .methods(boost::beast::http::verb::head)(std::bind_front(
            telemetry::handleMetricReportDefinitionCollectionHead,
            std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/TelemetryService/MetricReportDefinitions/")
        .privileges(redfish::privileges::getMetricReportDefinitionCollection)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            telemetry::handleMetricReportDefinitionCollectionGet,
            std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/TelemetryService/MetricReportDefinitions/")
        .privileges(redfish::privileges::postMetricReportDefinitionCollection)
        .methods(boost::beast::http::verb::post)(
            std::bind_front(handleMetricReportDefinitionsPost, std::ref(app)));
}

inline void requestRoutesMetricReportDefinition(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/TelemetryService/MetricReportDefinitions/<str>/")
        .privileges(redfish::privileges::getMetricReportDefinition)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleMetricReportHead, std::ref(app)));

    BMCWEB_ROUTE(app,
                 "/redfish/v1/TelemetryService/MetricReportDefinitions/<str>/")
        .privileges(redfish::privileges::getMetricReportDefinition)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleMetricReportGet, std::ref(app)));

    BMCWEB_ROUTE(app,
                 "/redfish/v1/TelemetryService/MetricReportDefinitions/<str>/")
        .privileges(redfish::privileges::deleteMetricReportDefinition)
        .methods(boost::beast::http::verb::delete_)(
            std::bind_front(handleMetricReportDelete, std::ref(app)));

    BMCWEB_ROUTE(app,
                 "/redfish/v1/TelemetryService/MetricReportDefinitions/<str>/")
        .privileges(redfish::privileges::patchMetricReportDefinition)
        .methods(boost::beast::http::verb::patch)(
            std::bind_front(telemetry::handleReportPatch, std::ref(app)));
}
} // namespace redfish
