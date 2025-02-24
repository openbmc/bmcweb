// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "bmcweb_config.h"

#include "io_context_singleton.hpp"
#include "logging.hpp"

#include <systemd/sd-daemon.h>

#include <boost/asio/steady_timer.hpp>
#include <boost/system/error_code.hpp>

#include <chrono>
#include <cstdint>
#include <functional>
#include <ratio>

namespace bmcweb
{

class ServiceWatchdog
{
  public:
    ServiceWatchdog() : timer(getIoContext())
    {
        uint64_t usecondTimeout = 0;
        if (sd_watchdog_enabled(0, &usecondTimeout) <= 0)
        {
            if (BMCWEB_WATCHDOG_TIMEOUT_SECONDS > 0)
            {
                BMCWEB_LOG_WARNING(
                    "Watchdog timeout was enabled at compile time, but disabled at runtime");
            }
            return;
        }
        // Pet the watchdog N times faster than required.
        uint64_t petRatio = 4;
        watchdogTime = std::chrono::duration<uint64_t, std::micro>(
            usecondTimeout / petRatio);
        startTimer();
    }

  private:
    void startTimer()
    {
        timer.expires_after(watchdogTime);
        timer.async_wait(
            std::bind_front(&ServiceWatchdog::handleTimeout, this));
    }

    void handleTimeout(const boost::system::error_code& ec)
    {
        if (ec)
        {
            BMCWEB_LOG_ERROR("Watchdog timer async_wait failed: {}",
                             ec.message());
            return;
        }

        int rc = sd_notify(0, "WATCHDOG=1");
        if (rc < 0)
        {
            BMCWEB_LOG_ERROR("sd_notify failed: {}", -rc);
            return;
        }

        startTimer();
    }

    boost::asio::steady_timer timer;
    std::chrono::duration<uint64_t, std::micro> watchdogTime{};
};

} // namespace bmcweb
