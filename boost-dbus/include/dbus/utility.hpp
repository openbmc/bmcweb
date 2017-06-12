// Copyright (c) Ed Tanous
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef DBUS_UTILITY_HPP
#define DBUS_UTILITY_HPP

#include <boost/iostreams/stream.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace dbus {

inline void read_dbus_xml_names(std::string& xml_data_in,
                                std::vector<std::string>& values_out) {
  // populate tree structure pt
  using boost::property_tree::ptree;
  ptree pt;
  std::cout << xml_data_in;
  std::stringstream ss;
  ss << xml_data_in;
  read_xml(ss, pt);
  
  // traverse node to find other nodes
  for (const auto& interface : pt.get_child("node")) {
    if (interface.first == "node") {
      auto t = interface.second.get<std::string>("<xmlattr>", "default");
      for (const auto& subnode : interface.second.get_child("<xmlattr>")) {
        if (subnode.first == "name") {
          std::string t = subnode.second.get("", "unknown");
          values_out.push_back(t);
        }
      }
    }
  }
}
}
#endif  // DBUS_UTILITY_HPP