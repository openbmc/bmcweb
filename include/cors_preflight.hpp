#pragma once

#include <app_class_decl.hpp>
#include <http_request_class_decl.hpp>
#include <http_response_class_decl.hpp>

using crow::App;

namespace cors_preflight
{
inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "<str>")
        .methods(boost::beast::http::verb::options)(
            [](const crow::Request&, const std::shared_ptr<bmcweb::AsyncResp>&,
               const std::string&) {
                // An empty body handler that simply returns the headers bmcweb
                // uses This allows browsers to do their CORS preflight checks
            });
}
} // namespace cors_preflight
