#pragma once

#include <boost/container/flat_map.hpp>

#include <utility>
#include <vector>

namespace crow
{
template <typename Key, typename Value>
struct flat_map :
    public boost::container::flat_map<Key, Value, std::less<>,
                                      std::vector<std::pair<Key, Value>>>
{};
} // namespace crow
