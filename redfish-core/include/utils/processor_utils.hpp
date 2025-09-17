// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include <array>
#include <string_view>

namespace redfish
{

// Interfaces which imply a D-Bus object represents a Processor
constexpr std::array<std::string_view, 2> processorInterfaces = {
    "xyz.openbmc_project.Inventory.Item.Cpu",
    "xyz.openbmc_project.Inventory.Item.Accelerator"};

// Interfaces which imply a D-Bus object represents a Processor Core
constexpr std::array<std::string_view, 1> processorCoreInterfaces = {
    "xyz.openbmc_project.Inventory.Item.CpuCore"};

// Interfaces which provide info about a Processor
constexpr std::array<std::string_view, 9> processorInfoInterfaces = {
    "xyz.openbmc_project.Common.UUID",
    "xyz.openbmc_project.Inventory.Decorator.Asset",
    "xyz.openbmc_project.Inventory.Decorator.Revision",
    "xyz.openbmc_project.Inventory.Item.Cpu",
    "xyz.openbmc_project.Inventory.Decorator.LocationCode",
    "xyz.openbmc_project.Inventory.Item.Accelerator",
    "xyz.openbmc_project.Control.Processor.CurrentOperatingConfig",
    "xyz.openbmc_project.Inventory.Decorator.UniqueIdentifier",
    "xyz.openbmc_project.Control.Power.Throttle",
};
} // namespace redfish
