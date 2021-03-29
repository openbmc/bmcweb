#pragma once

#include "node.hpp"
#include "utils/telemetry_utils.hpp"

#include <variant>

namespace redfish
{

class TelemetryService : public Node
{
  public:
    TelemetryService(App& app) : Node(app, "/redfish/v1/TelemetryService/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const crow::Request&, const std::vector<std::string>&) override
    {
        asyncResp->res.jsonValue["@odata.type"] =
            "#TelemetryService.v1_2_1.TelemetryService";
        asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/TelemetryService";
        asyncResp->res.jsonValue["Id"] = "TelemetryService";
        asyncResp->res.jsonValue["Name"] = "Telemetry Service";

        asyncResp->res.jsonValue["MetricReportDefinitions"]["@odata.id"] =
            "/redfish/v1/TelemetryService/MetricReportDefinitions";
        asyncResp->res.jsonValue["MetricReports"]["@odata.id"] =
            "/redfish/v1/TelemetryService/MetricReports";

        crow::connections::systemBus->async_method_call(
            [asyncResp](
                const boost::system::error_code ec,
                const std::vector<std::pair<
                    std::string, std::variant<uint32_t, uint64_t>>>& ret) {
                if (ec == boost::system::errc::host_unreachable)
                {
                    asyncResp->res.jsonValue["Status"]["State"] = "Absent";
                    return;
                }
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "respHandler DBus error " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }

                asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

                const size_t* maxReports = nullptr;
                const uint64_t* minInterval = nullptr;
                for (const auto& [key, var] : ret)
                {
                    if (key == "MaxReports")
                    {
                        maxReports = std::get_if<size_t>(&var);
                    }
                    else if (key == "MinInterval")
                    {
                        minInterval = std::get_if<uint64_t>(&var);
                    }
                }
                if (!maxReports || !minInterval)
                {
                    BMCWEB_LOG_ERROR
                        << "Property type mismatch or property is missing";
                    messages::internalError(asyncResp->res);
                    return;
                }

                asyncResp->res.jsonValue["MaxReports"] = *maxReports;
                asyncResp->res.jsonValue["MinCollectionInterval"] =
                    time_utils::toDurationString(std::chrono::milliseconds(
                        static_cast<time_t>(*minInterval)));
            },
            telemetry::service, "/xyz/openbmc_project/Telemetry/Reports",
            "org.freedesktop.DBus.Properties", "GetAll",
            "xyz.openbmc_project.Telemetry.ReportManager");
    }
};
} // namespace redfish
