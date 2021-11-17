#pragma once

#include "sensors.hpp"
#include "utils/finalizer.hpp"
#include "utils/telemetry_utils.hpp"
#include "utils/time_utils.hpp"

#include <app.hpp>
#include <registries/privilege_registry.hpp>

#include <tuple>
#include <variant>
#include <vector>

namespace redfish
{
namespace telemetry
{

static constexpr std::array<std::string_view, 4>
    supportedNumericThresholdNames = {"UpperCritical", "LowerCritical",
                                      "UpperWarning", "LowerWarning"};

using NumericThresholdParams =
    std::tuple<std::string, uint64_t, std::string, double>;

using DiscreteThresholdParams =
    std::tuple<std::string, std::string, uint64_t, std::string>;

using TriggerThresholdParams =
    std::variant<std::vector<NumericThresholdParams>,
                 std::vector<DiscreteThresholdParams>>;

using TriggerThresholdParamsExt =
    std::variant<std::monostate, std::vector<NumericThresholdParams>,
                 std::vector<DiscreteThresholdParams>>;

using TriggerSensorsParams =
    std::vector<std::pair<sdbusplus::message::object_path, std::string>>;

using TriggerGetParamsVariant =
    std::variant<std::monostate, bool, std::string, TriggerThresholdParamsExt,
                 TriggerSensorsParams, std::vector<std::string>>;

using TriggerGetParamsMap =
    std::unordered_map<std::string, TriggerGetParamsVariant>;

namespace add_trigger
{

enum class MetricType
{
    Discrete,
    Numeric
};

enum class DiscreteCondition
{
    Specified,
    Changed
};

struct Context
{
    struct
    {
        std::string id;
        std::string name;
        std::vector<std::string> actions;
        std::vector<std::pair<sdbusplus::message::object_path, std::string>>
            sensors;
        std::vector<std::string> reportNames;
        TriggerThresholdParams thresholds;
    } dbusArgs;

    struct
    {
        std::optional<DiscreteCondition> discreteCondition;
        std::optional<MetricType> metricType;
        std::optional<std::vector<std::string>> metricProperties;
    } parsedInfo;

