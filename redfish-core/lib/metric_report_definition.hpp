#pragma once

#include "sensors.hpp"
#include "utils/telemetry_utils.hpp"
#include "utils/time_utils.hpp"

#include <app.hpp>
#include <boost/container/flat_map.hpp>
#include <dbus_utility.hpp>
#include <query.hpp>
#include <registries/privilege_registry.hpp>
#include <utils/stl_utils.hpp>

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

    for (const auto& [key, var] : properties)
    {
        if (key == "ReportActions")
        {
            reportActions = std::get_if<std::vector<std::string>>(&var);
        }
        else if (key == "ReportUpdates")
        {
            reportUpdates = std::get_if<std::string>(&var);
        }
        else if (key == "AppendLimit")
        {
            appendLimit = std::get_if<uint64_t>(&var);
        }
        else if (key == "ReadingParametersFutureVersion")
        {
            readingParams = std::get_if<ReadingParameters>(&var);
        }
        else if (key == "ReportingType")
        {
            reportingType = std::get_if<std::string>(&var);
        }
        else if (key == "Interval")
        {
            interval = std::get_if<uint64_t>(&var);
        }
        else if (key == "Name")
        {
            name = std::get_if<std::string>(&var);
        }
        else if (key == "Enabled")
        {
            enabled = std::get_if<bool>(&var);
        }
        else if (key == "ErrorMessages")
        {
            errorMessages =
                std::get_if<std::vector<std::tuple<std::string, std::string>>>(
                    &var);
        }
        else if (key == "Triggers")
        {
            triggers =
                std::get_if<std::vector<sdbusplus::message::object_path>>(&var);
        }
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

    std::optional<nlohmann::json> linkedTriggers;
    if (triggers != nullptr)
    {
        linkedTriggers = getLinkedTriggers(*triggers);
        if (!linkedTriggers)
        {
            messages::internalError(asyncResp->res);
            return;
        }
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
        nlohmann::json& metrics = asyncResp->res.jsonValue["Metrics"];
        metrics = nlohmann::json::array();
        for (const auto& [sensorData, collectionFunction, id,
                          collectionTimeScope, collectionDuration] :
             *readingParams)
        {
            std::vector<std::string> metricProperties;

            for (const auto& [sensorPath, sensorMetadata] : sensorData)
            {
                metricProperties.emplace_back(sensorMetadata);
            }

            metrics.push_back(
                {{"MetricId", id},
                 {"MetricProperties", std::move(metricProperties)},
                 {"CollectionFunction", collectionFunction},
                 {"CollectionDuration",
                  time_utils::toDurationString(
                      std::chrono::milliseconds(collectionDuration))},
                 {"CollectionTimeScope", collectionTimeScope}});
        }
    }

    if (linkedTriggers)
    {
        asyncResp->res.jsonValue["Links"]["Triggers"] = *linkedTriggers;
    }
}

struct MetricArgs
{
    std::string id;
    std::vector<std::string> uris;
    std::optional<std::string> collectionFunction;
    std::optional<std::string> collectionTimeScope;
    std::optional<uint64_t> collectionDuration;
};

struct AddReportArgs
{
    std::optional<std::string> id;
    std::optional<std::string> name;
    std::string reportingType;
    std::optional<std::string> reportUpdates;
    std::optional<uint64_t> appendLimit;
    std::vector<std::string> reportActions;
    uint64_t interval = 0;
    std::vector<MetricArgs> metrics;
};

inline bool toDbusReportActions(crow::Response& res,
                                const std::vector<std::string>& redfishActions,
                                std::vector<std::string>& dbusActions)
{
    size_t index = 0;
    for (const auto& redfishAction : redfishActions)
    {
        std::string dbusAction = toDbusReportAction(redfishAction);

        if (dbusAction.empty())
        {
            messages::propertyValueNotInList(
                res, redfishAction, "ReportActions/" + std::to_string(index));
            return false;
        }

        dbusActions.emplace_back(std::move(dbusAction));
        index++;
    }
    return true;
}

