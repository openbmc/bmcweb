/*
// Copyright (c) 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#include "utils/json_utils.hpp"

namespace redfish
{

namespace json_util
{

bool processJsonFromRequest(crow::Response& res, const crow::Request& req,
                            nlohmann::json& reqJson)
{
    reqJson = nlohmann::json::parse(req.body, nullptr, false);

    if (reqJson.is_discarded())
    {
        messages::malformedJSON(res);

        res.end();

        return false;
    }

    return true;
}

uint64_t getEstimatedJsonSize(const nlohmann::json& root)
{
    if (root.is_number())
    {
        return 8;
    }
    if (root.is_boolean() || root.is_null())
    {
        return 1;
    }
    if (root.is_string())
    {
        return root.get<std::string>().size();
    }
    if (root.is_binary())
    {
        return root.get_binary().size();
    }
    if (root.is_array())
    {
        uint64_t sum = 0;
        for (const auto& element : root)
        {
            sum += getEstimatedJsonSize(element);
            if (sum > crow::httpResponseBodyLimit)
            {
                return crow::httpResponseBodyLimit;
            }
        }
        return sum;
    }
    if (root.is_object())
    {
        uint64_t sum = 0;
        for (const auto& [k, v] : root.items())
        {
            sum += k.size() + getEstimatedJsonSize(v);
            if (sum > crow::httpResponseBodyLimit)
            {
                return crow::httpResponseBodyLimit;
            }
        }
        return sum;
    }
    return 0;
}

} // namespace json_util
} // namespace redfish
