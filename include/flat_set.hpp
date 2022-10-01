#pragma once

#include <boost/container/flat_set.hpp>

#include <utility>
#include <vector>

namespace bmcweb
{
template <typename Key>
struct FlatSet :
    public boost::container::flat_set<Key, std::less<>, std::vector<Key>>
{};
} // namespace bmcweb
