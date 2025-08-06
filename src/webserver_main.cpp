// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "webserver_cli.hpp"
#include "webserver_run.hpp"

#include <span>
#include <string>

int main(int argc, char** argv) noexcept(false)
{
    // Using the busybox trick here to achieve a smaller combined binary size
    // for the daemon + cli.

    std::span<char*> args(argv, static_cast<size_t>(argc));

    if (argc >= 2 && std::string(args[1]) == "-cli")
    {
        return cliMain(args.last(args.size() - 1));
    }

    return run();
}
