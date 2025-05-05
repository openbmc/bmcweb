// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "webserver_run.hpp"
#include "logging.hpp"

int main(int /*argc*/, char** /*argv*/) noexcept(false)
{
    BMCWEB_LOG_INFO("Starting BMC Web Server");
    return run();
}
