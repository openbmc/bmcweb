// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "dbus_utility.hpp"

namespace redfish
{
namespace bios_utils
{

template <typename Callback>
inline void checkBiosSupport(Callback&& callback)
{
    dbus::utility::checkDbusPathExists(
        "/xyz/openbmc_project/bios_config/manager",
        [callback{std::forward<Callback>(callback)}](int rc) {
            if (rc > 0)
            {
                callback();
            }
        });
}
} // namespace bios_utils
} // namespace redfish
