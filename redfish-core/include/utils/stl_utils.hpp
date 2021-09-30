#pragma once

#include <algorithm>
#include <string>
#include <vector>

namespace redfish
{

namespace stl_utils
{

inline void removeDuplicates(std::vector<std::string>& strVec)
{
    // Remove empty string
    auto ret = std::remove(strVec.begin(), strVec.end(), "");
    strVec.erase(ret, strVec.end());

    // Remove duplicate strings
    for (size_t i = 0; i < strVec.size(); i++)
    {
        int num = std::count(strVec.begin(), strVec.end(), strVec[i]);
        if (num == 1)
        {
            continue;
        }

        auto ret = std::remove(strVec.begin() + static_cast<int>(i) + 1,
                               strVec.end(), strVec[i]);
        strVec.erase(ret, strVec.end());
    }
}
} // namespace stl_utils
} // namespace redfish
