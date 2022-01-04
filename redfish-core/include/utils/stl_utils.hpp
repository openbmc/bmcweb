#pragma once

#include <algorithm>
#include <vector>

namespace redfish
{

namespace stl_utils
{

template <typename ForwardIterator>
ForwardIterator firstDuplicate(ForwardIterator first, ForwardIterator last)
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

template <typename T>
void removeDuplicate(T& t)
{
    t.erase(firstDuplicate(t.begin(), t.end()), t.end());
}

} // namespace stl_utils
} // namespace redfish
