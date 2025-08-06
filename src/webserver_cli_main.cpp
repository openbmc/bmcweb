// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "webserver_cli.hpp"

#include <span>

int main(int argc, char** argv) noexcept(false)
{
    std::span<char*> args(argv, static_cast<size_t>(argc));
    cliMain(args);
    return 0;
}
