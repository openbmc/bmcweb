// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "bmcweb_config.h"

#include <sys/types.h>

#include <boost/url/segments_view.hpp>
#include <boost/url/url.hpp>
#include <boost/url/url_view_base.hpp>
#include <nlohmann/adl_serializer.hpp>
#include <nlohmann/json.hpp>

#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <functional>
#include <initializer_list>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace crow
{
namespace utility
{

constexpr uint64_t getParameterTag(std::string_view url)
{
    uint64_t tagValue = 0;
    size_t urlSegmentIndex = std::string_view::npos;

    for (size_t urlIndex = 0; urlIndex < url.size(); urlIndex++)
    {
        char character = url[urlIndex];
        if (character == '<')
        {
            if (urlSegmentIndex != std::string_view::npos)
            {
                return 0;
            }
            urlSegmentIndex = urlIndex;
        }
        if (character == '>')
        {
            if (urlSegmentIndex == std::string_view::npos)
            {
                return 0;
            }
            std::string_view tag =
                url.substr(urlSegmentIndex, urlIndex + 1 - urlSegmentIndex);

            if (tag == "<str>" || tag == "<string>")
            {
                tagValue++;
            }
            if (tag == "<path>")
            {
                tagValue++;
            }
            urlSegmentIndex = std::string_view::npos;
        }
    }
    if (urlSegmentIndex != std::string_view::npos)
    {
        return 0;
    }
    return tagValue;
}

constexpr static std::array<char, 64> base64key = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

static constexpr char nop = static_cast<char>(-1);
constexpr std::array<char, 256> getDecodeTable(bool urlSafe)
{
    std::array<char, 256> decodeTable{};
    decodeTable.fill(nop);

    for (size_t index = 0; index < base64key.size(); index++)
    {
        char character = base64key[index];
        decodeTable[std::bit_cast<uint8_t>(character)] =
            static_cast<char>(index);
    }

    if (urlSafe)
    {
        // Urlsafe decode tables replace the last two characters with - and _
        decodeTable['+'] = nop;
        decodeTable['/'] = nop;
        decodeTable['-'] = 62;
        decodeTable['_'] = 63;
    }

    return decodeTable;
}

class Base64Encoder
{
    char overflow1 = '\0';
    char overflow2 = '\0';
    uint8_t overflowCount = 0;

    // Takes 3 ascii chars, and encodes them as 4 base64 chars
    static void encodeTriple(char first, char second, char third,
                             std::string& output)
    {
        size_t keyIndex = 0;

        keyIndex = static_cast<size_t>(first & 0xFC) >> 2;
        output += base64key[keyIndex];

        keyIndex = static_cast<size_t>(first & 0x03) << 4;
        keyIndex += static_cast<size_t>(second & 0xF0) >> 4;
        output += base64key[keyIndex];

        keyIndex = static_cast<size_t>(second & 0x0F) << 2;
        keyIndex += static_cast<size_t>(third & 0xC0) >> 6;
        output += base64key[keyIndex];

        keyIndex = static_cast<size_t>(third & 0x3F);
        output += base64key[keyIndex];
    }

  public:
    // Accepts a partial string to encode, and writes the encoded characters to
    // the output stream. requires subsequently calling finalize to complete
    // stream.
    void encode(std::string_view data, std::string& output)
    {
        // Encode the last round of overflow chars first
        if (overflowCount == 2)
        {
            if (!data.empty())
            {
                encodeTriple(overflow1, overflow2, data[0], output);
                overflowCount = 0;
                data.remove_prefix(1);
            }
        }
        else if (overflowCount == 1)
        {
            if (data.size() >= 2)
            {
                encodeTriple(overflow1, data[0], data[1], output);
                overflowCount = 0;
                data.remove_prefix(2);
            }
        }

        while (data.size() >= 3)
        {
            encodeTriple(data[0], data[1], data[2], output);
            data.remove_prefix(3);
        }

        if (!data.empty() && overflowCount == 0)
        {
            overflow1 = data[0];
            overflowCount++;
            data.remove_prefix(1);
        }

        if (!data.empty() && overflowCount == 1)
        {
            overflow2 = data[0];
            overflowCount++;
            data.remove_prefix(1);
        }
    }

    // Completes a base64 output, by writing any MOD(3) characters to the
    // output, as well as any required trailing =
    void finalize(std::string& output)
    {
        if (overflowCount == 0)
        {
            return;
        }
        size_t keyIndex = static_cast<size_t>(overflow1 & 0xFC) >> 2;
        output += base64key[keyIndex];

        keyIndex = static_cast<size_t>(overflow1 & 0x03) << 4;
        if (overflowCount == 2)
        {
            keyIndex += static_cast<size_t>(overflow2 & 0xF0) >> 4;
            output += base64key[keyIndex];
            keyIndex = static_cast<size_t>(overflow2 & 0x0F) << 2;
            output += base64key[keyIndex];
        }
        else
        {
            output += base64key[keyIndex];
            output += '=';
        }
        output += '=';
        overflowCount = 0;
    }

    // Returns the required output buffer in characters for an input of size
    // inputSize
    static size_t constexpr encodedSize(size_t inputSize)
    {
        // Base64 encodes 3 character blocks as 4 character blocks
        // With a possibility of 2 trailing = characters
        return (inputSize + 2) / 3 * 4;
    }
};

inline std::string base64encode(std::string_view data)
{
    // Encodes a 3 character stream into a 4 character stream
    std::string out;
    Base64Encoder base64;
    out.reserve(Base64Encoder::encodedSize(data.size()));
    base64.encode(data, out);
    base64.finalize(out);
    return out;
}

template <bool urlsafe = false>
inline bool base64Decode(std::string_view input, std::string& output)
{
    size_t inputLength = input.size();

    // allocate space for output string
    output.clear();
    output.reserve(((inputLength + 2) / 3) * 4);

    static constexpr auto decodingData = getDecodeTable(urlsafe);

    auto getCodeValue = [](char c) {
        auto code = static_cast<unsigned char>(c);
        // Ensure we cannot index outside the bounds of the decoding array
        static_assert(
            std::numeric_limits<decltype(code)>::max() < decodingData.size());
        return decodingData[code];
    };

    // for each 4-bytes sequence from the input, extract 4 6-bits sequences by
    // dropping first two bits
    // and regenerate into 3 8-bits sequences

    for (size_t i = 0; i < inputLength; i++)
    {
        char base64code0 = 0;
        char base64code1 = 0;
        char base64code2 = 0; // initialized to 0 to suppress warnings

        base64code0 = getCodeValue(input[i]);
        if (base64code0 == nop)
        {
            // non base64 character
            return false;
        }
        if (!(++i < inputLength))
        {
            // we need at least two input bytes for first byte output
            return false;
        }
        base64code1 = getCodeValue(input[i]);
        if (base64code1 == nop)
        {
            // non base64 character
            return false;
        }
        output +=
            static_cast<char>((base64code0 << 2) | ((base64code1 >> 4) & 0x3));

        if (++i < inputLength)
        {
            char c = input[i];
            if (c == '=')
            {
                // padding , end of input
                return (base64code1 & 0x0f) == 0;
            }
            base64code2 = getCodeValue(input[i]);
            if (base64code2 == nop)
            {
                // non base64 character
                return false;
            }
            output += static_cast<char>(
                ((base64code1 << 4) & 0xf0) | ((base64code2 >> 2) & 0x0f));
        }

        if (++i < inputLength)
        {
            char c = input[i];
            if (c == '=')
            {
                // padding , end of input
                return (base64code2 & 0x03) == 0;
            }
            char base64code3 = getCodeValue(input[i]);
            if (base64code3 == nop)
            {
                // non base64 character
                return false;
            }
            output +=
                static_cast<char>((((base64code2 << 6) & 0xc0) | base64code3));
        }
    }

    return true;
}

class OrMorePaths
{};

template <typename... AV>
inline void appendUrlPieces(boost::urls::url& url, AV&&... args)
{
    // Unclear the correct fix here.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    for (const std::string_view arg : {args...})
    {
        url.segments().push_back(arg);
    }
}

namespace details
{

// std::reference_wrapper<std::string> - extracts segment to variable
//                    std::string_view - checks if segment is equal to variable
using UrlSegment = std::variant<std::reference_wrapper<std::string>,
                                std::string_view, OrMorePaths>;

enum class UrlParseResult
{
    Continue,
    Fail,
    Done,
};

class UrlSegmentMatcherVisitor
{
  public:
    UrlParseResult operator()(std::string& output)
    {
        output = segment;
        return UrlParseResult::Continue;
    }

    UrlParseResult operator()(std::string_view expected)
    {
        if (segment == expected)
        {
            return UrlParseResult::Continue;
        }
        return UrlParseResult::Fail;
    }

    UrlParseResult operator()(OrMorePaths /*unused*/)
    {
        return UrlParseResult::Done;
    }

    explicit UrlSegmentMatcherVisitor(std::string_view segmentIn) :
        segment(segmentIn)
    {}

  private:
    std::string_view segment;
};

inline bool readUrlSegments(const boost::urls::url_view_base& url,
                            std::initializer_list<UrlSegment> segments)
{
    const boost::urls::segments_view& urlSegments = url.segments();

    if (!urlSegments.is_absolute())
    {
        return false;
    }

    boost::urls::segments_view::const_iterator it = urlSegments.begin();
    boost::urls::segments_view::const_iterator end = urlSegments.end();

    for (const auto& segment : segments)
    {
        if (it == end)
        {
            // If the request ends with an "any" path, this was successful
            return std::holds_alternative<OrMorePaths>(segment);
        }
        UrlParseResult res = std::visit(UrlSegmentMatcherVisitor(*it), segment);
        if (res == UrlParseResult::Done)
        {
            return true;
        }
        if (res == UrlParseResult::Fail)
        {
            return false;
        }
        it++;
    }

    // There will be an empty segment at the end if the URI ends with a "/"
    // e.g. /redfish/v1/Chassis/
    if ((it != end) && urlSegments.back().empty())
    {
        it++;
    }
    return it == end;
}

} // namespace details

template <typename... Args>
inline bool readUrlSegments(const boost::urls::url_view_base& url,
                            Args&&... args)
{
    return details::readUrlSegments(url, {std::forward<Args>(args)...});
}

inline boost::urls::url replaceUrlSegment(
    const boost::urls::url_view_base& urlView, const uint replaceLoc,
    std::string_view newSegment)
{
    const boost::urls::segments_view& urlSegments = urlView.segments();
    boost::urls::url url("/");

    if (!urlSegments.is_absolute())
    {
        return url;
    }

    boost::urls::segments_view::iterator it = urlSegments.begin();
    boost::urls::segments_view::iterator end = urlSegments.end();

    for (uint idx = 0; it != end; it++, idx++)
    {
        if (idx == replaceLoc)
        {
            url.segments().push_back(newSegment);
        }
        else
        {
            url.segments().push_back(*it);
        }
    }

    return url;
}

inline void setProtocolDefaults(boost::urls::url& url,
                                std::string_view protocol)
{
    if (url.has_scheme())
    {
        return;
    }
    if (protocol == "Redfish" || protocol.empty())
    {
        if (url.port_number() == 443)
        {
            url.set_scheme("https");
        }
        if (url.port_number() == 80)
        {
            if constexpr (BMCWEB_INSECURE_PUSH_STYLE_NOTIFICATION)
            {
                url.set_scheme("http");
            }
        }
    }
    else if (protocol == "SNMPv2c")
    {
        url.set_scheme("snmp");
    }
}

inline void setPortDefaults(boost::urls::url& url)
{
    uint16_t port = url.port_number();
    if (port != 0)
    {
        return;
    }

    // If the user hasn't explicitly stated a port, pick one explicitly for them
    // based on the protocol defaults
    if (url.scheme() == "http")
    {
        url.set_port_number(80);
    }
    if (url.scheme() == "https")
    {
        url.set_port_number(443);
    }
    if (url.scheme() == "snmp")
    {
        url.set_port_number(162);
    }
}

} // namespace utility
} // namespace crow

namespace nlohmann
{
template <std::derived_from<boost::urls::url_view_base> URL>
struct adl_serializer<URL>
{
    // NOLINTNEXTLINE(readability-identifier-naming)
    static void to_json(json& j, const URL& url)
    {
        j = url.buffer();
    }
};
} // namespace nlohmann
