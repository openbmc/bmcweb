#pragma once

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
    std::variant<std::monostate, bool, TriggerThresholdParamsExt,
                 TriggerSensorsParams, std::vector<std::string>>;

using TriggerGetParamsMap =
    std::unordered_map<std::string, TriggerGetParamsVariant>;

template <typename T>
inline bool has_value(const std::string& key, const TriggerGetParamsMap& params)
{
    if (params.count(key) > 0)
    {
        return std::holds_alternative<T>(params.at(key));
    }
    return false;
}

template <typename T>
inline const T* get_value(const std::string& key,
                          const TriggerGetParamsMap& params)
{
    if (has_value<T>(key, params))
    {
        return std::get_if<T>(&params.at(key));
    }
    return nullptr;
}

inline TriggerGetParamsMap extractTriggerParams(
    const std::vector<std::pair<std::string, TriggerGetParamsVariant>>& data)
{
    TriggerGetParamsMap params;
    for (const auto& [key, var] : data)
    {
        params[key] = std::move(var);
    }
    return params;
}

inline bool areRequiredParamsProvided(const TriggerGetParamsMap& params)
{
    if (has_value<TriggerThresholdParamsExt>("Thresholds", params) &&
        has_value<TriggerSensorsParams>("Sensors", params) &&
        has_value<std::vector<std::string>>("ReportNames", params) &&
        has_value<bool>("Discrete", params) &&
        has_value<bool>("LogToJournal", params) &&
        has_value<bool>("LogToRedfish", params) &&
        has_value<bool>("UpdateReport", params))
    {
        return true;
    }
    return false;
}

inline std::vector<std::string>
    getTriggerActions(const TriggerGetParamsMap& params)
{
    std::vector<std::string> triggerActions;
    triggerActions.reserve(3);

    if (const bool* log = get_value<bool>("LogToJournal", params); true == *log)
    {
        triggerActions.emplace_back("LogToLogService");
    }
    if (const bool* log = get_value<bool>("LogToRedfish", params); true == *log)
    {
        triggerActions.emplace_back("RedfishEvent");
    }
    if (const bool* log = get_value<bool>("UpdateReport", params); true == *log)
    {
        triggerActions.emplace_back("RedfishMetricReport");
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
            get_value<std::vector<std::string>>("ReportNames", params))
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
            get_value<TriggerSensorsParams>("Sensors", params))
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
    asyncResp->res.jsonValue["@odata.type"] = "#Triggers.v1_1_4.Triggers";
    asyncResp->res.jsonValue["@odata.id"] = triggerUri + id;
    asyncResp->res.jsonValue["Id"] = id;
    asyncResp->res.jsonValue["Name"] = id;

    const TriggerGetParamsMap params = extractTriggerParams(ret);
    if (!areRequiredParamsProvided(params))
    {
        BMCWEB_LOG_ERROR << "Property type mismatch or property is missing";
        messages::internalError(asyncResp->res);
        return;
    }

    if (const bool* discrete = get_value<bool>("Discrete", params))
    {
        if (const TriggerThresholdParamsExt* thresholdParams =
                get_value<TriggerThresholdParamsExt>("Thresholds", params))
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

    asyncResp->res.jsonValue["TriggerActions"] = getTriggerActions(params);
    asyncResp->res.jsonValue["MetricProperties"] = getMetricProperties(params);
    asyncResp->res.jsonValue["Links"]["MetricReportDefinitions"] =
        getMetricReportDefinitions(params);
}

} // namespace telemetry

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
