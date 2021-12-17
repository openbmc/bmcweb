#pragma once

#include "app.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/collection.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/telemetry_utils.hpp"
#include "utils/time_utils.hpp"

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

using TriggerThresholdParamsExt =
    std::variant<std::monostate, std::vector<NumericThresholdParams>,
                 std::vector<DiscreteThresholdParams>>;

using TriggerSensorsParams =
    std::vector<std::pair<sdbusplus::message::object_path, std::string>>;

using TriggerGetParamsVariant =
    std::variant<std::monostate, bool, std::string, TriggerThresholdParamsExt,
                 TriggerSensorsParams, std::vector<std::string>,
                 std::vector<sdbusplus::message::object_path>>;

inline std::optional<std::string>
    getRedfishFromDbusAction(const std::string& dbusAction)
{
    std::optional<std::string> redfishAction = std::nullopt;
    if (dbusAction == "UpdateReport")
    {
        redfishAction = "RedfishMetricReport";
    }
    if (dbusAction == "LogToRedfishEventLog")
    {
        redfishAction = "RedfishEvent";
    }
    if (dbusAction == "LogToJournal")
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

    return {std::move(triggerActions)};
}

inline std::optional<nlohmann::json::array_t>
    getDiscreteTriggers(const TriggerThresholdParamsExt& thresholdParams)
{
    const std::vector<DiscreteThresholdParams>* discreteParams =
        std::get_if<std::vector<DiscreteThresholdParams>>(&thresholdParams);

    if (discreteParams == nullptr)
    {
        return std::nullopt;
    }

    nlohmann::json::array_t triggers;
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
        trigger["Severity"] = severity;
        trigger["DwellTime"] = *duration;
        trigger["Value"] = value;
        triggers.push_back(std::move(trigger));
    }

    return {std::move(triggers)};
}

inline std::optional<nlohmann::json>
    getNumericThresholds(const TriggerThresholdParamsExt& thresholdParams)
{
    const std::vector<NumericThresholdParams>* numericParams =
        std::get_if<std::vector<NumericThresholdParams>>(&thresholdParams);

    if (numericParams == nullptr)
    {
        return std::nullopt;
    }

    nlohmann::json::object_t thresholds;
    for (const auto& [type, dwellTime, activation, reading] : *numericParams)
    {
        std::optional<std::string> duration =
            time_utils::toDurationStringFromUint(dwellTime);

        if (!duration)
        {
            return std::nullopt;
        }
        nlohmann::json& threshold = thresholds[type];
        threshold["Reading"] = reading;
        threshold["Activation"] = activation;
        threshold["DwellTime"] = *duration;
    }

    return {std::move(thresholds)};
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
                BMCWEB_LOG_ERROR << "Property Reports contains invalid value: "
                                 << path.str;
                return std::nullopt;
            }
        }

        nlohmann::json::object_t report;
        report["@odata.id"] =
            crow::utility::urlFromPieces("redfish", "v1", "TelemetryService",
                                         "MetricReportDefinitions", reportId);
        reports.push_back(std::move(report));
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
        std::optional<std::vector<std::string>> redfishTriggerActions =
            getTriggerActions(*triggerActions);
        if (!redfishTriggerActions)
        {
            BMCWEB_LOG_ERROR
                << "Property TriggerActions is invalid in Trigger: " << id;
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
            BMCWEB_LOG_ERROR << "Property Reports is invalid in Trigger: "
                             << id;
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
                BMCWEB_LOG_ERROR
                    << "Property Thresholds is invalid for discrete "
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
                BMCWEB_LOG_ERROR
                    << "Property Thresholds is invalid for numeric "
                       "thresholds in Trigger: "
                    << id;
                return false;
            }

            json["NumericThresholds"] = *numericThresholds;
            json["MetricType"] = "Numeric";
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
    json["@odata.id"] = crow::utility::urlFromPieces(
        "redfish", "v1", "TelemetryService", "Triggers", id);
    json["Id"] = id;

    return true;
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
                BMCWEB_LOG_ERROR << "respHandler DBus error " << ec;
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
                BMCWEB_LOG_ERROR << "respHandler DBus error " << ec;
                messages::internalError(asyncResp->res);
                return;
            }

            asyncResp->res.result(boost::beast::http::status::no_content);
            },
            telemetry::service, triggerPath,
            "xyz.openbmc_project.Object.Delete", "Delete");
        });
}

} // namespace redfish
