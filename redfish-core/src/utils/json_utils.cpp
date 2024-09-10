/*
Copyright (c) 2018 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#include "utils/json_utils.hpp"

#include "error_messages.hpp"
#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "http/parsing.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>

namespace redfish
{

namespace json_util
{

bool processJsonFromRequest(crow::Response& res, const crow::Request& req,
                            nlohmann::json& reqJson)
{
    JsonParseResult ret = parseRequestAsJson(req, reqJson);
    if (ret == JsonParseResult::BadContentType)
    {
        messages::unrecognizedRequestBody(res);
        return false;
    }
    reqJson = nlohmann::json::parse(req.body(), nullptr, false);

    if (reqJson.is_discarded())
    {
        messages::malformedJSON(res);
        return false;
    }

    return true;
}

uint64_t getEstimatedJsonSize(const nlohmann::json& root)
{
    if (root.is_null())
    {
        return 4;
    }
    if (root.is_number())
    {
        return 8;
    }
    if (root.is_boolean())
    {
        return 5;
    }
    if (root.is_string())
    {
        constexpr uint64_t quotesSize = 2;
        return root.get<std::string>().size() + quotesSize;
    }
    if (root.is_binary())
    {
        return root.get_binary().size();
    }
    const nlohmann::json::array_t* arr =
        root.get_ptr<const nlohmann::json::array_t*>();
    if (arr != nullptr)
    {
        uint64_t sum = 0;
        for (const auto& element : *arr)
        {
            sum += getEstimatedJsonSize(element);
        }
        return sum;
    }
    const nlohmann::json::object_t* object =
        root.get_ptr<const nlohmann::json::object_t*>();
    if (object != nullptr)
    {
        uint64_t sum = 0;
        for (const auto& [k, v] : *object)
        {
            constexpr uint64_t colonQuoteSpaceSize = 4;
            sum += k.size() + getEstimatedJsonSize(v) + colonQuoteSpaceSize;
        }
        return sum;
    }
    return 0;
}

} // namespace json_util
} // namespace redfish