inline bool getUserParameters(crow::Response& res, const crow::Request& req,
                              AddReportArgs& args)
{
    std::vector<nlohmann::json> metrics;
    std::vector<std::string> reportActions;
    std::optional<nlohmann::json> schedule;
    if (!json_util::readJsonPatch(
            req, res, "Id", args.id, "Name", args.name, "Metrics", metrics,
            "MetricReportDefinitionType", args.reportingType, "ReportUpdates",
            args.reportUpdates, "AppendLimit", args.appendLimit,
            "ReportActions", reportActions, "Schedule", schedule))
    {
        return false;
    }

    if (args.reportingType != "Periodic" && args.reportingType != "OnRequest" &&
        args.reportingType != "OnChange")
    {
        messages::propertyValueNotInList(res, args.reportingType,
                                         "MetricReportDefinitionType");
        return false;
    }

    if (!toDbusReportActions(res, reportActions, args.reportActions))
    {
        return false;
    }

    if (args.reportingType == "Periodic")
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
        if (!durationNum)
        {
            messages::propertyValueIncorrect(res, "RecurrenceInterval",
                                             durationStr);
            return false;
        }
        args.interval = static_cast<uint64_t>(durationNum->count());
    }

    args.metrics.reserve(metrics.size());
    for (auto& m : metrics)
    {
        std::optional<std::string> collectionDurationStr;
        MetricArgs metricArgs;
        if (!json_util::readJson(
                m, res, "MetricId", metricArgs.id, "MetricProperties",
                metricArgs.uris, "CollectionFunction",
                metricArgs.collectionFunction, "CollectionTimeScope",
                metricArgs.collectionTimeScope, "CollectionDuration",
                collectionDurationStr))
        {
            return false;
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

        args.metrics.emplace_back(std::move(metricArgs));
    }

    return true;
}

