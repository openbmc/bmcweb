#pragma once

#include <boost/container/flat_set.hpp>
#include <vector>
#include <utility>

namespace crow{
template <typename Key>
struct flat_set: public boost::container::flat_set<Key, std::less<>, std::vector<Key>>{
};
}
