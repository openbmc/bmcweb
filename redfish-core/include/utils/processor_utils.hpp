// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include <array>
#include <string_view>

namespace redfish
{

static constexpr std::array<std::string_view, 2> processorInterfaces = {
    "xyz.openbmc_project.Inventory.Item.Cpu",
    "xyz.openbmc_project.Inventory.Item.Accelerator"};

} // namespace redfish
