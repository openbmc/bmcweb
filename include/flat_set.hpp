#pragma once

#include <boost/container/flat_set.hpp>

#include <utility>
#include <vector>

namespace bmcweb
{
template <typename Key>
using FlatSet = boost::container::flat_set<Key, std::less<>, std::vector<Key>>;
} // namespace bmcweb
