// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include "boost_formatters.hpp"

#include <boost/beast/http/write.hpp>

#include <memory>
#include <string_view>

namespace crow
{

namespace sse_socket
{
struct Connection : public std::enable_shared_from_this<Connection>
{
  public:
    Connection() = default;

    Connection(const Connection&) = delete;
    Connection(Connection&&) = delete;
    Connection& operator=(const Connection&) = delete;
    Connection& operator=(const Connection&&) = delete;
    virtual ~Connection() = default;

    virtual void close(std::string_view msg = "quit") = 0;
    virtual void sendSseEvent(std::string_view id, std::string_view msg) = 0;
};
} // namespace sse_socket
} // namespace crow
