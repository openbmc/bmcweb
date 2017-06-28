// Copyright (c) Benjamin Kietzman (github.com/bkietz)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef DBUS_MATCH_HPP
#define DBUS_MATCH_HPP

#include <string>
#include <boost/asio.hpp>

#include <dbus/connection.hpp>
#include <dbus/error.hpp>

namespace dbus {

/// Simple placeholder object for a match rule.
/**
 * A match rule determines what messages will be received by this application.
 *
 * Each rule will be represented by an instance of match. To remove that rule,
 * dispose of the object.
 */
class match {
  connection_ptr connection_;
  std::string expression_;

 public:
  match(connection_ptr c, BOOST_ASIO_MOVE_ARG(std::string) e)
      : connection_(c), expression_(BOOST_ASIO_MOVE_CAST(std::string)(e)) {
    connection_->new_match(*this);
  }

  ~match() { connection_->delete_match(*this); }

  const std::string& get_expression() const { return expression_; }

  match(match&&) = delete;
  match& operator=(match&&) = delete;
};

}  // namespace dbus

#include <dbus/impl/match.ipp>

#endif  // DBUS_MATCH_HPP
