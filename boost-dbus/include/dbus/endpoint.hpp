// Copyright (c) Benjamin Kietzman (github.com/bkietz)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef DBUS_ENDPOINT_HPP
#define DBUS_ENDPOINT_HPP

#include <dbus/dbus.h>
#include <dbus/element.hpp>
#include <dbus/message.hpp>

namespace dbus {

class endpoint {
  string process_name_;
  string path_;
  string interface_;

 public:
  endpoint(const string& process_name, const string& path,
           const string& interface)
      : process_name_(process_name), path_(path), interface_(interface) {}

  const string& get_path() const { return path_; }

  const string& get_interface() const { return interface_; }

  const string& get_process_name() const { return process_name_; }
};

}  // namespace dbus

#endif  // DBUS_ENDPOINT_HPP
