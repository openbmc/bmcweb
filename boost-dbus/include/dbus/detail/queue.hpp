// Copyright (c) Benjamin Kietzman (github.com/bkietz)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef DBUS_QUEUE_HPP
#define DBUS_QUEUE_HPP

#include <deque>
#include <functional>
#include <boost/asio.hpp>
#include <boost/asio/detail/mutex.hpp>

namespace dbus {
namespace detail {

template <typename Message>
class queue {
 public:
  typedef ::boost::asio::detail::mutex mutex_type;
  typedef Message message_type;
  typedef std::function<void(boost::system::error_code, Message)> handler_type;

 private:
  boost::asio::io_service& io;
  mutex_type mutex;
  std::deque<message_type> messages;
  std::deque<handler_type> handlers;

 public:
  queue(boost::asio::io_service& io_service) : io(io_service) {}

  queue(const queue<Message>& m)
      : io(m.io), messages(m.messages), handlers(m.handlers) {
        //TODO(ed) acquire the lock before copying messages and handlers
      }

 private:
  class closure {
    handler_type handler_;
    message_type message_;
    boost::system::error_code error_;

   public:
    void operator()() { handler_(error_, message_); }
    closure(handler_type h, Message m,
            boost::system::error_code e = boost::system::error_code())
        : handler_(h), message_(m), error_(e) {}
  };

 public:
  void push(message_type m) {
    mutex_type::scoped_lock lock(mutex);
    if (handlers.empty())
      messages.push_back(m);
    else {
      handler_type h = handlers.front();
      handlers.pop_front();

      lock.unlock();

      io.post(closure(h, m));
    }
  }

  template <typename MessageHandler>
  inline BOOST_ASIO_INITFN_RESULT_TYPE(MessageHandler,
                                       void(boost::system::error_code,
                                            message_type))
      async_pop(BOOST_ASIO_MOVE_ARG(MessageHandler) h) {
    typedef ::boost::asio::detail::async_result_init<
        MessageHandler, void(boost::system::error_code, message_type)>
        init_type;

    mutex_type::scoped_lock lock(mutex);
    if (messages.empty()) {
      init_type init(BOOST_ASIO_MOVE_CAST(MessageHandler)(h));

      handlers.push_back(init.handler);

      lock.unlock();

      return init.result.get();

    } else {
      message_type m = messages.front();
      messages.pop_front();

      lock.unlock();

      init_type init(BOOST_ASIO_MOVE_CAST(MessageHandler)(h));

      io.post(closure(init.handler, m));

      return init.result.get();
    }
  }
};

}  // namespace detail
}  // namespace dbus

#endif  // DBUS_QUEUE_HPP
