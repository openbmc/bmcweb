// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <boost/asio/io_context.hpp>
#include <boost/url/url_view.hpp>

#include <functional>
#include <memory>
#include <string_view>

namespace crow
{
namespace websocket
{

enum class MessageType
{
    Binary,
    Text,
};

struct Connection : std::enable_shared_from_this<Connection>
{
  public:
    Connection() = default;

    Connection(const Connection&) = delete;
    Connection(Connection&&) = delete;
    Connection& operator=(const Connection&) = delete;
    Connection& operator=(const Connection&&) = delete;

    virtual void sendBinary(std::string_view msg) = 0;
    virtual void sendEx(MessageType type, std::string_view msg,
                        std::function<void()>&& onDone) = 0;
    virtual void sendText(std::string_view msg) = 0;
    virtual void close(std::string_view msg = "quit") = 0;
    virtual void deferRead() = 0;
    virtual void resumeRead() = 0;
    virtual ~Connection() = default;
    virtual boost::urls::url_view url() = 0;
};
} // namespace websocket
} // namespace crow
