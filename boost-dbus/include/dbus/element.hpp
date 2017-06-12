// Copyright (c) Benjamin Kietzman (github.com/bkietz)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef DBUS_ELEMENT_HPP
#define DBUS_ELEMENT_HPP

#include <dbus/dbus.h>
#include <string>
#include <boost/cstdint.hpp>

namespace dbus {

/// Message elements
/**
 * D-Bus Messages are composed of simple elements of one of these types
 */
// bool // is this simply valid? It might pack wrong...
// http://maemo.org/api_refs/5.0/5.0-final/dbus/api/group__DBusTypes.html
typedef boost::uint8_t byte;

typedef boost::int16_t int16;
typedef boost::uint16_t uint16;
typedef boost::int32_t int32;
typedef boost::uint32_t uint32;

typedef boost::int64_t int64;
typedef boost::uint64_t uint64;
// double
// unix_fd

typedef std::string string;
struct object_path {
  string value;
};
struct signature {
  string value;
};

/// Traits template for message elements
/**
 * D-Bus Message elements are identified by unique integer type codes.
 */
template <typename InvalidType>
struct element {
  static const int code = DBUS_TYPE_INVALID;
};

template <>
struct element<bool> {
  static const int code = DBUS_TYPE_BOOLEAN;
};

template <>
struct element<byte> {
  static const int code = DBUS_TYPE_BYTE;
};

template <>
struct element<int16> {
  static const int code = DBUS_TYPE_INT16;
};

template <>
struct element<uint16> {
  static const int code = DBUS_TYPE_UINT16;
};

template <>
struct element<int32> {
  static const int code = DBUS_TYPE_INT32;
};

template <>
struct element<uint32> {
  static const int code = DBUS_TYPE_UINT32;
};

template <>
struct element<int64> {
  static const int code = DBUS_TYPE_INT64;
};

template <>
struct element<uint64> {
  static const int code = DBUS_TYPE_UINT64;
};

template <>
struct element<double> {
  static const int code = DBUS_TYPE_DOUBLE;
};

template <>
struct element<string> {
  static const int code = DBUS_TYPE_STRING;
};

template <>
struct element<object_path> {
  static const int code = DBUS_TYPE_OBJECT_PATH;
};

template <>
struct element<signature> {
  static const int code = DBUS_TYPE_SIGNATURE;
};

template <typename InvalidType>
struct is_fixed_type {
  static const int value = false;
};

template <>
struct is_fixed_type<bool> {
  static const int value = true;
};

template <>
struct is_fixed_type<byte> {
  static const int value = true;
};

template <>
struct is_fixed_type<int16> {
  static const int value = true;
};

template <>
struct is_fixed_type<uint16> {
  static const int value = true;
};

template <>
struct is_fixed_type<int32> {
  static const int value = true;
};

template <>
struct is_fixed_type<uint32> {
  static const int value = true;
};

template <>
struct is_fixed_type<int64> {
  static const int value = true;
};

template <>
struct is_fixed_type<uint64> {
  static const int value = true;
};

template <>
struct is_fixed_type<double> {
  static const int value = true;
};

template <typename InvalidType>
struct is_string_type {
  static const bool value = false;
};

template <>
struct is_string_type<string> {
  static const bool value = true;
};

template <>
struct is_string_type<object_path> {
  static const bool value = true;
};

template <>
struct is_string_type<signature> {
  static const bool value = true;
};

}  // namespace dbus

#endif  // DBUS_ELEMENT_HPP
