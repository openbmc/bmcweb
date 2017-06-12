// Copyright (c) Benjamin Kietzman (github.com/bkietz)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef DBUS_CONNECTION_SERVICE_HPP
#define DBUS_CONNECTION_SERVICE_HPP

#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>

#include <dbus/detail/async_send_op.hpp>
#include <dbus/element.hpp>
#include <dbus/error.hpp>
#include <dbus/message.hpp>

#include <dbus/impl/connection.ipp>

namespace dbus {
namespace bus {
static const int session = DBUS_BUS_SESSION;
static const int system = DBUS_BUS_SYSTEM;
static const int starter = DBUS_BUS_STARTER;
}  // namespace bus

class filter;
class match;
class connection;

class connection_service : public boost::asio::detail::service_base<connection_service> {
 public:
  typedef impl::connection implementation_type;

  inline explicit connection_service(boost::asio::io_service& io)
      : boost::asio::detail::service_base<connection_service>(io) {}

  inline void construct(implementation_type& impl) {}

  inline void destroy(implementation_type& impl) {}

  inline void shutdown_service() {
    // TODO is there anything that needs shutting down?
  }

  inline void open(implementation_type& impl, const string& address) {
    boost::asio::io_service& io = this->get_io_service();

    impl.open(io, address);
  }

  inline void open(implementation_type& impl, const int bus = bus::system) {
    boost::asio::io_service& io = this->get_io_service();

    impl.open(io, bus);
  }

  inline message send(implementation_type& impl, message& m) {
    return impl.send_with_reply_and_block(m);
  }

  template <typename Duration>
  inline message send(implementation_type& impl, message& m, const Duration& timeout) {
    if (timeout == Duration::zero()) {
      // TODO this can return false if it failed
      impl.send(m);
      return message();
    } else {
      return impl.send_with_reply_and_block(
          m, std::chrono::milliseconds(timeout).count());
    }
  }

  template <typename MessageHandler>
  inline BOOST_ASIO_INITFN_RESULT_TYPE(MessageHandler,
                                       void(boost::system::error_code, message))
      async_send(implementation_type& impl, message& m,
                 BOOST_ASIO_MOVE_ARG(MessageHandler) handler) {
    // begin asynchronous operation
    impl.start(this->get_io_service());

    boost::asio::detail::async_result_init<
        MessageHandler, void(boost::system::error_code, message)>
        init(BOOST_ASIO_MOVE_CAST(MessageHandler)(handler));
    detail::async_send_op<typename boost::asio::handler_type<
        MessageHandler, void(boost::system::error_code, message)>::type>(
        this->get_io_service(),
        BOOST_ASIO_MOVE_CAST(MessageHandler)(init.handler))(impl, m);

    return init.result.get();
  }

 private:
  friend connection;
  inline void new_match(implementation_type& impl, match& m);

  inline void delete_match(implementation_type& impl, match& m);

  inline void new_filter(implementation_type& impl, filter& f);

  inline void delete_filter(implementation_type& impl, filter& f);
};

}  // namespace dbus

#endif  // DBUS_CONNECTION_SERVICE_HPP
