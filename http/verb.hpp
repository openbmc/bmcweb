#pragma once

#include <boost/beast/http/verb.hpp>

#include <iostream>
#include <optional>
#include <string_view>

enum class HttpVerb
{
    Delete = 0,
    Get,
    Head,
    Options,
    Patch,
    Post,
    Put,
    Max,
};

static constexpr size_t maxVerbIndex = static_cast<size_t>(HttpVerb::Max) - 1U;

// MaxVerb + 1 is designated as the "not found" verb.  It is done this way
// to keep the BaseRule as a single bitfield (thus keeping the struct small)
// while still having a way to declare a route a "not found" route.
static constexpr size_t notFoundIndex = maxVerbIndex + 1;
static constexpr size_t methodNotAllowedIndex = notFoundIndex + 1;

inline std::optional<HttpVerb> httpVerbFromBoost(boost::beast::http::verb bv)
{
    switch (bv)
    {
        case boost::beast::http::verb::delete_:
            return HttpVerb::Delete;
        case boost::beast::http::verb::get:
            return HttpVerb::Get;
        case boost::beast::http::verb::head:
            return HttpVerb::Head;
        case boost::beast::http::verb::options:
            return HttpVerb::Options;
        case boost::beast::http::verb::patch:
            return HttpVerb::Patch;
        case boost::beast::http::verb::post:
            return HttpVerb::Post;
        case boost::beast::http::verb::put:
            return HttpVerb::Put;
        default:
            return std::nullopt;
    }

    return std::nullopt;
}

inline std::string_view httpVerbToString(HttpVerb verb)
{
    switch (verb)
    {
        case HttpVerb::Delete:
            return "DELETE";
        case HttpVerb::Get:
            return "GET";
        case HttpVerb::Head:
            return "HEAD";
        case HttpVerb::Patch:
            return "PATCH";
        case HttpVerb::Post:
            return "POST";
        case HttpVerb::Put:
            return "PUT";
        case HttpVerb::Options:
            return "OPTIONS";
        case HttpVerb::Max:
            return "";
    }

    // Should never reach here
    return "";
}
