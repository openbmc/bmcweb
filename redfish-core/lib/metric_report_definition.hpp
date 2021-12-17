#pragma once

#include "app.hpp"
#include "dbus_utility.hpp"
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

inline std::string toRedfishReportAction(std::string_view dbusValue)
{

    if (dbusValue ==
        "xyz.openbmc_project.Telemetry.Report.ReportActions.EmitsReadingsUpdate")
    {
        return "RedfishEvent";
    }
    if (dbusValue ==
        "xyz.openbmc_project.Telemetry.Report.ReportActions.LogToMetricReportsCollection")
    {
        return "LogToMetricReportsCollection";
    }
    return "";
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

inline std::string toRedfishReportingType(std::string_view dbusValue)
{
    if (dbusValue ==
        "xyz.openbmc_project.Telemetry.Report.ReportingType.OnChange")
    {
        return "OnChange";
    }
    if (dbusValue ==
        "xyz.openbmc_project.Telemetry.Report.ReportingType.OnRequest")
    {
        return "OnRequest";
    }
    if (dbusValue ==
        "xyz.openbmc_project.Telemetry.Report.ReportingType.Periodic")
    {
        return "Periodic";
    }
    return "";
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

inline std::string toRedfishCollectionTimeScope(std::string_view dbusValue)
{
    if (dbusValue ==
        "xyz.openbmc_project.Telemetry.Report.CollectionTimescope.Point")
    {
        return "Point";
    }
    if (dbusValue ==
        "xyz.openbmc_project.Telemetry.Report.CollectionTimescope.Interval")
    {
        return "Interval";
    }
    if (dbusValue ==
        "xyz.openbmc_project.Telemetry.Report.CollectionTimescope.StartupInterval")
    {
        return "StartupInterval";
    }
    return "";
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

inline std::string toRedfishReportUpdates(std::string_view dbusValue)
{
    if (dbusValue ==
        "xyz.openbmc_project.Telemetry.Report.ReportUpdates.Overwrite")
    {
        return "Overwrite";
    }
    if (dbusValue ==
        "xyz.openbmc_project.Telemetry.Report.ReportUpdates.AppendWrapsWhenFull")
    {
        return "AppendWrapsWhenFull";
    }
    if (dbusValue ==
        "xyz.openbmc_project.Telemetry.Report.ReportUpdates.AppendStopsWhenFull")
    {
        return "AppendStopsWhenFull";
    }
    return "";
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

    std::string redfishReportingType = toRedfishReportingType(reportingType);
    if (redfishReportingType.empty())
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
        std::string redfishAction = toRedfishReportAction(action);
        if (redfishAction.empty())
        {
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

        std::string redfishCollectionTimeScope =
            toRedfishCollectionTimeScope(collectionTimeScope);
        if (redfishCollectionTimeScope.empty())
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

    std::string redfishReportUpdates = toRedfishReportUpdates(reportUpdates);
    if (redfishReportUpdates.empty())
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

inline bool toDbusReportActions(crow::Response& res,
                                std::span<const std::string> actions,
                                std::vector<std::string>& outReportActions)
{
    outReportActions.clear();

    size_t index = 0;
    for (const auto& action : actions)
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
    std::optional<std::string> collectionTimeScope;

    if (!json_util::readJson(metric, res, "MetricProperties", uris,
                             "CollectionFunction", collectionFunction,
                             "CollectionTimeScope", collectionTimeScope,
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
        metricArgs.collectionFunction = dbusCollectionFunction;
    }

    if (collectionTimeScope)
    {
        std::string dbusCollectionTimeScope =
            toDbusCollectionTimeScope(*collectionTimeScope);
        if (dbusCollectionTimeScope.empty())
        {
            messages::propertyValueIncorrect(res, "CollectionTimeScope",
                                             *collectionTimeScope);
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

inline bool getUserParameters(crow::Response& res, const crow::Request& req,
                              AddReportArgs& args)
{
    std::optional<std::string> id;
    std::optional<std::string> name;
    std::optional<std::string> reportingType;
    std::optional<std::string> reportUpdates;
    std::optional<uint64_t> appendLimit;
    std::optional<bool> metricReportDefinitionEnabled;
    std::optional<std::vector<nlohmann::json>> metrics;
    std::optional<std::vector<std::string>> reportActions;
    std::optional<nlohmann::json> schedule;

    if (!json_util::readJsonPatch(
            req, res, "Id", id, "Name", name, "Metrics", metrics,
            "MetricReportDefinitionType", reportingType, "ReportUpdates",
            reportUpdates, "AppendLimit", appendLimit, "ReportActions",
            reportActions, "Schedule", schedule,
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

    if (reportingType)
    {
        std::string dbusReportingType = toDbusReportingType(*reportingType);
        if (dbusReportingType.empty())
        {
            messages::propertyValueNotInList(res, *reportingType,
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
            messages::propertyValueNotInList(res, *reportUpdates,
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

    if (reportingType == "Periodic")
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
              AddReportType typeIn) :
        asyncResp(asyncRespIn),
        args(std::move(argsIn)), type(typeIn)
    {}

    ~AddReport()
    {
        boost::asio::post(crow::connections::systemBus->get_io_context(),
                          std::bind_front(&performAddReport, asyncResp, args,
                                          uriToDbus, type));
    }

    static void performAddReport(
        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
        const AddReportArgs& args,
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
            [asyncResp, id = args.id, uriToDbus,
             type](const boost::system::error_code& ec, const std::string&) {
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
                messages::propertyValueIncorrect(asyncResp->res,
                                                 metricProperties.dump(),
                                                 "MetricProperties");
                return;
            }
            if (ec)
            {
                messages::internalError(asyncResp->res);
                BMCWEB_LOG_ERROR << "respHandler DBus error " << ec;
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
                  const std::shared_ptr<bmcweb::AsyncResp>& asyncRespIn) :
        id(idIn),
        asyncResp(asyncRespIn)
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

            std::get<0>(readingParams[index]) = *readingParam;
        }

        crow::connections::systemBus->async_method_call(
            [aResp = asyncResp,
             reportId = id](const boost::system::error_code ec) {
            if (!verifyCommonErrors(aResp->res, reportId, ec))
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
};

inline void setReportEnabled(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                             std::string_view id, bool enabled)
{
    crow::connections::systemBus->async_method_call(
        [aResp, id = std::string(id)](const boost::system::error_code ec) {
        if (!verifyCommonErrors(aResp->res, id, ec))
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
    const std::string& reportingType, uint64_t recurrenceInterval)
{
    crow::connections::systemBus->async_method_call(
        [aResp, id = std::string(id)](const boost::system::error_code ec) {
        if (!verifyCommonErrors(aResp->res, id, ec))
        {
            return;
        }
        },
        "xyz.openbmc_project.Telemetry", getDbusReportPath(id),
        "xyz.openbmc_project.Telemetry.Report", "SetReportingProperties",
        reportingType, recurrenceInterval);
}

inline void setReportUpdates(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                             std::string_view id,
                             const std::string& reportUpdates)
{
    crow::connections::systemBus->async_method_call(
        [aResp, id = std::string(id)](const boost::system::error_code ec) {
        if (!verifyCommonErrors(aResp->res, id, ec))
        {
            return;
        }
        },
        "xyz.openbmc_project.Telemetry", getDbusReportPath(id),
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Telemetry.Report", "ReportUpdates",
        dbus::utility::DbusVariantType{reportUpdates});
}

inline void setReportActions(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                             std::string_view id,
                             const std::vector<std::string>& dbusReportActions)
{
    crow::connections::systemBus->async_method_call(
        [aResp, id = std::string(id)](const boost::system::error_code ec) {
        if (!verifyCommonErrors(aResp->res, id, ec))
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

                pathAndUri = std::get<0>(existing);
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
    std::optional<std::string> reportingType;
    std::optional<std::string> reportUpdates;
    std::optional<uint64_t> appendLimit;
    std::optional<bool> metricReportDefinitionEnabled;
    std::optional<std::vector<nlohmann::json>> metrics;
    std::optional<std::vector<std::string>> reportActions;
    std::optional<nlohmann::json> schedule;

    if (!json_util::readJsonPatch(
            req, aResp->res, "Name", name, "Metrics", metrics,
            "MetricReportDefinitionType", reportingType, "ReportUpdates",
            reportUpdates, "AppendLimit", appendLimit, "ReportActions",
            reportActions, "Schedule", schedule,
            "MetricReportDefinitionEnabled", metricReportDefinitionEnabled))
    {
        return;
    }

    if (metricReportDefinitionEnabled)
    {
        setReportEnabled(aResp, id, *metricReportDefinitionEnabled);
    }

    if (reportUpdates)
    {
        std::string dbusReportUpdates = toDbusReportUpdates(*reportUpdates);
        if (dbusReportUpdates.empty())
        {
            messages::propertyValueNotInList(aResp->res, *reportUpdates,
                                             "ReportUpdates");
            return;
        }
        setReportUpdates(aResp, id, *reportUpdates);
    }

    if (reportActions)
    {
        std::vector<std::string> dbusReportActions;
        if (!toDbusReportActions(aResp->res, *reportActions, dbusReportActions))
        {
            return;
        }
        setReportActions(aResp, id, dbusReportActions);
    }

    if (reportingType || schedule)
    {
        std::string dbusReportingType;
        if (reportingType)
        {
            dbusReportingType = toDbusReportingType(*reportingType);
            if (dbusReportingType.empty())
            {
                messages::propertyValueNotInList(aResp->res, *reportingType,
                                                 "MetricReportDefinitionType");
                return;
            }
        }

        uint64_t recurrenceInterval = std::numeric_limits<uint64_t>::max();
        if (schedule)
        {
            std::string durationStr;
            if (!json_util::readJson(*schedule, aResp->res,
                                     "RecurrenceInterval", durationStr))
            {
                return;
            }

            std::optional<std::chrono::milliseconds> durationNum =
                time_utils::fromDurationString(durationStr);
            if (!durationNum || durationNum->count() < 0)
            {
                messages::propertyValueIncorrect(
                    aResp->res, "RecurrenceInterval", durationStr);
                return;
            }

            recurrenceInterval = static_cast<uint64_t>(durationNum->count());
        }

        setReportTypeAndInterval(aResp, id, dbusReportingType,
                                 recurrenceInterval);
    }

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

    if (!getUserParameters(aResp->res, req, args))
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

        auto addReportReq =
            std::make_shared<AddReport>(args, aResp, addReportMode);
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
        [aResp,
         reportId = std::string(id)](const boost::system::error_code ec) {
        if (!verifyCommonErrors(aResp->res, reportId, ec))
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

        auto addReportReq = std::make_shared<telemetry::AddReport>(
            std::move(args), asyncResp, telemetry::AddReportType::create);
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
            if (!redfish::telemetry::verifyCommonErrors(asyncResp->res, id, ec))
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
