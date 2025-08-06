// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "webserver_cli.hpp"
#include "webserver_run.hpp"

#include <string>

int main(int argc, char** argv) noexcept(false)
{
    // use the busybox trick to symlink the 'bmcwebd' binary as 'bmcweb' to make
    // it call into the cli specific code path

    // NOLINTNEXTLINE
    if (argc >= 2 && std::string(argv[1]) == "-cli")
    {
        // NOLINTNEXTLINE
        return cliMain(argc - 1, argv + 1);
        // provide the arguments offset by one.
    }

    return run();
}
