#include <variant>
#include <sdbusplus/message.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include "../../include/dbus_singleton.hpp"
#include "../../include/dbus_utility.hpp"
#include "sensors.hpp"
#include "../../http/logging.hpp"
#include <app_class_decl.hpp>
#include "utils/telemetry_utils.hpp"
#include "utils/time_utils.hpp"

#include "power.hpp"

using crow::App;

namespace redfish
{

// Forward declaration of internalError
// Found in error_messages.hpp
namespace messages
{
    void internalError(crow::Response& res);
    void resourceNotFound(crow::Response& res, const std::string& arg1,
                      const std::string& arg2);
}

using GetSubTreeType = std::vector<
    std::pair<std::string,
              std::vector<std::pair<std::string, std::vector<std::string>>>>>;

using SensorVariant =
    std::variant<int64_t, double, uint32_t, bool, std::string>;

using ManagedObjectsVectorType = std::vector<std::pair<
    sdbusplus::message::object_path,
    boost::container::flat_map<
        std::string, boost::container::flat_map<std::string, SensorVariant>>>>;

SensorsAsyncResp::SensorsAsyncResp(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                    const std::string& chassisIdIn,
                    const std::vector<const char*>& typesIn,
                    const std::string_view& subNode) :
    asyncResp(asyncResp),
    chassisId(chassisIdIn), types(typesIn), chassisSubNode(subNode)
{}

SensorsAsyncResp::SensorsAsyncResp(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                    const std::string& chassisIdIn,
                    const std::vector<const char*>& typesIn,
                    const std::string_view& subNode,
                    DataCompleteCb&& creationComplete) :
    asyncResp(asyncResp),
    chassisId(chassisIdIn), types(typesIn),
    chassisSubNode(subNode), metadata{std::vector<SensorData>()},
    dataComplete{std::move(creationComplete)}
{}

SensorsAsyncResp::~SensorsAsyncResp()
{
    if (asyncResp->res.result() ==
        boost::beast::http::status::internal_server_error)
    {
        // Reset the json object to clear out any data that made it in
        // before the error happened todo(ed) handle error condition with
        // proper code
        asyncResp->res.jsonValue = nlohmann::json::object();
    }

    if (dataComplete && metadata)
    {
        boost::container::flat_map<std::string, std::string> map;
        if (asyncResp->res.result() == boost::beast::http::status::ok)
        {
            for (auto& sensor : *metadata)
            {
                map.insert(std::make_pair(sensor.uri + sensor.valueKey,
                                            sensor.dbusPath));
            }
        }
        dataComplete(asyncResp->res.result(), map);
    }
}

void SensorsAsyncResp::addMetadata(const nlohmann::json& sensorObject,
                    const std::string& valueKey, const std::string& dbusPath)
{
    if (metadata)
    {
        metadata->emplace_back(SensorData{sensorObject["Name"],
                                            sensorObject["@odata.id"],
                                            valueKey, dbusPath});
    }
}

void SensorsAsyncResp::updateUri(const std::string& name, const std::string& uri)
{
    if (metadata)
    {
        for (auto& sensor : *metadata)
        {
            if (sensor.name == name)
            {
                sensor.uri = uri;
            }
        }
    }
}

void requestRoutesSensorCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Sensors/")
        .privileges(redfish::privileges::getSensorCollection)
        .methods(
            boost::beast::http::verb::get)([](const crow::Request&,
                                              const std::shared_ptr<
                                                  bmcweb::AsyncResp>& aResp,
                                              const std::string& chassisId) {
            BMCWEB_LOG_DEBUG << "SensorCollection doGet enter";

            std::shared_ptr<SensorsAsyncResp> asyncResp =
                std::make_shared<SensorsAsyncResp>(
                    aResp, chassisId,
                    sensors::dbus::paths.at(sensors::node::sensors),
                    sensors::node::sensors);

            auto getChassisCb =
                [asyncResp](
                    const std::shared_ptr<
                        boost::container::flat_set<std::string>>& sensorNames) {
                    BMCWEB_LOG_DEBUG << "getChassisCb enter";

                    nlohmann::json& entriesArray =
                        asyncResp->asyncResp->res.jsonValue["Members"];
                    for (auto& sensor : *sensorNames)
                    {
                        BMCWEB_LOG_DEBUG << "Adding sensor: " << sensor;

                        sdbusplus::message::object_path path(sensor);
                        std::string sensorName = path.filename();
                        if (sensorName.empty())
                        {
                            BMCWEB_LOG_ERROR << "Invalid sensor path: "
                                             << sensor;
                            messages::internalError(asyncResp->asyncResp->res);
                            return;
                        }
                        entriesArray.push_back(
                            {{"@odata.id", "/redfish/v1/Chassis/" +
                                               asyncResp->chassisId + "/" +
                                               asyncResp->chassisSubNode + "/" +
                                               sensorName}});
                    }

                    asyncResp->asyncResp->res.jsonValue["Members@odata.count"] =
                        entriesArray.size();
                    BMCWEB_LOG_DEBUG << "getChassisCb exit";
                };

            // Get set of sensors in chassis
            getChassis(asyncResp, std::move(getChassisCb));
            BMCWEB_LOG_DEBUG << "SensorCollection doGet exit";
        });
}

