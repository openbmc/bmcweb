#pragma once

#include <http_request.h>
#include <http_response.h>

#include <webserver_common.hpp>

namespace cors_preflight
{
void requestRoutes(CrowApp& app)
{
    BMCWEB_ROUTE(app, "<str>")
        .methods(boost::beast::http::verb::options)(
            [](const crow::Request& req, crow::Response& res) {
                // An empty body handler that simply returns the headers bmcweb
                // uses This allows browsers to do their CORS preflight checks
                res.end();
            });

}
} // namespace cors
