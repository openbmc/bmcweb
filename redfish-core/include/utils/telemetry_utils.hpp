#pragma once

#include "dbus_utility.hpp"

#include <boost/assign.hpp>
#include <boost/bimap.hpp>

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

inline void
    getReportCollection(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& uri)
{
    const std::array<const char*, 1> interfaces = {reportInterface};

    crow::connections::systemBus->async_method_call(
        [asyncResp, uri](const boost::system::error_code ec,
                         const std::vector<std::string>& reports) {
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

            for (const std::string& report : reports)
            {
                sdbusplus::message::object_path path(report);
                std::string name = path.filename();
                if (name.empty())
                {
                    BMCWEB_LOG_ERROR << "Received invalid path: " << report;
                    messages::internalError(asyncResp->res);
                    return;
                }
                members.push_back({{"@odata.id", uri + name}});
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

inline bool isIdValid(crow::Response& res, const std::string& id)
{
    constexpr const char* allowedCharactersInName =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
    if (id.empty() ||
        id.find_first_not_of(allowedCharactersInName) != std::string::npos)
    {
        BMCWEB_LOG_ERROR << "Failed to match " << id
                         << " with allowed character "
                         << allowedCharactersInName;
        messages::propertyValueIncorrect(res, "Id", id);
        return false;
    }
    return true;
}

inline std::optional<std::string>
    getReportNameFromReportDefinitionUri(const std::string& uri)
{
    std::optional<std::string> res;
    std::string metricReportDefinitonsUri =
        "/redfish/v1/TelemetryService/MetricReportDefinitions/";
    if (uri.length() > metricReportDefinitonsUri.length())
    {
        if (uri.find(metricReportDefinitonsUri) != std::string::npos)
        {
            res = uri.substr(metricReportDefinitonsUri.length());
        }
    }
    return res;
}

inline bool getChassisSensorNode(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::vector<std::string>& uris,
    boost::container::flat_set<std::pair<std::string, std::string>>& matched)
{
    for (size_t i = 0; i < uris.size(); i++)
    {
        const std::string& uri = uris[i];
        std::string chassis;
        std::string node;

        if (!boost::starts_with(uri, "/redfish/v1/Chassis/") ||
            !dbus::utility::getNthStringFromPath(uri, 3, chassis) ||
            !dbus::utility::getNthStringFromPath(uri, 4, node))
        {
            BMCWEB_LOG_ERROR << "Failed to get chassis and sensor Node "
                                "from "
                             << uri;
            messages::propertyValueIncorrect(
                asyncResp->res, uri, "MetricProperties/" + std::to_string(i));
            return false;
        }

        if (boost::ends_with(node, "#"))
        {
            node.pop_back();
        }

        matched.emplace(std::move(chassis), std::move(node));
    }
    return true;
}

using bimapStrStr = boost::bimap<std::string, std::string>;
bimapStrStr triggerActionsMap = boost::assign::list_of<bimapStrStr::relation>(
    "RedfishMetricReport", "UpdateReport");

inline std::string redfishActionToDbusAction(const std::string& redfishAction)
{
    if (auto it = triggerActionsMap.left.find(redfishAction);
        it != triggerActionsMap.left.end())
    {
        return it->second;
    }
    return redfishAction;
}

inline std::string dbusActionToRedfishAction(const std::string& dbusAction)
{
    if (auto it = triggerActionsMap.right.find(dbusAction);
        it != triggerActionsMap.right.end())
    {
        return it->first;
    }
    return dbusAction;
}

} // namespace telemetry
} // namespace redfish
