#pragma once

#include "http/http_request.hpp"
#include "logging.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <string_view>

enum class JsonParseResult
{
    BadContentType,
    BadJsonData,
    Success,
};

inline bool isJsonContentType(std::string_view contentTypeIn)
{
    std::string contentType(contentTypeIn);
    std::transform(contentType.begin(), contentType.end(), contentType.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return contentType == "application/json" ||
           contentType == "application/json; charset=utf-8";
}

inline JsonParseResult parseRequestAsJson(const crow::Request& req,
                                          nlohmann::json& jsonOut)
{
    if (!isJsonContentType(
            req.getHeaderValue(boost::beast::http::field::content_type)))
    {
        BMCWEB_LOG_WARNING << "Failed to parse content type on request";
#ifndef BMCWEB_INSECURE_IGNORE_CONTENT_TYPE
        return JsonParseResult::BadContentType;
#endif
    }
    jsonOut = nlohmann::json::parse(req.body(), nullptr, false);
    if (jsonOut.is_discarded())
    {
        BMCWEB_LOG_WARNING << "Failed to parse json in request";
        return JsonParseResult::BadJsonData;
    }

    return JsonParseResult::Success;
}
