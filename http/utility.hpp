#pragma once

#include <openssl/crypto.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/url/url.hpp>

#include <array>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <functional>
#include <limits>
#include <regex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace crow
{
namespace black_magic
{

template <typename T>
constexpr uint64_t getParameterTag()
{
    if constexpr (std::is_same_v<int, T>)
    {
        return 1;
    }
    if constexpr (std::is_same_v<char, T>)
    {
        return 1;
    }
    if constexpr (std::is_same_v<short, T>)
    {
        return 1;
    }
    if constexpr (std::is_same_v<long, T>)
    {
        return 1;
    }
    if constexpr (std::is_same_v<long long, T>)
    {
        return 1;
    }
    if constexpr (std::is_same_v<unsigned int, T>)
    {
        return 2;
    }
    if constexpr (std::is_same_v<unsigned char, T>)
    {
        return 2;
    }
    if constexpr (std::is_same_v<unsigned short, T>)
    {
        return 2;
    }
    if constexpr (std::is_same_v<unsigned long, T>)
    {
        return 2;
    }
    if constexpr (std::is_same_v<unsigned long long, T>)
    {
        return 2;
    }
    if constexpr (std::is_same_v<double, T>)
    {
        return 3;
    }
    if constexpr (std::is_same_v<std::string, T>)
    {
        return 4;
    }
    return 0;
}

template <typename... Args>
struct computeParameterTagFromArgsList;

template <>
struct computeParameterTagFromArgsList<>
{
    static constexpr int value = 0;
};

template <typename Arg, typename... Args>
struct computeParameterTagFromArgsList<Arg, Args...>
{
    static constexpr int subValue =
        computeParameterTagFromArgsList<Args...>::value;
    static constexpr int value =
        getParameterTag<typename std::decay<Arg>::type>() != 0
            ? subValue * 6 + getParameterTag<typename std::decay<Arg>::type>()
            : subValue;
};

inline bool isParameterTagCompatible(uint64_t a, uint64_t b)
{
    while (true)
    {
        if (a == 0)
        {
            return b == 0;
        }
        if (b == 0)
        {
            return a == 0;
        }
        uint64_t sa = a % 6;
        uint64_t sb = a % 6;
        if (sa == 5)
        {
            sa = 4;
        }
        if (sb == 5)
        {
            sb = 4;
        }
        if (sa != sb)
        {
            return false;
        }
        a /= 6;
        b /= 6;
    }
    return false;
}

constexpr inline uint64_t getParameterTag(std::string_view url)
{
    uint64_t tagValue = 0;
    size_t urlSegmentIndex = std::string_view::npos;

    size_t paramIndex = 0;

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

            // Note, this is a really lame way to do std::pow(6, paramIndex)
            // std::pow doesn't work in constexpr in clang.
            // Ideally in the future we'd move this to use a power of 2 packing
            // (probably 8 instead of 6) so that these just become bit shifts
            uint64_t insertIndex = 1;
            for (size_t unused = 0; unused < paramIndex; unused++)
            {
                insertIndex *= 6;
            }

            if (tag == "<int>")
            {
                tagValue += insertIndex * 1;
            }
            if (tag == "<uint>")
            {
                tagValue += insertIndex * 2;
            }
            if (tag == "<float>" || tag == "<double>")
            {
                tagValue += insertIndex * 3;
            }
            if (tag == "<str>" || tag == "<string>")
            {
                tagValue += insertIndex * 4;
            }
            if (tag == "<path>")
            {
                tagValue += insertIndex * 5;
            }
            paramIndex++;
            urlSegmentIndex = std::string_view::npos;
        }
    }
    if (urlSegmentIndex != std::string_view::npos)
    {
        return 0;
    }
    return tagValue;
}

template <typename... T>
struct S
{
    template <typename U>
    using push = S<U, T...>;
    template <typename U>
    using push_back = S<T..., U>;
    template <template <typename... Args> class U>
    using rebind = U<T...>;
};

template <typename F, typename Set>
struct CallHelper;

