#pragma once

#include <app.hpp>
#include <http_request.hpp>
#include <http_response.hpp>

#include <string>

namespace redfish
{

inline void redfishGet(App& app, const crow::Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.jsonValue["v1"] = "/redfish/v1/";
}

inline void redfish404(App& app, const crow::Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& path)
{
    asyncResp->res.addHeader(boost::beast::http::field::allow, "");

    // If we fall to this route, we didn't have a more specific route, so return
    // 404
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    BMCWEB_LOG_ERROR << "404 on path " << path;

    boost::urls::string_value name = req.urlView.segments().back();
    std::string_view nameStr(name.data(), name.size());
    // Note, if we hit the wildcard route, we don't know the "type" the user was
    // actually requesting, but giving them a return with an empty string is
    // still better than nothing.
    messages::resourceNotFound(asyncResp->res, "", nameStr);
}

inline void requestRoutesRedfish(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/")
        .methods(boost::beast::http::verb::get)(
            std::bind_front(redfishGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/<path>")
    (std::bind_front(redfish404, std::ref(app)));
}

} // namespace redfish
