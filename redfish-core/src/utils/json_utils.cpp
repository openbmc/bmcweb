// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2018 Intel Corporation
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
