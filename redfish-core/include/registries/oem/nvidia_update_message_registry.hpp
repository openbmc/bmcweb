// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
/****************************************************************
 *                 READ THIS WARNING FIRST
 * This is an auto-generated header which contains definitions
 * for Redfish DMTF defined messages.
 * DO NOT modify this registry outside of running the
 * parse_registries.py script.  The definitions contained within
 * this file are owned by DMTF.  Any modifications to these files
 * should be first pushed to the relevant registry in the DMTF
 * github organization.
 ***************************************************************/
#include "registries.hpp"

#include <array>

// clang-format off

namespace redfish::registries::nvidia_update
{
const Header header = {
    "Copyright 2024 Nvidia. All rights reserved.",
    "#MessageRegistry.v1_4_0.MessageRegistry",
    1,
    0,
    0,
    "Nvidia Update Message Registry",
    "en",
    "This registry defines the update messages for Nvidia.",
    "NvidiaUpdate",
    "Nvidia",
};
constexpr const char* url =
    "";

constexpr std::array registry =
{
    MessageEntry{
        "ComponentUpdateSkipped",
        {
            "Indicates that update of component has been skipped",
            "The update operation for the component %1 is skipped because %2.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "DebugTokenAlreadyInstalled",
        {
            "Indicates that the device has a token already installed and cannot finish current request.",
            "Debug token for device '%1' has already been installed.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "DebugTokenEraseFailed",
        {
            "Indicates that debug token erase operation has failed for the device.",
            "The operation to erase a debug token for device '%1' has failed with error '%2'",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "DebugTokenInstallationSuccess",
        {
            "Signifies the successful completion of debug token installation.",
            "The operation to install a debug token for device '%1' has been successfully completed.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "DebugTokenRequestSuccess",
        {
            "Signifies the successful completion of the debug token request.",
            "The operation to request a debug token for device '%1' has been successfully completed.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "DebugTokenStatusSuccess",
        {
            "Signifies the successful completion of the debug token status request.",
            "The operation to obtain a token status for device '%1' has been successfully completed.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "DebugTokenUnsupported",
        {
            "Indicates that the device does not support debug token functionality.",
            "Device '%1' does not support debug token functionality.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "FirmwareNotInRecovery",
        {
            "Indicates that a firmware is not in Recovery Mode",
            "Firmware %1 is not in Recovery.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "RecoveryStarted",
        {
            "Indicates that recovery has started on a component",
            "Firmware Recovery Started on %1.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "RecoverySuccessful",
        {
            "Indicates that recovery has successfully completed on a component",
            "Firmware %1 is successfully recovered.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "StageSuccessful",
        {
            "Indicates that image is successfully staged on the device",
            "Device %1 successfully staged with image %2.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},

};

enum class Index
{
    componentUpdateSkipped = 0,
    debugTokenAlreadyInstalled = 1,
    debugTokenEraseFailed = 2,
    debugTokenInstallationSuccess = 3,
    debugTokenRequestSuccess = 4,
    debugTokenStatusSuccess = 5,
    debugTokenUnsupported = 6,
    firmwareNotInRecovery = 7,
    recoveryStarted = 8,
    recoverySuccessful = 9,
    stageSuccessful = 10,
};
} // namespace redfish::registries::nvidia_update
