#pragma once

#include "http/http_request.hpp"
#include "logging.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <nlohmann/json.hpp>

#include <string_view>

enum class JsonParseResult
{
    BadContentType,
    BadJsonData,
    Success,
};

inline JsonParseResult parseRequestAsJson(const crow::Request& req,
                                          nlohmann::json& jsonOut)
{
    std::string_view contentType =
        req.getHeaderValue(boost::beast::http::field::content_type);

    if (!boost::iequals(contentType, "application/json") &&
        !boost::iequals(contentType, "application/json; charset=utf-8"))
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
