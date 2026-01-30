// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "bmcweb_config.h"

#include "http/http_request.hpp"
#include "http_utility.hpp"
#include "logging.hpp"

#include <boost/beast/http/field.hpp>
#include <nlohmann/json.hpp>

#include <string_view>

enum class JsonParseResult
{
    BadContentType,
    BadJsonData,
    Success,
};

inline bool isJsonContentType(std::string_view contentType)
{
    return http_helpers::getContentType(contentType).contentType ==
           http_helpers::ContentType::JSON;
}

inline JsonParseResult parseRequestAsJson(const crow::Request& req,
                                          nlohmann::json& jsonOut)
{
    std::string_view contentType =
        req.getHeaderValue(boost::beast::http::field::content_type);
    if (!isJsonContentType(contentType))
    {
        BMCWEB_LOG_WARNING("Request was not json content type, ignoring");
        if constexpr (!BMCWEB_INSECURE_IGNORE_CONTENT_TYPE)
        {
            return JsonParseResult::BadContentType;
        }
    }
    jsonOut = nlohmann::json::parse(req.body(), nullptr, false);
    if (jsonOut.is_discarded())
    {
        BMCWEB_LOG_WARNING("Failed to parse json in request");
        return JsonParseResult::BadJsonData;
    }

    return JsonParseResult::Success;
}
