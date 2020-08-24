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

constexpr unsigned findClosingTag(std::string_view s, unsigned p)
{
    return s[p] == '>' ? p : findClosingTag(s, p + 1);
}

constexpr bool isInt(std::string_view s, unsigned i)
{
    return s.substr(i, 5) == "<int>";
}

constexpr bool isUint(std::string_view s, unsigned i)
{
    return s.substr(i, 6) == "<uint>";
}

constexpr bool isFloat(std::string_view s, unsigned i)
{
    return s.substr(i, 7) == "<float>" || s.substr(i, 8) == "<double>";
}

constexpr bool isStr(std::string_view s, unsigned i)
{
    return s.substr(i, 5) == "<str>" || s.substr(i, 8) == "<string>";
}

constexpr bool isPath(std::string_view s, unsigned i)
{
    return s.substr(i, 6) == "<path>";
}

template <typename T>
constexpr int getParameterTag()
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
        getParameterTag<typename std::decay<Arg>::type>()
            ? subValue * 6 + getParameterTag<typename std::decay<Arg>::type>()
            : subValue;
};

inline bool isParameterTagCompatible(uint64_t a, uint64_t b)
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

constexpr uint64_t getParameterTag(std::string_view s, unsigned p = 0)
{
    if (p == s.size())
    {
        return 0;
    }

    if (s[p] != '<')
    {
        return getParameterTag(s, p + 1);
    }

    if (isInt(s, p))
    {
        return getParameterTag(s, findClosingTag(s, p)) * 6 + 1;
    }

    if (isUint(s, p))
    {
        return getParameterTag(s, findClosingTag(s, p)) * 6 + 2;
    }

    if (isFloat(s, p))
    {
        return getParameterTag(s, findClosingTag(s, p)) * 6 + 3;
    }

    if (isStr(s, p))
    {
        return getParameterTag(s, findClosingTag(s, p)) * 6 + 4;
    }

    if (isPath(s, p))
    {
        return getParameterTag(s, findClosingTag(s, p)) * 6 + 5;
    }

    throw std::runtime_error("invalid parameter type");
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
template <class T, class... Args>
T& getElementByType(std::tuple<Args...>& t)
{
    return std::get<
        detail::GetIndexOfElementFromTupleByTypeImpl<T, 0, Args...>::value>(t);
}

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
        char base64code0;
        char base64code1;
        char base64code2 = 0; // initialized to 0 to suppress warnings
        char base64code3;

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

inline std::time_t getTimestamp(uint64_t millisTimeStamp)
{
    // Retrieve Created property with format:
    // yyyy-mm-ddThh:mm:ss
    std::chrono::milliseconds chronoTimeStamp(millisTimeStamp);
    return std::chrono::duration_cast<std::chrono::duration<int>>(
               chronoTimeStamp)
        .count();
}

} // namespace utility
} // namespace crow
