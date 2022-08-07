#pragma once

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/constants.hpp>
#include <boost/algorithm/string/split.hpp>
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
// IWYU pragma: no_include <boost/algorithm/string/detail/classification.hpp>

namespace http_helpers
{
inline std::vector<std::string> parseAccept(std::string_view header)
{
    std::vector<std::string> encodings;
    // chrome currently sends 6 accepts headers, firefox sends 4.
    encodings.reserve(6);
    boost::split(encodings, header, boost::is_any_of(", "),
                 boost::token_compress_on);

    return encodings;
}

enum class ContentType
{
    NoMatch,
    HTML,
    JSON,
    MsgPack,
    OctetStream,
};

struct ContentTypePair
{
    std::string_view contentTypeString;
    ContentType contentTypeEnum;
};

constexpr std::array<ContentTypePair, 4> contentTypes{{
    {"application/json", ContentType::JSON},
    {"application/octet-stream", ContentType::OctetStream},
    {"application/x-msgpack", ContentType::MsgPack},
    {"text/html", ContentType::HTML},
}};

inline ContentType getPreferedContentType(std::string_view header,
                                          std::span<ContentType> preferedOrder)
{
    for (const std::string& encoding : parseAccept(header))
    {
        auto knownContentType =
            std::find_if(contentTypes.begin(), contentTypes.end(),
                         [&encoding](const ContentTypePair& pair) {
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
