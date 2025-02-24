// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "bmcweb_config.h"

#include "logging.hpp"

#include <systemd/sd-daemon.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/system/error_code.hpp>

#include <chrono>

namespace bmcweb
{

class ServiceWD
{
  public:
    explicit ServiceWD(boost::asio::io_context& io) : timer(io)
    {
        timer.expires_after(std::chrono::seconds(BMCWEB_WATCHDOG_TIMEOUT / 2));
        timer.async_wait(std::bind_front(&ServiceWD::handleTimeout, this));
    }

  private:
    void handleTimeout(const boost::system::error_code& error)
    {
        if (error)
        {
            BMCWEB_LOG_ERROR("ServiceWD async_wait failed: {}",
                             error.message());
        }

        int rc = sd_notify(0, "WATCHDOG=1");
        if (rc < 0)
        {
            BMCWEB_LOG_ERROR("ServiceWD sd_notify failed: {}", -rc);
            // Even on error cases we will retry after the expiry time
        }

        timer.expires_after(std::chrono::seconds(BMCWEB_WATCHDOG_TIMEOUT / 2));
        timer.async_wait(std::bind_front(&ServiceWD::handleTimeout, this));
    }

    boost::asio::steady_timer timer;
};

} // namespace bmcweb
