#pragma once

#include "sensors.hpp"
#include "utils/telemetry_utils.hpp"
#include "utils/time_utils.hpp"

#include <app.hpp>
#include <boost/container/flat_map.hpp>
#include <dbus_utility.hpp>
#include <query.hpp>
#include <registries/privilege_registry.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/unpack_properties.hpp>
#include <utils/dbus_utils.hpp>
#include <utils/stl_utils.hpp>

#include <map>
#include <tuple>

namespace redfish
{

namespace telemetry
{

constexpr const char* metricReportDefinitionUri =
    "/redfish/v1/TelemetryService/MetricReportDefinitions";

using ReadingParameters = std::vector<std::tuple<
    std::vector<std::tuple<sdbusplus::message::object_path, std::string>>,
    std::string, std::string, std::string, uint64_t>>;

enum class addReportType
{
    create,
    replace
};

std::string toDbusReportAction(std::string_view action)
{
    if (action == "RedfishEvent")
    {
        return "EmitsReadingsUpdate";
    }
    if (action == "LogToMetricReportsCollection")
    {
        return "LogToMetricReportsCollection";
    }
    return "";
}

inline std::optional<std::vector<std::string>>
    toDbusReportActions(crow::Response& res,
                        const std::vector<std::string>& redfishActions)
{
    std::vector<std::string> dbusActions;

    size_t index = 0;
    for (const auto& redfishAction : redfishActions)
    {
        std::string dbusAction = toDbusReportAction(redfishAction);

        if (dbusAction.empty())
        {
            messages::propertyValueNotInList(
                res, redfishAction, "ReportActions/" + std::to_string(index));
            return std::nullopt;
        }

        dbusActions.emplace_back(std::move(dbusAction));
        index++;
    }
    return dbusActions;
}

struct UserMetricArgs
{
    bool readUserParameters(crow::Response& res, nlohmann::json& json)
    {
        if (!json_util::readJson(json, res, "MetricId", id, "MetricProperties",
                                 metricProperties, "CollectionFunction",
                                 collectionFunction, "CollectionTimeScope",
                                 collectionTimeScope, "CollectionDuration",
                                 collectionDurationStr))
        {
            return false;
        }

        if (collectionDurationStr)
        {
            collectionDuration =
                time_utils::fromDurationString(*collectionDurationStr);

            if (!collectionDuration || collectionDuration->count() < 0)
            {
                messages::propertyValueIncorrect(res, "CollectionDuration",
                                                 *collectionDurationStr);
                return false;
            }
        }

        return true;
    }

    std::optional<std::string> id;
    std::optional<std::vector<std::string>> metricProperties;
    std::optional<std::string> collectionFunction;
    std::optional<std::string> collectionTimeScope;
    std::optional<std::string> collectionDurationStr;
    std::optional<std::chrono::milliseconds> collectionDuration;
};

struct UserReportArgs
{
    bool readUserParametersPatch(crow::Response& res, const crow::Request& req)
    {
        if (!json_util::readJsonPatch(
                req, res, "Metrics", redfishMetrics,
                "MetricReportDefinitionType", reportingType, "ReportUpdates",
                reportUpdates, "ReportActions", redfishReportActions,
                "Schedule", schedule, "MetricReportDefinitionEnabled",
                metricReportDefinitionEnabled))
        {
            return false;
        }

        return convertUserParameters(res);
    }

    bool readUserParameters(crow::Response& res, const crow::Request& req)
    {
        if (!json_util::readJsonPatch(
                req, res, "Id", id, "Name", name, "Metrics", redfishMetrics,
                "MetricReportDefinitionType", reportingType, "ReportUpdates",
                reportUpdates, "AppendLimit", appendLimit, "ReportActions",
                redfishReportActions, "Schedule", schedule,
                "MetricReportDefinitionEnabled", metricReportDefinitionEnabled))
        {
            return false;
        }

        return convertUserParameters(res);
    }

