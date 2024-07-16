#pragma once

#include "http/http_request.hpp"
#include "logging.hpp"
#include "str_utility.hpp"

#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/home/x3.hpp>
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

inline bool isJsonContentType(std::string_view contentType)
{
    return bmcweb::asciiIEquals(contentType, "application/json") ||
           bmcweb::asciiIEquals(contentType,
                                "application/json; charset=utf-8") ||
           bmcweb::asciiIEquals(contentType, "application/json;charset=utf-8");
}

inline JsonParseResult parseRequestAsJson(const crow::Request& req,
                                          nlohmann::json& jsonOut)
{
    if (!isJsonContentType(
            req.getHeaderValue(boost::beast::http::field::content_type)))
    {
        BMCWEB_LOG_WARNING("Failed to parse content type on request");
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

struct ByteRange
{
    size_t start = 0;
    std::optional<size_t> end;
};

BOOST_FUSION_ADAPT_STRUCT(ByteRange, start, end);

inline std::optional<ByteRange> parseRangeHeader(const crow::Request& req)
{
    std::string_view range =
        req.getHeaderValue(boost::beast::http::field::range);
    if (range.empty())
    {
        return std::nullopt;
    }

    ByteRange parsedRange;
    using boost::spirit::x3::lit;
    using boost::spirit::x3::parse;
    using boost::spirit::x3::uint64;

    auto parser = lit("bytes ") >> uint64 >> -(lit('-') >> uint64);
    if (!parse(range.begin(), range.end(), parser, parsedRange))
    {
        return std::nullopt;
    }
    return {parsedRange};
}
