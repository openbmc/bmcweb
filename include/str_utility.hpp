// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include <algorithm>
#include <cstddef>
#include <ranges>
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
    if (str.empty())
    {
        strings.emplace_back("");
    }
    for (const auto value : std::views::split(str, delim))
    {
        strings.emplace_back(std::string_view{value});
    }
}

template <size_t N>
inline bool splitn(std::array<std::string_view, N>& strings,
                   std::string_view str, char delim)
{
    auto sv = std::views::split(str, delim);
    if (static_cast<size_t>(std::ranges::distance(sv.begin(), sv.end())) !=
        strings.size())
    {
        return false;
    }
    for (const auto& [out, split] : std::views::zip(strings, sv))
    {
        out = std::string_view(split);
    }
    return true;
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

} // namespace bmcweb
