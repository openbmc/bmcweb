// Copyright (c) Benjamin Kietzman (github.com/bkietz)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef DBUS_IMPL_MESSAGE_ITERATOR_IPP
#define DBUS_IMPL_MESSAGE_ITERATOR_IPP

#include <dbus/impl/message_iterator.hpp>

namespace dbus {
namespace impl {

inline void message_iterator::init_append(message& m, message_iterator& i)
{
  dbus_message_iter_init_append(m, &i.DBusMessageIter_);
}
inline void message_iterator::append_basic(int code, const void *value)
{
  // returns false if not enough memory- throw bad_alloc
  dbus_message_iter_append_basic(&DBusMessageIter_, code, value);
}
inline void message_iterator::open_container(int code, const char *signature, message_iterator& sub)
{
  // returns false if not enough memory- throw bad_alloc
  dbus_message_iter_open_container(&DBusMessageIter_, code, signature, &sub.DBusMessageIter_);
}

inline void message_iterator::close_container(message_iterator& sub)
{
  // returns false if not enough memory- throw bad_alloc
  dbus_message_iter_close_container(&DBusMessageIter_, &sub.DBusMessageIter_);
}

inline void message_iterator::abandon_container(message_iterator& sub)
{
  dbus_message_iter_abandon_container(&DBusMessageIter_, &sub.DBusMessageIter_);
}

inline void message_iterator::append_fixed_array(int code, const void *value, int n_elements)
{
  // returns false if not enough memory- throw bad_alloc
  dbus_message_iter_append_fixed_array(&DBusMessageIter_, code, value, n_elements);
}

inline bool message_iterator::init(message& m, message_iterator& i)
{
  return dbus_message_iter_init(m, &i.DBusMessageIter_);
}

inline bool message_iterator::next()
{
  return dbus_message_iter_next(&DBusMessageIter_);
}

inline bool message_iterator::has_next()
{
  return dbus_message_iter_has_next(&DBusMessageIter_);
}

inline int message_iterator::get_arg_type()
{
  return dbus_message_iter_get_arg_type(&DBusMessageIter_);
}

inline void message_iterator::get_basic(void *value)
{
  dbus_message_iter_get_basic(&DBusMessageIter_, value);
}

inline void message_iterator::recurse(message_iterator& sub)
{
  dbus_message_iter_recurse(&DBusMessageIter_, &sub.DBusMessageIter_);
}

inline int message_iterator::get_element_type()
{
  return dbus_message_iter_get_element_type(&DBusMessageIter_);
}

inline void message_iterator::get_fixed_array(void *value, int *n_elements)
{
  dbus_message_iter_get_fixed_array(&DBusMessageIter_, value, n_elements);
}

} // namespace impl
} // namespace dbus

#endif // DBUS_IMPL_MESSAGE_ITERATOR_IPP
