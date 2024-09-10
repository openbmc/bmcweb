// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

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

struct ContentTypePair
{
    std::string_view contentTypeString;
    ContentType contentTypeEnum;
};

constexpr std::array<ContentTypePair, 5> contentTypes{{
    {"application/cbor", ContentType::CBOR},
    {"application/json", ContentType::JSON},
    {"application/octet-stream", ContentType::OctetStream},
    {"text/html", ContentType::HTML},
    {"text/event-stream", ContentType::EventStream},
}};

inline ContentType getPreferredContentType(
    std::string_view header, std::span<const ContentType> preferedOrder)
{
    size_t lastIndex = 0;
    while (lastIndex < header.size() + 1)
    {
        size_t index = header.find(',', lastIndex);
        if (index == std::string_view::npos)
        {
            index = header.size();
        }
        std::string_view encoding = header.substr(lastIndex, index);

        if (!header.empty())
        {
            header.remove_prefix(1);
        }
        lastIndex = index + 1;
        // ignore any q-factor weighting (;q=)
        std::size_t separator = encoding.find(";q=");

        if (separator != std::string_view::npos)
        {
            encoding = encoding.substr(0, separator);
        }
        // If the client allows any encoding, given them the first one on the
        // servers list
        if (encoding == "*/*")
        {
            return ContentType::ANY;
        }
        const auto* knownContentType = std::ranges::find_if(
            contentTypes, [encoding](const ContentTypePair& pair) {
                return pair.contentTypeString == encoding;
            });

        if (knownContentType == contentTypes.end())
        {
            // not able to find content type in list
            continue;
        }

        // Not one of the types requested
        if (std::ranges::find(preferedOrder,
                              knownContentType->contentTypeEnum) ==
            preferedOrder.end())
        {
            continue;
        }
        return knownContentType->contentTypeEnum;
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
