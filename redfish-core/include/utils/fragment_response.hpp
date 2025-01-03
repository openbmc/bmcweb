/*
 * SPDX-FileCopyrightText: Copyright (c) 2021-2024 NVIDIA CORPORATION &
 * AFFILIATES. All rights reserved. SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#pragma once
#include "bmcweb_config.h"

#include "async_resp.hpp"
#include "error_code.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "logging.hpp"
#include "utils/json_utils.hpp"

#include <nlohmann/json.hpp>

#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

namespace crow
{

inline nlohmann::json::json_pointer
    createJsonPointerFromFragment(std::string_view input)
{
    auto hashPos = input.find('#');
    if (hashPos == std::string_view::npos || hashPos + 1 >= input.size())
    {
        throw std::invalid_argument("No fragment found after #");
    }

    std::string_view fragment = input.substr(hashPos + 1);

    std::vector<std::string> parts;
    std::stringstream ss{std::string(fragment)};
    std::string token;

    while (std::getline(ss, token, '/'))
    {
        if (!token.empty())
        {
            parts.push_back(token);
        }
    }

    std::string pointerPath;
    for (const auto& part : parts)
    {
        pointerPath += "/";
        pointerPath += part;
    }

    return nlohmann::json::json_pointer(pointerPath);
}

class MultiRouteResp : public std::enable_shared_from_this<MultiRouteResp>
{
  public:
    MultiRouteResp(std::shared_ptr<bmcweb::AsyncResp> finalResIn) :
        finalRes(std::move(finalResIn))
    {}

    void addAwaitingResponse(const std::shared_ptr<bmcweb::AsyncResp>& res,
                             const nlohmann::json::json_pointer& finalLocation)
    {
        res->res.setCompleteRequestHandler(std::bind_front(
            placeResultStatic, shared_from_this(), finalLocation));
    }

    void placeResult(const nlohmann::json::json_pointer& locationToPlace,
                     crow::Response& res)
    {
        BMCWEB_LOG_DEBUG("got fragment response");
        redfish::propogateError(finalRes->res, res);
        if (!res.jsonValue.is_object() || res.jsonValue.empty())
        {
            return;
        }
        nlohmann::json& finalObj = finalRes->res.jsonValue[locationToPlace];
        finalObj = std::move(res.jsonValue);
    }

    static void
        handleFragmentRules(const std::shared_ptr<crow::Request>& req,
                            const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::vector<BaseRule*> fragments,
                            const std::vector<std::string> params,
                            const crow::Response& resIn)
    {
        asyncResp->res.jsonValue = std::move(resIn.jsonValue);
        auto multi = std::make_shared<MultiRouteResp>(asyncResp);
        auto rsp = std::make_shared<bmcweb::AsyncResp>();
        BMCWEB_LOG_DEBUG(
            "Handling fragment rules: setting completion handler on {}",
            logPtr(&rsp->res));

        for (auto fragment : fragments)
        {
            BaseRule& fragmentRule = *fragment;
            BMCWEB_LOG_DEBUG("Matched fragment rule '{}' {} / {}",
                             fragmentRule.rule, req->methodString(),
                             fragmentRule.getMethods());
            multi->addAwaitingResponse(
                rsp, createJsonPointerFromFragment(fragmentRule.rule));
            fragmentRule.handle(*req, rsp, params);
        }
    }

  private:
    static void
        placeResultStatic(const std::shared_ptr<MultiRouteResp>& multi,
                          const nlohmann::json::json_pointer& locationToPlace,
                          crow::Response& res)
    {
        multi->placeResult(locationToPlace, res);
    }

    std::shared_ptr<bmcweb::AsyncResp> finalRes;
};

inline bool handleMultiFragmentRouting(
    const std::shared_ptr<Request>& req,
    const std::vector<BaseRule*>& fragments,
    const std::vector<std::string>& params,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    std::shared_ptr<bmcweb::AsyncResp>& multiAsyncResp)
{
    auto multiResp = std::make_shared<bmcweb::AsyncResp>();
    multiResp->res.setCompleteRequestHandler(std::bind_front(
        MultiRouteResp::handleFragmentRules,
        std::make_shared<crow::Request>(*req), asyncResp, fragments, params));
    multiAsyncResp = multiResp;
    return true;
}
} // namespace crow