    std::vector<std::pair<std::string, dbus::utility::DbusVariantType>>
        getProperties(
            std::optional<telemetry::ReadingParameters> readingParams) const
    {
        std::vector<std::pair<std::string, dbus::utility::DbusVariantType>>
            properties;

        properties.emplace_back("Id", "TelemetryService/" + id.value_or(""));

        if (name)
        {
            properties.emplace_back("Name", *name);
        }

        if (reportingType)
        {
            properties.emplace_back("ReportingType", *reportingType);
        }

        if (reportUpdates)
        {
            properties.emplace_back("ReportUpdates", *reportUpdates);
        }

        if (appendLimit)
        {
            properties.emplace_back("AppendLimit", *appendLimit);
        }

        if (reportActions)
        {
            properties.emplace_back("ReportActions", *reportActions);
        }

        if (recurrenceInterval)
        {
            properties.emplace_back(
                "Interval", static_cast<uint64_t>(recurrenceInterval->count()));
        }

        if (readingParams)
        {
            properties.emplace_back("MetricParams", std::move(*readingParams));
        }

        if (metricReportDefinitionEnabled)
        {
            properties.emplace_back("Enabled", *metricReportDefinitionEnabled);
        }

        return properties;
    }

    std::optional<std::string> id;
    std::optional<std::string> name;
    std::optional<std::string> reportingType;
    std::optional<std::string> reportUpdates;
    std::optional<uint64_t> appendLimit;
    std::optional<std::vector<std::string>> redfishReportActions;
    std::optional<std::vector<std::string>> reportActions;
    std::optional<std::string> recurrenceIntervalStr;
    std::optional<std::chrono::milliseconds> recurrenceInterval;
    std::optional<std::vector<nlohmann::json>> redfishMetrics;
    std::optional<std::vector<UserMetricArgs>> metrics;
    std::optional<bool> metricReportDefinitionEnabled;
    std::map<std::string, std::string> metricPropertyToDbusPaths;

  private:
    bool convertUserParameters(crow::Response& res)
    {
        if (redfishReportActions)
        {
            reportActions = toDbusReportActions(res, *redfishReportActions);
            if (!reportActions)
            {
                return false;
            }
        }

        if (schedule)
        {
            if (!json_util::readJson(*schedule, res, "RecurrenceInterval",
                                     recurrenceIntervalStr))
            {
                return false;
            }

            if (recurrenceIntervalStr)
            {
                recurrenceInterval =
                    time_utils::fromDurationString(*recurrenceIntervalStr);
                if (!recurrenceInterval)
                {
                    messages::propertyValueIncorrect(res, "RecurrenceInterval",
                                                     *recurrenceIntervalStr);
                    return false;
                }
            }
        }

        if (redfishMetrics)
        {
            metrics.emplace();
            metrics->reserve(redfishMetrics->size());
            for (auto& m : *redfishMetrics)
            {
                UserMetricArgs metricArgs;
                if (!metricArgs.readUserParameters(res, m))
                {
                    return false;
                }
                metrics->emplace_back(metricArgs);
            }
        }

        return true;
    }

    std::optional<nlohmann::json> schedule;
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

std::string toReadfishReportAction(std::string_view action)
{

    if (action == "EmitsReadingsUpdate")
    {
        return "RedfishEvent";
    }
    if (action == "LogToMetricReportsCollection")
    {
        return "LogToMetricReportsCollection";
    }
    return "";
}

inline std::optional<nlohmann::json> getLinkedTriggers(
    const std::vector<sdbusplus::message::object_path>& triggerPaths)
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
                                          "Triggers", id)
                 .string()},
        });
    }

    return std::make_optional(triggers);
}

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