template <typename F, typename... Args>
struct CallHelper<F, S<Args...>>
{
    template <typename F1, typename... Args1,
              typename = decltype(std::declval<F1>()(std::declval<Args1>()...))>
    static char test(int);

    template <typename...>
    static int test(...);

    static constexpr bool value = sizeof(test<F, Args...>(0)) == sizeof(char);
};

template <uint64_t N>
struct SingleTagToType
{};

template <>
struct SingleTagToType<1>
{
    using type = int64_t;
};

template <>
struct SingleTagToType<2>
{
    using type = uint64_t;
};

template <>
struct SingleTagToType<3>
{
    using type = double;
};

template <>
struct SingleTagToType<4>
{
    using type = std::string;
};

template <>
struct SingleTagToType<5>
{
    using type = std::string;
};

template <uint64_t Tag>
struct Arguments
{
    using subarguments = typename Arguments<Tag / 6>::type;
    using type = typename subarguments::template push<
        typename SingleTagToType<Tag % 6>::type>;
};

template <>
struct Arguments<0>
{
    using type = S<>;
};

template <typename T>
struct Promote
{
    using type = T;
};

template <typename T>
using PromoteT = typename Promote<T>::type;

template <>
struct Promote<char>
{
    using type = int64_t;
};
template <>
struct Promote<short>
{
    using type = int64_t;
};
template <>
struct Promote<int>
{
    using type = int64_t;
};
template <>
struct Promote<long>
{
    using type = int64_t;
};
template <>
struct Promote<long long>
{
    using type = int64_t;
};
template <>
struct Promote<unsigned char>
{
    using type = uint64_t;
};
template <>
struct Promote<unsigned short>
{
    using type = uint64_t;
};
template <>
struct Promote<unsigned int>
{
    using type = uint64_t;
};
template <>
struct Promote<unsigned long>
{
    using type = uint64_t;
};
template <>
struct Promote<unsigned long long>
{
    using type = uint64_t;
};

} // namespace black_magic

namespace utility
{

template <typename T>
struct function_traits;

template <typename T>
struct function_traits : public function_traits<decltype(&T::operator())>
{
    using parent_t = function_traits<decltype(&T::operator())>;
    static const size_t arity = parent_t::arity;
    using result_type = typename parent_t::result_type;
    template <size_t i>
    using arg = typename parent_t::template arg<i>;
};

template <typename ClassType, typename r, typename... Args>
struct function_traits<r (ClassType::*)(Args...) const>
{
    static const size_t arity = sizeof...(Args);

    using result_type = r;

    template <size_t i>
    using arg = typename std::tuple_element<i, std::tuple<Args...>>::type;
};

template <typename ClassType, typename r, typename... Args>
struct function_traits<r (ClassType::*)(Args...)>
{
    static const size_t arity = sizeof...(Args);

    using result_type = r;

    template <size_t i>
    using arg = typename std::tuple_element<i, std::tuple<Args...>>::type;
};

template <typename r, typename... Args>
struct function_traits<std::function<r(Args...)>>
{
    static const size_t arity = sizeof...(Args);

    using result_type = r;

