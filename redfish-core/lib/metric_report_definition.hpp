#pragma once

#include "sensors.hpp"
#include "utils/telemetry_utils.hpp"
#include "utils/time_utils.hpp"

#include <app.hpp>
#include <boost/container/flat_map.hpp>
#include <dbus_utility.hpp>
#include <registries/privilege_registry.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <tuple>
#include <variant>

namespace redfish
{

namespace telemetry
{

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

inline void fillReportDefinition(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, const std::string& id,
    const std::vector<std::pair<std::string, dbus::utility::DbusVariantType>>&
        properties)
{
    try
    {
        std::vector<std::string> reportActions;
        ReadingParameters readingParams;
        std::string reportingType;
        std::string reportUpdates;
        std::string name;
        bool enabled = false;
        uint64_t appendLimit = 0u;
        uint64_t interval = 0u;

        sdbusplus::unpackProperties(
            properties, "ReportActions", reportActions, "ReportUpdates",
            reportUpdates, "AppendLimit", appendLimit,
            "ReadingParametersFutureVersion", readingParams, "ReportingType",
            reportingType, "Interval", interval, "Name", name, "Enabled",
            enabled);

        std::string state = enabled ? "Enabled" : "Disabled";

        for (std::string& action : reportActions)
        {
            action = toReadfishReportAction(action);

            if (action.empty())
            {
                messages::internalError(asyncResp->res);
                return;
            }
        }

        nlohmann::json metrics = nlohmann::json::array();
        for (auto& [sensorData, collectionFunction, id, collectionTimeScope,
                    collectionDuration] : readingParams)
        {
            std::vector<std::string> metricProperties;

            for (auto& [sensorPath, sensorMetadata] : sensorData)
            {
                metricProperties.emplace_back(std::move(sensorMetadata));
            }

            metrics.push_back(
                {{"MetricId", std::move(id)},
                 {"MetricProperties", std::move(metricProperties)},
                 {"CollectionFunction", std::move(collectionFunction)},
                 {"CollectionDuration",
                  time_utils::toDurationString(
                      std::chrono::milliseconds(collectionDuration))},
                 {"CollectionTimeScope", std::move(collectionTimeScope)}});
        }

        asyncResp->res.jsonValue["@odata.type"] =
            "#MetricReportDefinition.v1_3_0.MetricReportDefinition";
        asyncResp->res.jsonValue["@odata.id"] =
            metricReportDefinitionUri + std::string("/") + id;
        asyncResp->res.jsonValue["Id"] = id;
        asyncResp->res.jsonValue["Name"] = name;
        asyncResp->res.jsonValue["MetricReport"]["@odata.id"] =
            metricReportUri + std::string("/") + id;
        asyncResp->res.jsonValue["Status"]["State"] = state;
        asyncResp->res.jsonValue["ReportUpdates"] = reportUpdates;
        asyncResp->res.jsonValue["AppendLimit"] = appendLimit;
        asyncResp->res.jsonValue["Metrics"] = metrics;
        asyncResp->res.jsonValue["MetricReportDefinitionEnabled"] = enabled;
        asyncResp->res.jsonValue["MetricReportDefinitionType"] = reportingType;
        asyncResp->res.jsonValue["ReportActions"] = reportActions;
        asyncResp->res.jsonValue["Schedule"]["RecurrenceInterval"] =
            time_utils::toDurationString(std::chrono::milliseconds(interval));
    }
    catch (const sdbusplus::exception::UnpackPropertyError& error)
    {
        BMCWEB_LOG_ERROR << error.what() << ", property: "
                         << error.propertyName + ", reason: " << error.reason;
        messages::queryParameterValueFormatError(
            asyncResp->res,
            std::string(error.propertyName) + " " + std::string(error.reason),
            error.what());
        messages::internalError(asyncResp->res);
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
                                const std::vector<std::string>& actions,
                                AddReportArgs& args)
{
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

        args.reportActions.emplace_back(std::move(dbusReportAction));
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
    if (!json_util::readJson(
            req, res, "Id", args.id, "Name", args.name, "Metrics", metrics,
            "MetricReportDefinitionType", args.reportingType, "ReportUpdates",
            args.reportUpdates, "AppendLimit", args.appendLimit,
            "ReportActions", reportActions, "Schedule", schedule))
    {
        return false;
    }

    if (args.reportingType != "Periodic" && args.reportingType != "OnRequest")
    {
        messages::propertyValueNotInList(res, args.reportingType,
                                         "MetricReportDefinitionType");
        return false;
    }

    if (!toDbusReportActions(res, reportActions, args))
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

inline bool getChassisSensorNode(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::vector<MetricArgs>& metrics,
    boost::container::flat_set<std::pair<std::string, std::string>>& matched)
{
    for (const auto& metric : metrics)
    {
        for (size_t i = 0; i < metric.uris.size(); i++)
        {
            const std::string& uri = metric.uris[i];
            std::string chassis;
            std::string node;

            if (!boost::starts_with(uri, "/redfish/v1/Chassis/") ||
                !dbus::utility::getNthStringFromPath(uri, 3, chassis) ||
                !dbus::utility::getNthStringFromPath(uri, 4, node))
            {
                BMCWEB_LOG_ERROR
                    << "Failed to get chassis and sensor Node from " << uri;
                messages::propertyValueIncorrect(asyncResp->res, uri,
                                                 "MetricProperties/" +
                                                     std::to_string(i));
                return false;
            }

            if (boost::ends_with(node, "#"))
            {
                node.pop_back();
            }

            matched.emplace(std::move(chassis), std::move(node));
        }
    }
    return true;
}

inline bool verifyCommonErrors(crow::Response& res, const std::string& id,
                               const boost::system::error_code ec)
{
    /*
     * boost::system::errc and std::errc are missing value
     * for EBADR error that is defined in Linux.
     */
    if (ec.value() == EBADR)
    {
        messages::resourceNotFound(res, "MetricReportDefinition", id);
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

inline bool getReadingParametersFromMetrics(
    crow::Response& res, const std::vector<MetricArgs>& metrics,
    const boost::container::flat_map<std::string, std::string>& uriToDbus,
    ReadingParameters& readingParams)
{
    if (metrics.empty())
    {
        return true;
    }

    readingParams.reserve(metrics.size());
    for (const auto& metric : metrics)
    {
        std::vector<std::tuple<sdbusplus::message::object_path, std::string>>
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
                messages::propertyValueNotInList(
                    res, uri, "MetricProperties/" + std::to_string(i));
                return false;
            }

            const std::string& dbusPath = el->second;
            sensorParams.emplace_back(dbusPath, uri);
        }

        readingParams.emplace_back(
            std::move(sensorParams), metric.collectionFunction.value_or(""),
            metric.id, metric.collectionTimeScope.value_or(""),
            metric.collectionDuration.value_or(0u));
    }

    return true;
}

class UpdateMetrics
{
  public:
    UpdateMetrics(const std::string& idIn, std::vector<MetricArgs> metricsIn,
                  const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) :
        asyncResp(asyncResp),
        id(idIn), metrics{std::move(metricsIn)}
    {}

    ~UpdateMetrics()
    {
        if (asyncResp->res.result() != boost::beast::http::status::ok)
        {
            return;
        }

        if (!getReadingParametersFromMetrics(asyncResp->res, metrics, uriToDbus,
                                             readingParams))
        {
            return;
        }

        const std::shared_ptr<bmcweb::AsyncResp> aResp = asyncResp;
        crow::connections::systemBus->async_method_call(
            [aResp, id = id](const boost::system::error_code ec) {
                if (!verifyCommonErrors(aResp->res, id, ec))
                {
                    return;
                }

                messages::propertyValueModified(aResp->res, "Metrics",
                                                "Updated");
            },
            "xyz.openbmc_project.Telemetry",
            "/xyz/openbmc_project/Telemetry/Reports/TelemetryService/" + id,
            "org.freedesktop.DBus.Properties", "Set",
            "xyz.openbmc_project.Telemetry.Report",
            "ReadingParametersFutureVersion",
            std::variant<ReadingParameters>(readingParams));
    }

    void insert(const boost::container::flat_map<std::string, std::string>& el)
    {
        uriToDbus.insert(el.begin(), el.end());
    }

  private:
    const std::shared_ptr<bmcweb::AsyncResp> asyncResp;
    std::string id;
    std::vector<MetricArgs> metrics;
    boost::container::flat_map<std::string, std::string> uriToDbus{};
    ReadingParameters readingParams{};
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
                metric.collectionDuration.value_or(0u));
        }
        const std::shared_ptr<bmcweb::AsyncResp> aResp = asyncResp;
        crow::connections::systemBus->async_method_call(
            [aResp, id = args.id.value_or(""), uriToDbus = std::move(uriToDbus),
             type = type](const boost::system::error_code ec,
                          const std::string&) {
                if (ec == boost::system::errc::file_exists)
                {
                    messages::resourceAlreadyExists(
                        aResp->res, "MetricReportDefinition", "Id", id);
                    return;
                }
                if (ec == boost::system::errc::too_many_files_open)
                {
                    messages::createLimitReachedForResource(aResp->res);
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
                        aResp->res, metricProperties, "MetricProperties");
                    return;
                }
                if (ec)
                {
                    messages::internalError(aResp->res);
                    BMCWEB_LOG_ERROR << "respHandler DBus error " << ec;
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

    void insert(const boost::container::flat_map<std::string, std::string>& el)
    {
        uriToDbus.insert(el.begin(), el.end());
    }

  private:
    const std::shared_ptr<bmcweb::AsyncResp> asyncResp;
    AddReportArgs args;
    const addReportType type;
    boost::container::flat_map<std::string, std::string> uriToDbus{};
};

inline void setReportState(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                           const std::string& id, nlohmann::json& status)
{
    std::string state;
    if (!json_util::readJson(status, aResp->res, "State", state))
    {
        return;
    }

    if (state != "Enabled" && state != "Disabled")
    {
        messages::propertyValueNotInList(aResp->res, state, "State");
        return;
    }

    bool enabled = state == "Enabled" ? true : false;
    crow::connections::systemBus->async_method_call(
        [aResp, id, enabled, state](const boost::system::error_code ec,
                                    const std::variant<bool>& currEnabledVar) {
            if (!verifyCommonErrors(aResp->res, id, ec))
            {
                return;
            }

            const bool* currEnabled = std::get_if<bool>(&currEnabledVar);
            if (!currEnabled || *currEnabled == enabled)
            {
                return;
            }

            crow::connections::systemBus->async_method_call(
                [aResp, id, enabled,
                 state](const boost::system::error_code ec) {
                    if (!verifyCommonErrors(aResp->res, id, ec))
                    {
                        return;
                    }

                    messages::propertyValueModified(aResp->res, "State", state);
                },
                "xyz.openbmc_project.Telemetry",
                "/xyz/openbmc_project/Telemetry/Reports/TelemetryService/" + id,
                "org.freedesktop.DBus.Properties", "Set",
                "xyz.openbmc_project.Telemetry.Report", "Enabled",
                std::variant<bool>(enabled));
        },
        "xyz.openbmc_project.Telemetry",
        "/xyz/openbmc_project/Telemetry/Reports/TelemetryService/" + id,
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Telemetry.Report", "Enabled");
}

inline void setReportInterval(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                              const std::string& id, nlohmann::json& schedule)
{
    crow::connections::systemBus->async_method_call(
        [aResp, schedule = std::move(schedule),
         id](const boost::system::error_code ec,
             const std::variant<std::string>& typeVar) mutable {
            if (!verifyCommonErrors(aResp->res, id, ec))
            {
                return;
            }

            const std::string* reportingType =
                std::get_if<std::string>(&typeVar);
            if (!reportingType || *reportingType != "Periodic")
            {
                return;
            }

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
                [aResp, id, interval,
                 durationStr](const boost::system::error_code ec,
                              const std::variant<uint64_t>& currIntervalVar) {
                    if (!verifyCommonErrors(aResp->res, id, ec))
                    {
                        return;
                    }

                    const uint64_t* currInterval =
                        std::get_if<uint64_t>(&currIntervalVar);
                    if (!currInterval || *currInterval == interval)
                    {
                        return;
                    }
                    crow::connections::systemBus->async_method_call(
                        [aResp, id,
                         durationStr](const boost::system::error_code ec) {
                            if (!verifyCommonErrors(aResp->res, id, ec))
                            {
                                return;
                            }

                            messages::propertyValueModified(
                                aResp->res, "RecurrenceInterval", durationStr);
                        },
                        "xyz.openbmc_project.Telemetry",
                        "/xyz/openbmc_project/Telemetry/Reports/"
                        "TelemetryService/" +
                            id,
                        "org.freedesktop.DBus.Properties", "Set",
                        "xyz.openbmc_project.Telemetry.Report", "Interval",
                        std::variant<uint64_t>(interval));
                },
                "xyz.openbmc_project.Telemetry",
                "/xyz/openbmc_project/Telemetry/Reports/TelemetryService/" + id,
                "org.freedesktop.DBus.Properties", "Get",
                "xyz.openbmc_project.Telemetry.Report", "Interval");
        },
        "xyz.openbmc_project.Telemetry",
        "/xyz/openbmc_project/Telemetry/Reports/TelemetryService/" + id,
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Telemetry.Report", "ReportingType");
}

inline void setReportMetrics(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                             const std::string& id,
                             std::vector<nlohmann::json>& metricJsons)
{
    std::vector<MetricArgs> metrics;
    metrics.reserve(metricJsons.size());

    for (auto& m : metricJsons)
    {
        MetricArgs metricArgs;
        std::optional<std::string> collectionDurationStr;
        if (!json_util::readJson(
                m, aResp->res, "MetricId", metricArgs.id, "MetricProperties",
                metricArgs.uris, "CollectionFunction",
                metricArgs.collectionFunction, "CollectionTimeScope",
                metricArgs.collectionTimeScope, "CollectionDuration",
                collectionDurationStr))
        {
            return;
        }

        if (collectionDurationStr)
        {
            std::optional<std::chrono::milliseconds> duration =
                time_utils::fromDurationString(*collectionDurationStr);

            if (!duration || duration->count() < 0)
            {
                messages::propertyValueIncorrect(
                    aResp->res, "CollectionDuration", *collectionDurationStr);
                return;
            }

            metricArgs.collectionDuration =
                static_cast<uint64_t>(duration->count());
        }

        metrics.emplace_back(std::move(metricArgs));
    }

    boost::container::flat_set<std::pair<std::string, std::string>>
        chassisSensors;
    if (!getChassisSensorNode(aResp, metrics, chassisSensors))
    {
        return;
    }

    auto updateMetricsReq =
        std::make_shared<UpdateMetrics>(id, std::move(metrics), aResp);

    for (const auto& [chassis, sensorType] : chassisSensors)
    {
        retrieveUriToDbusMap(
            chassis, sensorType,
            [aResp, updateMetricsReq](
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
}

inline void handleReportPatch(const crow::Request& req,
                              const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                              const std::string& id)
{
    std::optional<nlohmann::json> status;
    std::optional<nlohmann::json> schedule;
    std::optional<std::vector<nlohmann::json>> metrics;

    if (!json_util::readJson(req, aResp->res, "Status", status, "Schedule",
                             schedule, "Metrics", metrics))
    {
        return;
    }

    if (status)
    {
        setReportState(aResp, id, *status);
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
    if (!getChassisSensorNode(aResp, args.metrics, chassisSensors))
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
                /*
                 * boost::system::errc and std::errc are missing value
                 * for EBADR error that is defined in Linux.
                 */
                if (ec.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR << "respHandler DBus error " << ec;
                    messages::internalError(aResp->res);
                    return;
                }
                BMCWEB_LOG_INFO << "Report not found, creating new report: "
                                << id;
                addReportMode = addReportType::create;
            }

            auto addReportReq =
                std::make_shared<AddReport>(args, aResp, addReportMode);
            for (const auto& [chassis, sensorType] : chassisSensors)
            {
                retrieveUriToDbusMap(
                    chassis, sensorType,
                    [aResp,
                     addReportReq](const boost::beast::http::status status,
                                   const boost::container::flat_map<
                                       std::string, std::string>& uriToDbus) {
                        if (status != boost::beast::http::status::ok)
                        {
                            BMCWEB_LOG_ERROR
                                << "Failed to retrieve URI to dbus "
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
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#MetricReportDefinitionCollection."
                    "MetricReportDefinitionCollection";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/TelemetryService/MetricReportDefinitions";
                asyncResp->res.jsonValue["Name"] =
                    "Metric Definition Collection";
                const std::vector<const char*> interfaces{
                    telemetry::reportInterface};
                collection_util::getCollectionMembers(
                    asyncResp, telemetry::metricReportDefinitionUri, interfaces,
                    "/xyz/openbmc_project/Telemetry/Reports/TelemetryService");
            });

    BMCWEB_ROUTE(app, "/redfish/v1/TelemetryService/MetricReportDefinitions/")
        .privileges(redfish::privileges::postMetricReportDefinitionCollection)
        .methods(
            boost::beast::http::verb::
                post)([](const crow::Request& req,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
            telemetry::AddReportArgs args;
            if (!telemetry::getUserParameters(asyncResp->res, req, args))
            {
                return;
            }

            boost::container::flat_set<std::pair<std::string, std::string>>
                chassisSensors;
            if (!telemetry::getChassisSensorNode(asyncResp, args.metrics,
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
                    [asyncResp,
                     addReportReq](const boost::beast::http::status status,
                                   const boost::container::flat_map<
                                       std::string, std::string>& uriToDbus) {
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
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& id) {
                sdbusplus::asio::getAllProperties(
                    *crow::connections::systemBus, telemetry::service,
                    telemetry::getDbusReportPath(id),
                    telemetry::reportInterface,
                    [asyncResp,
                     id](boost::system::error_code ec,
                         const std::vector<std::pair<
                             std::string, dbus::utility::DbusVariantType>>&
                             properties) {
                        if (ec.value() == EBADR ||
                            ec == boost::system::errc::host_unreachable)
                        {
                            messages::resourceNotFound(
                                asyncResp->res, "MetricReportDefinition", id);
                            return;
                        }
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR << "respHandler DBus error " << ec;
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        telemetry::fillReportDefinition(asyncResp, id,
                                                        properties);
                    });
            });

    BMCWEB_ROUTE(app,
                 "/redfish/v1/TelemetryService/MetricReportDefinitions/<str>/")
        .privileges({{"ConfigureManager"}})
        .methods(boost::beast::http::verb::put, boost::beast::http::verb::patch,
                 boost::beast::http::verb::delete_)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& id) {
                switch (req.method())
                {
                    case boost::beast::http::verb::put:
                        telemetry::handleReportPut(req, asyncResp, id);
                        break;
                    case boost::beast::http::verb::patch:
                        telemetry::handleReportPatch(req, asyncResp, id);
                        break;
                    case boost::beast::http::verb::delete_:
                        telemetry::handleReportDelete(asyncResp, id);
                        break;
                    default:
                        BMCWEB_LOG_ERROR << "Unsupported method requested";
                        messages::internalError(asyncResp->res);
                        break;
                }
            });
}
} // namespace redfish
