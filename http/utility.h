#pragma once

#include "nlohmann/json.hpp"

#include <openssl/crypto.h>

#include <cstdint>
#include <cstring>
#include <functional>
#include <regex>
#include <stdexcept>
#include <string>
#include <tuple>

namespace crow
{
namespace black_magic
{
struct OutOfRange
{
    OutOfRange(unsigned /*pos*/, unsigned /*length*/)
    {
    }
};
constexpr unsigned requiresInRange(unsigned i, unsigned len)
{
    return i >= len ? throw OutOfRange(i, len) : i;
}

class ConstStr
{
    const char* const beginPtr;
    unsigned sizeUint;

  public:
    template <unsigned N>
    constexpr ConstStr(const char (&arr)[N]) : beginPtr(arr), sizeUint(N - 1)
    {
        static_assert(N >= 1, "not a string literal");
    }
    constexpr char operator[](unsigned i) const
    {
        return requiresInRange(i, sizeUint), beginPtr[i];
    }

    constexpr operator const char*() const
    {
        return beginPtr;
    }

    constexpr const char* begin() const
    {
        return beginPtr;
    }
    constexpr const char* end() const
    {
        return beginPtr + sizeUint;
    }

    constexpr unsigned size() const
    {
        return sizeUint;
    }
};

constexpr unsigned findClosingTag(ConstStr s, unsigned p)
{
    return s[p] == '>' ? p : findClosingTag(s, p + 1);
}

constexpr bool isValid(ConstStr s, unsigned i = 0, int f = 0)
{
    return i == s.size()
               ? f == 0
               : f < 0 || f >= 2
                     ? false
                     : s[i] == '<' ? isValid(s, i + 1, f + 1)
                                   : s[i] == '>' ? isValid(s, i + 1, f - 1)
                                                 : isValid(s, i + 1, f);
}

constexpr bool isEquP(const char* a, const char* b, unsigned n)
{
    return *a == 0 && *b == 0 && n == 0
               ? true
               : (*a == 0 || *b == 0)
                     ? false
                     : n == 0 ? true
                              : *a != *b ? false : isEquP(a + 1, b + 1, n - 1);
}

constexpr bool isEquN(ConstStr a, unsigned ai, ConstStr b, unsigned bi,
                      unsigned n)
{
    return ai + n > a.size() || bi + n > b.size()
               ? false
               : n == 0 ? true
                        : a[ai] != b[bi] ? false
                                         : isEquN(a, ai + 1, b, bi + 1, n - 1);
}

constexpr bool isInt(ConstStr s, unsigned i)
{
    return isEquN(s, i, "<int>", 0, 5);
}

constexpr bool isUint(ConstStr s, unsigned i)
{
    return isEquN(s, i, "<uint>", 0, 6);
}

constexpr bool isFloat(ConstStr s, unsigned i)
{
    return isEquN(s, i, "<float>", 0, 7) || isEquN(s, i, "<double>", 0, 8);
}

constexpr bool isStr(ConstStr s, unsigned i)
{
    return isEquN(s, i, "<str>", 0, 5) || isEquN(s, i, "<string>", 0, 8);
}

constexpr bool isPath(ConstStr s, unsigned i)
{
    return isEquN(s, i, "<path>", 0, 6);
}

template <typename T> constexpr int getParameterTag()
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

template <typename... Args> struct compute_parameter_tag_from_args_list;

template <> struct compute_parameter_tag_from_args_list<>
{
    static constexpr int value = 0;
};

template <typename Arg, typename... Args>
struct compute_parameter_tag_from_args_list<Arg, Args...>
{
    static constexpr int subValue =
        compute_parameter_tag_from_args_list<Args...>::value;
    static constexpr int value =
        getParameterTag<typename std::decay<Arg>::type>()
            ? subValue * 6 + getParameterTag<typename std::decay<Arg>::type>()
            : subValue;
};

static inline bool isParameterTagCompatible(uint64_t a, uint64_t b)
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
    return isParameterTagCompatible(a / 6, b / 6);
}

static inline unsigned findClosingTagRuntime(const char* s, unsigned p)
{
    return s[p] == 0 ? throw std::runtime_error("unmatched tag <")
                     : s[p] == '>' ? p : findClosingTagRuntime(s, p + 1);
}

static inline uint64_t getParameterTagRuntime(const char* s, unsigned p = 0)
{
    return s[p] == 0
               ? 0
               : s[p] == '<'
                     ? (std::strncmp(s + p, "<int>", 5) == 0
                            ? getParameterTagRuntime(
                                  s, findClosingTagRuntime(s, p)) *
                                      6 +
                                  1
                            : std::strncmp(s + p, "<uint>", 6) == 0
                                  ? getParameterTagRuntime(
                                        s, findClosingTagRuntime(s, p)) *
                                            6 +
                                        2
                                  : (std::strncmp(s + p, "<float>", 7) == 0 ||
                                     std::strncmp(s + p, "<double>", 8) == 0)
                                        ? getParameterTagRuntime(
                                              s, findClosingTagRuntime(s, p)) *
                                                  6 +
                                              3
                                        : (std::strncmp(s + p, "<str>", 5) ==
                                               0 ||
                                           std::strncmp(s + p, "<string>", 8) ==
                                               0)
                                              ? getParameterTagRuntime(
                                                    s, findClosingTagRuntime(
                                                           s, p)) *
                                                        6 +
                                                    4
                                              : std::strncmp(s + p, "<path>",
                                                             6) == 0
                                                    ? getParameterTagRuntime(
                                                          s,
                                                          findClosingTagRuntime(
                                                              s, p)) *
                                                              6 +
                                                          5
                                                    : throw std::runtime_error(
                                                          "invalid parameter "
                                                          "type"))
                     : getParameterTagRuntime(s, p + 1);
}

constexpr uint64_t get_parameter_tag(ConstStr s, unsigned p = 0)
{
    return p == s.size()
               ? 0
               : s[p] == '<'
                     ? (isInt(s, p)
                            ? get_parameter_tag(s, findClosingTag(s, p)) * 6 + 1
                            : isUint(s, p)
                                  ? get_parameter_tag(s, findClosingTag(s, p)) *
                                            6 +
                                        2
                                  : isFloat(s, p)
                                        ? get_parameter_tag(
                                              s, findClosingTag(s, p)) *
                                                  6 +
                                              3
                                        : isStr(s, p)
                                              ? get_parameter_tag(
                                                    s, findClosingTag(s, p)) *
                                                        6 +
                                                    4
                                              : isPath(s, p)
                                                    ? get_parameter_tag(
                                                          s, findClosingTag(
                                                                 s, p)) *
                                                              6 +
                                                          5
                                                    : throw std::runtime_error(
                                                          "invalid parameter "
                                                          "type"))
                     : get_parameter_tag(s, p + 1);
}

template <typename... T> struct S
{
    template <typename U> using push = S<U, T...>;
    template <typename U> using push_back = S<T..., U>;
    template <template <typename... Args> class U> using rebind = U<T...>;
};
template <typename F, typename Set> struct CallHelper;
template <typename F, typename... Args> struct CallHelper<F, S<Args...>>
{
    template <typename F1, typename... Args1,
              typename = decltype(std::declval<F1>()(std::declval<Args1>()...))>
    static char __test(int);

    template <typename...> static int __test(...);

    static constexpr bool value = sizeof(__test<F, Args...>(0)) == sizeof(char);
};

template <uint64_t N> struct SingleTagToType
{
};

template <> struct SingleTagToType<1>
{
    using type = int64_t;
};

template <> struct SingleTagToType<2>
{
    using type = uint64_t;
};

template <> struct SingleTagToType<3>
{
    using type = double;
};

template <> struct SingleTagToType<4>
{
    using type = std::string;
};

template <> struct SingleTagToType<5>
{
    using type = std::string;
};

template <uint64_t Tag> struct Arguments
{
    using subarguments = typename Arguments<Tag / 6>::type;
    using type = typename subarguments::template push<
        typename SingleTagToType<Tag % 6>::type>;
};

template <> struct Arguments<0>
{
    using type = S<>;
};

template <typename... T> struct LastElementType
{
    using type =
        typename std::tuple_element<sizeof...(T) - 1, std::tuple<T...>>::type;
};

template <> struct LastElementType<>
{
};

// from
// http://stackoverflow.com/questions/13072359/c11-compile-time-array-with-logarithmic-evaluation-depth
template <class T> using Invoke = typename T::type;

template <unsigned...> struct Seq
{
    using type = Seq;
};

template <class S1, class S2> struct concat;

template <unsigned... I1, unsigned... I2>
struct concat<Seq<I1...>, Seq<I2...>> : Seq<I1..., (sizeof...(I1) + I2)...>
{
};

template <class S1, class S2> using Concat = Invoke<concat<S1, S2>>;

template <size_t N> struct gen_seq;
template <size_t N> using GenSeq = Invoke<gen_seq<N>>;

template <size_t N> struct gen_seq : Concat<GenSeq<N / 2>, GenSeq<N - N / 2>>
{
};

template <> struct gen_seq<0> : Seq<>
{
};
template <> struct gen_seq<1> : Seq<0>
{
};

template <typename Seq, typename Tuple> struct PopBackHelper;

template <unsigned... N, typename Tuple> struct PopBackHelper<Seq<N...>, Tuple>
{
    template <template <typename... Args> class U>
    using rebind = U<typename std::tuple_element<N, Tuple>::type...>;
};

template <typename... T>
struct PopBack //: public PopBackHelper<typename
               // gen_seq<sizeof...(T)-1>::type, std::tuple<T...>>
{
    template <template <typename... Args> class U>
    using rebind =
        typename PopBackHelper<typename gen_seq<sizeof...(T) - 1UL>::type,
                               std::tuple<T...>>::template rebind<U>;
};

template <> struct PopBack<>
{
    template <template <typename... Args> class U> using rebind = U<>;
};

// from
// http://stackoverflow.com/questions/2118541/check-if-c0x-parameter-pack-contains-a-type
template <typename Tp, typename... List> struct Contains : std::true_type
{
};

template <typename Tp, typename Head, typename... Rest>
struct Contains<Tp, Head, Rest...>
    : std::conditional<std::is_same<Tp, Head>::value, std::true_type,
                       Contains<Tp, Rest...>>::type
{
};

template <typename Tp> struct Contains<Tp> : std::false_type
{
};

template <typename T> struct EmptyContext
{
};

template <typename T> struct promote
{
    using type = T;
};

#define BMCWEB_INTERNAL_PROMOTE_TYPE(t1, t2)                                   \
    template <> struct promote<t1>                                             \
    {                                                                          \
        using type = t2;                                                       \
    }

BMCWEB_INTERNAL_PROMOTE_TYPE(char, int64_t);
BMCWEB_INTERNAL_PROMOTE_TYPE(short, int64_t);
BMCWEB_INTERNAL_PROMOTE_TYPE(int, int64_t);
BMCWEB_INTERNAL_PROMOTE_TYPE(long, int64_t);
BMCWEB_INTERNAL_PROMOTE_TYPE(long long, int64_t);
BMCWEB_INTERNAL_PROMOTE_TYPE(unsigned char, uint64_t);
BMCWEB_INTERNAL_PROMOTE_TYPE(unsigned short, uint64_t);
BMCWEB_INTERNAL_PROMOTE_TYPE(unsigned int, uint64_t);
BMCWEB_INTERNAL_PROMOTE_TYPE(unsigned long, uint64_t);
BMCWEB_INTERNAL_PROMOTE_TYPE(unsigned long long, uint64_t);
BMCWEB_INTERNAL_PROMOTE_TYPE(float, double);
#undef BMCWEB_INTERNAL_PROMOTE_TYPE

template <typename T> using promote_t = typename promote<T>::type;

} // namespace black_magic

