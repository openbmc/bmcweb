#pragma once

#include "app.hpp"
#include "generated/enums/metric_definition.hpp"
#include "generated/enums/resource.hpp"
#include "generated/enums/triggers.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "sensors.hpp"
#include "utility.hpp"
#include "utils/collection.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"
#include "utils/telemetry_utils.hpp"
#include "utils/time_utils.hpp"

#include <boost/url/format.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <string_view>
#include <tuple>
#include <variant>
#include <vector>

namespace redfish
{
namespace telemetry
{
constexpr const char* triggerInterface =
    "xyz.openbmc_project.Telemetry.Trigger";

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
                 TriggerSensorsParams, std::vector<std::string>,
                 std::vector<sdbusplus::message::object_path>>;

inline triggers::TriggerActionEnum
    toRedfishTriggerAction(std::string_view dbusValue)
{
    if (dbusValue ==
        "xyz.openbmc_project.Telemetry.Trigger.TriggerAction.UpdateReport")
    {
        return triggers::TriggerActionEnum::RedfishMetricReport;
    }
    if (dbusValue ==
        "xyz.openbmc_project.Telemetry.Trigger.TriggerAction.LogToRedfishEventLog")
    {
        return triggers::TriggerActionEnum::RedfishEvent;
    }
    if (dbusValue ==
        "xyz.openbmc_project.Telemetry.Trigger.TriggerAction.LogToJournal")
    {
        return triggers::TriggerActionEnum::LogToLogService;
    }
    return triggers::TriggerActionEnum::Invalid;
}

inline std::string toDbusTriggerAction(std::string_view redfishValue)
{
    if (redfishValue == "RedfishMetricReport")
    {
        return "xyz.openbmc_project.Telemetry.Trigger.TriggerAction.UpdateReport";
    }
    if (redfishValue == "RedfishEvent")
    {
        return "xyz.openbmc_project.Telemetry.Trigger.TriggerAction.LogToRedfishEventLog";
    }
    if (redfishValue == "LogToLogService")
    {
        return "xyz.openbmc_project.Telemetry.Trigger.TriggerAction.LogToJournal";
    }
    return "";
}

inline std::string toDbusSeverity(std::string_view redfishValue)
{
    if (redfishValue == "OK")
    {
        return "xyz.openbmc_project.Telemetry.Trigger.Severity.OK";
    }
    if (redfishValue == "Warning")
    {
        return "xyz.openbmc_project.Telemetry.Trigger.Severity.Warning";
    }
    if (redfishValue == "Critical")
    {
        return "xyz.openbmc_project.Telemetry.Trigger.Severity.Critical";
    }
    return "";
}

inline resource::Health toRedfishSeverity(std::string_view dbusValue)
{
    if (dbusValue == "xyz.openbmc_project.Telemetry.Trigger.Severity.OK")
    {
        return resource::Health::OK;
    }
    if (dbusValue == "xyz.openbmc_project.Telemetry.Trigger.Severity.Warning")
    {
        return resource::Health::Warning;
    }
    if (dbusValue == "xyz.openbmc_project.Telemetry.Trigger.Severity.Critical")
    {
        return resource::Health::Critical;
    }
    return resource::Health::Invalid;
}

inline std::string toDbusThresholdName(std::string_view redfishValue)
{
    if (redfishValue == "UpperCritical")
    {
        return "xyz.openbmc_project.Telemetry.Trigger.Type.UpperCritical";
    }

    if (redfishValue == "LowerCritical")
    {
        return "xyz.openbmc_project.Telemetry.Trigger.Type.LowerCritical";
    }

    if (redfishValue == "UpperWarning")
    {
        return "xyz.openbmc_project.Telemetry.Trigger.Type.UpperWarning";
    }

    if (redfishValue == "LowerWarning")
    {
        return "xyz.openbmc_project.Telemetry.Trigger.Type.LowerWarning";
    }

    return "";
}

inline std::string toRedfishThresholdName(std::string_view dbusValue)
{
    if (dbusValue == "xyz.openbmc_project.Telemetry.Trigger.Type.UpperCritical")
    {
        return "UpperCritical";
    }

    if (dbusValue == "xyz.openbmc_project.Telemetry.Trigger.Type.LowerCritical")
    {
        return "LowerCritical";
    }

    if (dbusValue == "xyz.openbmc_project.Telemetry.Trigger.Type.UpperWarning")
    {
        return "UpperWarning";
    }

    if (dbusValue == "xyz.openbmc_project.Telemetry.Trigger.Type.LowerWarning")
    {
        return "LowerWarning";
    }

    return "";
}

inline std::string toDbusActivation(std::string_view redfishValue)
{
    if (redfishValue == "Either")
    {
        return "xyz.openbmc_project.Telemetry.Trigger.Direction.Either";
    }

    if (redfishValue == "Decreasing")
    {
        return "xyz.openbmc_project.Telemetry.Trigger.Direction.Decreasing";
    }

    if (redfishValue == "Increasing")
    {
        return "xyz.openbmc_project.Telemetry.Trigger.Direction.Increasing";
    }

    return "";
}

inline triggers::ThresholdActivation
    toRedfishActivation(std::string_view dbusValue)
{
    if (dbusValue == "xyz.openbmc_project.Telemetry.Trigger.Direction.Either")
    {
        return triggers::ThresholdActivation::Either;
    }

    if (dbusValue ==
        "xyz.openbmc_project.Telemetry.Trigger.Direction.Decreasing")
    {
        return triggers::ThresholdActivation::Decreasing;
    }

    if (dbusValue ==
        "xyz.openbmc_project.Telemetry.Trigger.Direction.Increasing")
    {
        return triggers::ThresholdActivation::Increasing;
    }

    return triggers::ThresholdActivation::Invalid;
}

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
    std::string id;
    std::string name;
    std::vector<std::string> actions;
    std::vector<std::pair<sdbusplus::message::object_path, std::string>>
        sensors;
    std::vector<sdbusplus::message::object_path> reports;
    TriggerThresholdParams thresholds;

