// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include <boost/beast/http/verb.hpp>

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
        default:
            return "";
    }

    // Should never reach here
    return "";
}
