// Copyright (c) Benjamin Kietzman (github.com/bkietz)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef DBUS_IMPL_MESSAGE_ITERATOR_HPP
#define DBUS_IMPL_MESSAGE_ITERATOR_HPP

#include <dbus/dbus.h>

namespace dbus {

class message;

namespace impl {

class message_iterator {
  DBusMessageIter DBusMessageIter_;

 public:
  // writing
  static void init_append(message &m, message_iterator &i);

  void append_basic(int code, const void *value);

  void open_container(int code, const char *signature, message_iterator &);
  void close_container(message_iterator &);
  void abandon_container(message_iterator &);

  void append_fixed_array(int code, const void *value, int n_elements);

  // reading
  static bool init(message &m, message_iterator &i);

  bool next();
  bool has_next();
  char get_arg_type();
  int get_element_count();

  void get_basic(void *value);

  void recurse(message_iterator &);

  int get_element_type();
  void get_fixed_array(void *value, int *n_elements);
};

}  // namespace impl
}  // namespace dbus

#endif  // DBUS_IMPL_MESSAGE_ITERATOR_HPP