    std::optional<DiscreteCondition> discreteCondition;
    std::optional<MetricType> metricType;
    std::optional<std::vector<std::string>> metricProperties;
};

inline std::optional<sdbusplus::message::object_path>
    getReportPathFromReportDefinitionUri(const std::string& uri)
{
    boost::system::result<boost::urls::url_view> parsed =
        boost::urls::parse_relative_ref(uri);

    if (!parsed)
    {
        return std::nullopt;
    }

    std::string id;
    if (!crow::utility::readUrlSegments(
            *parsed, "redfish", "v1", "TelemetryService",
            "MetricReportDefinitions", std::ref(id)))
    {
        return std::nullopt;
    }

    return sdbusplus::message::object_path(
               "/xyz/openbmc_project/Telemetry/Reports") /
           "TelemetryService" / id;
}

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
    nlohmann::json::object_t* obj =
        numericThresholds.get_ptr<nlohmann::json::object_t*>();
    if (obj == nullptr)
    {
        messages::propertyValueTypeError(res, numericThresholds.dump(),
                                         "NumericThresholds");
        return false;
    }

    std::vector<NumericThresholdParams> parsedParams;
    parsedParams.reserve(numericThresholds.size());

    for (auto& key : *obj)
    {
        std::string dbusThresholdName = toDbusThresholdName(key.first);
        if (dbusThresholdName.empty())
        {
            messages::propertyUnknown(res, key.first);
            return false;
        }

        double reading = 0.0;
        std::string activation;
        std::string dwellTimeStr;

        if (!json_util::readJson(key.second, res, "Reading", reading,
                                 "Activation", activation, "DwellTime",
                                 dwellTimeStr))
        {
            return false;
        }

        std::string dbusActivation = toDbusActivation(activation);

        if (dbusActivation.empty())
        {
            messages::propertyValueIncorrect(res, "Activation", activation);
            return false;
        }

        std::optional<std::chrono::milliseconds> dwellTime =
            time_utils::fromDurationString(dwellTimeStr);
        if (!dwellTime)
        {
            messages::propertyValueIncorrect(res, "DwellTime", dwellTimeStr);
            return false;
        }

        parsedParams.emplace_back(dbusThresholdName,
                                  static_cast<uint64_t>(dwellTime->count()),
                                  dbusActivation, reading);
    }

    ctx.thresholds = std::move(parsedParams);
    return true;
}