inline void
    fillReportDefinition(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& id,
                         const dbus::utility::DBusPropertiesMap& properties)
{
    const std::vector<std::string>* reportActions = nullptr;
    const ReadingParameters* readingParams = nullptr;
    const std::string* reportingType = nullptr;
    const std::string* reportUpdates = nullptr;
    const std::string* name = nullptr;
    const uint64_t* appendLimit = nullptr;
    const uint64_t* interval = nullptr;
    const bool* enabled = nullptr;
    const std::vector<std::tuple<std::string, std::string>>* errorMessages =
        nullptr;
    const std::vector<sdbusplus::message::object_path>* triggers = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "ReportingType",
        reportingType, "Interval", interval, "ReportActions", reportActions,
        "ReportUpdates", reportUpdates, "AppendLimit", appendLimit,
        "ReadingParametersFutureVersion", readingParams, "Name", name,
        "Enabled", enabled, "ErrorMessages", errorMessages, "Triggers",
        triggers);

    if (!success)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    if (triggers != nullptr)
    {
        std::optional<nlohmann::json> linkedTriggers =
            getLinkedTriggers(*triggers);
        if (!linkedTriggers)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        asyncResp->res.jsonValue["Links"]["Triggers"] = *linkedTriggers;
    }

    std::vector<std::string> redfishReportActions;
    if (reportActions != nullptr)
    {
        for (const std::string& action : *reportActions)
        {
            std::string redfishAction = toReadfishReportAction(action);

            if (redfishAction.empty())
            {
                BMCWEB_LOG_ERROR << "Unknown ReportActions element: " << action;
                messages::internalError(asyncResp->res);
                return;
            }

            redfishReportActions.emplace_back(std::move(redfishAction));
        }
    }

    if (enabled != nullptr)
    {
        asyncResp->res.jsonValue["MetricReportDefinitionEnabled"] = *enabled;

        if (errorMessages != nullptr)
        {
            if (*enabled && errorMessages->empty())
            {
                asyncResp->res.jsonValue["Status"]["State"] = "Enabled";
            }
            else
            {
                asyncResp->res.jsonValue["Status"]["State"] = "Disabled";
            }
        }
        else
        {
            asyncResp->res.jsonValue["Status"]["State"] =
                *enabled ? "Enabled" : "Disabled";
        }
    }

    if (appendLimit != nullptr)
    {
        asyncResp->res.jsonValue["AppendLimit"] = *appendLimit;
    }

    if (reportUpdates != nullptr)
    {
        asyncResp->res.jsonValue["ReportUpdates"] = *reportUpdates;
    }

    if (name != nullptr)
    {
        asyncResp->res.jsonValue["Name"] = *name;
    }

    if (reportActions != nullptr)
    {
        asyncResp->res.jsonValue["ReportActions"] =
            std::move(redfishReportActions);
    }

    if (reportingType != nullptr)
    {
        asyncResp->res.jsonValue["MetricReportDefinitionType"] = *reportingType;
    }

    if (interval != nullptr)
    {
        asyncResp->res.jsonValue["Schedule"]["RecurrenceInterval"] =
            time_utils::toDurationString(std::chrono::milliseconds(*interval));
    }

    if (readingParams != nullptr)
    {
        nlohmann::json metrics = nlohmann::json::array();
        for (const auto& [sensorData, collectionFunction, metricId,
                          collectionTimeScope, collectionDuration] :
             *readingParams)
        {
            nlohmann::json::array_t metricProperties;

            for (const auto& [sensorPath, sensorMetadata] : sensorData)
            {
                metricProperties.emplace_back(sensorMetadata);
            }

            nlohmann::json::object_t metric;
            metric["MetricId"] = metricId;
            metric["MetricProperties"] = std::move(metricProperties);
            metric["CollectionFunction"] = collectionFunction;
            metric["CollectionDuration"] = time_utils::toDurationString(
                std::chrono::milliseconds(collectionDuration));
            metric["CollectionTimeScope"] = collectionTimeScope;
            metrics.push_back(metric);
        }
        asyncResp->res.jsonValue["Metrics"] = std::move(metrics);
    }

    asyncResp->res.jsonValue["@odata.type"] =
        "#MetricReportDefinition.v1_3_0.MetricReportDefinition";
    asyncResp->res.jsonValue["@odata.id"] =
        crow::utility::urlFromPieces("redfish", "v1", "TelemetryService",
                                     "MetricReportDefinitions", id)
            .string();
    asyncResp->res.jsonValue["Id"] = id;
    asyncResp->res.jsonValue["MetricReport"]["@odata.id"] =
        crow::utility::urlFromPieces("redfish", "v1", "TelemetryService",
                                     "MetricReports", id)
            .string();
}

