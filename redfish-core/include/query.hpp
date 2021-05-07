#pragma once

#include "utils/query_param.hpp"

namespace redfish
{
inline void
    setUpRedfishRoute(crow::App& app, const crow::Request& req,
                      const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (req.urlParams.size() > 0)
    {
        auto processParam =
            std::make_shared<query_param::ProcessParam>(app, asyncResp->res);
        if (!processParam->checkParameters(req.urlParams))
        {
            return;
        }
        processParam->setHandler(asyncResp->res.getHandler());
        asyncResp->res.setCompleteRequestHandler(
            [req, processParam]() { processParam->processAllParam(req); });
    }
}
} // namespace redfish