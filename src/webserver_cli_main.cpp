// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "webserver_cli.hpp"

int main(int argc, char** argv) noexcept(false)
{
    cliMain(argc, argv);
    return 0;
}
