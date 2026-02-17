// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "telemetry_readings.hpp"
#include "utils/collection.hpp"
#include "utils/telemetry_utils.hpp"

#include <asm-generic/errno.h>

#include <boost/beast/http/verb.hpp>
#include <boost/url/format.hpp>
#include <boost/url/url.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/asio/property.hpp>

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace redfish
{

inline void requestRoutesMetricReportCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/TelemetryService/MetricReports/")
        .privileges(redfish::privileges::getMetricReportCollection)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                if (!redfish::setUpRedfishRoute(app, req, asyncResp))
                {
                    return;
                }

                asyncResp->res.jsonValue["@odata.type"] =
                    "#MetricReportCollection.MetricReportCollection";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/TelemetryService/MetricReports";
                asyncResp->res.jsonValue["Name"] = "Metric Report Collection";
                asyncResp->res.jsonValue["Description"] =
                    "Collection of Metric Reports";
                constexpr std::array<std::string_view, 1> interfaces{
                    telemetry::reportInterface};
                collection_util::getCollectionMembers(
                    asyncResp,
                    boost::urls::url(
                        "/redfish/v1/TelemetryService/MetricReports"),
                    interfaces,
                    "/xyz/openbmc_project/Telemetry/Reports/TelemetryService");
            });
}

inline void requestRoutesMetricReport(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/TelemetryService/MetricReports/<str>/")
        .privileges(redfish::privileges::getMetricReport)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& id) {
                if (!redfish::setUpRedfishRoute(app, req, asyncResp))
                {
                    return;
                }
                const std::string reportPath = telemetry::getDbusReportPath(id);
                dbus::utility::async_method_call(
                    asyncResp,
                    [asyncResp, id,
                     reportPath](const boost::system::error_code& ec) {
                        if (ec.value() == EBADR ||
                            ec == boost::system::errc::host_unreachable)
                        {
                            messages::resourceNotFound(asyncResp->res,
                                                       "MetricReport", id);
                            return;
                        }
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR("respHandler DBus error {}", ec);
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        sdbusplus::asio::getProperty<
                            telemetry::TimestampReadings>(
                            *crow::connections::systemBus, telemetry::service,
                            reportPath, telemetry::reportInterface, "Readings",
                            [asyncResp,
                             id](const boost::system::error_code& ec2,
                                 const telemetry::TimestampReadings& ret) {
                                if (ec2)
                                {
                                    BMCWEB_LOG_ERROR(
                                        "respHandler DBus error {}", ec2);
                                    messages::internalError(asyncResp->res);
                                    return;
                                }

                                telemetry::fillReport(asyncResp->res.jsonValue,
                                                      id, ret);
                            });
                    },
                    telemetry::service, reportPath, telemetry::reportInterface,
                    "Update");
            });
}
} // namespace redfish
