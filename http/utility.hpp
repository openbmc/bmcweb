#pragma once

#include <bmcweb_config.h>
#include <openssl/crypto.h>

#include <boost/callable_traits.hpp>
#include <boost/url/url.hpp>
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

namespace crow
{
namespace black_magic
{

enum class TypeCode : uint8_t
{
    Unspecified = 0,
    Integer = 1,
    UnsignedInteger = 2,
    Float = 3,
    String = 4,
    Path = 5,
    Max = 6,
};

// Remove when we have c++23
template <typename E>
constexpr typename std::underlying_type<E>::type toUnderlying(E e) noexcept
{
    return static_cast<typename std::underlying_type<E>::type>(e);
}

template <typename T>
constexpr TypeCode getParameterTag()
{
    if constexpr (std::is_same_v<int, T>)
    {
        return TypeCode::Integer;
    }
    if constexpr (std::is_same_v<char, T>)
    {
        return TypeCode::Integer;
    }
    if constexpr (std::is_same_v<short, T>)
    {
        return TypeCode::Integer;
    }
    if constexpr (std::is_same_v<long, T>)
    {
        return TypeCode::Integer;
    }
    if constexpr (std::is_same_v<long long, T>)
    {
        return TypeCode::Integer;
    }
    if constexpr (std::is_same_v<unsigned int, T>)
    {
        return TypeCode::UnsignedInteger;
    }
    if constexpr (std::is_same_v<unsigned char, T>)
    {
        return TypeCode::UnsignedInteger;
    }
    if constexpr (std::is_same_v<unsigned short, T>)
    {
        return TypeCode::UnsignedInteger;
    }
    if constexpr (std::is_same_v<unsigned long, T>)
    {
        return TypeCode::UnsignedInteger;
    }
    if constexpr (std::is_same_v<unsigned long long, T>)
    {
        return TypeCode::UnsignedInteger;
    }
    if constexpr (std::is_same_v<double, T>)
    {
        return TypeCode::Float;
    }
    if constexpr (std::is_same_v<std::string, T>)
    {
        return TypeCode::String;
    }
    return TypeCode::Unspecified;
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
        getParameterTag<typename std::decay<Arg>::type>() !=
                TypeCode::Unspecified
            ? static_cast<unsigned long>(subValue *
                                         toUnderlying(TypeCode::Max)) +
                  static_cast<uint64_t>(
                      getParameterTag<typename std::decay<Arg>::type>())
            : subValue;
};

inline bool isParameterTagCompatible(uint64_t a, uint64_t b)
{
    while (true)
    {
        if (a == 0 && b == 0)
        {
            // Both tags were equivalent, parameters are compatible
            return true;
        }
        if (a == 0 || b == 0)
        {
            // one of the tags had more parameters than the other
            return false;
        }
        TypeCode sa = static_cast<TypeCode>(a % toUnderlying(TypeCode::Max));
        TypeCode sb = static_cast<TypeCode>(b % toUnderlying(TypeCode::Max));

        if (sa == TypeCode::Path)
        {
            sa = TypeCode::String;
        }
        if (sb == TypeCode::Path)
        {
            sb = TypeCode::String;
        }
        if (sa != sb)
        {
            return false;
        }
        a /= toUnderlying(TypeCode::Max);
        b /= toUnderlying(TypeCode::Max);
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
                tagValue += insertIndex * toUnderlying(TypeCode::Integer);
            }
            if (tag == "<uint>")
            {
                tagValue +=
                    insertIndex * toUnderlying(TypeCode::UnsignedInteger);
            }
            if (tag == "<float>" || tag == "<double>")
            {
                tagValue += insertIndex * toUnderlying(TypeCode::Float);
            }
            if (tag == "<str>" || tag == "<string>")
            {
                tagValue += insertIndex * toUnderlying(TypeCode::String);
            }
            if (tag == "<path>")
            {
                tagValue += insertIndex * toUnderlying(TypeCode::Path);
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
struct FunctionTraits
{
    template <size_t i>
    using arg = std::tuple_element_t<i, boost::callable_traits::args_t<T>>;
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
// Returns year/month/day triple in civil calendar
// Preconditions:  z is number of days since 1970-01-01 and is in the range:
//                   [numeric_limits<Int>::min(),
//                   numeric_limits<Int>::max()-719468].
// Algorithm sourced from
// https://howardhinnant.github.io/date_algorithms.html#civil_from_days All
// constants are explained in the above
template <class IntType>
constexpr std::tuple<IntType, unsigned, unsigned>
    civilFromDays(IntType z) noexcept
{
    z += 719468;
    IntType era = (z >= 0 ? z : z - 146096) / 146097;
    unsigned doe = static_cast<unsigned>(z - era * 146097); // [0, 146096]
    unsigned yoe =
        (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; // [0, 399]
    IntType y = static_cast<IntType>(yoe) + era * 400;
    unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100); // [0, 365]
    unsigned mp = (5 * doy + 2) / 153;                      // [0, 11]
    unsigned d = doy - (153 * mp + 2) / 5 + 1;              // [1, 31]
    unsigned m = mp < 10 ? mp + 3 : mp - 9;                 // [1, 12]

    return std::tuple<IntType, unsigned, unsigned>(y + (m <= 2), m, d);
}

// Creates a string from an integer in the most efficient way possible without
// using std::locale.  Adds a minimum zero pad.
inline std::string padZeros(int value, size_t pad)
{
    std::string result(pad, '0');
    int val = std::abs(value);
    for (; pad > 0; pad--)
    {
        result[pad - 1] = static_cast<char>('0' + val % 10);
        val /= 10;
    }
    return result;
}

template <typename IntType, typename Period>
std::string toISO8061ExtendedStr(std::chrono::duration<IntType, Period> t)
{
    using seconds = std::chrono::duration<int>;
    using minutes = std::chrono::duration<int, std::ratio<60>>;
    using hours = std::chrono::duration<int, std::ratio<3600>>;
    using days = std::chrono::duration<
        IntType, std::ratio_multiply<hours::period, std::ratio<24>>>;

    // d is days since 1970-01-01
    days d = std::chrono::duration_cast<days>(t);
    if (d > t)
    {
        --d;
    }
    // t is now time duration since midnight of day d
    t -= d;

    // break d down into year/month/day
    int year = 0;
    int month = 0;
    int day = 0;
    std::tie(year, month, day) = details::civilFromDays(d.count());
    // Check against limits.  Can't go above year 9999, and can't go below epoch
    // (1970)
    if (year >= 10000)
    {
        year = 9999;
        month = 12;
        day = 31;
        t = days(1) - std::chrono::duration<IntType, Period>(1);
    }
    else if (year < 1970)
    {
        year = 1970;
        month = 1;
        day = 1;
        t = std::chrono::duration<IntType, Period>::zero();
    }
    std::string out;
    out += details::padZeros(year, 4);
    out += '-';
    out += details::padZeros(month, 2);
    out += '-';
    out += details::padZeros(day, 2);

    out += 'T';
    hours hr = duration_cast<hours>(t);
    out += details::padZeros(hr.count(), 2);
    t -= hr;
    out += ':';

    minutes mt = duration_cast<minutes>(t);
    out += details::padZeros(mt.count(), 2);
    t -= mt;
    out += ':';

    seconds se = duration_cast<seconds>(t);
    out += details::padZeros(se.count(), 2);
    t -= se;

    if constexpr (std::is_same_v<typename decltype(t)::period, std::milli>)
    {
        out += '.';
        using MilliDuration = std::chrono::duration<int, std::milli>;
        MilliDuration subsec = duration_cast<MilliDuration>(t);
        out += details::padZeros(subsec.count(), 3);
    }
    else if constexpr (std::is_same_v<typename decltype(t)::period, std::micro>)
    {
        out += '.';

        using MicroDuration = std::chrono::duration<int, std::micro>;
        MicroDuration subsec = duration_cast<MicroDuration>(t);
        out += details::padZeros(subsec.count(), 6);
    }

    out += "+00:00";
    return out;
}
} // namespace details

// Returns the formatted date time string.
// Note that the maximum supported date is 9999-12-31T23:59:59+00:00, if
// the given |secondsSinceEpoch| is too large, we return the maximum supported
// date.
inline std::string getDateTimeUint(uint64_t secondsSinceEpoch)
{
    using DurationType = std::chrono::duration<uint64_t>;
    DurationType sinceEpoch(secondsSinceEpoch);
    return details::toISO8061ExtendedStr(sinceEpoch);
}

// Returns the formatted date time string with millisecond precision
// Note that the maximum supported date is 9999-12-31T23:59:59+00:00, if
// the given |secondsSinceEpoch| is too large, we return the maximum supported
// date.
inline std::string getDateTimeUintMs(uint64_t milliSecondsSinceEpoch)
{
    using DurationType = std::chrono::duration<uint64_t, std::milli>;
    DurationType sinceEpoch(milliSecondsSinceEpoch);
    return details::toISO8061ExtendedStr(sinceEpoch);
}

// Returns the formatted date time string with microsecond precision
inline std::string getDateTimeUintUs(uint64_t microSecondsSinceEpoch)
{
    using DurationType = std::chrono::duration<uint64_t, std::micro>;
    DurationType sinceEpoch(microSecondsSinceEpoch);
    return details::toISO8061ExtendedStr(sinceEpoch);
}

inline std::string getDateTimeStdtime(std::time_t secondsSinceEpoch)
{
    using DurationType = std::chrono::duration<std::time_t>;
    DurationType sinceEpoch(secondsSinceEpoch);
    return details::toISO8061ExtendedStr(sinceEpoch);
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

    explicit UrlSegmentMatcherVisitor(
        const boost::urls::string_value& segmentIn) :
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

inline std::string setProtocolDefaults(const boost::urls::url_view& url)
{
    if (url.scheme() == "https")
    {
        return "https";
    }
    if (url.scheme() == "http")
    {
        if (bmcwebInsecureEnableHttpPushStyleEventing)
        {
            return "http";
        }
        return "";
    }
    return "";
}

inline uint16_t setPortDefaults(const boost::urls::url_view& url)
{
    uint16_t port = url.port_number();
    if (port != 0)
    {
        // user picked a port already.
        return port;
    }

    // If the user hasn't explicitly stated a port, pick one explicitly for them
    // based on the protocol defaults
    if (url.scheme() == "http")
    {
        return 80;
    }
    if (url.scheme() == "https")
    {
        return 443;
    }
    return 0;
}

inline bool validateAndSplitUrl(std::string_view destUrl, std::string& urlProto,
                                std::string& host, uint16_t& port,
                                std::string& path)
{
    boost::string_view urlBoost(destUrl.data(), destUrl.size());
    boost::urls::result<boost::urls::url_view> url =
        boost::urls::parse_uri(urlBoost);
    if (!url)
    {
        return false;
    }
    urlProto = setProtocolDefaults(url.value());
    if (urlProto.empty())
    {
        return false;
    }

    port = setPortDefaults(url.value());

    host = std::string_view(url->encoded_host().data(),
                            url->encoded_host().size());

    path = std::string_view(url->encoded_path().data(),
                            url->encoded_path().size());
    if (path.empty())
    {
        path = "/";
    }
    if (url->has_fragment())
    {
        path += '#';
        path += std::string_view(url->encoded_fragment().data(),
                                 url->encoded_fragment().size());
    }

    if (url->has_query())
    {
        path += '?';
        path += std::string_view(url->encoded_query().data(),
                                 url->encoded_query().size());
    }

    return true;
}

} // namespace utility
} // namespace crow

namespace nlohmann
{
template <>
struct adl_serializer<boost::urls::url>
{
    // nlohmann requires a specific casing to look these up in adl
    // NOLINTNEXTLINE(readability-identifier-naming)
    static void to_json(json& j, const boost::urls::url& url)
    {
        j = url.string();
    }
};

template <>
struct adl_serializer<boost::urls::url_view>
{
    // NOLINTNEXTLINE(readability-identifier-naming)
    static void to_json(json& j, const boost::urls::url_view& url)
    {
        j = url.string();
    }
};
} // namespace nlohmann
