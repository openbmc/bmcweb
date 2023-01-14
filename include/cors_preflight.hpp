#pragma once

#include "app.hpp"
#include "http_request.hpp"
#include "http_response.hpp"

namespace cors_preflight
{
inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "<str>")
        .methods(boost::beast::http::verb::options)(
            [](const crow::Request& /*req*/,
               const std::shared_ptr<bmcweb::AsyncResp>&, const std::string&) {
        // An empty body handler that simply returns the headers bmcweb
        // uses This allows browsers to do their CORS preflight checks
        });
}
} // namespace cors_preflight
