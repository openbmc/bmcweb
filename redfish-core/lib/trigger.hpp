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
namespace add_trigger
{

using NumericThresholdParams =
    std::tuple<std::string, uint64_t, std::string, double>;

using DiscreteThresholdParams =
    std::tuple<std::string, std::string, uint64_t, std::string>;

using TriggerThresholdParams =
    std::variant<std::vector<NumericThresholdParams>,
                 std::vector<DiscreteThresholdParams>>;

struct Context
{
    struct
    {
        std::string name;
        std::vector<std::string> actions;
        std::vector<std::pair<sdbusplus::message::object_path, std::string>>
            sensors;
        std::vector<std::string> reportNames;
        TriggerThresholdParams thresholds;
    } dbusArgs;

    struct
    {
        std::optional<bool> isDiscreteConditionSpecified;
        std::optional<bool> isMetricDiscrete;
        std::vector<std::string> metricProperties;
    } parsedInfo;
};

inline bool validateMetricType(crow::Response& res,
                               std::optional<std::string>& metricType,
                               Context& ctx)
{
    if (metricType)
    {
        if (*metricType == "Discrete")
        {
            ctx.parsedInfo.isMetricDiscrete = true;
        }
        else if (*metricType == "Numeric")
        {
            ctx.parsedInfo.isMetricDiscrete = false;
        }
        else
        {
            messages::propertyValueIncorrect(res, "MetricType", *metricType);
            return false;
        }
    }
    return true;
}

inline bool validateDiscreteTriggerCondition(
    crow::Response& res, std::optional<std::string>& discreteTriggerCondition,
    Context& ctx)
{
    if (discreteTriggerCondition)
    {
        if (*discreteTriggerCondition == "Specified")
        {
            ctx.parsedInfo.isDiscreteConditionSpecified = true;
        }
        else if (*discreteTriggerCondition == "Changed")
        {
            ctx.parsedInfo.isDiscreteConditionSpecified = false;
        }
        else
        {
            messages::propertyValueIncorrect(res, "DiscreteTriggerCondition",
                                             *discreteTriggerCondition);
            return false;
        }
    }
    return true;
}

inline bool parseNumericThresholds(crow::Response& res,
                                   nlohmann::json& numericThresholds,
                                   Context& ctx)
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
    parsedParams.reserve(numericThresholds.size());

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

            parsedParams.emplace_back(thresholdType,
                                      static_cast<uint64_t>(dwellTime->count()),
                                      activation, reading);
        }
    }

    ctx.dbusArgs.thresholds = parsedParams;

    return true;
}

inline bool parseDiscreteTriggers(
    crow::Response& res,
    std::optional<std::vector<nlohmann::json>>& discreteTriggers, Context& ctx)
{
    std::vector<DiscreteThresholdParams> parsedParams;
    if (discreteTriggers)
    {
        parsedParams.reserve(discreteTriggers->size());
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

            parsedParams.emplace_back(*name, severity,
                                      static_cast<uint64_t>(dwellTime->count()),
                                      value);
        }
    }
    ctx.dbusArgs.thresholds = parsedParams;
    return true;
}

inline bool parseTriggerThresholds(
    crow::Response& res,
    std::optional<std::vector<nlohmann::json>>& discreteTriggers,
    std::optional<nlohmann::json>& numericThresholds, Context& ctx)
{
    if (discreteTriggers && numericThresholds)
    {
        messages::mutualExclusiveProperties(res, "DiscreteTriggers",
                                            "NumericThresholds");
        return false;
    }

    if (ctx.parsedInfo.isDiscreteConditionSpecified)
    {
        if (numericThresholds)
        {
            messages::mutualExclusiveProperties(res, "DiscreteTriggerCondition",
                                                "NumericThresholds");
            return false;
        }
    }

    if (ctx.parsedInfo.isMetricDiscrete)
    {
        if (*ctx.parsedInfo.isMetricDiscrete && numericThresholds)
        {
            messages::propertyValueConflict(res, "NumericThresholds",
                                            "MetricType");
            return false;
        }
        if (!*ctx.parsedInfo.isMetricDiscrete && discreteTriggers)
        {
            messages::propertyValueConflict(res, "DiscreteTriggers",
                                            "MetricType");
            return false;
        }
        if (!*ctx.parsedInfo.isMetricDiscrete &&
            ctx.parsedInfo.isDiscreteConditionSpecified)
        {
            messages::propertyValueConflict(res, "DiscreteTriggers",
                                            "DiscreteTriggerCondition");
            return false;
        }
    }

    if (discreteTriggers || ctx.parsedInfo.isDiscreteConditionSpecified)
    {
        if (ctx.parsedInfo.isDiscreteConditionSpecified)
        {
            if (*ctx.parsedInfo.isDiscreteConditionSpecified &&
                !discreteTriggers)
            {
                messages::createFailedMissingReqProperties(res,
                                                           "DiscreteTriggers");
            }
            if (discreteTriggers &&
                (*ctx.parsedInfo.isDiscreteConditionSpecified ==
                 discreteTriggers->empty()))
            {
                messages::propertyValueConflict(res, "DiscreteTriggers",
                                                "DiscreteTriggerCondition");
                return false;
            }
        }
        if (!parseDiscreteTriggers(res, discreteTriggers, ctx))
        {
            return false;
        }
    }
    else if (numericThresholds)
    {
        if (ctx.parsedInfo.isDiscreteConditionSpecified)
        {
            messages::mutualExclusiveProperties(res, "DiscreteTriggerCondition",
                                                "NumericThresholds");
            return false;
        }
        if (!parseNumericThresholds(res, *numericThresholds, ctx))
        {
            return false;
        }
    }
    else
    {
        messages::createFailedMissingReqProperties(
            res, "DiscreteTriggers\", \"NumericThresholds\" or "
                 "\"DiscreteTriggerCondition");
        return false;
    }
    return true;
}