    boost::container::flat_map<std::string, std::string> uriToDbusMerged{};
};

inline std::optional<MetricType> getMetricType(const std::string& metricType)
{
    if (metricType == "Discrete")
    {
        return MetricType::Discrete;
    }
    if (metricType == "Numeric")
    {
        return MetricType::Numeric;
    }
    return std::nullopt;
}

inline std::optional<DiscreteCondition>
    getDiscreteCondition(const std::string& discreteTriggerCondition)
{
    if (discreteTriggerCondition == "Specified")
    {
        return DiscreteCondition::Specified;
    }
    if (discreteTriggerCondition == "Changed")
    {
        return DiscreteCondition::Changed;
    }
    return std::nullopt;
}

inline bool parseNumericThresholds(crow::Response& res,
                                   nlohmann::json& numericThresholds,
                                   Context& ctx)
{
    if (!numericThresholds.is_object())
    {
        messages::propertyValueTypeError(res, numericThresholds.dump(),
                                         "NumericThresholds");
        return false;
    }

    std::vector<NumericThresholdParams> parsedParams;
    parsedParams.reserve(numericThresholds.size());

    for (auto& [thresholdName, thresholdData] : numericThresholds.items())
    {
        if (std::find(supportedNumericThresholdNames.begin(),
                      supportedNumericThresholdNames.end(),
                      thresholdName) == supportedNumericThresholdNames.end())
        {
            messages::propertyUnknown(res, thresholdName);
            return false;
        }

        double reading = .0;
        std::string activation;
        std::string dwellTimeStr;

        if (!json_util::readJson(thresholdData, res, "Reading", reading,
                                 "Activation", activation, "DwellTime",
                                 dwellTimeStr))
        {
            return false;
        }

        std::optional<std::chrono::milliseconds> dwellTime =
            time_utils::fromDurationString(dwellTimeStr);
        if (!dwellTime)
        {
            messages::propertyValueIncorrect(res, "DwellTime", dwellTimeStr);
            return false;
        }

        parsedParams.emplace_back(thresholdName,
                                  static_cast<uint64_t>(dwellTime->count()),
                                  activation, reading);
    }

    ctx.dbusArgs.thresholds = std::move(parsedParams);
    return true;
}

inline bool parseDiscreteTriggers(
    crow::Response& res,
    std::optional<std::vector<nlohmann::json>>& discreteTriggers, Context& ctx)
{
    std::vector<DiscreteThresholdParams> parsedParams;
    if (!discreteTriggers)
    {
        ctx.dbusArgs.thresholds = std::move(parsedParams);
        return true;
    }

    parsedParams.reserve(discreteTriggers->size());
    for (nlohmann::json& thresholdInfo : *discreteTriggers)
    {
        std::optional<std::string> name;
        std::string value;
        std::string dwellTimeStr;
        std::string severity;

        if (!json_util::readJson(thresholdInfo, res, "Name", name, "Value",
                                 value, "DwellTime", dwellTimeStr, "Severity",
                                 severity))
        {
            return false;
        }

        std::optional<std::chrono::milliseconds> dwellTime =
            time_utils::fromDurationString(dwellTimeStr);
        if (!dwellTime)
        {
            messages::propertyValueIncorrect(res, "DwellTime", dwellTimeStr);
            return false;
        }

        if (!name)
        {
            name = "";
        }

        parsedParams.emplace_back(
            *name, severity, static_cast<uint64_t>(dwellTime->count()), value);
    }

    ctx.dbusArgs.thresholds = std::move(parsedParams);
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

    if (ctx.parsedInfo.discreteCondition)
    {
        if (numericThresholds)
        {
            messages::mutualExclusiveProperties(res, "DiscreteTriggerCondition",
                                                "NumericThresholds");
            return false;
        }
    }

    if (ctx.parsedInfo.metricType)
    {
        if (*ctx.parsedInfo.metricType == MetricType::Discrete &&
            numericThresholds)
        {
            messages::propertyValueConflict(res, "NumericThresholds",
                                            "MetricType");
            return false;
        }
        if (*ctx.parsedInfo.metricType == MetricType::Numeric &&
            discreteTriggers)
        {
            messages::propertyValueConflict(res, "DiscreteTriggers",
                                            "MetricType");
            return false;
        }
        if (*ctx.parsedInfo.metricType == MetricType::Numeric &&
            ctx.parsedInfo.discreteCondition)
        {
            messages::propertyValueConflict(res, "DiscreteTriggers",
                                            "DiscreteTriggerCondition");
            return false;
        }
    }

    if (discreteTriggers || ctx.parsedInfo.discreteCondition ||
        (ctx.parsedInfo.metricType &&
         *ctx.parsedInfo.metricType == MetricType::Discrete))
    {
        if (ctx.parsedInfo.discreteCondition)
        {
            if (*ctx.parsedInfo.discreteCondition ==
                    DiscreteCondition::Specified &&
                !discreteTriggers)
            {
                messages::createFailedMissingReqProperties(res,
                                                           "DiscreteTriggers");
                return false;
            }
            if (discreteTriggers && ((*ctx.parsedInfo.discreteCondition ==
                                          DiscreteCondition::Specified &&
                                      discreteTriggers->empty()) ||
                                     (*ctx.parsedInfo.discreteCondition ==
                                          DiscreteCondition::Changed &&
                                      !discreteTriggers->empty())))
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
        if (!parseNumericThresholds(res, *numericThresholds, ctx))
        {
            return false;
        }
    }
    else
    {
        messages::createFailedMissingReqProperties(
            res, "'DiscreteTriggers', 'NumericThresholds', "
                 "'DiscreteTriggerCondition' or 'MetricType'");
        return false;
    }
    return true;
}

inline bool parseLinks(crow::Response& res, nlohmann::json& links, Context& ctx)
{
    if (links.empty())
    {
        return true;
    }

    std::optional<std::vector<std::string>> metricReportDefinitions;
    if (!json_util::readJson(links, res, "MetricReportDefinitions",
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
                messages::propertyValueIncorrect(res, "MetricReportDefinitions",
                                                 reportDefinionUri);
                return false;
            }
            ctx.dbusArgs.reportNames.push_back(*reportName);
        }
    }
    return true;
}

inline bool parseMetricProperties(crow::Response& res, Context& ctx)
{
    if (!ctx.parsedInfo.metricProperties)
    {
        return true;
    }

    ctx.dbusArgs.sensors.reserve(ctx.parsedInfo.metricProperties->size());

    size_t uriIdx = 0;
    for (const std::string& uri : *ctx.parsedInfo.metricProperties)
    {
        auto el = ctx.uriToDbusMerged.find(uri);
        if (el == ctx.uriToDbusMerged.end())
        {
            BMCWEB_LOG_ERROR << "Failed to find DBus sensor "
                                "corsresponding to URI "
                             << uri;
            messages::propertyValueNotInList(
                res, uri, "MetricProperties/" + std::to_string(uriIdx));
            return false;
        }

        const std::string& dbusPath = el->second;
        ctx.dbusArgs.sensors.emplace_back(dbusPath, uri);
        uriIdx++;
    }
    return true;
}

inline bool parsePostTriggerParams(crow::Response& res,
                                   const crow::Request& req, Context& ctx)
{
    std::optional<std::string> id;
    std::optional<std::string> name;
    std::optional<std::string> metricType;
    std::optional<std::vector<std::string>> triggerActions;
    std::optional<std::string> discreteTriggerCondition;
    std::optional<std::vector<nlohmann::json>> discreteTriggers;
    std::optional<nlohmann::json> numericThresholds;
    std::optional<nlohmann::json> links;
    if (!json_util::readJson(
            req, res, "Id", id, "Name", name, "MetricType", metricType,
            "TriggerActions", triggerActions, "DiscreteTriggerCondition",
            discreteTriggerCondition, "DiscreteTriggers", discreteTriggers,
            "NumericThresholds", numericThresholds, "MetricProperties",
            ctx.parsedInfo.metricProperties, "Links", links))
    {
        return false;
    }

    ctx.dbusArgs.id = id.value_or("");
    ctx.dbusArgs.name = name.value_or("");

    if (metricType)
    {
        if (!(ctx.parsedInfo.metricType = getMetricType(*metricType)))
        {
            messages::propertyValueIncorrect(res, "MetricType", *metricType);
            return false;
        }
    }

    if (discreteTriggerCondition)
    {
        if (!(ctx.parsedInfo.discreteCondition =
                  getDiscreteCondition(*discreteTriggerCondition)))
        {
            messages::propertyValueIncorrect(res, "DiscreteTriggerCondition",
                                             *discreteTriggerCondition);
            return false;
        }
    }

    if (triggerActions)
    {
        ctx.dbusArgs.actions.reserve(triggerActions->size());
        for (const std::string& action : *triggerActions)
        {
            if (const std::optional<std::string>& dbusAction =
                    redfishActionToDbusAction(action))
            {
                ctx.dbusArgs.actions.emplace_back(*dbusAction);
            }
            else
            {
                messages::propertyValueNotInList(res, action, "TriggerActions");
                return false;
            }
        }
    }

    if (!parseTriggerThresholds(res, discreteTriggers, numericThresholds, ctx))
    {
        return false;
    }

    if (links)
    {
        if (!parseLinks(res, *links, ctx))
        {
            return false;
        }
    }
    return true;
}

inline void createTrigger(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          Context& ctx)
{
    crow::connections::systemBus->async_method_call(
        [aResp = asyncResp, id = ctx.dbusArgs.id](
            const boost::system::error_code ec, const std::string& dbusPath) {
            if (ec == boost::system::errc::file_exists)
            {
                messages::resourceAlreadyExists(aResp->res, "Trigger", "Id",
                                                id);
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

            const std::optional<std::string>& triggerId =
                getTriggerIdFromDbusPath(dbusPath);
            if (!triggerId)
            {
                messages::internalError(aResp->res);
                BMCWEB_LOG_ERROR << "Unknown data returned by "
                                    "AddTrigger DBus method";
                return;
            }

            messages::created(aResp->res);
            aResp->res.addHeader("Location",
                                 triggerUri + std::string("/") + *triggerId);
        },
        service, "/xyz/openbmc_project/Telemetry/Triggers",
        "xyz.openbmc_project.Telemetry.TriggerManager", "AddTrigger",
        "TelemetryService/" + ctx.dbusArgs.id, ctx.dbusArgs.name,
        ctx.dbusArgs.actions, ctx.dbusArgs.sensors, ctx.dbusArgs.reportNames,
        ctx.dbusArgs.thresholds);
}

} // namespace add_trigger

namespace get_trigger
{

template <typename T>
inline bool hasValue(const std::string& key, const TriggerGetParamsMap& params)
{
    TriggerGetParamsMap::const_iterator it{params.find(key)};
    if (it != params.end())
    {
        return std::holds_alternative<T>(it->second);
    }
    return false;
}

template <typename T>
inline const T* getValue(const std::string& key,
                         const TriggerGetParamsMap& params)
{
    if (hasValue<T>(key, params))
    {
        TriggerGetParamsMap::const_iterator it{params.find(key)};
        return std::get_if<T>(&it->second);
    }
    return nullptr;
}

inline TriggerGetParamsMap extractTriggerParams(
    const std::vector<std::pair<std::string, TriggerGetParamsVariant>>& data)
{
    TriggerGetParamsMap params;
    for (const auto& [key, var] : data)
    {
        params[key] = var;
    }
    return params;
}

inline bool areRequiredParamsProvided(const TriggerGetParamsMap& params)
{
    if (hasValue<TriggerThresholdParamsExt>("Thresholds", params) &&
        hasValue<TriggerSensorsParams>("Sensors", params) &&
        hasValue<std::vector<std::string>>("ReportNames", params) &&
        hasValue<std::vector<std::string>>("TriggerActions", params) &&
        hasValue<std::string>("Name", params) &&
        hasValue<bool>("Discrete", params))
    {
        return true;
    }
    return false;
}

inline std::optional<std::string> dbusToRedfishAction(const std::string& action)
{
    static const std::unordered_map<std::string, std::string> actions{
        {"UpdateReport", "RedfishMetricReport"},
        {"RedfishEvent", "RedfishEvent"},
        {"LogToLogService", "LogToLogService"}};
    std::unordered_map<std::string, std::string>::const_iterator it{
        actions.find(action)};
    return it != actions.end() ? std::make_optional(it->second) : std::nullopt;
}

inline std::vector<std::string>
    getTriggerActions(const TriggerGetParamsMap& params)
{
    std::vector<std::string> triggerActions;

    if (const std::vector<std::string>* dbusActions =
            getValue<std::vector<std::string>>("TriggerActions", params))
    {
        for (const std::string& dbusAction : *dbusActions)
        {
            if (std::optional<std::string> redfishAction =
                    dbusToRedfishAction(dbusAction))
            {
                triggerActions.push_back(*redfishAction);
            }
        }
    }

    return triggerActions;
}

nlohmann::json
    getDiscreteTriggers(const TriggerThresholdParamsExt& thresholdParams)
{
    nlohmann::json thresholds = nlohmann::json::array();
    if (const std::vector<DiscreteThresholdParams>* discreteParams =
            std::get_if<std::vector<DiscreteThresholdParams>>(&thresholdParams))
    {
        for (const auto& [name, severity, dwellTime, value] : *discreteParams)
        {
            thresholds.push_back({
                {"Name", name},
                {"Severity", severity},
                {"DwellTime", time_utils::toDurationString(
                                  std::chrono::milliseconds(dwellTime))},
                {"Value", value},
            });
        }
    }

    return thresholds;
}

nlohmann::json
    getNumericThresholds(const TriggerThresholdParamsExt& thresholdParams)
{
    nlohmann::json thresholds;
    if (const std::vector<NumericThresholdParams>* numericParams =
            std::get_if<std::vector<NumericThresholdParams>>(&thresholdParams))
    {
        for (const auto& [type, dwellTime, activation, reading] :
             *numericParams)
        {
            thresholds[type]["Reading"] = reading;
            thresholds[type]["Activation"] = activation;
            thresholds[type]["DwellTime"] = time_utils::toDurationString(
                std::chrono::milliseconds(dwellTime));
        }
    }

    return thresholds;
}

nlohmann::json getMetricReportDefinitions(const TriggerGetParamsMap& params)
{
    nlohmann::json reports = nlohmann::json::array();
    if (const std::vector<std::string>* reportNames =
            getValue<std::vector<std::string>>("ReportNames", params))
    {
        for (const std::string& name : *reportNames)
        {
            reports.push_back({
                {"@odata.id", metricReportDefinitionUri + name},
            });
        }
    }

    return reports;
}

inline std::vector<std::string>
    getMetricProperties(const TriggerGetParamsMap& params)
{
    std::vector<std::string> metricProperties;
    if (const TriggerSensorsParams* sensors =
            getValue<TriggerSensorsParams>("Sensors", params))
    {
        metricProperties.reserve(sensors->size());
        for (const auto& [_, metadata] : *sensors)
        {
            metricProperties.emplace_back(metadata);
        }
    }

    return metricProperties;
}

inline void fillTrigger(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, const std::string& id,
    const std::vector<std::pair<std::string, TriggerGetParamsVariant>>& ret)
{
    const TriggerGetParamsMap params = extractTriggerParams(ret);
    if (!areRequiredParamsProvided(params))
    {
        BMCWEB_LOG_ERROR << "Property type mismatch or property is missing";
        messages::internalError(asyncResp->res);
        return;
    }

    asyncResp->res.jsonValue["@odata.type"] = "#Triggers.v1_2_0.Triggers";
    asyncResp->res.jsonValue["@odata.id"] = triggerUri + std::string("/") + id;
    asyncResp->res.jsonValue["Id"] = id;
    asyncResp->res.jsonValue["Name"] = id;

    if (const bool* discrete = getValue<bool>("Discrete", params))
    {
        if (const TriggerThresholdParamsExt* thresholdParams =
                getValue<TriggerThresholdParamsExt>("Thresholds", params))
        {
            if (*discrete)
            {
                nlohmann::json& triggers =
                    asyncResp->res.jsonValue["DiscreteTriggers"];
                triggers = getDiscreteTriggers(*thresholdParams);
                asyncResp->res.jsonValue["DiscreteTriggerCondition"] =
                    triggers.empty() ? "Changed" : "Specified";
            }
            else
            {
                asyncResp->res.jsonValue["NumericThresholds"] =
                    getNumericThresholds(*thresholdParams);
            }
        }
        asyncResp->res.jsonValue["MetricType"] =
            *discrete ? "Discrete" : "Numeric";
    }

    if (const std::string* name = getValue<std::string>("Name", params))
    {
        asyncResp->res.jsonValue["Name"] = *name;
    }

    asyncResp->res.jsonValue["TriggerActions"] = getTriggerActions(params);
    asyncResp->res.jsonValue["MetricProperties"] = getMetricProperties(params);
    asyncResp->res.jsonValue["Links"]["MetricReportDefinitions"] =
        getMetricReportDefinitions(params);
}

} // namespace get_trigger

} // namespace telemetry

inline void requestRoutesTriggerCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/TelemetryService/Triggers/")
        .privileges(redfish::privileges::getTriggersCollection)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#TriggersCollection.TriggersCollection";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/TelemetryService/Triggers";
                asyncResp->res.jsonValue["Name"] = "Triggers Collection";
                const std::vector<const char*> interfaces{
                    telemetry::triggerInterface};
                collection_util::getCollectionMembers(
                    asyncResp, telemetry::triggerUri, interfaces,
                    "/xyz/openbmc_project/Telemetry/Triggers/TelemetryService");
            });

    BMCWEB_ROUTE(app, "/redfish/v1/TelemetryService/Triggers/")
        .privileges(redfish::privileges::postTriggersCollection)
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                const auto ctx =
                    std::make_shared<telemetry::add_trigger::Context>();
                if (!telemetry::add_trigger::parsePostTriggerParams(
                        asyncResp->res, req, *ctx))
                {
                    return;
                }

                if (!ctx->parsedInfo.metricProperties ||
                    ctx->parsedInfo.metricProperties->empty())
                {
                    telemetry::add_trigger::createTrigger(asyncResp, *ctx);
                    return;
                }

                boost::container::flat_set<std::pair<std::string, std::string>>
                    chassisSensors;
                if (!telemetry::getChassisSensorNode(
                        asyncResp, *ctx->parsedInfo.metricProperties,
                        chassisSensors))
                {
                    return;
                }

                const auto finalizer =
                    std::make_shared<utils::Finalizer>([asyncResp, ctx] {
                        if (!telemetry::add_trigger::parseMetricProperties(
                                asyncResp->res, *ctx))
                        {
                            return;
                        }
                        telemetry::add_trigger::createTrigger(asyncResp, *ctx);
                    });

                for (const auto& [chassis, sensorType] : chassisSensors)
                {
                    retrieveUriToDbusMap(
                        chassis, sensorType,
                        [asyncResp, ctx,
                         finalizer](const boost::beast::http::status status,
                                    const boost::container::flat_map<
                                        std::string, std::string>& uriToDbus) {
                            if (status == boost::beast::http::status::ok)
                            {
                                ctx->uriToDbusMerged.insert(uriToDbus.begin(),
                                                            uriToDbus.end());
                            }
                        });
                }
            });
}

