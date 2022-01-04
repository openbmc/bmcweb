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

inline std::optional<std::string>
    getRedfishFromDbusAction(const std::string& dbusAction)
{
    std::optional<std::string> redfishAction = std::nullopt;
    if (dbusAction == "UpdateReport")
    {
        redfishAction = "RedfishMetricReport";
    }
    if (dbusAction == "RedfishEvent")
    {
        redfishAction = "RedfishEvent";
    }
    if (dbusAction == "LogToLogService")
    {
        redfishAction = "LogToLogService";
    }
    return redfishAction;
}

inline std::optional<std::vector<std::string>>
    getTriggerActions(const std::vector<std::string>& dbusActions)
{
    std::vector<std::string> triggerActions;
    for (const std::string& dbusAction : dbusActions)
    {
        std::optional<std::string> redfishAction =
            getRedfishFromDbusAction(dbusAction);

        if (!redfishAction)
        {
            return std::nullopt;
        }

        triggerActions.push_back(*redfishAction);
    }

    return std::make_optional(triggerActions);
}

inline std::optional<nlohmann::json>
    getDiscreteTriggers(const TriggerThresholdParamsExt& thresholdParams)
{
    const std::vector<DiscreteThresholdParams>* discreteParams =
        std::get_if<std::vector<DiscreteThresholdParams>>(&thresholdParams);

    if (!discreteParams)
    {
        return std::nullopt;
    }

    nlohmann::json triggers = nlohmann::json::array();
    for (const auto& [name, severity, dwellTime, value] : *discreteParams)
    {
        std::optional<std::string> duration =
            time_utils::toDurationStringFromUint(dwellTime);

        if (!duration)
        {
            return std::nullopt;
        }

        triggers.push_back({
            {"Name", name},
            {"Severity", severity},
            {"DwellTime", *duration},
            {"Value", value},
        });
    }

    return std::make_optional(triggers);
}

inline std::optional<nlohmann::json>
    getNumericThresholds(const TriggerThresholdParamsExt& thresholdParams)
{
    const std::vector<NumericThresholdParams>* numericParams =
        std::get_if<std::vector<NumericThresholdParams>>(&thresholdParams);

    if (!numericParams)
    {
        return std::nullopt;
    }

    nlohmann::json thresholds;
    for (const auto& [type, dwellTime, activation, reading] : *numericParams)
    {
        std::optional<std::string> duration =
            time_utils::toDurationStringFromUint(dwellTime);

        if (!duration)
        {
            return std::nullopt;
        }

        thresholds[type] = {{"Reading", reading},
                            {"Activation", activation},
                            {"DwellTime", *duration}};
    }

    return std::make_optional(thresholds);
}

inline nlohmann::json
    getMetricReportDefinitions(const std::vector<std::string>& reportNames)
{
    nlohmann::json reports = nlohmann::json::array();
    for (const std::string& name : reportNames)
    {
        reports.push_back({
            {"@odata.id", metricReportDefinitionUri + std::string("/") + name},
        });
    }

    return reports;
}

inline std::vector<std::string>
    getMetricProperties(const TriggerSensorsParams& sensors)
{
    std::vector<std::string> metricProperties;
    metricProperties.reserve(sensors.size());
    for (const auto& [_, metadata] : sensors)
    {
        metricProperties.emplace_back(metadata);
    }

    return metricProperties;
}

inline bool fillTrigger(
    nlohmann::json& json, const std::string& id,
    const std::vector<std::pair<std::string, TriggerGetParamsVariant>>&
        properties)
{
    const std::string* name = nullptr;
    const bool* discrete = nullptr;
    const TriggerSensorsParams* sensors = nullptr;
    const std::vector<std::string>* reports = nullptr;
    const std::vector<std::string>* actions = nullptr;
    const TriggerThresholdParamsExt* thresholds = nullptr;

    for (const auto& [key, var] : properties)
    {
        if (key == "Name")
        {
            name = std::get_if<std::string>(&var);
        }
        else if (key == "Discrete")
        {
            discrete = std::get_if<bool>(&var);
        }
        else if (key == "Sensors")
        {
            sensors = std::get_if<TriggerSensorsParams>(&var);
        }
        else if (key == "ReportNames")
        {
            reports = std::get_if<std::vector<std::string>>(&var);
        }
        else if (key == "TriggerActions")
        {
            actions = std::get_if<std::vector<std::string>>(&var);
        }
        else if (key == "Thresholds")
        {
            thresholds = std::get_if<TriggerThresholdParamsExt>(&var);
        }
    }

    if (!name || !discrete || !sensors || !reports || !actions || !thresholds)
    {
        BMCWEB_LOG_ERROR
            << "Property type mismatch or property is missing in Trigger: "
            << id;
        return false;
    }

    json["@odata.type"] = "#Triggers.v1_2_0.Triggers";
    json["@odata.id"] = triggerUri + std::string("/") + id;
    json["Id"] = id;
    json["Name"] = *name;

    if (*discrete)
    {
        std::optional<nlohmann::json> discreteTriggers =
            getDiscreteTriggers(*thresholds);

        if (!discreteTriggers)
        {
            BMCWEB_LOG_ERROR << "Property Thresholds is invalid for discrete "
                                "triggers in Trigger: "
                             << id;
            return false;
        }

        json["DiscreteTriggers"] = *discreteTriggers;
        json["DiscreteTriggerCondition"] =
            discreteTriggers->empty() ? "Changed" : "Specified";
        json["MetricType"] = "Discrete";
    }
    else
    {
        std::optional<nlohmann::json> numericThresholds =
            getNumericThresholds(*thresholds);

        if (!numericThresholds)
        {
            BMCWEB_LOG_ERROR << "Property Thresholds is invalid for numeric "
                                "thresholds in Trigger: "
                             << id;
            return false;
        }

        json["NumericThresholds"] = *numericThresholds;
        json["MetricType"] = "Numeric";
    }

    std::optional<std::vector<std::string>> triggerActions =
        getTriggerActions(*actions);

    if (!triggerActions)
    {
        BMCWEB_LOG_ERROR << "Property TriggerActions is invalid in Trigger: "
                         << id;
        return false;
    }

    json["TriggerActions"] = *triggerActions;
    json["MetricProperties"] = getMetricProperties(*sensors);
    json["Links"]["MetricReportDefinitions"] =
        getMetricReportDefinitions(*reports);

    return true;
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

                        if (!telemetry::fillTrigger(asyncResp->res.jsonValue,
                                                    id, ret))
                        {
                            messages::internalError(asyncResp->res);
                        }
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
