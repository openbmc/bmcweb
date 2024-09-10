// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <sdbusplus/asio/connection.hpp>

namespace crow
{
namespace connections
{

// Initialize before using!
// Please see webserver_main for the example how this variable is initialized,
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
extern sdbusplus::asio::connection* systemBus;

} // namespace connections
} // namespace crow
