#pragma once

#include "bmcweb_config.h"

#include <boost/callable_traits.hpp>
#include <boost/url/parse.hpp>
#include <boost/url/url.hpp>
#include <boost/url/url_view.hpp>
#include <boost/url/url_view_base.hpp>
#include <nlohmann/json.hpp>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <functional>
#include <iomanip>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace bmcweb
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

class Base64Encoder
{
    char overflow1 = '\0';
    char overflow2 = '\0';
    uint8_t overflowCount = 0;

    constexpr static std::array<char, 64> key = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
        'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
        'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

    // Takes 3 ascii chars, and encodes them as 4 base64 chars
    static void encodeTriple(char first, char second, char third,
                             std::string& output)
    {
        size_t keyIndex = 0;

        keyIndex = static_cast<size_t>(first & 0xFC) >> 2;
        output += key[keyIndex];

        keyIndex = static_cast<size_t>(first & 0x03) << 4;
        keyIndex += static_cast<size_t>(second & 0xF0) >> 4;
        output += key[keyIndex];

        keyIndex = static_cast<size_t>(second & 0x0F) << 2;
        keyIndex += static_cast<size_t>(third & 0xC0) >> 6;
        output += key[keyIndex];

        keyIndex = static_cast<size_t>(third & 0x3F);
        output += key[keyIndex];
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
        output += key[keyIndex];

        keyIndex = static_cast<size_t>(overflow1 & 0x03) << 4;
        if (overflowCount == 2)
        {
            keyIndex += static_cast<size_t>(overflow2 & 0xF0) >> 4;
            output += key[keyIndex];
            keyIndex = static_cast<size_t>(overflow2 & 0x0F) << 2;
            output += key[keyIndex];
        }
        else
        {
            output += key[keyIndex];
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

// TODO this is temporary and should be deleted once base64 is refactored out of
// crow
inline bool base64Decode(std::string_view input, std::string& output)
{
    static const char nop = static_cast<char>(-1);
    // See note on encoding_data[] in above function
    static const std::array<char, 256> decodingData = {
        nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop,
        nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop,
        nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop,
        nop, 62,  nop, nop, nop, 63,  52,  53,  54,  55,  56,  57,  58,  59,
        60,  61,  nop, nop, nop, nop, nop, nop, nop, 0,   1,   2,   3,   4,
        5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,  16,  17,  18,
        19,  20,  21,  22,  23,  24,  25,  nop, nop, nop, nop, nop, nop, 26,
        27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,
        41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  nop, nop, nop,
        nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop,
        nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop,
        nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop,
        nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop,
        nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop,
        nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop,
        nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop,
        nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop,
        nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop,
        nop, nop, nop, nop};

    size_t inputLength = input.size();

    // allocate space for output string
    output.clear();
    output.reserve(((inputLength + 2) / 3) * 4);

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
        { // non base64 character
            return false;
        }
        if (!(++i < inputLength))
        { // we need at least two input bytes for first
          // byte output
            return false;
        }
        base64code1 = getCodeValue(input[i]);
        if (base64code1 == nop)
        { // non base64 character
            return false;
        }
        output +=
            static_cast<char>((base64code0 << 2) | ((base64code1 >> 4) & 0x3));

        if (++i < inputLength)
        {
            char c = input[i];
            if (c == '=')
            { // padding , end of input
                return (base64code1 & 0x0f) == 0;
            }
            base64code2 = getCodeValue(input[i]);
            if (base64code2 == nop)
            { // non base64 character
                return false;
            }
            output += static_cast<char>(
                ((base64code1 << 4) & 0xf0) | ((base64code2 >> 2) & 0x0f));
        }

        if (++i < inputLength)
        {
            char c = input[i];
            if (c == '=')
            { // padding , end of input
                return (base64code2 & 0x03) == 0;
            }
            char base64code3 = getCodeValue(input[i]);
            if (base64code3 == nop)
            { // non base64 character
                return false;
            }
            output +=
                static_cast<char>((((base64code2 << 6) & 0xc0) | base64code3));
        }
    }

    return true;
}

namespace details
{
inline boost::urls::url appendUrlPieces(
    boost::urls::url& url, const std::initializer_list<std::string_view> args)
{
    for (std::string_view arg : args)
    {
        url.segments().push_back(arg);
    }
    return url;
}

} // namespace details

class OrMorePaths
{};

template <typename... AV>
inline void appendUrlPieces(boost::urls::url& url, const AV... args)
{
    details::appendUrlPieces(url, {args...});
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

inline boost::urls::url
    replaceUrlSegment(const boost::urls::url_view_base& urlView,
                      const uint replaceLoc, std::string_view newSegment)
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
} // namespace bmcweb

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

namespace crow = bmcweb;