inline bool getChassisSensorNodeFromMetrics(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::vector<UserMetricArgs>& metrics,
    boost::container::flat_set<std::pair<std::string, std::string>>& matched)
{
    for (const auto& metric : metrics)
    {
        if (metric.metricProperties)
        {
            std::optional<IncorrectMetricProperty> error =
                getChassisSensorNode(*metric.metricProperties, matched);
            if (error)
            {
                messages::propertyValueIncorrect(
                    asyncResp->res, error->metricProperty,
                    "MetricProperties/" + std::to_string(error->index));
                return false;
            }
        }
    }
    return true;
}

class UpdateMetrics
{
  public:
    UpdateMetrics(const UserReportArgs& argsIn,
                  const std::shared_ptr<bmcweb::AsyncResp>& asyncRespIn) :
        args(argsIn),
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

    void insert(
        const std::map<std::string, std::string>& metricPropertyToDbusPaths)
    {
        args.metricPropertyToDbusPaths.insert(metricPropertyToDbusPaths.begin(),
                                              metricPropertyToDbusPaths.end());
    }

    void emplace(const std::optional<std::vector<std::string>>& uris,
                 const std::vector<std::tuple<sdbusplus::message::object_path,
                                              std::string>>& pathAndUri,
                 const std::string& collectionFunction,
                 const std::string& metricId,
                 const std::string& collectionTimeScope,
                 const uint64_t collectionDuration)
    {
        readingParamsUris.emplace_back(uris);
        readingParams.emplace_back(pathAndUri, collectionFunction, metricId,
                                   collectionTimeScope, collectionDuration);
    }

    void setReadingParams()
    {
        if (!args.id ||
            asyncResp->res.result() != boost::beast::http::status::ok)
        {
            return;
        }

        for (size_t index = 0; index < readingParamsUris.size(); ++index)
        {
            const std::optional<std::vector<std::string>>& newUris =
                readingParamsUris[index];

            if (!newUris)
            {
                continue;
            }

            const std::optional<std::vector<
                std::tuple<sdbusplus::message::object_path, std::string>>>
                readingParam = sensorPathToUri(*newUris);

            if (!readingParam)
            {
                return;
            }

            std::get<0>(readingParams[index]) = *readingParam;
        }

        crow::connections::systemBus->async_method_call(
            [aResp = asyncResp,
             arguments = args](const boost::system::error_code ec) {
            if (!verifyCommonErrors(aResp->res, *arguments.id, ec))
            {
                return;
            }
            },
            "xyz.openbmc_project.Telemetry",
            "/xyz/openbmc_project/Telemetry/Reports/TelemetryService/" +
                *args.id,
            "org.freedesktop.DBus.Properties", "Set",
            "xyz.openbmc_project.Telemetry.Report",
            "ReadingParametersFutureVersion",
            dbus::utility::DbusVariantType{readingParams});
    }

    UserReportArgs args;