namespace detail
{

template <class T, std::size_t N, class... Args>
struct GetIndexOfElementFromTupleByTypeImpl
{
    static constexpr std::size_t value = N;
};

template <class T, std::size_t N, class... Args>
struct GetIndexOfElementFromTupleByTypeImpl<T, N, T, Args...>
{
    static constexpr std::size_t value = N;
};

template <class T, std::size_t N, class U, class... Args>
struct GetIndexOfElementFromTupleByTypeImpl<T, N, U, Args...>
{
    static constexpr std::size_t value =
        GetIndexOfElementFromTupleByTypeImpl<T, N + 1, Args...>::value;
};

} // namespace detail

namespace utility
{
template <class T, class... Args> T& getElementByType(std::tuple<Args...>& t)
{
    return std::get<
        detail::GetIndexOfElementFromTupleByTypeImpl<T, 0, Args...>::value>(t);
}

template <typename T> struct function_traits;

template <typename T>
struct function_traits : public function_traits<decltype(&T::operator())>
{
    using parent_t = function_traits<decltype(&T::operator())>;
    static const size_t arity = parent_t::arity;
    using result_type = typename parent_t::result_type;
    template <size_t i> using arg = typename parent_t::template arg<i>;
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

inline static std::string base64encode(
    const char* data, size_t size,
    const char* key =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/")
{
    std::string ret;
    ret.resize((size + 2) / 3 * 4);
    auto it = ret.begin();
    while (size >= 3)
    {
        *it++ = key[(static_cast<unsigned char>(*data) & 0xFC) >> 2];
        unsigned char h = static_cast<unsigned char>(
            (static_cast<unsigned char>(*data++) & 0x03u) << 4u);
        *it++ = key[h | ((static_cast<unsigned char>(*data) & 0xF0) >> 4)];
        h = static_cast<unsigned char>(
            (static_cast<unsigned char>(*data++) & 0x0F) << 2u);
        *it++ = key[h | ((static_cast<unsigned char>(*data) & 0xC0) >> 6)];
        *it++ = key[static_cast<unsigned char>(*data++) & 0x3F];

        size -= 3;
    }
    if (size == 1)
    {
        *it++ = key[(static_cast<unsigned char>(*data) & 0xFC) >> 2];
        unsigned char h = static_cast<unsigned char>(
            (static_cast<unsigned char>(*data++) & 0x03) << 4u);
        *it++ = key[h];
        *it++ = '=';
        *it++ = '=';
    }
    else if (size == 2)
    {
        *it++ = key[(static_cast<unsigned char>(*data) & 0xFC) >> 2];
        unsigned char h = static_cast<unsigned char>(
            (static_cast<unsigned char>(*data++) & 0x03) << 4u);
        *it++ = key[h | ((static_cast<unsigned char>(*data) & 0xF0) >> 4)];
        h = static_cast<unsigned char>(
            (static_cast<unsigned char>(*data++) & 0x0F) << 2u);
        *it++ = key[h];
        *it++ = '=';
    }
    return ret;
}

inline static std::string base64encodeUrlsafe(const char* data, size_t size)
{
    return base64encode(
        data, size,
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_");
}

// TODO this is temporary and should be deleted once base64 is refactored out of
// crow
inline bool base64Decode(const std::string_view input, std::string& output)
{
    static const char nop = static_cast<char>(-1);
    // See note on encoding_data[] in above function
    static const char decodingData[] = {
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

    // for each 4-bytes sequence from the input, extract 4 6-bits sequences by
    // droping first two bits
    // and regenerate into 3 8-bits sequences

    for (size_t i = 0; i < inputLength; i++)
    {
        char base64code0;
        char base64code1;
        char base64code2 = 0; // initialized to 0 to suppress warnings
        char base64code3;

        base64code0 = decodingData[static_cast<int>(input[i])]; // NOLINT
        if (base64code0 == nop)
        { // non base64 character
            return false;
        }
        if (!(++i < inputLength))
        { // we need at least two input bytes for first
          // byte output
            return false;
        }
        base64code1 = decodingData[static_cast<int>(input[i])]; // NOLINT
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
            base64code2 = decodingData[static_cast<int>(input[i])]; // NOLINT
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
            base64code3 = decodingData[static_cast<int>(input[i])]; // NOLINT
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

inline void escapeHtml(std::string& data)
{
    std::string buffer;
    // less than 5% of characters should be larger, so reserve a buffer of the
    // right size
    buffer.reserve(data.size() * 11 / 10);
    for (size_t pos = 0; pos != data.size(); ++pos)
    {
        switch (data[pos])
        {
            case '&':
                buffer.append("&amp;");
                break;
            case '\"':
                buffer.append("&quot;");
                break;
            case '\'':
                buffer.append("&apos;");
                break;
            case '<':
                buffer.append("&lt;");
                break;
            case '>':
                buffer.append("&gt;");
                break;
            default:
                buffer.append(&data[pos], 1);
                break;
        }
    }
    data.swap(buffer);
}

inline void convertToLinks(std::string& s)
{
    // Convert anything with a redfish path into a link
    const static std::regex redfishPath{
        "(&quot;((.*))&quot;[ \\n]*:[ "
        "\\n]*)(&quot;((?!&quot;)/redfish/.*)&quot;)"};
    s = std::regex_replace(s, redfishPath, "$1<a href=\"$5\">$4</a>");
}

/**
 * Method returns Date Time information according to requested format
 *
 * @param[in] time time in second since the Epoch
 *
 * @return Date Time according to requested format
 */
inline std::string getDateTime(const std::time_t& time)
{
    std::array<char, 128> dateTime;
    std::string redfishDateTime("0000-00-00T00:00:00Z00:00");

    if (std::strftime(dateTime.begin(), dateTime.size(), "%FT%T%z",
                      std::localtime(&time)))
    {
        // insert the colon required by the ISO 8601 standard
        redfishDateTime = std::string(dateTime.data());
        redfishDateTime.insert(redfishDateTime.end() - 2, ':');
    }

    return redfishDateTime;
}

inline std::string dateTimeNow()
{
    std::time_t time = std::time(nullptr);
    return getDateTime(time);
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

} // namespace utility
} // namespace crow
