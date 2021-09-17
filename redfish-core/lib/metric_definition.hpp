#pragma once

#include "async_resp.hpp"
#include "sensors.hpp"
#include "utils/get_chassis_names.hpp"
#include "utils/telemetry_utils.hpp"

#include <registries/privilege_registry.hpp>

namespace redfish
{

struct CopyJson
{
    CopyJson& required(std::string_view key)
    {
        keys.emplace_back(key, true);
        return *this;
    }

    CopyJson& optional(std::string_view key)
    {
        keys.emplace_back(key, false);
        return *this;
    }

    bool perform(nlohmann::json& dst, nlohmann::json&& src) const
    {
        for (const auto& [key, isRequired] : keys)
        {
            if (isRequired && src.find(key) == src.end())
            {
                return false;
            }
        }

        for (const auto& [key, isRequired] : keys)
        {
            if (auto it = src.find(key); it != src.end())
            {
                dst.emplace(key, *it);
            }
        }

        return true;
    }

  private:
    std::vector<std::pair<std::string_view, bool>> keys;
};

inline void requestRoutesMetricDefinitionCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/TelemetryService/MetricDefinitions/")
        .privileges(privileges::getTelemetryService)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                crow::connections::systemBus->async_method_call(
                    [asyncResp](boost::system::error_code ec,
                                const std::string& metricDefinitionsRaw) {
                        if (ec)
                        {
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        nlohmann::json metricDefinitions =
                            nlohmann::json::parse(metricDefinitionsRaw);

                        CopyJson copy = CopyJson()
                                            .required("@odata.id")
                                            .required("@odata.type")
                                            .required("Name")
                                            .required("Members")
                                            .required("Members@odata.count");

                        if (!copy.perform(asyncResp->res.jsonValue,
                                          std::move(metricDefinitions)))
                        {

                            messages::internalError(asyncResp->res);
                            return;
                        }
                    },
                    "xyz.openbmc_project.Telemetry",
                    "/xyz/openbmc_project/Telemetry/MetricDefinitions",
                    "xyz.openbmc_project.Telemetry.MetricDefinitionManager",
                    "GetMetricDefinitions");
            });
}

inline void requestRoutesMetricDefinition(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/TelemetryService/MetricDefinitions/<str>/")
        .privileges(privileges::getTelemetryService)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& name) {
                crow::connections::systemBus->async_method_call(
                    [asyncResp, name](boost::system::error_code ec,
                                      const std::string& metricDefinitionRaw) {
                        if (ec)
                        {
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        nlohmann::json metricDefinition =
                            nlohmann::json::parse(metricDefinitionRaw);

                        if (metricDefinition.empty())
                        {
                            messages::resourceNotFound(
                                asyncResp->res, "MetricDefinition", name);
                            return;
                        }

                        CopyJson copy = CopyJson()
                                            .required("@odata.id")
                                            .required("@odata.type")
                                            .required("Id")
                                            .required("Name")
                                            .optional("MetricProperties")
                                            .optional("MetricDataType")
                                            .optional("MetricType")
                                            .optional("IsLinear")
                                            .optional("Units")
                                            .optional("MinReadingRange")
                                            .optional("MaxReadingRange");

                        if (!copy.perform(asyncResp->res.jsonValue,
                                          std::move(metricDefinition)))
                        {

                            messages::internalError(asyncResp->res);
                            return;
                        }
                    },
                    "xyz.openbmc_project.Telemetry",
                    "/xyz/openbmc_project/Telemetry/MetricDefinitions",
                    "xyz.openbmc_project.Telemetry.MetricDefinitionManager",
                    "GetMetricDefinition", name);
            });
}

} // namespace redfish
