// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "dbus_singleton.hpp"

#include <sdbusplus/asio/connection.hpp>

namespace crow
{
namespace connections
{
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
sdbusplus::asio::connection* systemBus = nullptr;

} // namespace connections
} // namespace crow
