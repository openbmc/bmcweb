#pragma once

namespace redfish
{

namespace telemetry
{

constexpr const char* service = "xyz.openbmc_project.Telemetry";
constexpr const char* reportInterface = "xyz.openbmc_project.Telemetry.Report";
constexpr const char* metricReportDefinitionUri =
    "/redfish/v1/TelemetryService/MetricReportDefinitions/";
constexpr const char* metricReportUri =
    "/redfish/v1/TelemetryService/MetricReports/";

inline void getReportCollection(const std::shared_ptr<AsyncResp>& asyncResp,
                                const std::string& uri)
{
    const std::array<const char*, 1> interfaces = {reportInterface};

    crow::connections::systemBus->async_method_call(
        [asyncResp, uri](const boost::system::error_code ec,
                         const std::vector<std::string>& reportPaths) {
            if (ec)
            {
                asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
                asyncResp->res.jsonValue["Members@odata.count"] = 0;
                return;
            }

            nlohmann::json& members = asyncResp->res.jsonValue["Members"];
            members = nlohmann::json::array();

            for (const std::string& path : reportPaths)
            {
                std::size_t pos = path.rfind('/');
                if (pos == std::string::npos)
                {
                    BMCWEB_LOG_ERROR << "Failed to find '/' in " << path;
                    messages::internalError(asyncResp->res);
                    return;
                }
                if (path.size() <= (pos + 1))
                {
                    BMCWEB_LOG_ERROR << "Failed to parse path " << path;
                    messages::internalError(asyncResp->res);
                    return;
                }

                members.push_back({{"@odata.id", uri + path.substr(pos + 1)}});
            }

            asyncResp->res.jsonValue["Members@odata.count"] = members.size();
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
        "/xyz/openbmc_project/Telemetry/Reports/TelemetryService", 1,
        interfaces);
}

inline std::string getDbusReportPath(const std::string& id)
{
    std::string path =
        "/xyz/openbmc_project/Telemetry/Reports/TelemetryService/" + id;
    dbus::utility::escapePathForDbus(path);
    return path;
}

} // namespace telemetry
} // namespace redfish
