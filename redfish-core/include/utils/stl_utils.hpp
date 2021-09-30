#pragma once

#include <algorithm>
#include <string>
#include <vector>

namespace redfish
{

namespace stl_utils
{

template <typename ForwardIterator>
ForwardIterator hasDuplicates(ForwardIterator first, ForwardIterator last)
{
    auto newLast = first;

    for (auto current = first; current != last; ++current)
    {
        if (std::find(first, newLast, *current) == newLast)
        {
            if (newLast != current)
            {
                *newLast = *current;
            }
            ++newLast;
        }
    }

    return newLast;
}
} // namespace stl_utils
} // namespace redfish
