#pragma once

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
    while ((start = str.find_first_not_of(delim, end)) != std::string::npos)
    {
        end = str.find(delim, start);
        strings.emplace_back(str.substr(start, end - start));
    }
}
} // namespace bmcweb
