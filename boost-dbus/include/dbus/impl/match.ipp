// Copyright (c) Benjamin Kietzman (github.com/bkietz)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef DBUS_MATCH_IPP
#define DBUS_MATCH_IPP

namespace dbus {
void connection_service::new_match(implementation_type& impl, match& m) {
  error e;
  dbus_bus_add_match(impl, m.get_expression().c_str(), e);
  e.throw_if_set();
  // eventually, for complete asynchronicity, this should connect to
  // org.freedesktop.DBus and call AddMatch
}

void connection_service::delete_match(implementation_type& impl, match& m) {
  error e;
  dbus_bus_remove_match(impl, m.get_expression().c_str(), e);
  e.throw_if_set();
}

}  // namespace dbus

#endif  // DBUS_MATCH_IPP
