// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include <boost/parser/parser.hpp>

#include <algorithm>
#include <array>
#include <ranges>
#include <span>
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
    namespace bp = boost::parser;

    // A plain alternation of literals is used instead of bp::symbols to avoid
    // instantiating the (large) symbol trie.
    auto const knownMimeType =
        (bp::lit("application/cbor") >> bp::attr(ContentType::CBOR)) |
        (bp::lit("application/json") >> bp::attr(ContentType::JSON)) |
        (bp::lit("application/octet-stream") >>
         bp::attr(ContentType::OctetStream)) |
        (bp::lit("text/event-stream") >> bp::attr(ContentType::EventStream)) |
        (bp::lit("text/html") >> bp::attr(ContentType::HTML));

    ContentType ct = ContentType::NoMatch;

    auto const typeCharset = +(bp::char_('a', 'z') | bp::char_('A', 'Z') |
                               bp::char_('0', '9') | bp::char_(".+-"));

    auto const parameters = *(bp::lit(';') >> *bp::ws >> typeCharset >>
                              bp::lit('=') >> typeCharset);
    auto const parser = bp::no_case[knownMimeType] >> bp::omit[parameters];
    std::string_view::iterator begin = contentTypeHeader.begin();
    if (!bp::prefix_parse(begin, contentTypeHeader.end(), parser, ct))
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
    namespace bp = boost::parser;

    // A plain alternation of literals is used instead of bp::symbols to avoid
    // instantiating the (large) symbol trie.
    auto const knownMimeType =
        (bp::lit("application/cbor") >> bp::attr(ContentType::CBOR)) |
        (bp::lit("application/json") >> bp::attr(ContentType::JSON)) |
        (bp::lit("application/octet-stream") >>
         bp::attr(ContentType::OctetStream)) |
        (bp::lit("text/html") >> bp::attr(ContentType::HTML)) |
        (bp::lit("text/event-stream") >> bp::attr(ContentType::EventStream)) |
        (bp::lit("*/*") >> bp::attr(ContentType::ANY));

    std::vector<ContentType> ct;

    auto const typeCharset = +(bp::char_('a', 'z') | bp::char_('A', 'Z') |
                               bp::char_('0', '9') | bp::char_(".+-"));

    auto const parameters =
        *(bp::lit(';') >> typeCharset >> bp::lit('=') >> typeCharset);
    // Unknown mime types produce NoMatch, which is ignored below
    auto const mimeType =
        bp::no_case[knownMimeType] |
        (bp::omit[+typeCharset >> bp::lit('/') >> +typeCharset] >>
         bp::attr(ContentType::NoMatch));
    auto const parser =
        +(mimeType >> bp::omit[parameters >> -bp::char_(',') >> *bp::ws]);
    std::string_view::iterator begin = acceptsHeader.begin();
    if (!bp::prefix_parse(begin, acceptsHeader.end(), parser, ct))
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

inline Encoding getPreferredEncoding(
    std::string_view acceptEncoding,
    const std::span<const Encoding> availableEncodings)
{
    if (acceptEncoding.empty())
    {
        return Encoding::UnencodedBytes;
    }

    namespace bp = boost::parser;

    // A plain alternation of literals is used instead of bp::symbols to avoid
    // instantiating the (large) symbol trie.
    auto const knownAcceptEncoding =
        (bp::lit("gzip") >> bp::attr(Encoding::GZIP)) |
        (bp::lit("zstd") >> bp::attr(Encoding::ZSTD)) |
        (bp::lit('*') >> bp::attr(Encoding::ANY));

    std::vector<Encoding> ct;

    auto const parameters = *(bp::lit(';') >> bp::lit("q=") >> bp::uint_ >>
                              -(bp::lit('.') >> bp::uint_));
    auto const typeCharset =
        bp::char_('a', 'z') | bp::char_('A', 'Z') | bp::char_(".+-");
    // Unknown encodings produce NoMatch, which is ignored below
    auto const encodeType =
        knownAcceptEncoding | (bp::omit[+typeCharset] >> bp::attr(Encoding::NoMatch));
    auto const parser =
        +(encodeType >> bp::omit[parameters >> -bp::char_(',') >> *bp::ws]);
    std::string_view::iterator begin = acceptEncoding.begin();
    if (!bp::prefix_parse(begin, acceptEncoding.end(), parser, ct))
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
