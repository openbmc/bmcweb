#pragma once

#include "dbus_utility.hpp"

namespace redfish
{

namespace telemetry
{

constexpr const char* service = "xyz.openbmc_project.Telemetry";
constexpr const char* reportSubtree =
    "/xyz/openbmc_project/Telemetry/Reports/TelemetryService";
constexpr const char* reportInterface = "xyz.openbmc_project.Telemetry.Report";
constexpr const char* metricReportDefinitionUri =
    "/redfish/v1/TelemetryService/MetricReportDefinitions/";
constexpr const char* metricReportUri =
    "/redfish/v1/TelemetryService/MetricReports/";
constexpr const char* triggerSubtree =
    "/xyz/openbmc_project/Telemetry/Triggers/TelemetryService";
constexpr const char* triggerInterface =
    "xyz.openbmc_project.Telemetry.Trigger";
constexpr const char* triggerUri = "/redfish/v1/TelemetryService/Triggers/";

struct CollectionParams
{
    const char* subtree;
    int depth;
    std::array<const char*, 1> interfaces;

    CollectionParams() = delete;
    CollectionParams(const char* st, int dp,
                     const std::array<const char*, 1>& ifaces) :
        subtree{st},
        depth{dp}, interfaces{ifaces}
    {}
};

inline void getCollection(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& uri,
                          const CollectionParams& params)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, uri](const boost::system::error_code ec,
                         const std::vector<std::string>& items) {
            if (ec == boost::system::errc::io_error)
            {
                asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
                asyncResp->res.jsonValue["Members@odata.count"] = 0;
                return;
            }
            if (ec)
            {
                BMCWEB_LOG_ERROR << "Dbus method call failed: " << ec;
                messages::internalError(asyncResp->res);
                return;
            }

            nlohmann::json& members = asyncResp->res.jsonValue["Members"];
            members = nlohmann::json::array();

            for (const std::string& item : items)
            {
                sdbusplus::message::object_path path(item);
                std::string name = path.filename();
                if (name.empty())
                {
                    BMCWEB_LOG_ERROR << "Received invalid path: " << item;
                    messages::internalError(asyncResp->res);
                    return;
                }
                members.push_back({{"@odata.id", uri + name}});
            }

            asyncResp->res.jsonValue["Members@odata.count"] = members.size();
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths", params.subtree,
        params.depth, params.interfaces);
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
