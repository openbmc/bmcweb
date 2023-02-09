#pragma once

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/constants.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/type_index/type_index_facade.hpp>

#include <cctype>
#include <iomanip>
#include <ostream>
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
};

struct ContentTypePair
{
    std::string_view contentTypeString;
    ContentType contentTypeEnum;
};

constexpr std::array<ContentTypePair, 4> contentTypes{{
    {"application/cbor", ContentType::CBOR},
    {"application/json", ContentType::JSON},
    {"application/octet-stream", ContentType::OctetStream},
    {"text/html", ContentType::HTML},
}};

std::array<std::string, 8> userAgentTypes = {
    "AppleWebKit",
    "Chrome",
    "Edg",
    "Firefox",
    "Mozilla",
    "OPR",
    "Opera",
    "Safari",
};

inline bool checkUserAgent(std::string_view userAgent, bool isUserAgentMatched)
{
    for(const auto& type: userAgentTypes)
    {
        size_t found = userAgent.find(type);
        if (found != std::string::npos)
            return isUserAgentMatched;
    }
    return !isUserAgentMatched;
}

inline ContentType
    getPreferedContentType(std::string_view header,
                           std::span<const ContentType> preferedOrder)
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
        const auto* knownContentType =
            std::find_if(contentTypes.begin(), contentTypes.end(),
                         [encoding](const ContentTypePair& pair) {
            return pair.contentTypeString == encoding;
            });

        if (knownContentType == contentTypes.end())
        {
            // not able to find content type in list
            continue;
        }

        // Not one of the types requested
        if (std::find(preferedOrder.begin(), preferedOrder.end(),
                      knownContentType->contentTypeEnum) == preferedOrder.end())
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
    ContentType allowed = getPreferedContentType(header, types);
    if (allowed == ContentType::ANY)
    {
        return allowWildcard;
    }

    return type == allowed;
}

inline std::string urlEncode(const std::string_view value)
{
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (const char c : value)
    {
        // Keep alphanumeric and other accepted characters intact
        if ((isalnum(c) != 0) || c == '-' || c == '_' || c == '.' || c == '~')
        {
            escaped << c;
            continue;
        }

        // Any other characters are percent-encoded
        escaped << std::uppercase;
        escaped << '%' << std::setw(2)
                << static_cast<int>(static_cast<unsigned char>(c));
        escaped << std::nouppercase;
    }

    return escaped.str();
}
} // namespace http_helpers