inline bool getChassisSensorNodeFromMetrics(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::vector<MetricArgs>& metrics,
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

class UpdateMetrics
{
  public:
    UpdateMetrics(const std::string& idIn,
                  const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) :
        asyncResp(asyncResp),
        id(idIn)
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

    void emplace(const std::optional<std::vector<std::string>>& uris,
                 const std::vector<std::tuple<sdbusplus::message::object_path,
                                              std::string>>& pathAndUri,
                 const std::string& collectionFunction, const std::string& id,
                 const std::string& collectionTimeScope,
                 const uint64_t collectionDuration)
    {
        readingParamsUris.emplace_back(uris);
        readingParams.emplace_back(pathAndUri, collectionFunction, id,
                                   collectionTimeScope, collectionDuration);
    }

    void insert(const boost::container::flat_map<std::string, std::string>& el)
    {
        uriToDbus.insert(el.begin(), el.end());
    }

    void setReadingParams()
    {
        if (asyncResp->res.result() != boost::beast::http::status::ok)
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

        const std::shared_ptr<bmcweb::AsyncResp> aResp = asyncResp;
        crow::connections::systemBus->async_method_call(
            [aResp, id = id](const boost::system::error_code ec) {
            if (!verifyCommonErrors(aResp->res, id, ec))
            {
                return;
            }
            },
            "xyz.openbmc_project.Telemetry",
            "/xyz/openbmc_project/Telemetry/Reports/TelemetryService/" + id,
            "org.freedesktop.DBus.Properties", "Set",
            "xyz.openbmc_project.Telemetry.Report",
            "ReadingParametersFutureVersion",
            dbus::utility::DbusVariantType{readingParams});
    }

  private:
    std::optional<
        std::vector<std::tuple<sdbusplus::message::object_path, std::string>>>
        sensorPathToUri(const std::vector<std::string>& uris) const
    {
        std::vector<std::tuple<sdbusplus::message::object_path, std::string>>
            result;

        for (const std::string& uri : uris)
        {
            auto it = uriToDbus.find(uri);
            if (it == uriToDbus.end())
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
    std::string id;
    std::vector<std::optional<std::vector<std::string>>> readingParamsUris;
    ReadingParameters readingParams{};
    boost::container::flat_map<std::string, std::string> uriToDbus{};
};

class AddReport
{
  public:
    AddReport(AddReportArgs argsIn,
              const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
              addReportType type) :
        asyncResp(asyncResp),
        args{std::move(argsIn)}, type(type)
    {}
    ~AddReport()
    {
        if (asyncResp->res.result() != boost::beast::http::status::ok)
        {
            return;
        }

        ReadingParameters readingParams;
        readingParams.reserve(args.metrics.size());

        for (auto& metric : args.metrics)
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
                std::move(sensorParams), metric.collectionFunction.value_or(""),
                std::move(metric.id), metric.collectionTimeScope.value_or(""),
                metric.collectionDuration.value_or(0U));
        }

        const std::shared_ptr<bmcweb::AsyncResp> aResp = asyncResp;
        crow::connections::systemBus->async_method_call(
            [aResp, id = args.id.value_or(""), uriToDbus = std::move(uriToDbus),
             type = type](const boost::system::error_code ec,
                          const std::string&) {
            if (ec == boost::system::errc::argument_list_too_long)
            {
                nlohmann::json metricProperties = nlohmann::json::array();
                for (const auto& [uri, _] : uriToDbus)
                {
                    metricProperties.emplace_back(uri);
                }
                messages::propertyValueIncorrect(
                    aResp->res, metricProperties.dump(), "MetricProperties");
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
            "TelemetryService/" + args.id.value_or(""), args.name.value_or(""),
            args.reportingType, args.reportUpdates.value_or("Overwrite"),
            args.appendLimit.value_or(0), args.reportActions, args.interval,
            readingParams);
    }

    AddReport(const AddReport&) = delete;
    AddReport(AddReport&&) = delete;
    AddReport& operator=(const AddReport&) = delete;
    AddReport& operator=(AddReport&&) = delete;

    void insert(const boost::container::flat_map<std::string, std::string>& el)
    {
        uriToDbus.insert(el.begin(), el.end());
    }

  private:
    const std::shared_ptr<bmcweb::AsyncResp> asyncResp;
    AddReportArgs args;
    const addReportType type;
    boost::container::flat_map<std::string, std::string> uriToDbus{};
}; // namespace telemetry

inline void setReportEnabled(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                             const std::string& id, bool enabled)
{
    crow::connections::systemBus->async_method_call(
        [aResp, id, enabled](const boost::system::error_code ec) {
        if (!verifyCommonErrors(aResp->res, id, ec))
        {
            return;
        }
        },
        "xyz.openbmc_project.Telemetry",
        "/xyz/openbmc_project/Telemetry/Reports/TelemetryService/" + id,
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Telemetry.Report", "Enabled",
        dbus::utility::DbusVariantType{enabled});
}

inline void setReportType(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                          const std::string& id, const std::string& type)
{
    if (type != "Periodic" && type != "OnChange" && type != "OnRequest")
    {
        messages::propertyValueNotInList(aResp->res, type,
                                         "MetricReportDefinitionType");
        return;
    }

    crow::connections::systemBus->async_method_call(
        [aResp, id, type](const boost::system::error_code ec) {
        if (!verifyCommonErrors(aResp->res, id, ec))
        {
            return;
        }
        },
        "xyz.openbmc_project.Telemetry",
        "/xyz/openbmc_project/Telemetry/Reports/TelemetryService/" + id,
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Telemetry.Report", "ReportingType",
        dbus::utility::DbusVariantType{type});
}

inline void setReportUpdates(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                             const std::string& id, const std::string& updates)
{
    if (updates != "Overwrite" && updates != "AppendWrapsWhenFull" &&
        updates != "AppendStopsWhenFull")
    {
        messages::propertyValueNotInList(aResp->res, updates, "ReportUpdates");
        return;
    }

    crow::connections::systemBus->async_method_call(
        [aResp, id, updates](const boost::system::error_code ec) {
        if (!verifyCommonErrors(aResp->res, id, ec))
        {
            return;
        }
        },
        "xyz.openbmc_project.Telemetry",
        "/xyz/openbmc_project/Telemetry/Reports/TelemetryService/" + id,
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Telemetry.Report", "ReportUpdates",
        dbus::utility::DbusVariantType{updates});
}

inline void setReportActions(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                             const std::string& id,
                             std::vector<std::string>& redfishActions)
{
    if (redfishActions.size() > 1)
    {
        stl_utils::removeDuplicate(redfishActions);
    }

    std::vector<std::string> newDbusActions;
    if (!toDbusReportActions(aResp->res, redfishActions, newDbusActions))
    {
        return;
    }

    crow::connections::systemBus->async_method_call(
        [aResp, id, redfishActions](const boost::system::error_code ec) {
        if (!verifyCommonErrors(aResp->res, id, ec))
        {
            return;
        }
        },
        "xyz.openbmc_project.Telemetry",
        "/xyz/openbmc_project/Telemetry/Reports/TelemetryService/" + id,
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Telemetry.Report", "ReportActions",
        dbus::utility::DbusVariantType{newDbusActions});
}

inline void setReportInterval(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                              const std::string& id, nlohmann::json& schedule)
{
    std::string durationStr;
    if (!json_util::readJson(schedule, aResp->res, "RecurrenceInterval",
                             durationStr))
    {
        return;
    }

    std::optional<std::chrono::milliseconds> durationNum =
        time_utils::fromDurationString(durationStr);
    uint64_t interval = static_cast<uint64_t>(durationNum->count());

    crow::connections::systemBus->async_method_call(
        [aResp, id, durationStr](const boost::system::error_code ec) {
        if (!verifyCommonErrors(aResp->res, id, ec))
        {
            return;
        }
        },
        "xyz.openbmc_project.Telemetry",
        "/xyz/openbmc_project/Telemetry/Reports/"
        "TelemetryService/" +
            id,
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Telemetry.Report", "Interval",
        dbus::utility::DbusVariantType{interval});
}

inline void
    setReportMetrics(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const std::string& id,
                     std::vector<nlohmann::json>& metricJsons)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, id, metricJsons](
            boost::system::error_code ec,
            const std::vector<
                std::pair<std::string, dbus::utility::DbusVariantType>>&
                properties) mutable {
        if (!redfish::telemetry::verifyCommonErrors(asyncResp->res, id, ec))
        {
            return;
        }

        const ReadingParameters* readingParams = nullptr;

        for (const auto& [key, var] : properties)
        {
            if (key == "ReadingParametersFutureVersion")
            {
                readingParams = std::get_if<ReadingParameters>(&var);
            }
        }

        if (readingParams == nullptr)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        auto updateMetricsReq = std::make_shared<UpdateMetrics>(id, asyncResp);

        boost::container::flat_set<std::pair<std::string, std::string>>
            chassisSensors;

        for (size_t index = 0; index < metricJsons.size(); ++index)
        {
            nlohmann::json& m = metricJsons[index];

            if (m.is_null())
            {
                continue;
            }

            std::optional<std::string> id;
            std::optional<std::vector<std::string>> uris;
            std::optional<std::string> collectionFunction;
            std::optional<std::string> collectionTimeScope;
            std::optional<std::string> collectionDurationStr;

            if (!json_util::readJson(
                    m, asyncResp->res, "MetricId", id, "MetricProperties", uris,
                    "CollectionFunction", collectionFunction,
                    "CollectionTimeScope", collectionTimeScope,
                    "CollectionDuration", collectionDurationStr))
            {
                return;
            }

            std::optional<uint64_t> collectionDuration;

            if (collectionDurationStr)
            {
                std::optional<std::chrono::milliseconds> duration =
                    time_utils::fromDurationString(*collectionDurationStr);

                if (!duration || duration->count() < 0)
                {
                    messages::propertyValueIncorrect(asyncResp->res,
                                                     "CollectionDuration",
                                                     *collectionDurationStr);
                    return;
                }

                collectionDuration = static_cast<uint64_t>(duration->count());
            }

            if (uris)
            {
                std::optional<IncorrectMetricUri> error =
                    getChassisSensorNode(*uris, chassisSensors);
                if (error)
                {
                    messages::propertyValueIncorrect(
                        asyncResp->res, error->uri,
                        "MetricProperties/" + std::to_string(error->index));
                    return;
                }
            }

            if (index < readingParams->size())
            {
                const ReadingParameters::value_type& existing =
                    (*readingParams)[index];
                updateMetricsReq->emplace(
                    uris, std::get<0>(existing),
                    collectionFunction.value_or(std::get<1>(existing)),
                    id.value_or(std::get<2>(existing)),
                    collectionTimeScope.value_or(std::get<3>(existing)),
                    collectionDuration.value_or(std::get<4>(existing)));
            }
            else
            {
                updateMetricsReq->emplace(
                    uris, {}, collectionFunction.value_or(""), id.value_or(""),
                    collectionTimeScope.value_or(""),
                    collectionDuration.value_or(0));
            }
        }

        for (const auto& [chassis, sensorType] : chassisSensors)
        {
            retrieveUriToDbusMap(
                chassis, sensorType,
                [asyncResp, updateMetricsReq](
                    const boost::beast::http::status status,
                    const boost::container::flat_map<std::string, std::string>&
                        uriToDbus) {
                if (status != boost::beast::http::status::ok)
                {
                    BMCWEB_LOG_ERROR
                        << "Failed to retrieve URI to dbus sensors map with err "
                        << static_cast<unsigned>(status);
                    return;
                }
                updateMetricsReq->insert(uriToDbus);
                });
        }
        },
        telemetry::service, telemetry::getDbusReportPath(id),
        "org.freedesktop.DBus.Properties", "GetAll",
        telemetry::reportInterface);
}

