// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "logging.hpp"

#include <systemd/sd-daemon.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/system/error_code.hpp>

#include <chrono>
#include <functional>
#include <memory>

namespace bmcweb
{

class ServiceWD
{
  public:
    ServiceWD(const int expiryInS,
              boost::asio::io_context& io) :
        timer(io), expiryTimeInS(expiryInS)
    {
        timer.expires_after(std::chrono::seconds(expiryTimeInS));
        handler = [this](const boost::system::error_code& error) {
            if (error)
            {
                BMCWEB_LOG_ERROR("ServiceWD async_wait failed: {}",
                                 error.message());
            }
            sd_notify(0, "WATCHDOG=1");
            timer.expires_after(std::chrono::seconds(this->expiryTimeInS));
            timer.async_wait(handler);
        };
        timer.async_wait(handler);
    }

  private:
    boost::asio::steady_timer timer;
    const int expiryTimeInS;
    std::function<void(const boost::system::error_code& error)> handler;
};

} // namespace bmcweb
