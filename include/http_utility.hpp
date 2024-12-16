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

inline ContentType getContentType(std::string_view contentTypeHeader)
{
    using boost::spirit::x3::char_;
    using boost::spirit::x3::lit;
    using boost::spirit::x3::no_case;
    using boost::spirit::x3::omit;
    using boost::spirit::x3::parse;
    using boost::spirit::x3::space;
    using boost::spirit::x3::symbols;
    using boost::spirit::x3::uint_;

    const symbols<ContentType> knownMimeType{
        {"application/cbor", ContentType::CBOR},
        {"application/json", ContentType::JSON},
        {"application/octet-stream", ContentType::OctetStream},
        {"text/event-stream", ContentType::EventStream},
        {"text/html", ContentType::HTML}};

    ContentType ct;

    auto typeCharset = +(char_("a-zA-Z0-9.+-"));

    auto parameters =
        *(lit(';') >> *space >> typeCharset >> lit("=") >> typeCharset);
    auto parser = no_case[knownMimeType] >> omit[parameters];
    std::string_view::iterator begin = contentTypeHeader.begin();
    if (!parse(begin, contentTypeHeader.end(), parser, ct))
    {
        return ContentType::NoMatch;
    }
    if (begin != contentTypeHeader.end())
    {
        return ContentType::NoMatch;
    }

    return ct;
}

inline ContentType getPreferredContentType(
    std::string_view acceptsHeader, std::span<const ContentType> preferredOrder)
{
    using boost::spirit::x3::char_;
    using boost::spirit::x3::lit;
    using boost::spirit::x3::no_case;
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

    auto typeCharset = +(char_("a-zA-Z0-9.+-"));

    auto parameters = *(lit(';') >> typeCharset >> lit("=") >> typeCharset);
    auto mimeType = no_case[knownMimeType] |
                    omit[+typeCharset >> lit('/') >> +typeCharset];
    auto parser = +(mimeType >> omit[parameters >> -char_(',') >> *space]);
    if (!parse(acceptsHeader.begin(), acceptsHeader.end(), parser, ct))
    {
        return ContentType::NoMatch;
    }

    for (const ContentType parsedType : ct)
    {
        if (parsedType == ContentType::ANY)
        {
            return parsedType;
        }
        auto it = std::ranges::find(preferredOrder, parsedType);
        if (it != preferredOrder.end())
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

enum class Encoding
{
    ParseError,
    NoMatch,
    UnencodedBytes,
    GZIP,
    ZSTD,
    ANY, // represents *. Never returned.  Only used for string matching
};

inline Encoding
    getPreferredEncoding(std::string_view acceptEncoding,
                         const std::span<const Encoding> availableEncodings)
{
    if (acceptEncoding.empty())
    {
        return Encoding::UnencodedBytes;
    }

    using boost::spirit::x3::char_;
    using boost::spirit::x3::lit;
    using boost::spirit::x3::omit;
    using boost::spirit::x3::parse;
    using boost::spirit::x3::space;
    using boost::spirit::x3::symbols;
    using boost::spirit::x3::uint_;

    const symbols<Encoding> knownAcceptEncoding{{"gzip", Encoding::GZIP},
                                                {"zstd", Encoding::ZSTD},
                                                {"*", Encoding::ANY}};

    std::vector<Encoding> ct;

    auto parameters = *(lit(';') >> lit("q=") >> uint_ >> -(lit('.') >> uint_));
    auto typeCharset = char_("a-zA-Z.+-");
    auto encodeType = knownAcceptEncoding | omit[+typeCharset];
    auto parser = +(encodeType >> omit[parameters >> -char_(',') >> *space]);
    if (!parse(acceptEncoding.begin(), acceptEncoding.end(), parser, ct))
    {
        return Encoding::ParseError;
    }

    for (const Encoding parsedType : ct)
    {
        if (parsedType == Encoding::ANY)
        {
            if (!availableEncodings.empty())
            {
                return *availableEncodings.begin();
            }
        }
        auto it = std::ranges::find(availableEncodings, parsedType);
        if (it != availableEncodings.end())
        {
            return *it;
        }
    }

    // Fall back to raw bytes if it was allowed
    auto it = std::ranges::find(availableEncodings, Encoding::UnencodedBytes);
    if (it != availableEncodings.end())
    {
        return *it;
    }

    return Encoding::NoMatch;
}

} // namespace http_helpers
