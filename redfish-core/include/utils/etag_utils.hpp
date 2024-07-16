// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "async_resp.hpp"
#include "http_response.hpp"
#include "json_utils.hpp"

#include <nlohmann/json.hpp>

#include <functional>
#include <memory>
#include <utility>

namespace redfish
{

namespace etag_utils
{

namespace details
{

inline void etagOmitDateTimeHandler(
    const std::function<void(crow::Response&)>& oldCompleteRequestHandler,
    crow::Response& res)
{
    size_t hash = json_util::hashJsonWithoutKey(res.jsonValue, "DateTime");
    std::string etag = std::format("\"{:08X}\"", hash);
    res.setCurrentOverrideEtag(etag);
    oldCompleteRequestHandler(res);
}

} // namespace details

inline void setEtagOmitDateTimeHandler(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    std::function<void(crow::Response&)> oldCompleteRequestHandler =
        asyncResp->res.releaseCompleteRequestHandler();
    asyncResp->res.setCompleteRequestHandler(
        std::bind_front(details::etagOmitDateTimeHandler,
                        std::move(oldCompleteRequestHandler)));
}

} // namespace etag_utils

} // namespace redfish
