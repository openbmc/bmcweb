// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "logging.hpp"

#include <boost/asio/buffer.hpp>
#include <boost/asio/connect_pipe.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/readable_pipe.hpp>
#include <boost/asio/writable_pipe.hpp>
#include <boost/asio/write.hpp>

#include <array>
#include <string>

// Wrapper for boost::async_pipe ensuring proper pipe cleanup
class CredentialsPipe
{
  public:
    explicit CredentialsPipe(boost::asio::io_context& io) : impl(io), read(io)
    {
        boost::system::error_code ec;
        boost::asio::connect_pipe(read, impl, ec);
        if (ec)
        {
            BMCWEB_LOG_CRITICAL("Failed to connect pipe {}", ec.what());
        }
    }

    CredentialsPipe(const CredentialsPipe&) = delete;
    CredentialsPipe(CredentialsPipe&&) = delete;
    CredentialsPipe& operator=(const CredentialsPipe&) = delete;
    CredentialsPipe& operator=(CredentialsPipe&&) = delete;

    ~CredentialsPipe()
    {
        explicit_bzero(user.data(), user.capacity());
        explicit_bzero(pass.data(), pass.capacity());
    }

    int releaseFd()
    {
        return read.release();
    }

    template <typename WriteHandler>
    void asyncWrite(std::string&& username, std::string&& password,
                    WriteHandler&& handler)
    {
        user = std::move(username);
        pass = std::move(password);

        // Add +1 to ensure that the null terminator is included.
        std::array<boost::asio::const_buffer, 2> buffer{
            {{user.data(), user.size() + 1}, {pass.data(), pass.size() + 1}}};
        boost::asio::async_write(impl, buffer,
                                 std::forward<WriteHandler>(handler));
    }

    boost::asio::writable_pipe impl;
    boost::asio::readable_pipe read;

  private:
    std::string user;
    std::string pass;
};
