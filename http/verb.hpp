#pragma once

#include <boost/beast/http/verb.hpp>

#include <optional>

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
static constexpr const size_t notFoundIndex = maxVerbIndex + 1;
static constexpr const size_t methodNotAllowedIndex = notFoundIndex + 1;

inline std::optional<HttpVerb> httpVerbFromBoost(boost::beast::http::verb bv)
{
    if (bv == boost::beast::http::verb::delete_)
    {
        return HttpVerb::Delete;
    }
    if (bv == boost::beast::http::verb::get)
    {
        return HttpVerb::Get;
    }
    if (bv == boost::beast::http::verb::head)
    {
        return HttpVerb::Head;
    }
    if (bv == boost::beast::http::verb::options)
    {
        return HttpVerb::Options;
    }
    if (bv == boost::beast::http::verb::patch)
    {
        return HttpVerb::Patch;
    }
    if (bv == boost::beast::http::verb::post)
    {
        return HttpVerb::Post;
    }
    if (bv == boost::beast::http::verb::put)
    {
        return HttpVerb::Put;
    }
    return std::nullopt;
};

inline std::string httpVerbToString(HttpVerb verb)
{
    if (verb == HttpVerb::Delete)
    {
        return "DELETE";
    }
    if (verb == HttpVerb::Get)
    {
        return "GET";
    }
    if (verb == HttpVerb::Head)
    {
        return "HEAD";
    }
    if (verb == HttpVerb::Patch)
    {
        return "PATCH";
    }
    if (verb == HttpVerb::Post)
    {
        return "POST";
    }
    if (verb == HttpVerb::Put)
    {
        return "PUT";
    }

    // Return the least used option as last resort, given it's unlikely to cause
    // conflicts.
    return "OPTIONS";
}
