// Copyright (c) Benjamin Kietzman (github.com/bkietz)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <dbus/connection.hpp>
#include <dbus/endpoint.hpp>
#include <dbus/filter.hpp>
#include <dbus/match.hpp>
#include <dbus/message.hpp>
#include <functional>

#include <unistd.h>
#include <gtest/gtest.h>

using namespace boost::asio;
using namespace dbus;
using boost::system::error_code;

TEST(ErrorTest, GetHostName) {
  io_service io;
  EXPECT_THROW(connection system_bus(io, "unix:path=/foo/bar/baz_socket"),
               boost::system::system_error);

  io.run();
}