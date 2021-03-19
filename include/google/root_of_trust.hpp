#pragma once

#include <app.hpp>
#include <async_resp.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/container/flat_set.hpp>
#include <error_messages.hpp>
#include <event_service_manager.hpp>
#include <nlohmann/json.hpp>
#include <resource_messages.hpp>
#include <sdbusplus/message/types.hpp>
#include <utils/json_utils.hpp>

#include <filesystem>
#include <fstream>

namespace crow
{
namespace google_rot
{

inline void requestRoutes(App& app) {
    BMCWEB_ROUTE(app, "/google/v1/")
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request &req, crow::Response& res) {
                BMCWEB_LOG_DEBUG << "Request: " << req.keepAlive();
                res.jsonValue["@odata.type"] =
                    "#googleServiceRoot.v1_0_0.googleServiceRoot";
                res.jsonValue["@odata.id"] = "/google/v1/";
                res.jsonValue["Id"] = "Google Rest RootService";
                res.jsonValue["Name"] = "Google Service Root";
                res.jsonValue["RootOfTrust"] = {
                    {"@odata.id", "/google/v1/RootOfTrust/"}};
                res.end();
            });

}

} // namespace google_rot
} // namespace crow
