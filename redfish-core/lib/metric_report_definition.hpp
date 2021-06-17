#pragma once

#include "sensors.hpp"
#include "utils/telemetry_utils.hpp"
#include "utils/time_utils.hpp"

#include <app.hpp>
#include <boost/container/flat_map.hpp>
#include <dbus_utility.hpp>
#include <query.hpp>
#include <registries/privilege_registry.hpp>

#include <tuple>
#include <variant>

namespace redfish
{

namespace telemetry
{

constexpr const char* metricReportDefinitionUri =
    "/redfish/v1/TelemetryService/MetricReportDefinitions";

using ReadingParameters = std::vector<std::tuple<
    std::vector<std::tuple<sdbusplus::message::object_path, std::string>>,
    std::string, std::string, std::string, uint64_t>>;

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
}

struct AddReportArgs
{
    struct MetricArgs
    {
        std::string id;
        std::vector<std::string> uris;
        std::optional<std::string> collectionFunction;
        std::optional<std::string> collectionTimeScope;
        std::optional<uint64_t> collectionDuration;
    };

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
        AddReportArgs::MetricArgs metricArgs;
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
    const std::vector<AddReportArgs::MetricArgs>& metrics,
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
    AddReport(AddReportArgs argsIn,
              const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) :
        asyncResp(asyncResp),
        args{std::move(argsIn)}
    {}
    ~AddReport()
    {
        if (asyncResp->res.result() != boost::beast::http::status::ok)
        {
            return;
        }

        telemetry::ReadingParameters readingParams;
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
            [aResp, id = args.id.value_or(""),
             uriToDbus = std::move(uriToDbus)](
                const boost::system::error_code ec, const std::string&) {
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
                    aResp->res, metricProperties.dump(), "MetricProperties");
                return;
            }
            if (ec)
            {
                messages::internalError(aResp->res);
                BMCWEB_LOG_ERROR << "respHandler DBus error " << ec;
                return;
            }

            messages::created(aResp->res);
            },
            telemetry::service, "/xyz/openbmc_project/Telemetry/Reports",
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
    boost::container::flat_map<std::string, std::string> uriToDbus{};
};
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

        auto addReportReq =
            std::make_shared<telemetry::AddReport>(std::move(args), asyncResp);
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
            if (ec.value() == EBADR ||
                ec == boost::system::errc::host_unreachable)
            {
                messages::resourceNotFound(asyncResp->res,
                                           "MetricReportDefinition", id);
                return;
            }
            if (ec)
            {
                BMCWEB_LOG_ERROR << "respHandler DBus error " << ec;
                messages::internalError(asyncResp->res);
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
        .privileges(redfish::privileges::deleteMetricReportDefinitionCollection)
        .methods(boost::beast::http::verb::delete_)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& id)

            {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
        {
            return;
        }

        const std::string reportPath = telemetry::getDbusReportPath(id);

        crow::connections::systemBus->async_method_call(
            [asyncResp, id](const boost::system::error_code ec) {
            /*
             * boost::system::errc and std::errc are missing value
             * for EBADR error that is defined in Linux.
             */
            if (ec.value() == EBADR)
            {
                messages::resourceNotFound(asyncResp->res,
                                           "MetricReportDefinition", id);
                return;
            }

            if (ec)
            {
                BMCWEB_LOG_ERROR << "respHandler DBus error " << ec;
                messages::internalError(asyncResp->res);
                return;
            }

            asyncResp->res.result(boost::beast::http::status::no_content);
            },
            telemetry::service, reportPath, "xyz.openbmc_project.Object.Delete",
            "Delete");
        });
}
} // namespace redfish
