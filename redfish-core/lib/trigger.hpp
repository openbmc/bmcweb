#pragma once

#include "utils/collection.hpp"
#include "utils/telemetry_utils.hpp"

#include <app.hpp>
#include <registries/privilege_registry.hpp>

#include <tuple>
#include <variant>
#include <vector>

namespace redfish
{
namespace telemetry
{
constexpr const char* triggerInterface =
    "xyz.openbmc_project.Telemetry.Trigger";
constexpr const char* triggerUri = "/redfish/v1/TelemetryService/Triggers";

using NumericThresholdParams =
    std::tuple<std::string, uint64_t, std::string, double>;

using DiscreteThresholdParams =
    std::tuple<std::string, std::string, uint64_t, std::string>;

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

inline std::string getDbusTriggerPath(const std::string& id)
{
    std::string path =
        "/xyz/openbmc_project/Telemetry/Triggers/TelemetryService/" + id;
    dbus::utility::escapePathForDbus(path);
    return path;
}

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

                        telemetry::fillTrigger(asyncResp, id, ret);
                    },
                    telemetry::service, telemetry::getDbusTriggerPath(id),
                    "org.freedesktop.DBus.Properties", "GetAll",
                    telemetry::triggerInterface);
            });
}

} // namespace redfish
