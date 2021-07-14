#include "sensors.hpp"
#include "utils/telemetry_utils.hpp"
#include "utils/time_utils.hpp"

#include <app.hpp>
#include <boost/container/flat_map.hpp>
#include <registries/privilege_registry.hpp>

#include <tuple>
#include <variant>
#include <vector>

namespace redfish
{
namespace telemetry
{

using NumericThresholdParams =
    std::tuple<std::string, uint64_t, std::string, double>;

using DiscreteThresholdParams =
    std::tuple<std::string, std::string, uint64_t, std::string>;

using TriggerThresholdParams =
    std::variant<std::vector<NumericThresholdParams>,
                 std::vector<DiscreteThresholdParams>>;

struct AddTriggerArgs
{
    std::string name;
    bool isDiscrete;
    bool logToJournal;
    bool logToRedfish;
    bool updateReport;
    std::vector<std::pair<sdbusplus::message::object_path, std::string>>
        sensors;
    std::vector<std::string> reportNames;
    TriggerThresholdParams thresholds;
};

inline bool parseMetricType(crow::Response& res, std::string& metricType,
                            AddTriggerArgs& args)
{
    if (metricType == "Discrete")
    {
        args.isDiscrete = true;
    }
    else if (metricType == "Numeric")
    {
        args.isDiscrete = false;
    }
    else
    {
        messages::propertyValueIncorrect(res, "MetricType", metricType);
        return false;
    }
    return true;
}

inline bool parseTriggerActions(crow::Response& res,
                                std::vector<std::string>& actions,
                                AddTriggerArgs& args)
{
    for (auto& action : actions)
    {
        if (action == "RedfishEvent")
        {
            args.logToRedfish = true;
        }
        else if (action == "LogToLogService")
        {
            args.logToJournal = true;
        }
        else if (action == "RedfishMetricReport")
        {
            args.updateReport = true;
        }
        else
        {
            messages::propertyValueNotInList(res, action, "TriggerActionType");
            return false;
        }
    }
    return true;
}

inline bool parseNumericThresholds(crow::Response& res,
                                   nlohmann::json& numericThresholds,
                                   AddTriggerArgs& args)
{
    boost::container::flat_map<std::string, std::optional<nlohmann::json>>
        numericThresholdsMap = {
            {"UpperCritical", std::optional<nlohmann::json>()},
            {"LowerCritical", std::optional<nlohmann::json>()},
            {"UpperWarning", std::optional<nlohmann::json>()},
            {"LowerWarning", std::optional<nlohmann::json>()}};

    if (!json_util::readJson(
            numericThresholds, res, "UpperCritical",
            numericThresholdsMap["UpperCritical"], "LowerCritical",
            numericThresholdsMap["LowerCritical"], "UpperWarning",
            numericThresholdsMap["UpperWarning"], "LowerWarning",
            numericThresholdsMap["LowerWarning"]))
    {
        return false;
    }

    std::vector<NumericThresholdParams> parsedParams;

    for (auto& [thresholdType, thresholdInfo] : numericThresholdsMap)
    {
        if (thresholdInfo)
        {
            double reading;
            std::string activation;
            std::string dwellTimeStr;

            if (!json_util::readJson(*thresholdInfo, res, "Reading", reading,
                                     "Activation", activation, "DwellTime",
                                     dwellTimeStr))
            {
                return false;
            }

            std::optional<std::chrono::milliseconds> dwellTime =
                time_utils::fromDurationString(dwellTimeStr);
            if (!dwellTime)
            {
                messages::propertyValueIncorrect(res, "DwellTime",
                                                 dwellTimeStr);
                return false;
            }

            parsedParams.push_back({thresholdType,
                                    static_cast<uint64_t>(dwellTime->count()),
                                    activation, reading});
        }
    }

    args.thresholds = parsedParams;

    return true;
}

inline bool parseDiscreteTriggers(
    crow::Response& res, std::string& discreteTriggerCondition,
    std::optional<std::vector<nlohmann::json>>& discreteTriggers,
    AddTriggerArgs& args)
{
    std::vector<DiscreteThresholdParams> parsedParams;
    if (discreteTriggerCondition == "Specified")
    {
        for (size_t i = 0; i < discreteTriggers->size(); i++)
        {
            nlohmann::json& thresholdInfo = discreteTriggers->at(i);
            std::optional<std::string> name;
            std::string value;
            std::string dwellTimeStr;
            std::string severity;

            if (!json_util::readJson(thresholdInfo, res, "Name", name, "Value",
                                     value, "DwellTime", dwellTimeStr,
                                     "Severity", severity))
            {
                return false;
            }

            std::optional<std::chrono::milliseconds> dwellTime =
                time_utils::fromDurationString(dwellTimeStr);
            if (!dwellTime)
            {
                messages::propertyValueIncorrect(res, "DwellTime",
                                                 dwellTimeStr);
                return false;
            }

            if (!name)
            {
                name = "Discrete Trigger " + std::to_string(i);
            }

            parsedParams.push_back({*name, severity,
                                    static_cast<uint64_t>(dwellTime->count()),
                                    value});
        }
    }
    args.thresholds = parsedParams;
    return true;
}

inline bool parseTriggerThresholds(
    crow::Response& res, std::optional<std::string>& discreteTriggerCondition,
    std::optional<std::vector<nlohmann::json>>& discreteTriggers,
    std::optional<nlohmann::json>& numericThresholds, AddTriggerArgs& args)
{
    if (args.isDiscrete)
    {
        if (!discreteTriggerCondition)
        {
            messages::createFailedMissingReqProperties(
                res, "DiscreteTriggerCondition");
            return false;
        }
        if (numericThresholds)
        {
            messages::mutualExclusiveProperties(res, "NumericThresholds",
                                                "DiscreteTriggerCondition");
            return false;
        }
        if (*discreteTriggerCondition != "Specified" &&
            *discreteTriggerCondition != "Changed")
        {
            messages::propertyValueIncorrect(res, "DiscreteTriggerCondition",
                                             *discreteTriggerCondition);
        }
        if (*discreteTriggerCondition == "Specified")
        {
            if (!discreteTriggers)
            {
                messages::createFailedMissingReqProperties(res,
                                                           "DiscreteTriggers");
                return false;
            }

            if ((*discreteTriggers).empty())
            {
                messages::propertyValueIncorrect(
                    res, "DiscreteTriggers", nlohmann::json::array().dump());
                return false;
            }
        }
        if (!parseDiscreteTriggers(res, *discreteTriggerCondition,
                                   discreteTriggers, args))
        {
            return false;
        }
    }
    else
    {
        if (!numericThresholds)
        {
            messages::createFailedMissingReqProperties(res,
                                                       "NumericThresholds");
            return false;
        }
        if (discreteTriggers)
        {
            messages::mutualExclusiveProperties(res, "DiscreteTriggers",
                                                "NumericThresholds");
            return false;
        }
        if (discreteTriggerCondition)
        {
            messages::mutualExclusiveProperties(res, "DiscreteTriggerCondition",
                                                "NumericThresholds");
            return false;
        }
        if ((*numericThresholds).empty())
        {
            messages::propertyValueIncorrect(res, "NumericThresholds",
                                             (*numericThresholds).dump());
            return false;
        }
        if (!parseNumericThresholds(res, *numericThresholds, args))
        {
            return false;
        }
    }
    return true;
}

inline bool parseLinks(crow::Response& res,
                       std::optional<nlohmann::json>& links,
                       AddTriggerArgs& args)
{
    if (args.updateReport)
    {
        if (!links)
        {
            messages::createFailedMissingReqProperties(res, "Links");
            return false;
        }
        std::vector<std::string> metricReportDefinitions;
        if (!json_util::readJson(*links, res, "MetricReportDefinitions",
                                 metricReportDefinitions))
        {
            return false;
        }
        for (auto& reportDefinionUri : metricReportDefinitions)
        {
            std::optional<std::string> reportName =
                getReportNameFromReportDefinitionUri(reportDefinionUri);
            if (!reportName)
            {
                messages::propertyValueIncorrect(res, "MetricReportDefinitions",
                                                 reportDefinionUri);
            }
            args.reportNames.push_back(*reportName);
        }
    }
    return true;
}

inline bool parseMetricProperties(
    crow::Response& res, std::vector<std::string>& metricProperties,
    AddTriggerArgs& args,
    boost::container::flat_map<std::string, std::string>& uriToDbus)
{
    args.sensors.reserve(metricProperties.size());

    for (size_t i = 0; i < metricProperties.size(); i++)
    {
        const std::string& uri = metricProperties[i];
        auto el = uriToDbus.find(uri);
        if (el == uriToDbus.end())
        {
            BMCWEB_LOG_ERROR << "Failed to find DBus sensor "
                                "corresponding to URI "
                             << uri;
            messages::propertyValueNotInList(
                res, uri, "MetricProperties/" + std::to_string(i));
            return false;
        }

        const std::string& dbusPath = el->second;
        args.sensors.emplace_back(dbusPath, uri);
    }
    return true;
}

inline bool parsePostTriggerParams(crow::Response& res,
                                   const crow::Request& req,
                                   AddTriggerArgs& args,
                                   std::vector<std::string>& metricProperties)
{
    std::string metricType;
    std::vector<std::string> triggerActions;
    std::optional<std::string> discreteTriggerCondition;
    std::optional<std::vector<nlohmann::json>> discreteTriggers;
    std::optional<nlohmann::json> numericThresholds;
    std::optional<nlohmann::json> links;
    if (!json_util::readJson(
            req, res, "Id", args.name, "MetricType", metricType,
            "TriggerActions", triggerActions, "DiscreteTriggerCondition",
            discreteTriggerCondition, "DiscreteTriggers", discreteTriggers,
            "NumericThresholds", numericThresholds, "MetricProperties",
            metricProperties, "Links", links))
    {
        return false;
    }

    if (!parseMetricType(res, metricType, args))
    {
        return false;
    }

    if (!parseTriggerActions(res, triggerActions, args))
    {
        return false;
    }

    if (!parseTriggerThresholds(res, discreteTriggerCondition, discreteTriggers,
                                numericThresholds, args))
    {
        return false;
    }

    if (!parseLinks(res, links, args))
    {
        return false;
    }

    return true;
}

class AddTrigger
{
  public:
    AddTrigger(AddTriggerArgs argsIn,
               std::vector<std::string> metricPropertiesIn,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) :
        asyncResp(asyncResp),
        args{std::move(argsIn)}, metricProperties(metricPropertiesIn)
    {}
    ~AddTrigger()
    {
        if (asyncResp->res.result() != boost::beast::http::status::ok)
        {
            return;
        }

        if (!parseMetricProperties(asyncResp->res, metricProperties, args,
                                   uriToDbus))
        {
            return;
        }

        const std::shared_ptr<bmcweb::AsyncResp> aResp = asyncResp;
        crow::connections::systemBus->async_method_call(
            [aResp, name = args.name](const boost::system::error_code ec,
                                      const std::string&) {
                if (ec == boost::system::errc::file_exists)
                {
                    messages::resourceAlreadyExists(aResp->res, "Trigger", "Id",
                                                    name);
                    return;
                }
                if (ec == boost::system::errc::too_many_files_open)
                {
                    messages::createLimitReachedForResource(aResp->res);
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
            telemetry::service, "/xyz/openbmc_project/Telemetry/Triggers",
            "xyz.openbmc_project.Telemetry.TriggerManager", "AddTrigger",
            "TelemetryService/" + args.name, args.isDiscrete, args.logToJournal,
            args.logToRedfish, args.updateReport, args.sensors,
            args.reportNames, args.thresholds);
    }

    void insert(const boost::container::flat_map<std::string, std::string>& el)
    {
        uriToDbus.insert(el.begin(), el.end());
    }

  private:
    const std::shared_ptr<bmcweb::AsyncResp> asyncResp;
    AddTriggerArgs args;
    std::vector<std::string> metricProperties;
    boost::container::flat_map<std::string, std::string> uriToDbus{};
};

} // namespace telemetry

inline void requestRoutesTrigger(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/TelemetryService/Triggers/")
        .privileges(redfish::privileges::postTriggersCollection)
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                telemetry::AddTriggerArgs args;
                std::vector<std::string> metricProperties;
                if (!telemetry::parsePostTriggerParams(asyncResp->res, req,
                                                       args, metricProperties))
                {
                    return;
                }

                boost::container::flat_set<std::pair<std::string, std::string>>
                    chassisSensors;
                if (!telemetry::getChassisSensorNode(
                        asyncResp, metricProperties, chassisSensors))
                {
                    return;
                }

                auto addTriggerReq = std::make_shared<telemetry::AddTrigger>(
                    std::move(args), std::move(metricProperties), asyncResp);
                for (const auto& [chassis, sensorType] : chassisSensors)
                {
                    retrieveUriToDbusMap(
                        chassis, sensorType,
                        [asyncResp, addTriggerReq](
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
                            addTriggerReq->insert(uriToDbus);
                        });
                }
            });
}

} // namespace redfish