inline void requestRoutesTrigger(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/TelemetryService/Triggers/<str>/")
        .privileges(redfish::privileges::getTriggers)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& id) {
                crow::connections::systemBus->async_method_call(
                    [asyncResp,
                     id](const boost::system::error_code ec,
                         const std::vector<std::pair<
                             std::string, telemetry::TriggerGetParamsVariant>>&
                             ret) {
                        if (ec.value() == EBADR ||
                            ec == boost::system::errc::host_unreachable)
                        {
                            messages::resourceNotFound(asyncResp->res,
                                                       "Triggers", id);
                            return;
                        }
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR << "respHandler DBus error " << ec;
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        telemetry::get_trigger::fillTrigger(asyncResp, id, ret);
                    },
                    telemetry::service, telemetry::getDbusTriggerPath(id),
                    "org.freedesktop.DBus.Properties", "GetAll",
                    telemetry::triggerInterface);
            });

    BMCWEB_ROUTE(app, "/redfish/v1/TelemetryService/Triggers/<str>/")
        .privileges(redfish::privileges::deleteTriggers)
        .methods(boost::beast::http::verb::delete_)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& id) {
                const std::string triggerPath =
                    telemetry::getDbusTriggerPath(id);

                crow::connections::systemBus->async_method_call(
                    [asyncResp, id](const boost::system::error_code ec) {
                        if (ec.value() == EBADR)
                        {
                            messages::resourceNotFound(asyncResp->res,
                                                       "Triggers", id);
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
                    telemetry::service, triggerPath,
                    "xyz.openbmc_project.Object.Delete", "Delete");
            });
}

} // namespace redfish