inline void handleReportPatch(const crow::Request& req,
                              const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                              const std::string& id)
{
    std::optional<bool> enabled;
    std::optional<std::string> type;
    std::optional<std::string> updates;
    std::optional<std::vector<std::string>> actions;
    std::optional<nlohmann::json> schedule;
    std::optional<std::vector<nlohmann::json>> metrics;

    if (!json_util::readJsonPatch(
            req, aResp->res, "MetricReportDefinitionEnabled", enabled,
            "Schedule", schedule, "ReportActions", actions, "Metrics", metrics,
            "MetricReportDefinitionType", type, "ReportUpdates", updates))
    {
        return;
    }

    if (enabled)
    {
        setReportEnabled(aResp, id, *enabled);
    }
    if (type)
    {
        setReportType(aResp, id, *type);
    }
    if (updates)
    {
        setReportUpdates(aResp, id, *updates);
    }
    if (actions)
    {
        setReportActions(aResp, id, *actions);
    }
    if (schedule)
    {
        setReportInterval(aResp, id, *schedule);
    }
    if (metrics)
    {
        setReportMetrics(aResp, id, *metrics);
    }
}

inline void handleReportPut(const crow::Request& req,
                            const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                            const std::string& id)
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
        for (const auto& [chassis, sensorType] : chassisSensors)
        {
            retrieveUriToDbusMap(
                chassis, sensorType,
                [aResp, addReportReq](
                    const boost::beast::http::status status,
                    const boost::container::flat_map<std::string, std::string>&
                        uriToDbus) {
                if (status != boost::beast::http::status::ok)
                {
                    BMCWEB_LOG_ERROR << "Failed to retrieve URI to dbus "
                                        "sensors map with err "
                                     << static_cast<unsigned>(status);
                    return;
                }
                addReportReq->insert(uriToDbus);
                });
        }
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
        if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
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
        if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
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
            std::move(args), asyncResp, telemetry::addReportType::create);
        for (const auto& [chassis, sensorType] : chassisSensors)
        {
            retrieveUriToDbusMap(
                chassis, sensorType,
                [asyncResp, addReportReq](
                    const boost::beast::http::status status,
                    const boost::container::flat_map<std::string, std::string>&
                        uriToDbus) {
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
        if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
        {
            return;
        }

        crow::connections::systemBus->async_method_call(
            [asyncResp, id](
                boost::system::error_code ec,
                const std::vector<std::pair<
                    std::string, dbus::utility::DbusVariantType>>& properties) {
            if (!redfish::telemetry::verifyCommonErrors(asyncResp->res, id, ec))
            {
                return;
            }

            telemetry::fillReportDefinition(asyncResp, id, properties);
            },
            telemetry::service, telemetry::getDbusReportPath(id),
            "org.freedesktop.DBus.Properties", "GetAll",
            telemetry::reportInterface);
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
