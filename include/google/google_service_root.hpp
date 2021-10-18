#pragma once

#include <app.hpp>
#include <async_resp.hpp>
#include <nlohmann/json.hpp>
#include <utils/json_utils.hpp>

#include <vector>

namespace crow
{
namespace google_api
{

inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/google/v1/")
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#GoogleServiceRoot.v1_0_0.GoogleServiceRoot";
                asyncResp->res.jsonValue["@odata.id"] = "/google/v1";
                asyncResp->res.jsonValue["Id"] = "Google Rest RootService";
                asyncResp->res.jsonValue["Name"] = "Google Service Root";
                asyncResp->res.jsonValue["Version"] = "1.0.0";
                asyncResp->res.jsonValue["Chassis"] = {"@odata.id",
                                                       "google/v1/Chassis"};
                asyncResp->res.jsonValue["RootOfTrust"] = {
                    "@odata.id", "google/v1/Chassis/RootOfTrust"};
            });

    BMCWEB_ROUTE(app, "/google/v1/Chassis")
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#GoogleChassis.v1_0_0.GoogleChassis";
                asyncResp->res.jsonValue["@odata.id"] = "/google/v1/Chassis/";
                asyncResp->res.jsonValue["Id"] = "Chassis";
                asyncResp->res.jsonValue["Name"] = "Chassis";
            });

    BMCWEB_ROUTE(app, "/google/v1/Chassis/RootOfTrust/")
        .methods(
            boost::beast::http::verb::
                get)([](const crow::Request&,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
            asyncResp->res.jsonValue["@odata.type"] =
                "#RootOfTrust.v1_0_0.RootOfTrust";
            asyncResp->res.jsonValue["@odata.id"] =
                "/google/v1/Chassis/RootOfTrust";
            asyncResp->res.jsonValue["Id"] = "RootOfTrust";
            asyncResp->res.jsonValue["Name"] = "RootOfTrust";
            asyncResp->res.jsonValue["Actions"]["#Mailbox.SendCommand"] = {
                {"target",
                 "/google/v1/Chassis/RootOfTrust/Actions/Mailbox.SendCommand"}};
        });

    BMCWEB_ROUTE(app,
                 "/google/v1/Chassis/RootOfTrust/Actions/Mailbox.SendCommand")
        .methods(
            boost::beast::http::verb::
                post)([](const crow::Request& req,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
            std::vector<uint8_t> reqBytes;
            if (!redfish::json_util::readJson(req, asyncResp->res,
                                              "requestBytes", reqBytes))
            {
                asyncResp->res.jsonValue["readJson"] = "failed to read request";
                BMCWEB_LOG_DEBUG << "couldn't parse the request";
                return;
            }
            crow::connections::systemBus->async_method_call(
                [reqBytes, asyncResp](const boost::system::error_code ec,
                                      std::vector<uint8_t>& resp) {
                    if (ec)
                    {
                        BMCWEB_LOG_ERROR
                            << "Chassis.RootOfTrust.Actions.Mailbox Failed: "
                            << ec;
                        asyncResp->res.jsonValue["RootOfTrustErrorMessage"] =
                            ec.message();
                        redfish::messages::internalError(asyncResp->res);
                        return;
                    }
                    asyncResp->res.jsonValue["RootOfTrustResponse"] = resp;
                },
                "xyz.openbmc_project.Control.Hoth",
                "/xyz/openbmc_project/Control/Hoth",
                "xyz.openbmc_project.Control.Hoth", "SendHostCommand",
                reqBytes);
        });
}

} // namespace google_api
} // namespace crow
