#pragma once

#include <algorithm>
#include <iostream>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/functional/hash.hpp>

namespace crow {

struct ci_key_eq {
  bool operator()(const std::string& left, const std::string& right) const {
    unsigned int lsz = left.size();
    unsigned int rsz = right.size();
    for (unsigned int i = 0; i < std::min(lsz, rsz); ++i) {
      auto lchar = tolower(left[i]);
      auto rchar = tolower(right[i]);
      if (lchar != rchar) {
        return lchar < rchar;
      }
    }

    if (rsz != lsz) {
      return lsz < rsz;
    }
    return 0;
  }
};

using ci_map = boost::container::flat_map<std::string, std::string, ci_key_eq>;
}