void requestRoutesSensor(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Sensors/<str>/")
        .privileges(redfish::privileges::getSensor)
        .methods(
            boost::beast::http::verb::get)([](const crow::Request&,
                                              const std::shared_ptr<
                                                  bmcweb::AsyncResp>& aResp,
                                              const std::string& chassisId,
                                              const std::string& sensorName) {
            BMCWEB_LOG_DEBUG << "Sensor doGet enter";
            std::shared_ptr<SensorsAsyncResp> asyncResp =
                std::make_shared<SensorsAsyncResp>(aResp, chassisId,
                                                   std::vector<const char*>(),
                                                   sensors::node::sensors);

            const std::array<const char*, 1> interfaces = {
                "xyz.openbmc_project.Sensor.Value"};

            // Get a list of all of the sensors that implement Sensor.Value
            // and get the path and service name associated with the sensor
            crow::connections::systemBus->async_method_call(
                [asyncResp, sensorName](const boost::system::error_code ec,
                                        const GetSubTreeType& subtree) {
                    BMCWEB_LOG_DEBUG << "respHandler1 enter";
                    if (ec)
                    {
                        messages::internalError(asyncResp->asyncResp->res);
                        BMCWEB_LOG_ERROR
                            << "Sensor getSensorPaths resp_handler: "
                            << "Dbus error " << ec;
                        return;
                    }

                    GetSubTreeType::const_iterator it = std::find_if(
                        subtree.begin(), subtree.end(),
                        [sensorName](
                            const std::pair<
                                std::string,
                                std::vector<std::pair<
                                    std::string, std::vector<std::string>>>>&
                                object) {
                            sdbusplus::message::object_path path(object.first);
                            std::string name = path.filename();
                            if (name.empty())
                            {
                                BMCWEB_LOG_ERROR << "Invalid sensor path: "
                                                 << object.first;
                                return false;
                            }

                            return name == sensorName;
                        });

                    if (it == subtree.end())
                    {
                        BMCWEB_LOG_ERROR << "Could not find path for sensor: "
                                         << sensorName;
                        messages::resourceNotFound(asyncResp->asyncResp->res,
                                                   "Sensor", sensorName);
                        return;
                    }
                    std::string_view sensorPath = (*it).first;
                    BMCWEB_LOG_DEBUG << "Found sensor path for sensor '"
                                     << sensorName << "': " << sensorPath;

                    const std::shared_ptr<
                        boost::container::flat_set<std::string>>
                        sensorList = std::make_shared<
                            boost::container::flat_set<std::string>>();

                    sensorList->emplace(sensorPath);
                    processSensorList(asyncResp, sensorList);
                    BMCWEB_LOG_DEBUG << "respHandler1 exit";
                },
                "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/object_mapper",
                "xyz.openbmc_project.ObjectMapper", "GetSubTree",
                "/xyz/openbmc_project/sensors", 2, interfaces);
        });
}

