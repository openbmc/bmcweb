// Copyright (c) Benjamin Kietzman (github.com/bkietz)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef DBUS_CONNECTION_IPP
#define DBUS_CONNECTION_IPP

#include <dbus/dbus.h>
#include <dbus/detail/watch_timeout.hpp>

#include <boost/atomic.hpp>

namespace dbus {
namespace impl {

class connection {
 public:
  boost::atomic<bool> is_paused;
  DBusConnection* conn;

  connection() : is_paused(true), conn(NULL) {}

  void open(boost::asio::io_service& io, int bus) {
    error e;
    conn = dbus_bus_get_private((DBusBusType)bus, e);
    e.throw_if_set();

    dbus_connection_set_exit_on_disconnect(conn, false);

    detail::set_watch_timeout_dispatch_functions(conn, io);
  }

  void open(boost::asio::io_service& io, const string& address) {
    error e;
    conn = dbus_connection_open_private(address.c_str(), e);
    e.throw_if_set();

    dbus_bus_register(conn, e);
    e.throw_if_set();

    dbus_connection_set_exit_on_disconnect(conn, false);

    detail::set_watch_timeout_dispatch_functions(conn, io);
  }

  ~connection() {
    if (conn != NULL) {
      dbus_connection_close(conn);
      dbus_connection_unref(conn);
    }
  }

  operator DBusConnection*() { return conn; }
  operator const DBusConnection*() const { return conn; }

  message send_with_reply_and_block(message& m,
                                    int timeout_in_milliseconds = -1) {
    error e;
    DBusMessage* out = dbus_connection_send_with_reply_and_block(
        conn, m, timeout_in_milliseconds, e);
    e.throw_if_set();
    message reply(out);

    return reply;
  }

  void send(message& m) {
    // ignoring message serial for now
    dbus_connection_send(conn, m, NULL);
  }

  void send_with_reply(message& m, DBusPendingCall** p,
                       int timeout_in_milliseconds = -1) {
    dbus_connection_send_with_reply(conn, m, p, timeout_in_milliseconds);
  }

  // begin asynchronous operation
  // FIXME should not get io from an argument
  void start(boost::asio::io_service& io) {
    bool old_value(true);
    if (is_paused.compare_exchange_strong(old_value, false)) {
      // If two threads call connection::async_send()
      // simultaneously on a paused connection, then
      // only one will pass the CAS instruction and
      // only one dispatch_handler will be injected.
      io.post(detail::dispatch_handler(io, conn));
    }
  }

  void cancel(boost::asio::io_service& io) {
    bool old_value(false);
    if (is_paused.compare_exchange_strong(old_value, true)) {
      // TODO
    }
  }
};

}  // namespace impl
}  // namespace dbus

#endif  // DBUS_CONNECTION_IPP
