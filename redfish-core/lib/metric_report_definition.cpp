#include <app_class_decl.hpp>
using crow::App;
#include "../../include/dbus_utility.hpp"
#include "metric_report_definition.hpp"
#include "metric_report.hpp"

namespace redfish
{

namespace telemetry
{
AddReport::AddReport(AddReportArgs argsIn,
              const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) :
        asyncResp(asyncResp),
        args{std::move(argsIn)} {}

AddReport::~AddReport()
{
    if (asyncResp->res.result() != boost::beast::http::status::ok)
    {
        return;
    }

    telemetry::ReadingParameters readingParams;
    readingParams.reserve(args.metrics.size());

    for (const auto& [id, uris] : args.metrics)
    {
        for (size_t i = 0; i < uris.size(); i++)
        {
            const std::string& uri = uris[i];
            auto el = uriToDbus.find(uri);
            if (el == uriToDbus.end())
            {
                BMCWEB_LOG_ERROR << "Failed to find DBus sensor "
                                    "corresponding to URI "
                                  << uri;
                messages::propertyValueNotInList(asyncResp->res, uri,
                                                  "MetricProperties/" +
                                                      std::to_string(i));
                return;
            }

            const std::string& dbusPath = el->second;
            readingParams.emplace_back(dbusPath, "SINGLE", id, uri);
        }
    }
    const std::shared_ptr<bmcweb::AsyncResp> aResp = asyncResp;
    crow::connections::systemBus->async_method_call(
        [aResp, name = args.name, uriToDbus = std::move(uriToDbus)](
            const boost::system::error_code ec, const std::string&) {
            if (ec == boost::system::errc::file_exists)
            {
                messages::resourceAlreadyExists(
                    aResp->res, "MetricReportDefinition", "Id", name);
                return;
            }
            if (ec == boost::system::errc::too_many_files_open)
            {
                messages::createLimitReachedForResource(aResp->res);
                return;
            }
            if (ec == boost::system::errc::argument_list_too_long)
            {
                nlohmann::json metricProperties = nlohmann::json::array();
                for (const auto& [uri, _] : uriToDbus)
                {
                    metricProperties.emplace_back(uri);
                }
                messages::propertyValueIncorrect(
                    aResp->res, metricProperties, "MetricProperties");
                return;
            }
            if (ec)
            {
                messages::internalError(aResp->res);
                BMCWEB_LOG_ERROR << "respHandler DBus error " << ec;
                return;
            }

            messages::created(aResp->res);
        },
        telemetry::service, "/xyz/openbmc_project/Telemetry/Reports",
        "xyz.openbmc_project.Telemetry.ReportManager", "AddReport",
        "TelemetryService/" + args.name, args.reportingType,
        args.emitsReadingsUpdate, args.logToMetricReportsCollection,
        args.interval, readingParams);
}

void AddReport::insert(const boost::container::flat_map<std::string, std::string>& el)
{
    uriToDbus.insert(el.begin(), el.end());
}
} // telemetry

}