inline bool parseLinks(crow::Response& res,
                       std::optional<nlohmann::json>& links, Context& ctx)
{
    if (links)
    {
        std::optional<std::vector<std::string>> metricReportDefinitions;
        if (!json_util::readJson(*links, res, "MetricReportDefinitions",
                                 metricReportDefinitions))
        {
            return false;
        }

        if (metricReportDefinitions)
        {
            ctx.dbusArgs.reportNames.reserve(metricReportDefinitions->size());
            for (std::string& reportDefinionUri : *metricReportDefinitions)
            {
                std::optional<std::string> reportName =
                    getReportNameFromReportDefinitionUri(reportDefinionUri);
                if (!reportName)
                {
                    messages::propertyValueIncorrect(
                        res, "MetricReportDefinitions", reportDefinionUri);
                    return false;
                }
                ctx.dbusArgs.reportNames.push_back(*reportName);
            }
        }
    }
    return true;
}

inline bool parseMetricProperties(
    crow::Response& res, Context& ctx,
    boost::container::flat_map<std::string, std::string>& uriToDbus)
{
    ctx.dbusArgs.sensors.reserve(ctx.parsedInfo.metricProperties.size());

    for (size_t i = 0; i < ctx.parsedInfo.metricProperties.size(); i++)
    {
        const std::string& uri = ctx.parsedInfo.metricProperties[i];
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
        ctx.dbusArgs.sensors.emplace_back(dbusPath, uri);
    }
    return true;
}

inline bool parsePostTriggerParams(crow::Response& res,
                                   const crow::Request& req, Context& ctx)
{
    std::optional<std::string> metricType;
    std::optional<std::string> discreteTriggerCondition;
    std::optional<std::vector<nlohmann::json>> discreteTriggers;
    std::optional<nlohmann::json> numericThresholds;
    std::optional<nlohmann::json> links;
    if (!json_util::readJson(
            req, res, "Id", ctx.dbusArgs.name, "MetricType", metricType,
            "TriggerActions", ctx.dbusArgs.actions, "DiscreteTriggerCondition",
            discreteTriggerCondition, "DiscreteTriggers", discreteTriggers,
            "NumericThresholds", numericThresholds, "MetricProperties",
            ctx.parsedInfo.metricProperties, "Links", links))
    {
        return false;
    }

    if (!isIdValid(res, ctx.dbusArgs.name))
    {
        return false;
    }

    if (!validateMetricType(res, metricType, ctx))
    {
        return false;
    }

    if (!validateDiscreteTriggerCondition(res, discreteTriggerCondition, ctx))
    {
        return false;
    }

    std::transform(ctx.dbusArgs.actions.begin(), ctx.dbusArgs.actions.end(),
                   ctx.dbusArgs.actions.begin(), [](std::string& action) {
                       return redfishActionToDbusAction(action);
                   });

    if (!parseTriggerThresholds(res, discreteTriggers, numericThresholds, ctx))
    {
        return false;
    }

    if (!parseLinks(res, links, ctx))
    {
        return false;
    }

    return true;
}

class AddTrigger
{
  public:
    AddTrigger(Context ctxIn,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) :
        asyncResp(asyncResp),
        ctx{std::move(ctxIn)}
    {}
    ~AddTrigger()
    {
        if (asyncResp->res.result() != boost::beast::http::status::ok)
        {
            return;
        }

        if (!parseMetricProperties(asyncResp->res, ctx, uriToDbus))
        {
            return;
        }

        crow::connections::systemBus->async_method_call(
            [aResp = asyncResp, name = ctx.dbusArgs.name](
                const boost::system::error_code ec, const std::string&) {
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
            service, "/xyz/openbmc_project/Telemetry/Triggers",
            "xyz.openbmc_project.Telemetry.TriggerManager", "AddTrigger",
            "TelemetryService/" + ctx.dbusArgs.name, ctx.dbusArgs.actions,
            ctx.dbusArgs.sensors, ctx.dbusArgs.reportNames,
            ctx.dbusArgs.thresholds);
    }

    void insert(const boost::container::flat_map<std::string, std::string>& el)
    {
        uriToDbus.insert(el.begin(), el.end());
    }

  private:
    const std::shared_ptr<bmcweb::AsyncResp> asyncResp;
    Context ctx;
    boost::container::flat_map<std::string, std::string> uriToDbus{};
};

} // namespace add_trigger

} // namespace telemetry

inline void requestRoutesTriggersCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/TelemetryService/Triggers/")
        .privileges(redfish::privileges::postTriggersCollection)
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                telemetry::add_trigger::Context ctx;
                if (!telemetry::add_trigger::parsePostTriggerParams(
                        asyncResp->res, req, ctx))
                {
                    return;
                }

                boost::container::flat_set<std::pair<std::string, std::string>>
                    chassisSensors;
                if (!telemetry::getChassisSensorNode(
                        asyncResp, ctx.parsedInfo.metricProperties,
                        chassisSensors))
                {
                    return;
                }

                auto addTriggerReq =
                    std::make_shared<telemetry::add_trigger::AddTrigger>(
                        std::move(ctx), asyncResp);
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