    template <size_t i>
    using arg = typename std::tuple_element<i, std::tuple<Args...>>::type;
};

inline std::string base64encode(const std::string_view data)
{
    const std::array<char, 64> key = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
        'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
        'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

    size_t size = data.size();
    std::string ret;
    ret.resize((size + 2) / 3 * 4);
    auto it = ret.begin();

    size_t i = 0;
    while (i < size)
    {
        size_t keyIndex = 0;

        keyIndex = static_cast<size_t>(data[i] & 0xFC) >> 2;
        *it++ = key[keyIndex];

        if (i + 1 < size)
        {
            keyIndex = static_cast<size_t>(data[i] & 0x03) << 4;
            keyIndex += static_cast<size_t>(data[i + 1] & 0xF0) >> 4;
            *it++ = key[keyIndex];

            if (i + 2 < size)
            {
                keyIndex = static_cast<size_t>(data[i + 1] & 0x0F) << 2;
                keyIndex += static_cast<size_t>(data[i + 2] & 0xC0) >> 6;
                *it++ = key[keyIndex];

                keyIndex = static_cast<size_t>(data[i + 2] & 0x3F);
                *it++ = key[keyIndex];
            }
            else
            {
                keyIndex = static_cast<size_t>(data[i + 1] & 0x0F) << 2;
                *it++ = key[keyIndex];
                *it++ = '=';
            }
        }
        else
        {
            keyIndex = static_cast<size_t>(data[i] & 0x03) << 4;
            *it++ = key[keyIndex];
            *it++ = '=';
            *it++ = '=';
        }

        i += 3;
    }

    return ret;
}

// TODO this is temporary and should be deleted once base64 is refactored out of
// crow
inline bool base64Decode(const std::string_view input, std::string& output)
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
        static_assert(std::numeric_limits<decltype(code)>::max() <
                      decodingData.size());
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
        char base64code3 = 0;

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
            output += static_cast<char>(((base64code1 << 4) & 0xf0) |
                                        ((base64code2 >> 2) & 0x0f));
        }

        if (++i < inputLength)
        {
            char c = input[i];
            if (c == '=')
            { // padding , end of input
                return (base64code2 & 0x03) == 0;
            }
            base64code3 = getCodeValue(input[i]);
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
constexpr uint64_t maxMilliSeconds = 253402300799999;
constexpr uint64_t maxSeconds = 253402300799;
inline std::string getDateTime(boost::posix_time::milliseconds timeSinceEpoch)
{
    boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));
    boost::posix_time::ptime time = epoch + timeSinceEpoch;
    // append zero offset to the end according to the Redfish spec for Date-Time
    return boost::posix_time::to_iso_extended_string(time) + "+00:00";
}
} // namespace details

// Returns the formatted date time string.
// Note that the maximum supported date is 9999-12-31T23:59:59+00:00, if
// the given |secondsSinceEpoch| is too large, we return the maximum supported
// date. This behavior is to avoid exceptions throwed by Boost.
inline std::string getDateTimeUint(uint64_t secondsSinceEpoch)
{
    secondsSinceEpoch = std::min(secondsSinceEpoch, details::maxSeconds);
    boost::posix_time::seconds boostSeconds(secondsSinceEpoch);
    return details::getDateTime(
        boost::posix_time::milliseconds(boostSeconds.total_milliseconds()));
}

// Returns the formatted date time string.
// Note that the maximum supported date is 9999-12-31T23:59:59.999+00:00, if
// the given |millisSecondsSinceEpoch| is too large, we return the maximum
// supported date.
inline std::string getDateTimeUintMs(uint64_t milliSecondsSinceEpoch)
{
    milliSecondsSinceEpoch =
        std::min(details::maxMilliSeconds, milliSecondsSinceEpoch);
    return details::getDateTime(
        boost::posix_time::milliseconds(milliSecondsSinceEpoch));
}

inline std::string getDateTimeStdtime(std::time_t secondsSinceEpoch)
{
    // secondsSinceEpoch >= maxSeconds
    if constexpr (std::cmp_less_equal(details::maxSeconds,
                                      std::numeric_limits<std::time_t>::max()))
    {
        if (std::cmp_greater_equal(secondsSinceEpoch, details::maxSeconds))
        {
            secondsSinceEpoch = details::maxSeconds;
        }
    }
    boost::posix_time::ptime time =
        boost::posix_time::from_time_t(secondsSinceEpoch);
    return boost::posix_time::to_iso_extended_string(time) + "+00:00";
}

/**
 * Returns the current Date, Time & the local Time Offset
 * infromation in a pair
 *
 * @param[in] None
 *
 * @return std::pair<std::string, std::string>, which consist
 * of current DateTime & the TimeOffset strings respectively.
 */
inline std::pair<std::string, std::string> getDateTimeOffsetNow()
{
    std::time_t time = std::time(nullptr);
    std::string dateTime = getDateTimeStdtime(time);

    /* extract the local Time Offset value from the
     * recevied dateTime string.
     */
    std::string timeOffset("Z00:00");
    std::size_t lastPos = dateTime.size();
    std::size_t len = timeOffset.size();
    if (lastPos > len)
    {
        timeOffset = dateTime.substr(lastPos - len);
    }

    return std::make_pair(dateTime, timeOffset);
}

