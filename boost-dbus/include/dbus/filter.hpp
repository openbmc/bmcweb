// Copyright (c) Benjamin Kietzman (github.com/bkietz)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef DBUS_FILTER_HPP
#define DBUS_FILTER_HPP

#include <dbus/connection.hpp>
#include <dbus/detail/queue.hpp>
#include <dbus/message.hpp>
#include <functional>
#include <boost/asio.hpp>

namespace dbus {

/// Represents a filter of incoming messages.
/**
 * Filters examine incoming messages, demuxing them to multiple queues.
 */
class filter {
  connection_ptr connection_;
  std::function<bool(message&)> predicate_;
  detail::queue<message> queue_;

 public:
  bool offer(message& m) {
    bool filtered = predicate_(m);
    if (filtered) queue_.push(m);
    return filtered;
  }

  template <typename MessagePredicate>
  filter(connection_ptr c, BOOST_ASIO_MOVE_ARG(MessagePredicate) p)
      : connection_(c),
        predicate_(BOOST_ASIO_MOVE_CAST(MessagePredicate)(p)),
        queue_(connection_->get_io_service()) {
    connection_->new_filter(*this);
  }

  ~filter() { connection_->delete_filter(*this); }

  template <typename MessageHandler>
  inline BOOST_ASIO_INITFN_RESULT_TYPE(MessageHandler,
                                       void(boost::system::error_code, message))
      async_dispatch(BOOST_ASIO_MOVE_ARG(MessageHandler) handler) {
    // begin asynchronous operation
    connection_->get_implementation().start(connection_->get_io_service());

    return queue_.async_pop(BOOST_ASIO_MOVE_CAST(MessageHandler)(handler));
  }

};
}  // namespace dbus

#include <dbus/impl/filter.ipp>
#endif  // DBUS_FILTER_HPP