inline bool parseDiscreteTriggers(
    crow::Response& res,
    std::optional<std::vector<nlohmann::json>>& discreteTriggers, Context& ctx)
{
    std::vector<DiscreteThresholdParams> parsedParams;
    if (!discreteTriggers)
    {
        ctx.thresholds = std::move(parsedParams);
        return true;
    }

    parsedParams.reserve(discreteTriggers->size());
    for (nlohmann::json& thresholdInfo : *discreteTriggers)
    {
        std::optional<std::string> name = "";
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

        std::string dbusSeverity = toDbusSeverity(severity);
        if (dbusSeverity.empty())
        {
            messages::propertyValueIncorrect(res, "Severity", severity);
            return false;
        }

        parsedParams.emplace_back(*name, dbusSeverity,
                                  static_cast<uint64_t>(dwellTime->count()),
                                  value);
    }

    ctx.thresholds = std::move(parsedParams);
    return true;
}

inline bool parseTriggerThresholds(
    crow::Response& res,
    std::optional<std::vector<nlohmann::json>>& discreteTriggers,
    std::optional<nlohmann::json>& numericThresholds, Context& ctx)
{
    if (discreteTriggers && numericThresholds)
    {
        messages::propertyValueConflict(res, "DiscreteTriggers",
                                        "NumericThresholds");
        messages::propertyValueConflict(res, "NumericThresholds",
                                        "DiscreteTriggers");
        return false;
    }

    if (ctx.discreteCondition)
    {
        if (numericThresholds)
        {
            messages::propertyValueConflict(res, "DiscreteTriggerCondition",
                                            "NumericThresholds");
            messages::propertyValueConflict(res, "NumericThresholds",
                                            "DiscreteTriggerCondition");
            return false;
        }
    }

    if (ctx.metricType)
    {
        if (*ctx.metricType == MetricType::Discrete && numericThresholds)
        {
            messages::propertyValueConflict(res, "NumericThresholds",
                                            "MetricType");
            return false;
        }
        if (*ctx.metricType == MetricType::Numeric && discreteTriggers)
        {
            messages::propertyValueConflict(res, "DiscreteTriggers",
                                            "MetricType");
            return false;
        }
        if (*ctx.metricType == MetricType::Numeric && ctx.discreteCondition)
        {
            messages::propertyValueConflict(res, "DiscreteTriggers",
                                            "DiscreteTriggerCondition");
            return false;
        }
    }

    if (discreteTriggers || ctx.discreteCondition ||
        (ctx.metricType && *ctx.metricType == MetricType::Discrete))
    {
        if (ctx.discreteCondition)
        {
            if (*ctx.discreteCondition == DiscreteCondition::Specified &&
                !discreteTriggers)
            {
                messages::createFailedMissingReqProperties(res,
                                                           "DiscreteTriggers");
                return false;
            }
            if (discreteTriggers &&
                ((*ctx.discreteCondition == DiscreteCondition::Specified &&
                  discreteTriggers->empty()) ||
                 (*ctx.discreteCondition == DiscreteCondition::Changed &&
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
        ctx.reports.reserve(metricReportDefinitions->size());
        for (std::string& reportDefinionUri : *metricReportDefinitions)
        {
            std::optional<sdbusplus::message::object_path> reportPath =
                getReportPathFromReportDefinitionUri(reportDefinionUri);
            if (!reportPath)
            {
                messages::propertyValueIncorrect(res, "MetricReportDefinitions",
                                                 reportDefinionUri);
                return false;
            }
            ctx.reports.emplace_back(*reportPath);
        }
    }
    return true;
}

inline bool parseMetricProperties(crow::Response& res, Context& ctx)
{
    if (!ctx.metricProperties)
    {
        return true;
    }

    ctx.sensors.reserve(ctx.metricProperties->size());

    size_t uriIdx = 0;
    for (const std::string& uriStr : *ctx.metricProperties)
    {
        boost::system::result<boost::urls::url> uri =
            boost::urls::parse_relative_ref(uriStr);
        if (!uri)
        {
            messages::propertyValueIncorrect(
                res, "MetricProperties/" + std::to_string(uriIdx), uriStr);
            return false;
        }
        std::string chassisName;
        std::string sensorName;
        if (!crow::utility::readUrlSegments(*uri, "redfish", "v1", "Chassis",
                                            std::ref(chassisName), "Sensors",
                                            std::ref(sensorName)))
        {
            messages::propertyValueIncorrect(
                res, "MetricProperties/" + std::to_string(uriIdx), uriStr);
            return false;
        }

        std::pair<std::string, std::string> split =
            splitSensorNameAndType(sensorName);
        if (split.first.empty() || split.second.empty())
        {
            messages::propertyValueIncorrect(
                res, "MetricProperties/" + std::to_string(uriIdx), uriStr);
            return false;
        }

        std::string sensorPath = "/xyz/openbmc_project/sensors/" + split.first +
                                 '/' + split.second;

        ctx.sensors.emplace_back(sensorPath, uriStr);
        uriIdx++;
    }
    return true;
}

inline bool parsePostTriggerParams(crow::Response& res,
                                   const crow::Request& req, Context& ctx)
{
    std::optional<std::string> id = "";
    std::optional<std::string> name = "";
    std::optional<std::string> metricType;
    std::optional<std::vector<std::string>> triggerActions;
    std::optional<std::string> discreteTriggerCondition;
    std::optional<std::vector<nlohmann::json>> discreteTriggers;
    std::optional<nlohmann::json> numericThresholds;
    std::optional<nlohmann::json> links;
    if (!json_util::readJsonPatch(
            req, res, "Id", id, "Name", name, "MetricType", metricType,
            "TriggerActions", triggerActions, "DiscreteTriggerCondition",
            discreteTriggerCondition, "DiscreteTriggers", discreteTriggers,
            "NumericThresholds", numericThresholds, "MetricProperties",
            ctx.metricProperties, "Links", links))
    {
        return false;
    }

    ctx.id = *id;
    ctx.name = *name;

    if (metricType)
    {
        ctx.metricType = getMetricType(*metricType);
        if (!ctx.metricType)
        {
            messages::propertyValueIncorrect(res, "MetricType", *metricType);
            return false;
        }
    }

    if (discreteTriggerCondition)
    {
        ctx.discreteCondition = getDiscreteCondition(*discreteTriggerCondition);
        if (!ctx.discreteCondition)
        {
            messages::propertyValueIncorrect(res, "DiscreteTriggerCondition",
                                             *discreteTriggerCondition);
            return false;
        }
    }

    if (triggerActions)
    {
        ctx.actions.reserve(triggerActions->size());
        for (const std::string& action : *triggerActions)
        {
            std::string dbusAction = toDbusTriggerAction(action);

            if (dbusAction.empty())
            {
                messages::propertyValueNotInList(res, action, "TriggerActions");
                return false;
            }

            ctx.actions.emplace_back(dbusAction);
        }
    }
    if (!parseMetricProperties(res, ctx))
    {
        return false;
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

inline void afterCreateTrigger(
    const boost::system::error_code& ec, const std::string& dbusPath,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, const std::string& id)
{
    if (ec == boost::system::errc::file_exists)
    {
        messages::resourceAlreadyExists(asyncResp->res, "Trigger", "Id", id);
        return;
    }
    if (ec == boost::system::errc::too_many_files_open)
    {
        messages::createLimitReachedForResource(asyncResp->res);
        return;
    }
    if (ec)
    {
        messages::internalError(asyncResp->res);
        BMCWEB_LOG_ERROR("respHandler DBus error {}", ec);
        return;
    }

    const std::optional<std::string>& triggerId =
        getTriggerIdFromDbusPath(dbusPath);
    if (!triggerId)
    {
        messages::internalError(asyncResp->res);
        BMCWEB_LOG_ERROR("Unknown data returned by "
                         "AddTrigger DBus method");
        return;
    }

    messages::created(asyncResp->res);
    boost::urls::url locationUrl = boost::urls::format(
        "/redfish/v1/TelemetryService/Triggers/{}", *triggerId);
    asyncResp->res.addHeader("Location", locationUrl.buffer());
}

inline std::optional<nlohmann::json::array_t>
    getTriggerActions(const std::vector<std::string>& dbusActions)
{
    nlohmann::json::array_t triggerActions;
    for (const std::string& dbusAction : dbusActions)
    {
        triggers::TriggerActionEnum redfishAction =
            toRedfishTriggerAction(dbusAction);

        if (redfishAction == triggers::TriggerActionEnum::Invalid)
        {
            return std::nullopt;
        }

        triggerActions.emplace_back(redfishAction);
    }

    return triggerActions;
}

inline std::optional<nlohmann::json::array_t>
    getDiscreteTriggers(const TriggerThresholdParamsExt& thresholdParams)
{
    nlohmann::json::array_t triggers;
    const std::vector<DiscreteThresholdParams>* discreteParams =
        std::get_if<std::vector<DiscreteThresholdParams>>(&thresholdParams);

    if (discreteParams == nullptr)
    {
        return std::nullopt;
    }

    for (const auto& [name, severity, dwellTime, value] : *discreteParams)
    {
        std::optional<std::string> duration =
            time_utils::toDurationStringFromUint(dwellTime);

        if (!duration)
        {
            return std::nullopt;
        }
        nlohmann::json::object_t trigger;
        trigger["Name"] = name;
        trigger["Severity"] = toRedfishSeverity(severity);
        trigger["DwellTime"] = *duration;
        trigger["Value"] = value;
        triggers.emplace_back(std::move(trigger));
    }

    return triggers;
}

inline std::optional<nlohmann::json>
    getNumericThresholds(const TriggerThresholdParamsExt& thresholdParams)
{
    nlohmann::json::object_t thresholds;
    const std::vector<NumericThresholdParams>* numericParams =
        std::get_if<std::vector<NumericThresholdParams>>(&thresholdParams);

    if (numericParams == nullptr)
    {
        return std::nullopt;
    }

    for (const auto& [type, dwellTime, activation, reading] : *numericParams)
    {
        std::optional<std::string> duration =
            time_utils::toDurationStringFromUint(dwellTime);

        if (!duration)
        {
            return std::nullopt;
        }
        nlohmann::json& threshold = thresholds[toRedfishThresholdName(type)];
        threshold["Reading"] = reading;
        threshold["Activation"] = toRedfishActivation(activation);
        threshold["DwellTime"] = *duration;
    }

    return thresholds;
}

inline std::optional<nlohmann::json> getMetricReportDefinitions(
    const std::vector<sdbusplus::message::object_path>& reportPaths)
{
    nlohmann::json reports = nlohmann::json::array();

    for (const sdbusplus::message::object_path& path : reportPaths)
    {
        std::string reportId = path.filename();
        if (reportId.empty())
        {
            {
                BMCWEB_LOG_ERROR("Property Reports contains invalid value: {}",
                                 path.str);
                return std::nullopt;
            }
        }

        nlohmann::json::object_t report;
        report["@odata.id"] = boost::urls::format(
            "/redfish/v1/TelemetryService/MetricReportDefinitions/{}",
            reportId);
        reports.emplace_back(std::move(report));
    }

    return {std::move(reports)};
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
    const std::vector<sdbusplus::message::object_path>* reports = nullptr;
    const std::vector<std::string>* triggerActions = nullptr;
    const TriggerThresholdParamsExt* thresholds = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "Name", name, "Discrete",
        discrete, "Sensors", sensors, "Reports", reports, "TriggerActions",
        triggerActions, "Thresholds", thresholds);

    if (!success)
    {
        return false;
    }

    if (triggerActions != nullptr)
    {
        std::optional<nlohmann::json::array_t> redfishTriggerActions =
            getTriggerActions(*triggerActions);
        if (!redfishTriggerActions)
        {
            BMCWEB_LOG_ERROR(
                "Property TriggerActions is invalid in Trigger: {}", id);
            return false;
        }
        json["TriggerActions"] = *redfishTriggerActions;
    }

    if (reports != nullptr)
    {
        std::optional<nlohmann::json> linkedReports =
            getMetricReportDefinitions(*reports);
        if (!linkedReports)
        {
            BMCWEB_LOG_ERROR("Property Reports is invalid in Trigger: {}", id);
            return false;
        }
        json["Links"]["MetricReportDefinitions"] = *linkedReports;
    }

    if (discrete != nullptr)
    {
        if (*discrete)
        {
            std::optional<nlohmann::json::array_t> discreteTriggers =
                getDiscreteTriggers(*thresholds);

            if (!discreteTriggers)
            {
                BMCWEB_LOG_ERROR("Property Thresholds is invalid for discrete "
                                 "triggers in Trigger: {}",
                                 id);
                return false;
            }

            json["DiscreteTriggers"] = *discreteTriggers;
            json["DiscreteTriggerCondition"] =
                discreteTriggers->empty() ? "Changed" : "Specified";
            json["MetricType"] = metric_definition::MetricType::Discrete;
        }
        else
        {
            std::optional<nlohmann::json> numericThresholds =
                getNumericThresholds(*thresholds);

            if (!numericThresholds)
            {
                BMCWEB_LOG_ERROR("Property Thresholds is invalid for numeric "
                                 "thresholds in Trigger: {}",
                                 id);
                return false;
            }

            json["NumericThresholds"] = *numericThresholds;
            json["MetricType"] = metric_definition::MetricType::Numeric;
        }
    }

    if (name != nullptr)
    {
        json["Name"] = *name;
    }

    if (sensors != nullptr)
    {
        json["MetricProperties"] = getMetricProperties(*sensors);
    }

    json["@odata.type"] = "#Triggers.v1_2_0.Triggers";
    json["@odata.id"] =
        boost::urls::format("/redfish/v1/TelemetryService/Triggers/{}", id);
    json["Id"] = id;

    return true;
}

inline void handleTriggerCollectionPost(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    telemetry::Context ctx;
    if (!telemetry::parsePostTriggerParams(asyncResp->res, req, ctx))
    {
        return;
    }

    crow::connections::systemBus->async_method_call(
        [asyncResp, id = ctx.id](const boost::system::error_code& ec,
                                 const std::string& dbusPath) {
        afterCreateTrigger(ec, dbusPath, asyncResp, id);
    },
        service, "/xyz/openbmc_project/Telemetry/Triggers",
        "xyz.openbmc_project.Telemetry.TriggerManager", "AddTrigger",
        "TelemetryService/" + ctx.id, ctx.name, ctx.actions, ctx.sensors,
        ctx.reports, ctx.thresholds);
}

} // namespace telemetry

inline void requestRoutesTriggerCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/TelemetryService/Triggers/")
        .privileges(redfish::privileges::getTriggersCollection)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        asyncResp->res.jsonValue["@odata.type"] =
            "#TriggersCollection.TriggersCollection";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/TelemetryService/Triggers";
        asyncResp->res.jsonValue["Name"] = "Triggers Collection";
        constexpr std::array<std::string_view, 1> interfaces{
            telemetry::triggerInterface};
        collection_util::getCollectionMembers(
            asyncResp,
            boost::urls::url("/redfish/v1/TelemetryService/Triggers"),
            interfaces,
            "/xyz/openbmc_project/Telemetry/Triggers/TelemetryService");
    });

    BMCWEB_ROUTE(app, "/redfish/v1/TelemetryService/Triggers/")
        .privileges(redfish::privileges::postTriggersCollection)
        .methods(boost::beast::http::verb::post)(std::bind_front(
            telemetry::handleTriggerCollectionPost, std::ref(app)));
}

inline void requestRoutesTrigger(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/TelemetryService/Triggers/<str>/")
        .privileges(redfish::privileges::getTriggers)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& id) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        sdbusplus::asio::getAllProperties(
            *crow::connections::systemBus, telemetry::service,
            telemetry::getDbusTriggerPath(id), telemetry::triggerInterface,
            [asyncResp,
             id](const boost::system::error_code& ec,
                 const std::vector<std::pair<
                     std::string, telemetry::TriggerGetParamsVariant>>& ret) {
            if (ec.value() == EBADR ||
                ec == boost::system::errc::host_unreachable)
            {
                messages::resourceNotFound(asyncResp->res, "Triggers", id);
                return;
            }
            if (ec)
            {
                BMCWEB_LOG_ERROR("respHandler DBus error {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }

            if (!telemetry::fillTrigger(asyncResp->res.jsonValue, id, ret))
            {
                messages::internalError(asyncResp->res);
            }
        });
    });

    BMCWEB_ROUTE(app, "/redfish/v1/TelemetryService/Triggers/<str>/")
        .privileges(redfish::privileges::deleteTriggers)
        .methods(boost::beast::http::verb::delete_)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& id) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        const std::string triggerPath = telemetry::getDbusTriggerPath(id);

        crow::connections::systemBus->async_method_call(
            [asyncResp, id](const boost::system::error_code& ec) {
            if (ec.value() == EBADR)
            {
                messages::resourceNotFound(asyncResp->res, "Triggers", id);
                return;
            }

            if (ec)
            {
                BMCWEB_LOG_ERROR("respHandler DBus error {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }

            asyncResp->res.result(boost::beast::http::status::no_content);
        }, telemetry::service, triggerPath, "xyz.openbmc_project.Object.Delete",
            "Delete");
    });
}

} // namespace redfish
