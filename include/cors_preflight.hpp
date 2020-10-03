#pragma once

#include <app.hpp>
#include <http_request.hpp>
#include <http_response.hpp>

namespace cors_preflight
{
void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "<str>")
        .methods(boost::beast::http::verb::options)(
            [](const crow::Request& req, crow::Response& res) {
                // An empty body handler that simply returns the headers bmcweb
                // uses This allows browsers to do their CORS preflight checks
                res.end();
            });
}
} // namespace cors_preflight
