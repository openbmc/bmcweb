#pragma once

#include <iterator>
#include <regex>
#include <string>
#include <vector>

namespace redfish
{
namespace string_util
{

inline std::vector<std::string> split(const std::string& input,
                                      const std::string& delim)
{
    std::vector<std::string> ret;
    try
    {
        std::regex reg{delim};
        ret = std::vector<std::string>{
            std::sregex_token_iterator(input.begin(), input.end(), reg, -1),
            std::sregex_token_iterator()};

        for (auto& v : ret)
        {
            v.erase(0, v.find_first_not_of(" "));
            v.erase(v.find_last_not_of(" ") + 1);
        }
    }
    catch (const std::exception& e)
    {}

    return ret;
}

} // namespace string_util
} // namespace redfish
