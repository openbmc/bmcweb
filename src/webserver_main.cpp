// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "logging.hpp"
#include "webserver_cli.hpp"

#include <cstdlib>
#include <exception>

int main(int argc, char** argv) noexcept(false)
try
{
    return runCLI(argc, argv);
}
catch (const std::exception& e)
{
    BMCWEB_LOG_CRITICAL("Uncaught exception in main: {}", e.what());
    return EXIT_FAILURE;
}
catch (...)
{
    BMCWEB_LOG_CRITICAL("Uncaught unknown exception in main");
    return EXIT_FAILURE;
}
