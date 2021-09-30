#pragma once

#include <algorithm>
#include <string>
#include <vector>

namespace redfish
{

namespace stl_utils
{

inline bool removeDuplicates(std::vector<std::string>& strVec)
{
    bool ret = false;

    // Remove duplicate strings
    for (size_t i = 0; i < strVec.size(); i++)
    {
        if (std::count(strVec.begin(), strVec.end(), strVec[i]) == 1)
        {
            continue;
        }

        ret = true;
        auto ret = std::remove(strVec.begin() + static_cast<int>(i) + 1,
                               strVec.end(), strVec[i]);
        strVec.erase(ret, strVec.end());
    }

    return ret;
}
} // namespace stl_utils
} // namespace redfish
