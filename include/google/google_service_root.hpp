#pragma once

#include <app.hpp>
#include <async_resp.hpp>
#include <boost/algorithm/hex.hpp>
#include <error_messages.hpp>
#include <nlohmann/json.hpp>
#include <utils/collection.hpp>
#include <utils/json_utils.hpp>

#include <vector>

namespace crow
{
namespace google_api
{
inline void fillRoTList(nlohmann::json::object_t& rots)
{
    rots["@odata.id"] = "/google/v1/RootOfTrustCollection";
    rots["@odata.type"] = "#RootOfTrustCollection.RootOfTrustCollection";
    nlohmann::json members = nlohmann::json::array();
    // Hardcoding a single RoT right now.
    members.push_back(
        {{"@odata.id",
          "/google/v1/RootOfTrustCollection/" BMCWEB_GOOGLE_SINGLETON_ROTID}});

    rots["Members"] = members;
    rots["Members@odata.count"] = members.size();
}

inline bool validateRoTId(const std::string& rotId)
{
    return rotId == BMCWEB_GOOGLE_SINGLETON_ROTID;
}

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
                nlohmann::json::object_t rots;
                fillRoTList(rots);
                asyncResp->res.jsonValue["RootOfTrustCollection"] = rots;
            });

    BMCWEB_ROUTE(app, "/google/v1/RootOfTrustCollection")
        .privileges({{"Login"}})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                nlohmann::json::object_t rots;
                fillRoTList(rots);
                asyncResp->res.jsonValue = rots;
            });

    BMCWEB_ROUTE(app, "/google/v1/RootOfTrustCollection/<str>")
        .privileges({{"Login"}})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& rotId) {
                if (!validateRoTId(rotId))
                {
                    BMCWEB_LOG_ERROR << "RootOfTrust ID not recognized ";
                    redfish::messages::resourceNotFound(asyncResp->res,
                                                        "RootOfTrust", rotId);

                    return;
                }

                asyncResp->res.jsonValue["@odata.type"] =
                    "#RootOfTrust.v1_0_0.RootOfTrust";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/google/v1/RootOfTrustCollection/" + rotId;

                nlohmann::json& status =
                    asyncResp->res.jsonValue["Status"].emplace_back(
                        nlohmann::json::object());
                status["State"] = "Enabled";
                asyncResp->res.jsonValue["Id"] = rotId;
                asyncResp->res.jsonValue["Name"] = "RootOfTrust-" + rotId;
                asyncResp->res
                    .jsonValue["Actions"]["#RootOfTrust.SendCommand"] = {
                    {"target", "/google/v1/RootOfTrustCollection/" + rotId +
                                   "/Actions/RootOfTrust.SendCommand"}};
            });

    BMCWEB_ROUTE(
        app,
        "/google/v1/RootOfTrustCollection/<str>/Actions/RootOfTrust.SendCommand")
        .privileges({{"Login"}})
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& rotId) {
                if (!validateRoTId(rotId))
                {
                    BMCWEB_LOG_ERROR << "RootOfTrust ID not recognized ";
                    redfish::messages::resourceNotFound(
                        asyncResp->res, "RootOfTrust.SendCommand", rotId);

                    return;
                }

                std::string command;
                if (!redfish::json_util::readJsonAction(req, asyncResp->res,
                                                        "Command", command))
                {

                    BMCWEB_LOG_DEBUG << "Missing property Command.";
                    redfish::messages::actionParameterMissing(
                        asyncResp->res, "SendCommand", "Command");
                    return;
                }
                std::string str = boost::algorithm::unhex(command);
                std::vector<uint8_t> bytes(str.begin(), str.end());

                crow::connections::systemBus->async_method_call(
                    [command, asyncResp](const boost::system::error_code ec,
                                         std::vector<uint8_t>& resp_bytes) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR
                                << "RootOfTrust.Actions.SendCommand failed: "
                                << ec.message();
                            redfish::messages::internalError(asyncResp->res);
                            return;
                        }
                        std::string resp;
                        boost::algorithm::hex(resp_bytes.begin(),
                                              resp_bytes.end(),
                                              std::back_inserter(resp));
                        asyncResp->res.jsonValue["CommandResponse"] = resp;
                    },
                    "xyz.openbmc_project.Control.Hoth",
                    "/xyz/openbmc_project/Control/Hoth",
                    "xyz.openbmc_project.Control.Hoth", "SendHostCommand",
                    bytes);
            });
}

} // namespace google_api
} // namespace crow
