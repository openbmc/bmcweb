#pragma once

#include "sensors.hpp"
#include "utils/telemetry_utils.hpp"
#include "utils/time_utils.hpp"

#include <app.hpp>
#include <boost/container/flat_map.hpp>
#include <registries/privilege_registry.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <tuple>
#include <variant>

namespace redfish
{

namespace telemetry
{

using ReadingParameters = std::vector<
    std::tuple<std::vector<sdbusplus::message::object_path>, std::string,
               std::string, std::string, std::string, uint64_t>>;

enum class MethodType
{
    get,
    post
};

inline std::string getOperationType(std::string_view item, MethodType method)
{
    static const std::vector<std::pair<std::string, std::string>> operation = {
        {"MAX", "Maximum"},
        {"MIN", "Minimum"},
        {"AVG", "Average"},
        {"SUM", "Summation"}};

    for (const auto& [getName, postName] : operation)
    {
        if (MethodType::get == method && getName == item)
        {
            return postName;
        }
        if (MethodType::post == method && postName == item)
        {
            return getName;
        }
    }
    return "";
}

inline bool isTimeScopeValid(std::string_view item)
{
    static const std::vector<std::string> scope = {"Point", "Interval",
                                                   "StartupInterval"};
    return std::find(scope.begin(), scope.end(), item) != scope.end();
}

inline std::vector<std::string> getReportActions(bool emitsUpdate,
                                                 bool logToMetricReports)
{
    std::vector<std::string> redfishReportActions;
    redfishReportActions.reserve(2);
    if (emitsUpdate)
    {
        redfishReportActions.emplace_back("RedfishEvent");
    }
    if (logToMetricReports)
    {
        redfishReportActions.emplace_back("LogToMetricReportsCollection");
    }

    return redfishReportActions;
}

inline std::optional<nlohmann::json>
    getMetrics(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const ReadingParameters& params)
{
    nlohmann::json metrics = nlohmann::json::array();
    for (auto& [sensorPath, operationType, id, metadata, collectionTimeScope,
                collectionDuration] : params)
    {
        std::vector<std::string> metricProperties;

        nlohmann::json parsedMetadata = nlohmann::json::parse(metadata);
        if (!json_util::readJson(parsedMetadata, asyncResp->res,
                                 "MetricProperties", metricProperties))
        {
            BMCWEB_LOG_ERROR << "Failed to read metadata";
            messages::internalError(asyncResp->res);
            return std::nullopt;
        }

        metrics.push_back({
            {"MetricId", id},
            {"MetricProperties", std::move(metricProperties)},
            {"CollectionFunction",
             getOperationType(operationType, MethodType::get)},
            {"CollectionTimeScope", collectionTimeScope},
            {"CollectionDuration",
             time_utils::toDurationString(
                 std::chrono::milliseconds(collectionDuration))},
        });
    }

    return metrics;
}

inline void fillReportDefinition(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, const std::string& id,
    const std::vector<
        std::pair<std::string, std::variant<std::monostate, std::string, bool,
                                            uint64_t, ReadingParameters>>>&
        properties)
{
    try
    {
        bool emitsReadingsUpdate = false;
        bool logToMetricReportsCollection = false;
        ReadingParameters readingParams;
        std::string reportingType;
        uint64_t interval = 0u;

        sdbusplus::unpackProperties(
            properties, "EmitsReadingsUpdate", emitsReadingsUpdate,
            "LogToMetricReportsCollection", logToMetricReportsCollection,
            "ReadingParametersFutureVersion", readingParams, "ReportingType",
            reportingType, "Interval", interval);

        if (std::optional<nlohmann::json> metrics =
                getMetrics(asyncResp, readingParams))
        {
            asyncResp->res.jsonValue["@odata.type"] =
                "#MetricReportDefinition.v1_4_1.MetricReportDefinition";
            asyncResp->res.jsonValue["@odata.id"] =
                metricReportDefinitionUri + id;
            asyncResp->res.jsonValue["Id"] = id;
            asyncResp->res.jsonValue["Name"] = id;
            asyncResp->res.jsonValue["MetricReport"]["@odata.id"] =
                metricReportUri + id;
            asyncResp->res.jsonValue["Status"]["State"] = "Enabled";
            asyncResp->res.jsonValue["ReportUpdates"] = "Overwrite";
            asyncResp->res.jsonValue["Metrics"] = *metrics;
            asyncResp->res.jsonValue["MetricReportDefinitionType"] =
                reportingType;
            asyncResp->res.jsonValue["ReportActions"] = getReportActions(
                emitsReadingsUpdate, logToMetricReportsCollection);
            asyncResp->res.jsonValue["Schedule"]["RecurrenceInterval"] =
                time_utils::toDurationString(
                    std::chrono::milliseconds(interval));
        }
    }
    catch (const sdbusplus::exception::UnpackPropertyError& error)
    {
        BMCWEB_LOG_ERROR << error.what() << ", property: "
                         << error.propertyName + ", reason: " << error.reason;
        messages::internalError(asyncResp->res);
    }
    catch (const nlohmann::json::parse_error& e)
    {
        BMCWEB_LOG_ERROR << "Failed to parse metadata: " << e.what();
        messages::internalError(asyncResp->res);
    }
}

struct AddReportArgs
{
    std::string name;
    std::string reportingType;
    bool emitsReadingsUpdate = false;
    bool logToMetricReportsCollection = false;
    uint64_t interval = 0;
    std::vector<std::tuple<std::string, std::vector<std::string>, std::string,
                           std::string, uint64_t>>
        metrics;
};

inline bool isIdValid(crow::Response& res, const std::string& id)
{
    constexpr const char* allowedCharactersInName =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
    if (id.empty() ||
        id.find_first_not_of(allowedCharactersInName) != std::string::npos)
    {
        BMCWEB_LOG_ERROR << "Failed to match " << id
                         << " with allowed character "
                         << allowedCharactersInName;
        messages::propertyValueIncorrect(res, "Id", id);
        return false;
    }

    return true;
}

inline bool toDbusReportActions(crow::Response& res,
                                const std::vector<std::string>& actions,
                                AddReportArgs& args)
{
    size_t index = 0;
    for (const auto& action : actions)
    {
        if (action == "RedfishEvent")
        {
            args.emitsReadingsUpdate = true;
        }
        else if (action == "LogToMetricReportsCollection")
        {
            args.logToMetricReportsCollection = true;
        }
        else
        {
            messages::propertyValueNotInList(
                res, action, "ReportActions/" + std::to_string(index));
            return false;
        }
        index++;
    }
    return true;
}

inline bool toDbusReportingType(crow::Response& res,
                                std::optional<nlohmann::json>& schedule,
                                AddReportArgs& args)
{
    if (args.reportingType != "Periodic" && args.reportingType != "OnRequest")
    {
        messages::propertyValueNotInList(res, args.reportingType,
                                         "MetricReportDefinitionType");
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

    return true;
}
inline bool toDbusMetrics(crow::Response& res,
                          std::vector<nlohmann::json>& metrics,
                          AddReportArgs& args)
{
    uint16_t itemNb = 0;
    args.metrics.reserve(metrics.size());
    for (auto& m : metrics)
    {
        std::string id;
        std::vector<std::string> uris;
        std::optional<std::string> function;
        std::optional<std::string> scope;
        std::optional<std::string> durationStr;
        if (!json_util::readJson(m, res, "MetricId", id, "MetricProperties",
                                 uris, "CollectionFunction", function,
                                 "CollectionTimeScope", scope,
                                 "CollectionDuration", durationStr))
        {
            return false;
        }

        function = function.has_value() ? function : "Average";
        scope = scope.has_value() ? scope : "Point";
        durationStr = durationStr.has_value() ? durationStr : "PT0S";

        std::string operation = getOperationType(*function, MethodType::post);
        if (operation.empty())
        {
            messages::propertyValueNotInList(
                res, *function, "CollectionFunction/" + std::to_string(itemNb));
            return false;
        }
        if (!isTimeScopeValid(*scope))
        {
            messages::propertyValueNotInList(
                res, *scope, "CollectionTimeScope/" + std::to_string(itemNb));
            return false;
        }
        std::optional<std::chrono::milliseconds> durationNum =
            time_utils::fromDurationString(*durationStr);
        if (!durationNum)
        {
            messages::propertyValueIncorrect(res, "CollectionDuration",
                                             *durationStr);
            return false;
        }
        args.metrics.emplace_back(std::move(id), std::move(uris),
                                  std::move(operation), std::move(*scope),
                                  static_cast<uint64_t>(durationNum->count()));
        itemNb++;
    }

    return true;
}

inline bool getUserParameters(crow::Response& res, const crow::Request& req,
                              AddReportArgs& args)
{
    std::vector<nlohmann::json> metrics;
    std::vector<std::string> reportActions;
    std::optional<nlohmann::json> schedule;
    if (!json_util::readJson(req, res, "Id", args.name, "Metrics", metrics,
                             "MetricReportDefinitionType", args.reportingType,
                             "ReportActions", reportActions, "Schedule",
                             schedule) ||
        !isIdValid(res, args.name) ||
        !toDbusReportActions(res, reportActions, args) ||
        !toDbusReportingType(res, schedule, args) ||
        !toDbusMetrics(res, metrics, args))
    {
        return false;
    }

    return true;
}

inline bool getChassisSensorNode(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::vector<std::tuple<std::string, std::vector<std::string>,
                                 std::string, std::string, uint64_t>>& metrics,
    boost::container::flat_set<std::pair<std::string, std::string>>& matched)
{
    for (const auto& [id, uris, operation, scope, duration] : metrics)
    {
        for (size_t i = 0; i < uris.size(); i++)
        {
            const std::string& uri = uris[i];
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

        ReadingParameters readingParams;
        readingParams.reserve(args.metrics.size());

        for (const auto& [id, uris, operation, scope, duration] : args.metrics)
        {
            std::vector<sdbusplus::message::object_path> dbusPaths;
            dbusPaths.reserve(uris.size());
            nlohmann::json metadata;
            metadata["MetricProperties"] = nlohmann::json::array();

            for (size_t i = 0; i < uris.size(); i++)
            {
                const std::string& uri = uris[i];
                auto el = uriToDbus.find(uri);
                if (el == uriToDbus.end())
                {
                    BMCWEB_LOG_ERROR << "Failed to find DBus sensor "
                                        "corresponding to URI "
                                     << uri;
                    messages::propertyValueNotInList(asyncResp->res, uri,
                                                     "MetricProperties/" +
                                                         std::to_string(i));
                    return;
                }

                const std::string& dbusPath = el->second;
                dbusPaths.emplace_back(dbusPath);
                metadata["MetricProperties"].emplace_back(uri);
            }

            readingParams.emplace_back(dbusPaths, operation, id,
                                       metadata.dump(), scope, duration);
        }
        const std::shared_ptr<bmcweb::AsyncResp> aResp = asyncResp;
        crow::connections::systemBus->async_method_call(
            [aResp, name = args.name, uriToDbus = std::move(uriToDbus)](
                const boost::system::error_code ec, const std::string&) {
                if (ec == boost::system::errc::file_exists)
                {
                    messages::resourceAlreadyExists(
                        aResp->res, "MetricReportDefinition", "Id", name);
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

                messages::created(aResp->res);
            },
            service, "/xyz/openbmc_project/Telemetry/Reports",
            "xyz.openbmc_project.Telemetry.ReportManager",
            "AddReportFutureVersion", "TelemetryService/" + args.name,
            args.reportingType, args.emitsReadingsUpdate,
            args.logToMetricReportsCollection, args.interval, readingParams);
    }

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
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#MetricReportDefinitionCollection."
                    "MetricReportDefinitionCollection";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/TelemetryService/MetricReportDefinitions";
                asyncResp->res.jsonValue["Name"] =
                    "Metric Definition Collection";

                telemetry::getReportCollection(
                    asyncResp, telemetry::metricReportDefinitionUri);
            });

    BMCWEB_ROUTE(app, "/redfish/v1/TelemetryService/MetricReportDefinitions/")
        .privileges(redfish::privileges::postMetricReportDefinitionCollection)
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request& req,
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
                    std::move(args), asyncResp);
                for (const auto& [chassis, sensorType] : chassisSensors)
                {
                    retrieveUriToDbusMap(
                        asyncResp, chassis, sensorType,
                        [asyncResp, addReportReq](
                            const boost::beast::http::status status,
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
            });
}

inline void requestRoutesMetricReportDefinition(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/TelemetryService/MetricReportDefinitions/<str>/")
        .privileges(redfish::privileges::getMetricReportDefinition)
        .methods(
            boost::beast::http::verb::get)([](const crow::Request&,
                                              const std::shared_ptr<
                                                  bmcweb::AsyncResp>& asyncResp,
                                              const std::string& id) {
            sdbusplus::asio::getAllProperties(
                *crow::connections::systemBus, telemetry::service,
                telemetry::getDbusReportPath(id), telemetry::reportInterface,
                [asyncResp,
                 id](boost::system::error_code ec,
                     const std::vector<std::pair<
                         std::string,
                         std::variant<std::monostate, std::string, bool,
                                      uint64_t, telemetry::ReadingParameters>>>&
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

                    telemetry::fillReportDefinition(asyncResp, id, properties);
                });
        });

    BMCWEB_ROUTE(app,
                 "/redfish/v1/TelemetryService/MetricReportDefinitions/<str>/")
        .privileges(redfish::privileges::deleteMetricReportDefinitionCollection)
        .methods(boost::beast::http::verb::delete_)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& id)

            {
                const std::string reportPath = telemetry::getDbusReportPath(id);

                crow::connections::systemBus->async_method_call(
                    [asyncResp, id](const boost::system::error_code ec) {
                        /*
                         * boost::system::errc and std::errc are missing value
                         * for EBADR error that is defined in Linux.
                         */
                        if (ec.value() == EBADR)
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

                        asyncResp->res.result(
                            boost::beast::http::status::no_content);
                    },
                    telemetry::service, reportPath,
                    "xyz.openbmc_project.Object.Delete", "Delete");
            });
}
} // namespace redfish
