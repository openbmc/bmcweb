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
/****************************************************************
 * This is an auto-generated header which contains definitions
 * for Redfish DMTF defined messages.
 ***************************************************************/
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
        "ResetButtonPressed",
        {
            .description = "Indicates that the reset button was pressed.",
            .message = "Reset Button Pressed.",
            .severity = "OK",
            .numberOfArgs = 0,
            .paramTypes = {},
            .resolution = "None.",
        }},
};
} // namespace redfish::message_registries::openbmc