inline bool constantTimeStringCompare(const std::string_view a,
                                      const std::string_view b)
{
    // Important note, this function is ONLY constant time if the two input
    // sizes are the same
    if (a.size() != b.size())
    {
        return false;
    }
    return CRYPTO_memcmp(a.data(), b.data(), a.size()) == 0;
}

struct ConstantTimeCompare
{
    bool operator()(const std::string_view a, const std::string_view b) const
    {
        return constantTimeStringCompare(a, b);
    }
};

namespace details
{
inline boost::urls::url
    urlFromPiecesDetail(const std::initializer_list<std::string_view> args)
{
    boost::urls::url url("/");
    for (const std::string_view& arg : args)
    {
        url.segments().push_back(arg);
    }
    return url;
}
} // namespace details

template <typename... AV>
inline boost::urls::url urlFromPieces(const AV... args)
{
    return details::urlFromPiecesDetail({args...});
}

namespace details
{

// std::reference_wrapper<std::string> - extracts segment to variable
//                    std::string_view - checks if segment is equal to variable
using UrlSegment =
    std::variant<std::reference_wrapper<std::string>, std::string_view>;

class UrlSegmentMatcherVisitor
{
  public:
    bool operator()(std::string& output)
    {
        output = std::string_view(segment.data(), segment.size());
        return true;
    }

    bool operator()(std::string_view expected)
    {
        return std::string_view(segment.data(), segment.size()) == expected;
    }

    UrlSegmentMatcherVisitor(const boost::urls::string_value& segmentIn) :
        segment(segmentIn)
    {}

  private:
    const boost::urls::string_value& segment;
};

inline bool readUrlSegments(const boost::urls::url_view& urlView,
                            std::initializer_list<UrlSegment>&& segments)
{
    const boost::urls::segments_view& urlSegments = urlView.segments();

    if (!urlSegments.is_absolute() || segments.size() != urlSegments.size())
    {
        return false;
    }

    boost::urls::segments_view::iterator it = urlSegments.begin();
    boost::urls::segments_view::iterator end = urlSegments.end();

    for (const auto& segment : segments)
    {
        if (!std::visit(UrlSegmentMatcherVisitor(*it), segment))
        {
            return false;
        }
        it++;
    }
    return true;
}

} // namespace details

template <typename... Args>
inline bool readUrlSegments(const boost::urls::url_view& urlView,
                            Args&&... args)
{
    return details::readUrlSegments(urlView, {std::forward<Args>(args)...});
}

inline bool validateAndSplitUrl(std::string_view destUrl, std::string& urlProto,
                                std::string& host, std::string& port,
                                std::string& path)
{
    // Validate URL using regex expression
    // Format: <protocol>://<host>:<port>/<path>
    // protocol: http/https
    const std::regex urlRegex(
        "(http|https)://([^/\\x20\\x3f\\x23\\x3a]+):?([0-9]*)(/"
        "([^\\x20\\x23\\x3f]*\\x3f?([^\\x20\\x23\\x3f])*)?)");
    std::cmatch match;
    if (!std::regex_match(destUrl.begin(), destUrl.end(), match, urlRegex))
    {
        return false;
    }

    urlProto = std::string(match[1].first, match[1].second);
    if (urlProto == "http")
    {
#ifndef BMCWEB_INSECURE_ENABLE_HTTP_PUSH_STYLE_EVENTING
        return false;
#endif
    }

    host = std::string(match[2].first, match[2].second);
    port = std::string(match[3].first, match[3].second);
    path = std::string(match[4].first, match[4].second);
    if (port.empty())
    {
        if (urlProto == "http")
        {
            port = "80";
        }
        else
        {
            port = "443";
        }
    }
    if (path.empty())
    {
        path = "/";
    }
    return true;
}

} // namespace utility
} // namespace crow