void retrieveUriToDbusMap(const std::string& chassis,
                                 const std::string& node,
                                 SensorsAsyncResp::DataCompleteCb&& mapComplete)
{
    auto pathIt = sensors::dbus::paths.find(node);
    if (pathIt == sensors::dbus::paths.end())
    {
        BMCWEB_LOG_ERROR << "Wrong node provided : " << node;
        mapComplete(boost::beast::http::status::bad_request, {});
        return;
    }

    auto res = std::make_shared<crow::Response>();
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>(*res);
    auto callback =
        [res, asyncResp, mapCompleteCb{std::move(mapComplete)}](
            const boost::beast::http::status status,
            const boost::container::flat_map<std::string, std::string>&
                uriToDbus) { mapCompleteCb(status, uriToDbus); };

    auto resp = std::make_shared<SensorsAsyncResp>(
        asyncResp, chassis, pathIt->second, node, std::move(callback));
    getChassisData(resp);
}

namespace telemetry
{
struct AddReportArgs
{
    std::string name;
    std::string reportingType;
    bool emitsReadingsUpdate = false;
    bool logToMetricReportsCollection = false;
    uint64_t interval = 0;
    std::vector<std::pair<std::string, std::vector<std::string>>> metrics;
};


class AddReport
{
  public:
    AddReport(AddReportArgs argsIn,
              const std::shared_ptr<bmcweb::AsyncResp>& asyncResp);
    ~AddReport();
    void insert(const boost::container::flat_map<std::string, std::string>& el);

  private:
    const std::shared_ptr<bmcweb::AsyncResp> asyncResp;
    AddReportArgs args;
    boost::container::flat_map<std::string, std::string> uriToDbus{};
};
}

inline bool toDbusReportActions(crow::Response& res,
                                std::vector<std::string>& actions,
                                telemetry::AddReportArgs& args)
{
    size_t index = 0;
    for (auto& action : actions)
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

inline bool getUserParameters(crow::Response& res, const crow::Request& req,
                              telemetry::AddReportArgs& args)
{
    std::vector<nlohmann::json> metrics;
    std::vector<std::string> reportActions;
    std::optional<nlohmann::json> schedule;
    if (!json_util::readJson(req, res, "Id", args.name, "Metrics", metrics,
                             "MetricReportDefinitionType", args.reportingType,
                             "ReportActions", reportActions, "Schedule",
                             schedule))
    {
        return false;
    }

    constexpr const char* allowedCharactersInName =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
    if (args.name.empty() || args.name.find_first_not_of(
                                 allowedCharactersInName) != std::string::npos)
    {
        BMCWEB_LOG_ERROR << "Failed to match " << args.name
                         << " with allowed character "
                         << allowedCharactersInName;
        messages::propertyValueIncorrect(res, "Id", args.name);
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
        std::string id;
        std::vector<std::string> uris;
        if (!json_util::readJson(m, res, "MetricId", id, "MetricProperties",
                                 uris))
        {
            return false;
        }

        args.metrics.emplace_back(std::move(id), std::move(uris));
    }

    return true;
}

inline bool getChassisSensorNode(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::vector<std::pair<std::string, std::vector<std::string>>>&
        metrics,
    boost::container::flat_set<std::pair<std::string, std::string>>& matched)
{
    for (const auto& [id, uris] : metrics)
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

void requestRoutesMetricReportDefinitionCollection(App& app)
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
                if (!getUserParameters(asyncResp->res, req, args))
                {
                    return;
                }

                boost::container::flat_set<std::pair<std::string, std::string>>
                    chassisSensors;
                if (!getChassisSensorNode(asyncResp, args.metrics,
                                                     chassisSensors))
                {
                    return;
                }

                auto addReportReq = std::make_shared<telemetry::AddReport>(
                    std::move(args), asyncResp);
                for (const auto& [chassis, sensorType] : chassisSensors)
                {
                    retrieveUriToDbusMap(
                        chassis, sensorType,
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

}