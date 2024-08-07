#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace bmcweb
{
// This is a naive replacement for boost::split until
// https://github.com/llvm/llvm-project/issues/40486
// is resolved
inline void split(std::vector<std::string>& strings, std::string_view str,
                  char delim)
{
    size_t start = 0;
    size_t end = 0;
    while (end <= str.size())
    {
        end = str.find(delim, start);
        strings.emplace_back(str.substr(start, end - start));
        start = end + 1;
    }
}

inline char asciiToLower(char c)
{
    // Converts a character to lower case without relying on std::locale
    if ('A' <= c && c <= 'Z')
    {
        c -= ('A' - 'a');
    }
    return c;
}

inline bool asciiIEquals(std::string_view left, std::string_view right)
{
    return std::ranges::equal(left, right, [](char lChar, char rChar) {
        return asciiToLower(lChar) == asciiToLower(rChar);
    });
}

/**
 * Method returns ASCII string of a 64 bit number if ascii convertable
 *
 * @param[in] 64 bit value
 *
 * @return ascii converted string
 */

inline std::string convertToAscii(const uint64_t& element)
{
    uint64_t tmpelement = element;
    uint8_t* p = static_cast<uint8_t*>(static_cast<void*>(&tmpelement));
    std::span<unsigned char> bytearray{p, 8};

    if (std::count_if(bytearray.begin(), bytearray.end(), [](unsigned char c) {
        return (std::isprint(c) == 0);
    }) != 0)
    {
        return {};
    }

    return {std::string(bytearray.begin(), bytearray.end())};
}

} // namespace bmcweb
