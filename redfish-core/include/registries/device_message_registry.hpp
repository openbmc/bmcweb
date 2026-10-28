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

namespace redfish::registries
{
struct Device
{
static constexpr Header header = {
    "Copyright 2026 DMTF. All rights reserved.",
    "#MessageRegistry.v1_8_0.MessageRegistry",
    1,
    0,
    0,
    "Device Message Registry",
    "en",
    "This registry defines messages for individual devices or components in a larger system.",
    "Device",
    "DMTF",
};

static constexpr const char* url =
    "https://redfish.dmtf.org/registries/Device.1.0.0.json";

static constexpr std::array registry =
{
    MessageEntry{
        "AssemblyDataInaccessible",
        {
            "Indicates that the assembly data, historically 'FRU data', for a device is inaccessible.",
            "Unable to access the assembly data for device '%1'.",
            "Warning",
            1,
            {
                "string",
            },
            "Check additional diagnostic data if available.",
        }},
    MessageEntry{
        "AssemblyDataMalformed",
        {
            "Indicates that the assembly data, historically 'FRU data', for a device is malformed, such as due to a checksum error.",
            "Malformed assembly data detected for device '%1'.",
            "Warning",
            1,
            {
                "string",
            },
            "Check additional diagnostic data if available.",
        }},
    MessageEntry{
        "BootTimeout",
        {
            "Indicates that a device encountered a boot timeout.  This occurs when a device attempts to boot to an image that passed initial verification and other policy checks, but could not complete its boot sequence.",
            "Device '%1' encountered a boot timeout.",
            "Critical",
            1,
            {
                "string",
            },
            "Check additional diagnostic data if available.  Verify the correct firmware is installed on the device.",
        }},
    MessageEntry{
        "Disabled",
        {
            "Indicates that a device was disabled or halted.",
            "Device '%1' was disabled or halted.",
            "Warning",
            1,
            {
                "string",
            },
            "Check additional diagnostic data if available.",
        }},
    MessageEntry{
        "ErrorDetected",
        {
            "Indicates that an error was detected on a device.",
            "An error was detected on device '%1'.",
            "Critical",
            1,
            {
                "string",
            },
            "Check additional diagnostic data if available.",
        }},
    MessageEntry{
        "FirmwareIntegrityFailure",
        {
            "Indicates that a device failed firmware integrity verification checks.  This occurs when the image in the device's storage could not pass checksum, signature, or other validation methods.",
            "Device '%1' failed firmware integrity verification checks.",
            "Critical",
            1,
            {
                "string",
            },
            "Check additional diagnostic data if available.  Verify the correct firmware is installed on the device.",
        }},
    MessageEntry{
        "FirmwareStorageAccessFailure",
        {
            "Indicates that a device failed to access its firmware storage.",
            "Device '%1' failed to access its firmware storage.",
            "Critical",
            1,
            {
                "string",
            },
            "Check additional diagnostic data if available.",
        }},
    MessageEntry{
        "InRecovery",
        {
            "Indicates that a device is not operating properly and entered recovery mode to restore firmware or critical data to a state of integrity.  This type of condition is a runtime recovery where the device was previously in an operational state and there were no attempts to update the firmware.",
            "Device '%1' entered recovery mode due to: %2.",
            "Critical",
            2,
            {
                "string",
                "string",
            },
            "Check additional diagnostic data if available.",
        }},
    MessageEntry{
        "InitializationFailure",
        {
            "Indicates that a device failed to initialize.",
            "Device '%1' initialization failed.",
            "Critical",
            1,
            {
                "string",
            },
            "Check additional diagnostic data if available.  Check the device seating, power delivery, and compatibility.",
        }},
    MessageEntry{
        "ManagementInterfaceError",
        {
            "Indicates that the management interface for a device encountered an error.",
            "The management interface for device '%1' encountered the error: %2.",
            "Critical",
            2,
            {
                "string",
                "string",
            },
            "Check additional diagnostic data if available.",
        }},
    MessageEntry{
        "ManagementInterfaceUnresponsive",
        {
            "Indicates that the management interface for a device is unresponsive.",
            "The management interface for device '%1' is unresponsive.",
            "Critical",
            1,
            {
                "string",
            },
            "Check additional diagnostic data if available.",
        }},
    MessageEntry{
        "Missing",
        {
            "Indicates that the device is unexpectedly missing.",
            "Device '%1' could not be detected.",
            "Critical",
            1,
            {
                "string",
            },
            "Check additional diagnostic data if available.  Inspect the device to ensure it's present and seated properly.",
        }},
    MessageEntry{
        "PresenceMismatch",
        {
            "Indicates that a device presence mismatch was detected.",
            "Device presence mismatch detected for device '%1'.",
            "Warning",
            1,
            {
                "string",
            },
            "Check the device configuration.",
        }},
    MessageEntry{
        "SecureBootFailure",
        {
            "Indicates that a device encountered a secure boot failure.  This occurs when a device attempts to boot to an image that passed initial verification, but other policies or runtime checks are in place that restrict the device from booting with its current image.",
            "Device '%1' encountered a secure boot failure.",
            "Critical",
            1,
            {
                "string",
            },
            "Check additional diagnostic data if available.  Verify the correct firmware is installed on the device.",
        }},
    MessageEntry{
        "ThermalTrip",
        {
            "Indicates that a device thermal trip occurred.",
            "A thermal trip occurred on '%1'.",
            "Critical",
            1,
            {
                "string",
            },
            "Check additional diagnostic data if available.  Check thermal sensors, fan health, and cooling device operations.",
        }},
    MessageEntry{
        "Throttled",
        {
            "Indicates that a device throttling occurred.",
            "Throttling occurred on '%1'.",
            "Warning",
            1,
            {
                "string",
            },
            "Check additional diagnostic data if available.  Check thermal sensors, fan health, and cooling device operations.",
        }},
    MessageEntry{
        "WatchdogTimeout",
        {
            "Indicates that a device encountered a watchdog timeout.",
            "Device '%1' encountered a watchdog timeout.",
            "Critical",
            1,
            {
                "string",
            },
            "Check additional diagnostic data if available.",
        }},

};

enum class Index
{
    assemblyDataInaccessible = 0,
    assemblyDataMalformed = 1,
    bootTimeout = 2,
    disabled = 3,
    errorDetected = 4,
    firmwareIntegrityFailure = 5,
    firmwareStorageAccessFailure = 6,
    inRecovery = 7,
    initializationFailure = 8,
    managementInterfaceError = 9,
    managementInterfaceUnresponsive = 10,
    missing = 11,
    presenceMismatch = 12,
    secureBootFailure = 13,
    thermalTrip = 14,
    throttled = 15,
    watchdogTimeout = 16,
};
}; // struct device

[[gnu::constructor]] inline void registerDevice()
{ registerRegistry<Device>(); }

} // namespace redfish::registries
