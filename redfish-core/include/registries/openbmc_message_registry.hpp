/*
// Copyright (c) 2019 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#pragma once
#include <registries.hpp>

namespace redfish::message_registries::openbmc
{
const Header header = {
    .copyright = "Copyright 2018 Intel. All rights reserved.",
    .type = "#MessageRegistry.v1_0_0.MessageRegistry",
    .id = "OpenBMC.0.1.0",
    .name = "OpenBMC Message Registry",
    .language = "en",
    .description = "This registry defines the base messages for OpenBMC.",
    .registryPrefix = "OpenBMC",
    .registryVersion = "0.1.0",
    .owningEntity = "OpenBMC",
};
const std::array registry = {
    MessageEntry{"CPUError",
                 {
                     .description = "Indicates that a CPU Error occurred of "
                                    "the specified type or cause.",
                     .message = "CPU Error Occurred: %1.",
                     .severity = "Critical",
                     .numberOfArgs = 1,
                     .paramTypes = {"string"},
                     .resolution = "None.",
                 }},
    MessageEntry{
        "DCPowerOff",
        {
            .description = "Indicates that the system DC power is off.",
            .message = "Host system DC power is off",
            .severity = "OK",
            .numberOfArgs = 0,
            .paramTypes = {},
            .resolution = "None.",
        }},
    MessageEntry{"DCPowerOn",
                 {
                     .description = "Indicates that the system DC power is on.",
                     .message = "Host system DC power is on",
                     .severity = "OK",
                     .numberOfArgs = 0,
                     .paramTypes = {},
                     .resolution = "None.",
                 }},
    MessageEntry{
        "MemoryThermTrip",
        {
            .description =
                "Indicates that the system memory ThermTrip is asserted.",
            .message = "Memory ThermTrip asserted.",
            .severity = "Critical",
            .numberOfArgs = 0,
            .paramTypes = {},
            .resolution = "None.",
        }},
    MessageEntry{
        "EventLogCleared",
        {
            .description = "Indicates that the event log has been cleared.",
            .message = "Event Log Cleared.",
            .severity = "OK",
            .numberOfArgs = 0,
            .paramTypes = {},
            .resolution = "None.",
        }},
    MessageEntry{
        "FanInserted",
        {
            .description = "Indicates that a system fan has been inserted.",
            .message = "Fan %1 inserted.",
            .severity = "OK",
            .numberOfArgs = 1,
            .paramTypes = {"number"},
            .resolution = "None.",
        }},
    MessageEntry{"FanRedundancyLost",
                 {
                     .description =
                         "Indicates that system fan redundancy has been lost.",
                     .message = "Fan redundancy lost.",
                     .severity = "Warning",
                     .numberOfArgs = 0,
                     .paramTypes = {},
                     .resolution = "None.",
                 }},
    MessageEntry{
        "FanRedundancyRegained",
        {
            .description =
                "Indicates that system fan redundancy has been regained.",
            .message = "Fan redundancy regained.",
            .severity = "OK",
            .numberOfArgs = 0,
            .paramTypes = {},
            .resolution = "None.",
        }},
    MessageEntry{
        "FanRemoved",
        {
            .description = "Indicates that a system fan has been removed.",
            .message = "Fan %1 removed.",
            .severity = "Warning",
            .numberOfArgs = 1,
            .paramTypes = {"number"},
            .resolution = "None.",
        }},
    MessageEntry{
        "FirmwareUpdateCompleted",
        {
            .description =
                "Indicates a firmware update has completed successfully.",
            .message = "%1 firmware update to version %2 completed "
                       "successfully.",
            .severity = "OK",
            .numberOfArgs = 2,
            .paramTypes = {"string", "string"},
            .resolution = "None.",
        }},
    MessageEntry{"FirmwareUpdateFailed",
                 {
                     .description = "Indicates a firmware update has failed.",
                     .message = "%1 firmware update to version %2 failed.",
                     .severity = "Warning",
                     .numberOfArgs = 2,
                     .paramTypes = {"string", "string"},
                     .resolution = "None.",
                 }},
    MessageEntry{"FirmwareUpdateStarted",
                 {
                     .description = "Indicates a firmware update has started.",
                     .message = "%1 firmware update to version %2 started.",
                     .severity = "OK",
                     .numberOfArgs = 2,
                     .paramTypes = {"string", "string"},
                     .resolution = "None.",
                 }},
    MessageEntry{
        "GeneralFirmwareSecurityViolation",
        {
            .description =
                "Indicates a general firmware security violation has occurred.",
            .message = "Firmware security violation: %1.",
            .severity = "Critical",
            .numberOfArgs = 1,
            .paramTypes = {"string"},
            .resolution = "None.",
        }},
    MessageEntry{
        "InvalidLoginAttempted",
        {
            .description =
                "Indicates that a login was attempted on the specified "
                "interface with an invalid username or password.",
            .message = "Invalid username or password attempted on %1.",
            .severity = "Warning",
            .numberOfArgs = 1,
            .paramTypes = {"string"},
            .resolution = "None.",
        }},
    MessageEntry{"ManufacturingModeEntered",
                 {
                     .description = "Indicates that Factory, Manufacturing, or "
                                    "Test mode has been entered.",
                     .message = "Entered Manufacturing Mode.",
                     .severity = "Warning",
                     .numberOfArgs = 0,
                     .paramTypes = {},
                     .resolution = "None.",
                 }},
    MessageEntry{
        "NMIButtonPressed",
        {
            .description = "Indicates that the NMI button was pressed.",
            .message = "NMI Button Pressed.",
            .severity = "Critical",
            .numberOfArgs = 0,
            .paramTypes = {},
            .resolution = "None.",
        }},
    MessageEntry{
        "PowerButtonPressed",
        {
            .description = "Indicates that the power button was pressed.",
            .message = "Power Button Pressed.",
            .severity = "OK",
            .numberOfArgs = 0,
            .paramTypes = {},
            .resolution = "None.",
        }},
    MessageEntry{
        "PowerSupplyFanFailed",
        {
            .description =
                "Indicates that the specified power supply fan has failed.",
            .message = "Power supply %1 fan %2 failed.",
            .severity = "Critical",
            .numberOfArgs = 2,
            .paramTypes = {"number", "number"},
            .resolution = "None.",
        }},
    MessageEntry{
        "ResetButtonPressed",
        {
            .description = "Indicates that the reset button was pressed.",
            .message = "Reset Button Pressed.",
            .severity = "OK",
            .numberOfArgs = 0,
            .paramTypes = {},
            .resolution = "None.",
        }},
    MessageEntry{"SELEntryAdded",
                 {
                     .description = "Indicates a SEL entry was added using the "
                                    "Add SEL Entry or Platform Event command.",
                     .message = "SEL Entry Added: %1",
                     .severity = "OK",
                     .numberOfArgs = 1,
                     .paramTypes =
                         {
                             "string",
                         },
                     .resolution = "None.",
                 }},
    MessageEntry{"SystemPowerLost",
                 {
                     .description = "Indicates that power was lost while the "
                                    "system was powered on.",
                     .message = "System Power Lost.",
                     .severity = "Critical",
                     .numberOfArgs = 0,
                     .paramTypes = {},
                     .resolution = "None.",
                 }},
    MessageEntry{
        "SystemPowerOffFailed",
        {
            .description = "Indicates that the system failed to power off.",
            .message = "System Power-Off Failed.",
            .severity = "Critical",
            .numberOfArgs = 0,
            .paramTypes = {},
            .resolution = "None.",
        }},
    MessageEntry{
        "SystemPowerOnFailed",
        {
            .description = "Indicates that the system failed to power on.",
            .message = "System Power-On Failed.",
            .severity = "Critical",
            .numberOfArgs = 0,
            .paramTypes = {},
            .resolution = "None.",
        }},
};
} // namespace redfish::message_registries::openbmc
