#pragma once

#include <boost/container/flat_map.hpp>

#include <utility>
#include <vector>

namespace bmcweb
{
template <typename Key, typename Value>
struct FlatMap :
    public boost::container::flat_map<Key, Value, std::less<>,
                                      std::vector<std::pair<Key, Value>>>
{};
} // namespace bmcweb
