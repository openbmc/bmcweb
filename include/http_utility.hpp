#pragma once

#include <boost/spirit/home/x3.hpp>

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <ostream>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

// IWYU pragma: no_include <ctype.h>

namespace http_helpers
{

enum class ContentType
{
    NoMatch,
    ANY, // Accepts: */*
    CBOR,
    HTML,
    JSON,
    OctetStream,
    EventStream,
};

inline ContentType getPreferredContentType(
    std::string_view header, std::span<const ContentType> preferedOrder)
{
    using boost::spirit::x3::char_;
    using boost::spirit::x3::lit;
    using boost::spirit::x3::omit;
    using boost::spirit::x3::parse;
    using boost::spirit::x3::space;
    using boost::spirit::x3::symbols;
    using boost::spirit::x3::uint_;

    const symbols<ContentType> knownMimeType{
        {"application/cbor", ContentType::CBOR},
        {"application/json", ContentType::JSON},
        {"application/octet-stream", ContentType::OctetStream},
        {"text/html", ContentType::HTML},
        {"text/event-stream", ContentType::EventStream},
        {"*/*", ContentType::ANY}};

    std::vector<ContentType> ct;

    auto parameters = *(lit(';') >> lit("q=") >> uint_ >> -(lit('.') >> uint_));
    auto typeCharset = char_("a-zA-Z.+-");
    auto mimeType = knownMimeType |
                    omit[+typeCharset >> lit('/') >> +typeCharset];
    auto parser = +(mimeType >> omit[parameters >> -char_(',') >> *space]);
    if (!parse(header.begin(), header.end(), parser, ct))
    {
        return ContentType::NoMatch;
    }

    for (const ContentType parsedType : ct)
    {
        if (parsedType == ContentType::ANY)
        {
            return parsedType;
        }
        auto it = std::ranges::find(preferedOrder, parsedType);
        if (it != preferedOrder.end())
        {
            return *it;
        }
    }

    return ContentType::NoMatch;
}

inline bool isContentTypeAllowed(std::string_view header, ContentType type,
                                 bool allowWildcard)
{
    auto types = std::to_array({type});
    ContentType allowed = getPreferredContentType(header, types);
    if (allowed == ContentType::ANY)
    {
        return allowWildcard;
    }

    return type == allowed;
}

} // namespace http_helpers