  private:
    std::optional<
        std::vector<std::tuple<sdbusplus::message::object_path, std::string>>>
        sensorPathToUri(const std::vector<std::string>& uris) const
    {
        std::vector<std::tuple<sdbusplus::message::object_path, std::string>>
            result;

        for (const std::string& uri : uris)
        {
            auto it = args.metricPropertyToDbusPaths.find(uri);
            if (it == args.metricPropertyToDbusPaths.end())
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
    std::vector<std::optional<std::vector<std::string>>> readingParamsUris;
    ReadingParameters readingParams{};
};

class AddReport
{
  public:
    AddReport(UserReportArgs argsIn,
              const std::shared_ptr<bmcweb::AsyncResp>& asyncRespIn,
              addReportType typeIn) :
        args{std::move(argsIn)},
        asyncResp(asyncRespIn), type(typeIn)
    {}

    ~AddReport()
    {
        if (asyncResp->res.result() != boost::beast::http::status::ok)
        {
            return;
        }

        std::optional<telemetry::ReadingParameters> readingParams;
        if (args.metrics)
        {
            readingParams.emplace();
            readingParams->reserve(args.metrics->size());

            for (auto& metric : *args.metrics)
            {
                std::vector<
                    std::tuple<sdbusplus::message::object_path, std::string>>
                    sensorParams;

                if (metric.metricProperties)
                {
                    sensorParams.reserve(metric.metricProperties->size());

                    for (size_t i = 0; i < metric.metricProperties->size(); i++)
                    {
                        const std::string& metricProperty =
                            (*metric.metricProperties)[i];
                        auto el =
                            args.metricPropertyToDbusPaths.find(metricProperty);
                        if (el == args.metricPropertyToDbusPaths.end())
                        {
                            BMCWEB_LOG_ERROR
                                << "Failed to find DBus sensor corresponding to MetricProperty "
                                << metricProperty;
                            messages::propertyValueNotInList(
                                asyncResp->res, metricProperty,
                                "MetricProperties/" + std::to_string(i));
                            return;
                        }

                        const std::string& dbusPath = el->second;
                        sensorParams.emplace_back(dbusPath, metricProperty);
                    }
                }

                readingParams->emplace_back(
                    std::move(sensorParams),
                    metric.collectionFunction.value_or(""),
                    metric.id.value_or(""),
                    metric.collectionTimeScope.value_or(""),
                    static_cast<uint64_t>(
                        metric.collectionDuration
                            .value_or(std::chrono::milliseconds{})
                            .count()));
            }
        }

        try
        {
            crow::connections::systemBus->async_method_call(
                [aResp = asyncResp, id = args.id.value_or(""),
                 uriToDbus = args.metricPropertyToDbusPaths, type = type](
                    const boost::system::error_code ec, const std::string&) {
                if (ec == boost::system::errc::argument_list_too_long)
                {
                    nlohmann::json metricProperties = nlohmann::json::array();
                    for (const auto& [metricProperty, _] : uriToDbus)
                    {
                        metricProperties.emplace_back(metricProperty);
                    }
                    messages::propertyValueIncorrect(aResp->res,
                                                     metricProperties.dump(),
                                                     "MetricProperties");
                    return;
                }

                if (!verifyCommonErrors(aResp->res, id, ec))
                {
                    return;
                }

                if (type == addReportType::create)
                {
                    messages::created(aResp->res);
                }
                else
                {
                    messages::success(aResp->res);
                }
                },
                service, "/xyz/openbmc_project/Telemetry/Reports",
                "xyz.openbmc_project.Telemetry.ReportManager",
                "AddReportFutureVersion",
                args.getProperties(std::move(readingParams)));
        }
        catch (const std::exception& e)
        {
            BMCWEB_LOG_ERROR << e.what();
        }
        catch (...)
        {
            BMCWEB_LOG_ERROR << "AddReport failed!";
        }
    }

    void insert(
        const std::map<std::string, std::string>& metricPropertyToDbusPaths)
    {
        args.metricPropertyToDbusPaths.insert(metricPropertyToDbusPaths.begin(),
                                              metricPropertyToDbusPaths.end());
    }

    AddReport(const AddReport&) = delete;
    AddReport(AddReport&&) = delete;
    AddReport& operator=(const AddReport&) = delete;
    AddReport& operator=(AddReport&&) = delete;

    UserReportArgs args;

  private:
    std::shared_ptr<bmcweb::AsyncResp> asyncResp;
    addReportType type;
}; // namespace telemetry

inline void setReportEnabled(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                             const UserReportArgs& args)
{
    if (!args.id || !args.metricReportDefinitionEnabled)
    {
        return;
    }

    crow::connections::systemBus->async_method_call(
        [aResp, args](const boost::system::error_code ec) {
        if (!verifyCommonErrors(aResp->res, *args.id, ec))
        {
            return;
        }
        },
        "xyz.openbmc_project.Telemetry",
        "/xyz/openbmc_project/Telemetry/Reports/TelemetryService/" + *args.id,
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Telemetry.Report", "Enabled",
        dbus::utility::DbusVariantType{*args.metricReportDefinitionEnabled});
}

inline void setReportType(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                          const UserReportArgs& args)
{
    if (!args.id || !args.reportingType)
    {
        return;
    }

    crow::connections::systemBus->async_method_call(
        [aResp, args](const boost::system::error_code ec) {
        if (!verifyCommonErrors(aResp->res, *args.id, ec))
        {
            return;
        }
        },
        "xyz.openbmc_project.Telemetry",
        "/xyz/openbmc_project/Telemetry/Reports/TelemetryService/" + *args.id,
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Telemetry.Report", "ReportingType",
        dbus::utility::DbusVariantType{*args.reportingType});
}

inline void setReportUpdates(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                             const UserReportArgs& args)
{
    if (!args.id || !args.reportUpdates)
    {
        return;
    }

    crow::connections::systemBus->async_method_call(
        [aResp, args](const boost::system::error_code ec) {
        if (!verifyCommonErrors(aResp->res, *args.id, ec))
        {
            return;
        }
        },
        "xyz.openbmc_project.Telemetry",
        "/xyz/openbmc_project/Telemetry/Reports/TelemetryService/" + *args.id,
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Telemetry.Report", "ReportUpdates",
        dbus::utility::DbusVariantType{*args.reportUpdates});
}

inline void setReportActions(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                             const UserReportArgs& args)
{
    if (!args.id || !args.reportActions)
    {
        return;
    }

    crow::connections::systemBus->async_method_call(
        [aResp, args](const boost::system::error_code ec) {
        if (!verifyCommonErrors(aResp->res, *args.id, ec))
        {
            return;
        }
        },
        "xyz.openbmc_project.Telemetry",
        "/xyz/openbmc_project/Telemetry/Reports/TelemetryService/" + *args.id,
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Telemetry.Report", "ReportActions",
        dbus::utility::DbusVariantType{*args.reportActions});
}

inline void setReportInterval(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                              const UserReportArgs& args)
{
    if (!args.id || !args.recurrenceInterval)
    {
        return;
    }

    crow::connections::systemBus->async_method_call(
        [aResp, args](const boost::system::error_code ec) {
        if (!verifyCommonErrors(aResp->res, *args.id, ec))
        {
            return;
        }
        },
        "xyz.openbmc_project.Telemetry",
        "/xyz/openbmc_project/Telemetry/Reports/"
        "TelemetryService/" +
            *args.id,
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Telemetry.Report", "Interval",
        dbus::utility::DbusVariantType{
            static_cast<uint64_t>(args.recurrenceInterval->count())});
}

inline void
    setReportMetrics(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     UserReportArgs& args)
{
    if (!args.id || !args.metrics)
    {
        return;
    }

    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, telemetry::service,
        telemetry::getDbusReportPath(*args.id), telemetry::reportInterface,
        [asyncResp,
         args](boost::system::error_code ec,
               const dbus::utility::DBusPropertiesMap& properties) mutable {
        if (!redfish::telemetry::verifyCommonErrors(asyncResp->res, *args.id,
                                                    ec))
        {
            return;
        }

        const ReadingParameters* readingParams = nullptr;

        const bool success = sdbusplus::unpackPropertiesNoThrow(
            dbus_utils::UnpackErrorPrinter(), properties,
            "ReadingParametersFutureVersion", readingParams);

        if (!success || readingParams == nullptr)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        auto updateMetricsReq =
            std::make_shared<UpdateMetrics>(args, asyncResp);

        boost::container::flat_set<std::pair<std::string, std::string>>
            chassisSensors;

        for (size_t index = 0; index < args.redfishMetrics->size(); ++index)
        {
            const nlohmann::json& m = (*args.redfishMetrics)[index];
            const UserMetricArgs& metricArgs = (*args.metrics)[index];

            if (m.is_null())
            {
                continue;
            }

            if (metricArgs.metricProperties)
            {
                std::optional<IncorrectMetricProperty> error =
                    getChassisSensorNode(*metricArgs.metricProperties,
                                         chassisSensors);
                if (error)
                {
                    messages::propertyValueIncorrect(
                        asyncResp->res, error->metricProperty,
                        "MetricProperties/" + std::to_string(error->index));
                    return;
                }
            }

            if (index < readingParams->size())
            {
                const ReadingParameters::value_type& existing =
                    (*readingParams)[index];
                updateMetricsReq->emplace(
                    metricArgs.metricProperties, std::get<0>(existing),
                    metricArgs.collectionFunction.value_or(
                        std::get<1>(existing)),
                    metricArgs.id.value_or(std::get<2>(existing)),
                    metricArgs.collectionTimeScope.value_or(
                        std::get<3>(existing)),
                    static_cast<uint64_t>(
                        metricArgs.collectionDuration
                            .value_or(std::chrono::milliseconds(
                                std::get<4>(existing)))
                            .count()));
            }
            else
            {
                updateMetricsReq->emplace(
                    metricArgs.metricProperties.value_or(
                        std::vector<std::string>()),
                    {}, metricArgs.collectionFunction.value_or(""),
                    metricArgs.id.value_or(""),
                    metricArgs.collectionTimeScope.value_or(""),
                    static_cast<uint64_t>(
                        metricArgs.collectionDuration
                            .value_or(std::chrono::milliseconds(0))
                            .count()));
            }
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
                              const std::string& id)
{
    UserReportArgs args;

    args.id = id;

    if (!args.readUserParametersPatch(aResp->res, req))
    {
        return;
    }

    setReportEnabled(aResp, args);
    setReportType(aResp, args);
    setReportUpdates(aResp, args);
    setReportActions(aResp, args);
    setReportInterval(aResp, args);
    setReportMetrics(aResp, args);
}

inline void handleReportPut(const crow::Request& req,
                            const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                            const std::string& id)
{
    UserReportArgs args;

    if (!args.readUserParameters(aResp->res, req))
    {
        return;
    }

    boost::container::flat_set<std::pair<std::string, std::string>>
        chassisSensors;
    if (args.metrics &&
        !getChassisSensorNodeFromMetrics(aResp, *args.metrics, chassisSensors))
    {
        return;
    }

    const std::string reportPath = getDbusReportPath(id);

    crow::connections::systemBus->async_method_call(
        [aResp, id, args = std::move(args),
         chassisSensors =
             std::move(chassisSensors)](const boost::system::error_code ec) {
        addReportType addReportMode = addReportType::replace;
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR << "respHandler DBus error " << ec;
                messages::internalError(aResp->res);
                return;
            }
            BMCWEB_LOG_INFO << "Report not found, creating new report: " << id;
            addReportMode = addReportType::create;
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
                               const std::string& id)
{
    const std::string reportPath = getDbusReportPath(id);

    crow::connections::systemBus->async_method_call(
        [aResp, id](const boost::system::error_code ec) {
        if (!verifyCommonErrors(aResp->res, id, ec))
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
            telemetry::metricReportDefinitionUri;
        asyncResp->res.jsonValue["Name"] = "Metric Definition Collection";
        const std::vector<const char*> interfaces{telemetry::reportInterface};
        collection_util::getCollectionMembers(
            asyncResp, telemetry::metricReportDefinitionUri, interfaces,
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

        telemetry::UserReportArgs args;
        if (!args.readUserParameters(asyncResp->res, req))
        {
            return;
        }

        boost::container::flat_set<std::pair<std::string, std::string>>
            chassisSensors;

        if (args.metrics)
        {
            if (!telemetry::getChassisSensorNodeFromMetrics(
                    asyncResp, *args.metrics, chassisSensors))
            {
                return;
            }
        }

        auto addReportReq = std::make_shared<telemetry::AddReport>(
            std::move(args), asyncResp, telemetry::addReportType::create);
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
             id](const boost::system::error_code ec,
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
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& id) {
        telemetry::handleReportDelete(asyncResp, id);
        });

    BMCWEB_ROUTE(app,
                 "/redfish/v1/TelemetryService/MetricReportDefinitions/<str>/")
        .privileges(redfish::privileges::putMetricReportDefinition)
        .methods(boost::beast::http::verb::put)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& id) {
        telemetry::handleReportPut(req, asyncResp, id);
        });

    BMCWEB_ROUTE(app,
                 "/redfish/v1/TelemetryService/MetricReportDefinitions/<str>/")
        .privileges(redfish::privileges::patchMetricReportDefinition)
        .methods(boost::beast::http::verb::patch)(
            redfish::telemetry::handleReportPatch);
}
} // namespace redfish
