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

#include "registries.hpp"

#include <array>

/*
    Please read this before making edits to the structure below.

    Messages in this registry are intended to be generally useful across
   OpenBMC systems.  To that end, there are a number of guidelines needed for
   messages in this file to remain useful across systems.

    1. Messages should not be specific to a piece or type of hardware. Generally
   this involves not using terms like "CPU", "Drive", "Backplane", etc in the
   message strings, unless that behavior is specific to that particular piece of
   hardware.
   Incorrect: MemoryOverheated
   Correct: DeviceOverheated
   Correct: MemoryECCError (ECC is specific to memory, therefore class specific
   is acceptable)

    2. Messages should not use any proprietary, copyright, or company-specific
   terms.  In general, messages should prefer the industry accepted term.

    3. Message strings should be human readable, and read like a normal
   sentence.  This generally involves placing the substitution parameters in the
   appropriate place in the string.

   Incorrect: "An error occurred.  Device: %1"

   Correct: "An error occurred on device %1".

    4. Message registry versioning semantics shall be obeyed.  Adding new
   messages require an increment to the subminor revision.  Changes to existing
   messages require an increment to the patch version.  If the copyright year is
   different than the current date, increment it when the version is changed.

   5. If you are changing this in your own downstream company-specific fork,
   please change the "id" field below away from "OpenBMC.0.X.X" to something of
   the form of "CompanyName.0.X.X".  This is to ensure that when a system is
   found in industry that has modified this registry, it can be differentiated
   from the OpenBMC project maintained registry.

   6. Keep the below list alphabetized by message id.
*/

namespace redfish::registries::openbmc
{
const Header header = {
    "Copyright 2022 OpenBMC. All rights reserved.",
    "#MessageRegistry.v1_4_0.MessageRegistry",
    "OpenBMC.0.4.0",
    "OpenBMC Message Registry",
    "en",
    "This registry defines the base messages for OpenBMC.",
    "OpenBMC",
    "0.4.0",
    "OpenBMC",
};
constexpr std::array registry = {
    Message{
        "ADDDCCorrectable",
        "Indicates an ADDDC Correctable Error.",
        "ADDDC Correctable Error.Socket=%1 Channel=%2 DIMM=%3 Rank=%4.",
        "Warning",
        4,
        {
            "number",
            "string",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "AtScaleDebugFeatureEnabledAtHardware",
        "Indicates that At-Scale Debug enable is detected in hardware.",
        "At-Scale Debug Feature is enabled in hardware.",
        "Critical",
        0,
        {},
        "None.",
    },
    Message{
        "AtScaleDebugFeatureDisabledAtHardware",
        "Indicates that At-Scale Debug disable is detected in hardware.",
        "At-Scale Debug Feature is disabled in hardware.",
        "OK",
        0,
        {},
        "None.",
    },
    Message{
        "AtScaleDebugFeatureEnabled",
        "Indicates that At-Scale Debug service is started.",
        "At-Scale Debug service is started.",
        "Critical",
        0,
        {},
        "None.",
    },
    Message{
        "AtScaleDebugFeatureDisabled",
        "Indicates that At-Scale Debug service is stopped.",
        "At-Scale Debug service is stopped.",
        "OK",
        0,
        {},
        "None.",
    },
    Message{
        "AtScaleDebugConnected",
        "Indicates At-Scale Debug connection has been established",
        "At-Scale Debug service is now connected %1",
        "Critical",
        1,
        {"string"},
        "None.",
    },
    Message{
        "AtScaleDebugDisconnected",
        "Indicates At-Scale Debug connection has ended",
        "At-Scale Debug service is now disconnected",
        "OK",
        0,
        {},
        "None.",
    },
    Message{
        "AtScaleDebugConnectionFailed",
        "Indicates At-Scale Debug connection aborted/failed",
        "At-Scale Debug connection aborted/failed",
        "Critical",
        0,
        {},
        "None.",
    },
    Message{
        "AtScaleDebugSpecialUserEnabled",
        "Indicates that special user is enabled.",
        "At-Scale Debug special user is enabled",
        "Critical",
        0,
        {},
        "None.",
    },
    Message{
        "AtScaleDebugSpecialUserDisabled",
        "Indicates that special user is disabled.",
        "At-Scale Debug special user is disabled",
        "OK",
        0,
        {},
        "None.",
    },

    Message{
        "BIOSAttributesChanged",
        "Indicates that a set of BIOS Attributes has changed.",
        "Set of BIOS Attributes changed.",
        "OK",
        0,
        {},
        "None.",
    },
    Message{
        "BIOSBoot",
        "Indicates BIOS has transitioned control to the OS Loader.",
        "BIOS System Boot.",
        "OK",
        0,
        {},
        "None.",
    },
    Message{
        "BIOSFirmwarePanicReason",
        "Indicates the reason for BIOS firmware panic.",
        "BIOS firmware panic occurred due to %1.",
        "Warning",
        1,
        {
            "string",
        },
        "None.",
    },
    Message{
        "BIOSFirmwareRecoveryReason",
        "Indicates the reason for BIOS firmware recovery.",
        "BIOS firmware recovery occurred due to %1.",
        "Warning",
        1,
        {
            "string",
        },
        "None.",
    },
    Message{
        "BIOSFirmwareResiliencyError",
        "Indicates BIOS firmware encountered resilience error.",
        "BIOS firmware resiliency error. Error reason: %1.",
        "Critical",
        1,
        {
            "string",
        },
        "None.",
    },
    Message{
        "BIOSPOSTCode",
        "BIOS Power-On Self-Test Code received",
        "Boot Count: %1; Time Stamp Offset: %2 seconds; POST Code: %3",
        "OK",
        3,
        {"number", "number", "number"},
        "None.",
    },
    Message{
        "BIOSPOSTError",
        "Indicates BIOS POST has encountered an error.",
        "BIOS POST Error. Error Code=%1",
        "Warning",
        1,
        {"number"},
        "None.",
    },
    Message{
        "BIOSRecoveryComplete",
        "Indicates BIOS Recovery has completed.",
        "BIOS Recovery Complete.",
        "OK",
        0,
        {},
        "None.",
    },
    Message{
        "BIOSRecoveryStart",
        "Indicates BIOS Recovery has started.",
        "BIOS Recovery Start.",
        "Warning",
        0,
        {},
        "None.",
    },
    Message{
        "BMCBootReason",
        "Indicates the reason why BMC firmware booted.",
        "BMC firmware version %1 booted due to %2.",
        "OK",
        2,
        {
            "string",
            "string",
        },
        "None.",
    },
    Message{
        "BMCFirmwarePanicReason",
        "Indicates the reason for last BMC firmware panic.",
        "BMC firmware panic occurred due to %1.",
        "Warning",
        1,
        {
            "string",
        },
        "None.",
    },
    Message{
        "BMCFirmwareRecoveryReason",
        "Indicates the reason for last BMC firmware recovery.",
        "BMC firmware recovery occurred due to %1.",
        "Warning",
        1,
        {
            "string",
        },
        "None.",
    },
    Message{
        "BMCFirmwareResiliencyError",
        "Indicates BMC firmware encountered resilience error.",
        "BMC firmware resiliency error. Error reason: %1.",
        "Critical",
        1,
        {
            "string",
        },
        "None.",
    },
    Message{
        "BMCKernelPanic",
        "Indicates that BMC kernel panic occurred.",
        "BMC rebooted due to kernel panic.",
        "OK",
        0,
        {},
        "None.",
    },
    Message{
        "ChassisIntrusionDetected",
        "Indicates that a physical security event "
        "of the chassis intrusion has occurred.",
        "Chassis Intrusion Detected.",
        "Warning",
        0,
        {},
        "None.",
    },
    Message{
        "ChassisIntrusionReset",
        "Indicates that chassis intrusion status has recovered.",
        "Chassis Intrusion Reset.",
        "OK",
        0,
        {},
        "None.",
    },
    Message{
        "ComponentOverTemperature",
        "Indicates that the specified component is over temperature.",
        "%1 over temperature and being throttled.",
        "Critical",
        1,
        {"string"},
        "None.",
    },
    Message{
        "CPLDFirmwarePanicReason",
        "Indicates the reason for CPLD firmware panic.",
        "CPLD firmware panic occurred due to %1.",
        "Warning",
        1,
        {
            "string",
        },
        "None.",
    },
    Message{
        "CPLDFirmwareRecoveryReason",
        "Indicates the reason for CPLD firmware recovery.",
        "CPLD firmware recovery occurred due to %1.",
        "Warning",
        1,
        {
            "string",
        },
        "None.",
    },
    Message{
        "CPLDFirmwareResiliencyError",
        "Indicates CPLD firmware encountered resilience error.",
        "CPLD firmware resiliency error. Error reason: %1.",
        "Critical",
        1,
        {
            "string",
        },
        "None.",
    },
    Message{
        "CPUError",
        "Indicates that a CPU Error occurred of "
        "the specified type or cause.",
        "CPU Error Occurred: %1.",
        "Critical",
        1,
        {"string"},
        "None.",
    },
    Message{
        "CPUMismatch",
        "Indicates that the specified CPU power/current "
        "rating is incompatible with the board.",
        "CPU %1 Mismatch.",
        "Critical",
        1,
        {"number"},
        "Install the supported CPU.",
    },
    Message{
        "CPUThermalTrip",
        "Indicates that the specified CPU thermal "
        "trip has been asserted.",
        "CPU %1 Thermal Trip.",
        "Critical",
        1,
        {"number"},
        "None.",
    },
    Message{
        "DCPowerOff",
        "Indicates that the system DC power is off.",
        "Host system DC power is off",
        "OK",
        0,
        {},
        "None.",
    },
    Message{
        "DCPowerOn",
        "Indicates that the system DC power is on.",
        "Host system DC power is on",
        "OK",
        0,
        {},
        "None.",
    },
    Message{
        "DriveError",
        "Indicates that a Drive Error occurred of "
        "the specified type or cause.",
        "Drive Error Occurred: %1.",
        "Warning",
        1,
        {"string"},
        "None.",
    },
    Message{
        "EventLogCleared",
        "Indicates that the event log has been cleared.",
        "Event Log Cleared.",
        "OK",
        0,
        {},
        "None.",
    },
    Message{
        "FanInserted",
        "Indicates that a system fan has been inserted.",
        "%1 inserted.",
        "OK",
        1,
        {"string"},
        "None.",
    },
    Message{
        "FanRedundancyLost",
        "Indicates that system fan redundancy has been lost.",
        "Fan redundancy lost.",
        "Warning",
        0,
        {},
        "None.",
    },
    Message{
        "FanRedundancyRegained",
        "Indicates that system fan redundancy has been regained.",
        "Fan redundancy regained.",
        "OK",
        0,
        {},
        "None.",
    },
    Message{
        "FanRemoved",
        "Indicates that a system fan has been removed.",
        "%1 removed.",
        "OK",
        1,
        {"string"},
        "None.",
    },
    Message{
        "FirmwareActivationCompleted",
        "Indicates a firmware activation has completed successfully.",
        "%1 firmware activation completed successfully.",
        "OK",
        1,
        {"string"},
        "None.",
    },
    Message{
        "FirmwareActivationFailed",
        "Indicates a firmware activation has failed.",
        "%1 firmware activation failed: %2.",
        "Warning",
        2,
        {"string", "string"},
        "None.",
    },
    Message{
        "FirmwareActivationStarted",
        "Indicates a firmware activation has started.",
        "%1 firmware activation started.",
        "OK",
        1,
        {"string"},
        "None.",
    },
    Message{
        "FirmwareResiliencyError",
        "Indicates firmware encountered resilience error.",
        "Firmware resiliency error. Error reason: %1.",
        "Critical",
        1,
        {
            "string",
        },
        "None.",
    },
    Message{
        "FirmwareUpdateCompleted",
        "Indicates a firmware update has completed successfully.",
        "%1 firmware update to version %2 completed "
        "successfully.",
        "OK",
        2,
        {"string", "string"},
        "None.",
    },
    Message{
        "FirmwareUpdateFailed",
        "Indicates a firmware update has failed.",
        "%1 firmware update to version %2 failed: %3.",
        "Warning",
        3,
        {"string", "string", "string"},
        "None.",
    },
    Message{
        "FirmwareUpdateStaged",
        "Indicates a firmware update has staged successfully.",
        "%1 firmware update to version %2 staged successfully.",
        "OK",
        2,
        {"string", "string"},
        "None.",
    },
    Message{
        "FirmwareUpdateStarted",
        "Indicates a firmware update has started.",
        "%1 firmware update to version %2 started.",
        "OK",
        2,
        {"string", "string"},
        "None.",
    },
    Message{
        "GeneralFirmwareSecurityViolation",
        "Indicates a general firmware security violation has occurred.",
        "Firmware security violation: %1.",
        "Critical",
        1,
        {"string"},
        "None.",
    },
    Message{
        "InvalidLoginAttempted",
        "Indicates that a login was attempted on the specified "
        "interface with an invalid username or password.",
        "Invalid username or password attempted on %1.",
        "Warning",
        1,
        {"string"},
        "None.",
    },
    Message{
        "InvalidUpload",
        "Indicates that the uploaded file was invalid.",
        "Invalid file uploaded to %1: %2.",
        "Warning",
        2,
        {"string", "string"},
        "None.",
    },
    Message{
        "InventoryAdded",
        "Indicates that an inventory item with the specified model, "
        "type, and serial number was installed.",
        "%1 %2 with serial number %3 was installed.",
        "OK",
        3,

        {
            "string",
            "string",
            "string",
        },
        "None.",
    },
    Message{
        "InventoryRemoved",
        "Indicates that an inventory item with the specified model, "
        "type, and serial number was removed.",
        "%1 %2 with serial number %3 was removed.",
        "OK",
        3,

        {
            "string",
            "string",
            "string",
        },
        "None.",
    },
    Message{
        "IntelUPILinkWidthReducedToHalf",
        "Indicates Intel UPI link width has reduced to half width.",
        "Intel UPI link width reduced to half. Node=%1.",
        "Warning",
        1,

        {
            "number",
        },
        "None.",
    },
    Message{
        "IntelUPILinkWidthReducedToQuarter",
        "Indicates Intel UPI link width has reduced to quarter width.",
        "Intel UPI link width reduced to quarter. Node=%1.",
        "Warning",
        1,

        {
            "number",
        },
        "None.",
    },

    Message{
        "IPMIWatchdog",
        "Indicates that there is a host watchdog event.",
        "Host Watchdog Event: %1",
        "OK",
        1,
        {
            "string",
        },
        "None.",
    },
    Message{
        "LanLost",
        "Indicates that a physical security event "
        "of the LAN leash has lost.",
        "%1 LAN leash lost.",
        "Warning",
        1,
        {
            "string",
        },
        "None.",
    },
    Message{
        "LanRegained",
        "Indicates that LAN link status has reconnected.",
        "%1 LAN leash regained.",
        "OK",
        1,
        {
            "string",
        },
        "None.",
    },
    Message{
        "LegacyPCIPERR",
        "Indicates a Legacy PCI PERR.",
        "Legacy PCI PERR. Bus=%1 Device=%2 Function=%3.",
        "Warning",
        3,

        {
            "number",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "LegacyPCISERR",
        "Indicates a Legacy PCI SERR.",
        "Legacy PCI SERR. Bus=%1 Device=%2 Function=%3.",
        "Critical",
        3,

        {
            "number",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "ManufacturingModeEntered",
        "Indicates that the BMC entered Factory, "
        "or Manufacturing mode.",
        "Entered Manufacturing Mode.",
        "Critical",
        0,
        {},
        "None.",
    },
    Message{
        "ManufacturingModeExited",
        "Indicates that the BMC exited Factory, "
        "or Manufacturing mode.",
        "Exited Manufacturing Mode.",
        "OK",
        0,
        {},
        "None.",
    },
    Message{
        "MEAutoConfigFailed",
        "Indicates that Intel ME power sensor "
        "auto-configuration has failed.",
        "Intel ME power sensor auto-configuration failed. Power "
        "monitoring, limiting and HW protection features might "
        "be unavailable. Failure reason: %1",
        "Critical",
        1,
        {"string"},
        "Ensure that Intel ME configuration for power "
        "sources is correct.",
    },
    Message{
        "MEAutoConfigSuccess",
        "Indicates that Intel ME has performed successful "
        "power sensor auto-configuration.",
        "Intel ME power sensor auto-configuration succeeded. "
        "Determined sources for domain readings are: DC Power: %1 ; "
        "Chassis Power: %2 ; PSU Efficiency: %3 ; Unamanaged power: %4",
        "OK",
        4,
        {"string", "string", "string", "string"},
        "None.",
    },
    Message{
        "MEBootGuardHealthEvent",
        "Indicates that Intel ME has detected error during "
        "operations of Intel Boot Guard",
        "Intel ME has detected following issue with Intel Boot "
        "Guard: %1",
        "Critical",
        1,
        {"string"},
        "None.",
    },
    Message{
        "MECpuDebugCapabilityDisabled",
        "Indicates that Intel ME has detected situation in "
        "which CPU Debug Capability is disabled.",
        "CPU Debug Capability disabled",
        "Warning",
        0,
        {},
        "None.",
    },
    Message{
        "MEDirectFlashUpdateRequested",
        "Indicates that BIOS has requested Direct Flash "
        "Update (DFU) of Intel ME",
        "Intel ME Firmware switched to recovery mode to perform "
        "full update from BIOS.",
        "OK",
        0,
        {},
        "This is transient state. Intel ME Firmware should "
        "return to operational mode after successful image "
        "update performed by the BIOS.",
    },
    Message{
        "MEExceptionDuringShutdown",
        "Indicates that Intel ME could not successfully "
        "perform emergency host shutdown.",
        "Power Down command triggered by Intel Node Manager policy "
        "failure action and Intel ME forced shutdown. BMC probably did "
        "not respond correctly to Chassis Control.",
        "Warning",
        0,
        {},
        "Verify the Intel Node Manager policy configuration.",
    },
    Message{
        "MEFactoryResetError",
        "Indicates that Intel ME has ben restored to factory preset.",
        "Intel ME has performed automatic reset to factory "
        "presets due to following reason: %1",
        "Critical",
        1,
        {"string"},
        "If error is persistent the Flash device must be replaced.",
    },
    Message{
        "MEFactoryRestore",
        "Indicates that Intel ME has ben restored to factory preset.",
        "Intel ME has performed automatic reset to factory "
        "presets due to following reason: %1",
        "OK",
        1,
        {"string"},
        "If error is persistent the Flash device must be replaced.",
    },
    Message{
        "MEFirmwareException",
        "Indicates that Intel ME has encountered firmware "
        "exception during execution.",
        "Intel ME has encountered firmware exception. Error code = %1",
        "Warning",
        1,
        {"string"},
        "Restore factory presets using Force ME Recovery IPMI "
        "command or by doing AC power cycle with Recovery jumper "
        "asserted. If this does not clear the issue, reflash the SPI "
        "flash. If the issue persists, provide the content of error "
        "code to Intel support team for interpretation. (Error codes "
        "are not documented because they only provide clues that must "
        "be interpreted individually..",
    },
    Message{
        "MEFirmwarePanicReason",
        "Indicates the reason for ME firmware panic.",
        "ME firmware panic occurred due to %1.",
        "Warning",
        1,
        {
            "string",
        },
        "None.",
    },
    Message{
        "MEFirmwareRecoveryReason",
        "Indicates the reason for ME firmware recovery.",
        "ME firmware recovery occurred due to %1.",
        "Warning",
        1,
        {
            "string",
        },
        "None.",
    },
    Message{
        "MEFirmwareResiliencyError",
        "Indicates ME firmware encountered resilience error.",
        "ME firmware resiliency error. Error reason: %1.",
        "Critical",
        1,
        {
            "string",
        },
        "None.",
    },

    Message{
        "MEFlashEraseError",
        "Indicates that Intel ME was unable to finish flash "
        "erase procedure.",
        "Intel ME has encountered an error during Flash erasure "
        "procedure probably due to Flash part corruption.",
        "Critical",
        0,
        {},
        "The Flash device must be replaced.",
    },
    Message{
        "MEFlashStateInformation",
        "Indicates that Intel ME has encountered a problem "
        "during IO to flash device.",
        "Intel ME has encountered problem during IO to flash "
        "device. Reason: %1",
        "Critical",
        1,
        {"string"},
        "If flash wear-out protection occurred wait until it "
        "expires. Otherwise - flash device must be replaced.",
    },
    Message{
        "MEFlashStateInformationWritingEnabled",
        "Indicates that Intel ME has encountered a problem "
        "during IO to flash device.",
        "Intel ME has encountered problem during IO to flash "
        "device. Reason: %1",
        "OK",
        1,
        {"string"},
        "If flash wear-out protection occurred wait until it "
        "expires. Otherwise - flash device must be replaced.",
    },
    Message{
        "MEFlashVerificationError",
        "Indicates that Intel ME encountered invalid flash "
        "descriptor region.",
        "Intel ME has detected invalid flash descriptor region. "
        "Following error is detected: %1",
        "Critical",
        1,
        {"string"},
        "Flash Descriptor Region must be created correctly.",
    },
    Message{
        "MEFlashWearOutWarning",
        "Indicates that Intel ME has reached certain "
        "threshold of flash write operations.",
        "Warning threshold for number of flash operations has been "
        "exceeded. Current percentage of write operations capacity: %1",
        "Warning",
        1,
        {"number"},
        "No immediate repair action needed.",
    },

    Message{
        "MEImageExecutionFailed",
        "Indicates that Intel ME could not load primary FW image.",
        "Intel ME Recovery Image or backup operational image "
        "loaded because operational image is corrupted. This "
        "may be either caused by Flash device corruption or "
        "failed upgrade procedure.",
        "Critical",
        0,
        {},
        "Either the Flash device must be replaced (if error is "
        "persistent) or the upgrade procedure must be started again.",
    },

    Message{
        "MEInternalError",
        "Indicates that Intel ME encountered "
        "internal error leading to watchdog reset.",
        "Error during Intel ME execution. Watchdog "
        "timeout has expired.",
        "Critical",
        0,
        {},
        "Firmware should automatically recover from error state. "
        "If error is persistent then operational image shall be updated "
        "or hardware board repair is needed.",
    },
    Message{
        "MEManufacturingError",
        "Indicates that Intel ME is unable to start in "
        "operational mode due to wrong configuration.",
        "Wrong manufacturing configuration detected by Intel ME "
        "Firmware. Unable to start operational mode. Reason: %1",
        "Critical",
        1,
        {"string"},
        " If error is persistent the Flash device must be "
        "replaced or FW configuration must be updated. Trace "
        "logs might be gathered for detailed information.",
    },
    Message{
        "MEMctpInterfaceError",
        "Indicates that Intel ME has encountered an error "
        "in MCTP protocol.",
        "Intel ME has detected MCTP interface failure and it is "
        "not functional any more. It may indicate the situation "
        "when MCTP was not configured by BIOS or a defect which "
        "may need a Host reset to recover from. Details: %1",
        "Critical",
        1,
        {"string"},
        "Recovery via CPU Host reset or platform reset. If error "
        "is persistent, deep-dive platform-level debugging is "
        "required.",
    },
    Message{
        "MemoryECCCorrectable",
        "Indicates a Correctable Memory ECC error.",
        "Memory ECC correctable error. Socket=%1 "
        "Channel=%2 DIMM=%3 Rank=%4.",
        "Warning",
        4,

        {
            "number",
            "string",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "MemoryECCUncorrectable",
        "Indicates an Uncorrectable Memory ECC error.",
        "Memory ECC uncorrectable error. Socket=%1 Channel=%2 "
        "DIMM=%3 Rank=%4.",
        "Critical",
        4,

        {
            "number",
            "string",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "MemoryParityCommandAndAddress",
        "Indicates a Command and Address parity error.",
        "Command and Address parity error. Socket=%1 Channel=%2 "
        "DIMM=%3 ChannelValid=%4 DIMMValid=%5.",
        "Critical",
        5,

        {
            "number",
            "string",
            "number",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "MemoryParityNotKnown",
        "Indicates an unknown parity error.",
        "Memory parity error. Socket=%1 Channel=%2 "
        "DIMM=%3 ChannelValid=%4 DIMMValid=%5.",
        "Critical",
        5,

        {
            "number",
            "string",
            "number",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "MemoryRASConfigurationDisabled",
        "Indicates Memory RAS Disabled Configuration Status.",
        "Memory RAS Configuration Disabled. Error=%1 Mode=%2.",
        "OK",
        2,
        {
            "string",
            "string",
        },
        "None.",
    },
    Message{
        "MemoryRASConfigurationEnabled",
        "Indicates Memory RAS Enabled Configuration Status.",
        "Memory RAS Configuration Enabled. Error=%1 Mode=%2.",
        "OK",
        2,
        {
            "string",
            "string",
        },
        "None.",
    },
    Message{
        "MemoryRASModeDisabled",
        "Indicates Memory RAS Disabled Mode Selection.",
        "Memory RAS Mode Select Disabled. Prior Mode=%1 "
        "Selected Mode=%2.",
        "OK",
        2,
        {
            "string",
            "string",
        },
        "None.",
    },
    Message{
        "MemoryRASModeEnabled",
        "Indicates Memory RAS Enabled Mode Selection.",
        "Memory RAS Mode Select Enabled. Prior Mode=%1 Selected "
        "Mode=%2.",
        "OK",
        2,
        {
            "string",
            "string",
        },
        "None.",
    },
    Message{
        "MemoryThermTrip",
        "Indicates that the system memory ThermTrip is asserted "
        "by the specified component.",
        "Memory ThermTrip asserted: %1.",
        "Critical",
        1,
        {"string"},
        "None.",
    },
    Message{
        "MEMultiPchModeMisconfig",
        "Indicates that Intel ME has encountered "
        "problems in initializing Multi-PCH mode.",
        "Intel ME error in Multi-PCH mode: %1",
        "Critical",
        1,
        {"string"},
        "None.",
    },
    Message{
        "MEPeciOverDmiError",
        "Indicates that Intel ME is unable to communicate "
        "using PECI over DMI.",
        "Intel ME has detected  PECI over DMI interface failure "
        "and it is not functional any more. It may indicate the "
        "situation when PECI over DMI was not configured by "
        "BIOS or a defect which may require a CPU Host reset to "
        "recover from. Details: %1",
        "Critical",
        1,
        {"string"},
        "Recovery via CPU Host reset or platform reset. If error is "
        "persistent, deep-dive platform-level debugging is required.",
    },
    Message{
        "MEPttHealthEvent",
        "Indicates that Intel ME has encountered issue with Intel PTT",
        "Intel ME has detected following issue with Intel PTT: %1",
        "Warning",
        1,
        {"string"},
        "None.",
    },
    Message{
        "MERecoveryGpioForced",
        "Indicates that Intel ME image is booted in "
        "recovery mode due to GPIO assertion.",
        "Intel ME Recovery Image loaded due to recovery MGPIO "
        "pin asserted. Pin number is configurable in factory "
        "presets, Default recovery pin is MGPIO1.",
        "OK",
        0,
        {},
        "Deassert recovery GPIO and reset the Intel ME back to "
        "operational mode. If Recovery Jumper is in legacy behavior, "
        "a ME reset (eg. Cold Reset IPMI cmd) is needed to have ME "
        "back in operational mode.",
    },
    Message{
        "MERestrictedMode",
        "Indicates events related to Intel ME restricted mode.",
        "Intel ME restricted mode information: %1",
        "Critical",
        1,
        {"string"},
        "None.",
    },
    Message{
        "MESmbusLinkFailure",
        "Indicate that Intel ME encountered SMBus link error.",
        "Intel ME has detected SMBus link error. "
        "Sensor Bus: %1 , MUX Address: %2 ",
        "Critical",
        2,
        {"string", "string"},
        "Devices connected to given SMLINK might cause communication "
        "corruption. See error code and refer to Intel ME External "
        "Interfaces Specification for details.",
    },
    Message{
        "MEUmaError",
        "Indicates that Intel ME has encountered UMA operation error.",
        "Intel ME has encountered UMA operation error. Details: %1",
        "Critical",
        1,
        {"string"},
        "Platform reset when UMA not configured correctly, or when "
        "error occurred during normal operation on correctly "
        "configured UMA multiple times leading to Intel ME entering "
        "Recovery or restricted operational mode.",
    },
    Message{
        "MEUnsupportedFeature",
        "Indicates that Intel ME is configuration with "
        "feature which is not supported on this platform.",
        "Feature not supported in current segment detected by "
        "Intel ME Firmware. Details: %1",
        "Critical",
        1,
        {"string"},
        "Proper FW configuration must be updated or use the "
        "Flash device with proper FW configuration",
    },
    Message{
        "MirroringRedundancyDegraded",
        "Indicates the mirroring redundancy state is degraded.",
        "Mirroring redundancy state degraded. Socket=%1 "
        "Channel=%2 DIMM=%3 Pair=%4 Rank=%5.",
        "Warning",
        5,

        {
            "number",
            "string",
            "number",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "MirroringRedundancyFull",
        "Indicates the mirroring redundancy state is fully redundant.",
        "Mirroring redundancy state fully redundant. Socket=%1 "
        "Channel=%2 DIMM=%3 Pair=%4 Rank=%5.",
        "OK",
        5,

        {
            "number",
            "string",
            "number",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "NMIButtonPressed",
        "Indicates that the NMI button was pressed.",
        "NMI Button Pressed.",
        "Critical",
        0,
        {},
        "None.",
    },
    Message{
        "NMIDiagnosticInterrupt",
        "Indicates that an NMI Diagnostic "
        "Interrupt has been generated.",
        "NMI Diagnostic Interrupt.",
        "Critical",
        0,
        {},
        "None.",
    },
    Message{
        "PCIeCorrectableAdvisoryNonFatal",
        "Indicates a PCIe Correctable Advisory Non-fatal Error.",
        "PCIe Correctable Advisory Non-fatal Error. Bus=%1 "
        "Device=%2 Function=%3.",
        "Warning",
        3,

        {
            "number",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "PCIeCorrectableBadDLLP",
        "Indicates a PCIe Correctable Bad DLLP Error.",

        "PCIe Correctable Bad DLLP. Bus=%1 Device=%2 Function=%3.",
        "Warning",
        3,

        {
            "number",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "PCIeCorrectableBadTLP",
        "Indicates a PCIe Correctable Bad TLP Error.",

        "PCIe Correctable Bad TLP. Bus=%1 Device=%2 Function=%3.",
        "Warning",
        3,

        {
            "number",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "PCIeCorrectableHeaderLogOverflow",
        "Indicates a PCIe Correctable Header Log Overflow Error.",
        "PCIe Correctable Header Log Overflow. Bus=%1 Device=%2 "
        "Function=%3.",
        "Warning",
        3,

        {
            "number",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "PCIeCorrectableInternal",
        "Indicates a PCIe Correctable Internal Error.",
        "PCIe Correctable Internal Error. Bus=%1 Device=%2 "
        "Function=%3.",
        "Warning",
        3,

        {
            "number",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "PCIeCorrectableLinkBWChanged",
        "Indicates a PCIe Correctable Link BW Changed Error.",
        "PCIe Correctable Link BW Changed. Bus=%1 "
        "Device=%2 Function=%3.",
        "Warning",
        3,

        {
            "number",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "PCIeCorrectableReceiverError",
        "Indicates a PCIe Correctable Receiver Error.",
        "PCIe Correctable Receiver Error. Bus=%1 Device=%2 "
        "Function=%3.",
        "Warning",
        3,

        {
            "number",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "PCIeCorrectableReplayNumRollover",
        "Indicates a PCIe Correctable Replay Num Rollover.",
        "PCIe Correctable Replay Num Rollover. Bus=%1 Device=%2 "
        "Function=%3.",
        "Warning",
        3,

        {
            "number",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "PCIeCorrectableReplayTimerTimeout",
        "Indicates a PCIe Correctable Replay Timer Timeout.",
        "PCIe Correctable Replay Timer Timeout. Bus=%1 "
        "Device=%2 Function=%3.",
        "Warning",
        3,

        {
            "number",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "PCIeCorrectableUnspecifiedAERError",
        "Indicates a PCIe Correctable Unspecified AER Error.",
        "PCIe Correctable Unspecified AER Error. "
        "Bus=%1 Device=%2 Function=%3.",
        "Warning",
        3,

        {
            "number",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "PCIeFatalACSViolation",
        "Indicates a PCIe ACS Violation Error.",

        "PCIe Fatal ACS Violation. Bus=%1 Device=%2 Function=%3.",
        "Critical",
        3,

        {
            "number",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "PCIeFatalAtomicEgressBlocked",
        "Indicates a PCIe Atomic Egress Blocked Error.",
        "PCIe Fatal Atomic Egress Blocked. Bus=%1 Device=%2 "
        "Function=%3.",
        "Critical",
        3,

        {
            "number",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "PCIeFatalCompleterAbort",
        "Indicates a PCIe Completer Abort Error.",

        "PCIe Fatal Completer Abort. Bus=%1 Device=%2 Function=%3.",
        "Critical",
        3,

        {
            "number",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "PCIeFatalCompletionTimeout",
        "Indicates a PCIe Completion Timeout Error.",

        "PCIe Fatal Completion Timeout. Bus=%1 Device=%2 Function=%3.",
        "Critical",
        3,

        {
            "number",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "PCIeFatalDataLinkLayerProtocol",
        "Indicates a PCIe Data Link Layer Protocol Error.",

        "PCIe Fatal Data Link Layer Protocol Error. Bus=%1 Device=%2 "
        "Function=%3.",
        "Critical",
        3,

        {
            "number",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "PCIeFatalECRCError",
        "Indicates a PCIe ECRC Error.",
        "PCIe Fatal ECRC Error. Bus=%1 Device=%2 Function=%3.",
        "Critical",
        3,

        {
            "number",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "PCIeFatalFlowControlProtocol",
        "Indicates a PCIe Flow Control Protocol Error.",

        "PCIe Fatal Flow Control Protocol Error. Bus=%1 Device=%2 "
        "Function=%3.",
        "Critical",
        3,

        {
            "number",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "PCIeFatalMalformedTLP",
        "Indicates a PCIe Malformed TLP Error.",

        "PCIe Fatal Malformed TLP Error. Bus=%1 Device=%2 Function=%3.",
        "Critical",
        3,

        {
            "number",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "PCIeFatalMCBlockedTLP",
        "Indicates a PCIe MC Blocked TLP Error.",
        "PCIe Fatal MC Blocked TLP Error. Bus=%1 "
        "Device=%2 Function=%3.",
        "Critical",
        3,

        {
            "number",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "PCIeFatalPoisonedTLP",
        "Indicates a PCIe Poisoned TLP Error.",

        "PCIe Fatal Poisoned TLP Error. Bus=%1 Device=%2 Function=%3.",
        "Critical",
        3,

        {
            "number",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "PCIeFatalReceiverBufferOverflow",
        "Indicates a PCIe Receiver Buffer Overflow Error.",
        "PCIe Fatal Receiver Buffer Overflow. Bus=%1 Device=%2 "
        "Function=%3.",
        "Critical",
        3,

        {
            "number",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "PCIeFatalReceivedErrNonFatalMessage",
        "Indicates a PCIe Received ERR_NONFATAL Message Error.",

        "PCIe Fatal Received ERR_NONFATAL Message. Bus=%1 Device=%2 "
        "Function=%3.",
        "Critical",
        3,

        {
            "number",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "PCIeFatalReceivedFatalMessageFromDownstream",
        "Indicates a PCIe Received Fatal Message "
        "From Downstream Error.",

        "PCIe Fatal Received Fatal Message From Downstream. "
        "Bus=%1 Device=%2 Function=%3.",
        "Critical",
        3,

        {
            "number",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "PCIeFatalSurpriseLinkDown",
        "Indicates a PCIe Surprise Link Down Error.",
        "PCIe Fatal Surprise Link Down Error. Bus=%1 Device=%2 "
        "Function=%3.",
        "Critical",
        3,

        {
            "number",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "PCIeFatalTLPPrefixBlocked",
        "Indicates a PCIe TLP Prefix Blocked Error.",
        "PCIe Fatal TLP Prefix Blocked Error. Bus=%1 Device=%2 "
        "Function=%3.",
        "Critical",
        3,

        {
            "number",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "PCIeFatalUncorrectableInternal",
        "Indicates a PCIe Uncorrectable Internal Error.",

        "PCIe Fatal Uncorrectable Internal Error. Bus=%1 Device=%2 "
        "Function=%3.",
        "Critical",
        3,

        {
            "number",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "PCIeFatalUnexpectedCompletion",
        "Indicates a PCIe Unexpected Completion Error.",
        "PCIe Fatal Unexpected Completion. Bus=%1 Device=%2 "
        "Function=%3.",
        "Critical",
        3,

        {
            "number",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "PCIeFatalUnspecifiedNonAERFatalError",
        "Indicates a PCIe Unspecified Non-AER Fatal Error.",
        "PCIe Fatal Unspecified Non-AER Fatal Error. Bus=%1 "
        "Device=%2 Function=%3.",
        "Critical",
        3,

        {
            "number",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "PCIeFatalUnsupportedRequest",
        "Indicates a PCIe Unsupported Request Error.",

        "PCIe Fatal Unsupported Request. Bus=%1 Device=%2 Function=%3.",
        "Critical",
        3,

        {
            "number",
            "number",
            "number",
        },
        "None.",
    },
    Message{
        "PowerButtonPressed",
        "Indicates that the power button was pressed.",
        "Power Button Pressed.",
        "OK",
        0,
        {},
        "None.",
    },
    Message{
        "PowerRestorePolicyApplied",
        "Indicates that power was restored and the "
        "BMC has applied the restore policy.",
        "Power restore policy applied.",
        "OK",
        0,
        {},
        "None.",
    },
    Message{
        "PowerSupplyConfigurationError",
        "Indicates an error in power supply configuration.",
        "Power supply %1 configuration error.",
        "Warning",
        1,
        {"string"},
        "None.",
    },
    Message{
        "PowerSupplyConfigurationErrorRecovered",
        "Indicates that power supply configuration error recovered "
        "from a failure.",
        "Power supply %1 configuration error recovered.",
        "OK",
        1,
        {"string"},
        "None.",
    },
    Message{
        "PowerSupplyFanFailed",
        "Indicates that the specified power supply fan has failed.",
        "Power supply %1 fan %2 failed.",
        "Warning",
        2,
        {"string", "string"},
        "None.",
    },
    Message{
        "PowerSupplyFanRecovered",
        "Indicates that the power supply fan recovered from a failure.",
        "Power supply %1 fan %2 recovered.",
        "OK",
        2,
        {"string", "string"},
        "None.",
    },
    Message{
        "PowerSupplyFailed",
        "Indicates that a power supply has failed.",
        "Power supply %1 failed.",
        "Warning",
        1,
        {"string"},
        "None.",
    },
    Message{
        "PowerSupplyFailurePredicted",
        "Indicates that a power supply is predicted to fail.",
        "Power supply %1 failure predicted.",
        "Warning",
        1,
        {"string"},
        "None.",
    },
    Message{
        "PowerSupplyInserted",
        "Indicates that a power supply has been inserted.",
        "Power supply %1 inserted.",
        "OK",
        1,
        {"string"},
        "None.",
    },
    Message{
        "PowerSupplyPowerGoodFailed",
        "Indicates that the power supply power good signal "
        "failed to assert within the specified time.",
        "Power supply power good failed to assert within %1 "
        "milliseconds.",
        "Critical",
        1,
        {"number"},
        "None.",
    },
    Message{
        "PowerSupplyPowerLost",
        "Indicates that a power supply has lost input power.",
        "Power supply %1 power lost.",
        "Warning",
        1,
        {"string"},
        "None.",
    },
    Message{
        "PowerSupplyPowerRestored",
        "Indicates that a power supply input power was restored.",
        "Power supply %1 power restored.",
        "OK",
        1,
        {"string"},
        "None.",
    },
    Message{
        "PowerSupplyPredictedFailureRecovered",
        "Indicates that a power supply recovered "
        "from a predicted failure.",
        "Power supply %1 predicted failure recovered.",
        "OK",
        1,
        {"string"},
        "None.",
    },
    Message{
        "PowerSupplyRecovered",
        "Indicates that a power supply recovered from a failure.",
        "Power supply %1 recovered.",
        "OK",
        1,
        {"string"},
        "None.",
    },
    Message{
        "PowerSupplyRemoved",
        "Indicates that a power supply has been removed.",
        "Power supply %1 removed.",
        "Warning",
        1,
        {"string"},
        "None.",
    },
    Message{
        "PowerUnitDegradedFromNonRedundant",
        "Indicates that power unit is come back to redundant from"
        "nonredundant but is still not in full redundancy mode.",
        "Power Unit degraded from nonredundant.",
        "Warning",
        0,
        {},
        "None.",
    },
    Message{
        "PowerUnitDegradedFromRedundant",
        "Indicates that power unit is degarded from full "
        "redundancy mode.",
        "Power Unit degraded from redundant.",
        "Warning",
        0,
        {},
        "None.",
    },
    Message{
        "PowerUnitRedundancyDegraded",
        "Indicates that power unit redundancy has been degraded.",
        "Power Unit Redundancy degraded.",
        "Warning",
        0,
        {},
        "None.",
    },
    Message{
        "PowerUnitNonRedundantFromInsufficient",
        "Indicates that power unit is not in redundancy mode and get"
        "sufficient power to support redundancy from insufficient"
        "power.",

        "Power Unit NonRedundant from insufficient to sufficient.",
        "Warning",
        0,
        {},
        "None.",
    },
    Message{
        "PowerUnitNonRedundantInsufficient",
        "Indicates that power unit do not have sufficient "
        "power to support redundancy.",
        "Power Unit NonRedundant and has insufficient resource.",
        "Critical",
        0,
        {},
        "None.",
    },
    Message{
        "PowerUnitRedundancyLost",
        "Indicates that power unit redundancy has been lost.",
        "Power Unit Redundancy lost.",
        "Warning",
        0,
        {},
        "None.",
    },
    Message{
        "PowerUnitRedundancyRegained",
        "Indicates that power unit full redundancy has been regained.",
        "Power Unit Redundancy regained.",
        "OK",
        0,
        {},
        "None.",
    },
    Message{
        "PowerUnitNonRedundantSufficient",
        "Indicates that power unit is not in redundancy mode but still"
        "has sufficient power to support redundancy.",
        "Power Unit Nonredundant but has sufficient resource.",
        "Warning",
        0,
        {},
        "None.",
    },
    Message{
        "ResetButtonPressed",
        "Indicates that the reset button was pressed.",
        "Reset Button Pressed.",
        "OK",
        0,
        {},
        "None.",
    },
    Message{
        "SecurityBoot2ndFlashEnabled",
        "Indicates that the BMC 2nd boot flash is enabled.",
        "BMC 2nd boot flash is enabled.",
        "Critical",
        0,
        {},
        "None.",
    },
    Message{
        "SecurityP2aBridgeEnabled",
        "Indicates that the P2A bridge is enabled.",
        "P2A(PCIe to AHB) bridge is enabled.",
        "Critical",
        0,
        {},
        "None.",
    },
    Message{
        "SecurityUartPortDebugEnabled",
        "Indicates that the uart port debug is enabled.",
        "Uart port debug is enabled.",
        "Critical",
        0,
        {},
        "None.",
    },
    Message{
        "SecurityUserStrongHashAlgoRestored",
        "Indicates that password computing hash algorithm changed.",
        "Password computing hash algorithm is changed to sha256/sha512.",
        "OK",
        0,
        {},
        "None.",
    },

    Message{
        "SecurityUserNonRootUidZeroAssigned",
        "Indicates that non root user assigned with user ID zero.",
        "User ID Zero is assigned with non-root user.",
        "Critical",
        0,
        {},
        "None.",
    },
    Message{
        "SecurityUserNonRootUidZeroRemoved",
        "Indicates that non root user ID is removed",
        "Non root user assigned with user ID zero is removed.",
        "OK",
        0,
        {},
        "None.",
    },

    Message{
        "SecurityUserRootEnabled",
        "Indicates that system root user is enabled.",
        "User root is enabled.",
        "Critical",
        0,
        {},
        "None.",
    },
    Message{
        "SecurityUserRootDisabled",
        "Indicates that system root user is disabled.",
        "User root is disabled.",
        "OK",
        0,
        {},
        "None.",
    },

    Message{
        "SecurityUserUnsupportedShellEnabled",
        "Indicates that unsupported shell is enabled.",
        "Unsupported shell is enabled.",
        "Critical",
        0,
        {},
        "None.",
    },
    Message{
        "SecurityUserUnsupportedShellRemoved",
        "Indicates that unsupported shell is removed.",
        "Unsupported shell is removed.",
        "OK",
        0,
        {},
        "None.",
    },

    Message{
        "SecurityUserWeakHashAlgoEnabled",
        "Indicates that weak password computing hash algorithm is enabled.",
        "Weak password computing hash algorithm is enabled.",
        "Critical",
        0,
        {},
        "None.",
    },
    Message{
        "SELEntryAdded",
        "Indicates a SEL entry was added using the "
        "Add SEL Entry or Platform Event command.",
        "SEL Entry Added: %1",
        "OK",
        1,
        {
            "string",
        },
        "None.",
    },
    Message{
        "SensorThresholdCriticalHighGoingHigh",
        "Indicates that a threshold sensor has crossed a "
        "critical high threshold going high.",
        "%1 sensor crossed a critical high threshold going "
        "high. Reading=%2 Threshold=%3.",
        "Critical",
        3,
        {"string", "number", "number"},
        "Check the sensor or subsystem for errors.",
    },
    Message{
        "SensorThresholdCriticalHighGoingLow",
        "Indicates that a threshold sensor has crossed a "
        "critical high threshold going low.",
        "%1 sensor crossed a critical high threshold going low. "
        "Reading=%2 Threshold=%3.",
        "OK",
        3,
        {"string", "number", "number"},
        "None.",
    },
    Message{
        "SensorThresholdCriticalLowGoingHigh",
        "Indicates that a threshold sensor has crossed a "
        "critical low threshold going high.",
        "%1 sensor crossed a critical low threshold going high. "
        "Reading=%2 Threshold=%3.",
        "OK",
        3,
        {"string", "number", "number"},
        "None.",
    },
    Message{
        "SensorThresholdCriticalLowGoingLow",
        "Indicates that a threshold sensor has crossed a "
        "critical low threshold going low.",
        "%1 sensor crossed a critical low threshold going low. "
        "Reading=%2 Threshold=%3.",
        "Critical",
        3,
        {"string", "number", "number"},
        "Check the sensor or subsystem for errors.",
    },
    Message{
        "SensorThresholdWarningHighGoingHigh",
        "Indicates that a threshold sensor has crossed a "
        "warning high threshold going high.",
        "%1 sensor crossed a warning high threshold going high. "
        "Reading=%2 Threshold=%3.",
        "Warning",
        3,
        {"string", "number", "number"},
        "Check the sensor or subsystem for errors.",
    },
    Message{
        "SensorThresholdWarningHighGoingLow",
        "Indicates that a threshold sensor has crossed a "
        "warning high threshold going low.",
        "%1 sensor crossed a warning high threshold going low. "
        "Reading=%2 Threshold=%3.",
        "OK",
        3,
        {"string", "number", "number"},
        "None.",
    },
    Message{
        "SensorThresholdWarningLowGoingHigh",
        "Indicates that a threshold sensor has crossed a "
        "warning low threshold going high.",
        "%1 sensor crossed a warning low threshold going high. "
        "Reading=%2 Threshold=%3.",
        "OK",
        3,
        {"string", "number", "number"},
        "None.",
    },
    Message{
        "SensorThresholdWarningLowGoingLow",
        "Indicates that a threshold sensor has crossed a "
        "warning low threshold going low.",
        "%1 sensor crossed a warning low threshold going low. "
        "Reading=%2 Threshold=%3.",
        "Warning",
        3,
        {"string", "number", "number"},
        "Check the sensor or subsystem for errors.",
    },
    Message{
        "ServiceFailure",
        "Indicates that a service has exited unsuccessfully.",
        "Service %1 has exited unsuccessfully.",
        "Warning",
        1,
        {"string"},
        "None.",
    },
    Message{
        "ServiceStarted",
        "Indicates that a service has started successfully.",
        "Service %1 has started successfully.",
        "OK",
        1,
        {"string"},
        "None.",
    },
    Message{
        "SparingRedundancyDegraded",
        "Indicates the sparing redundancy state is degraded.",
        "Sparing redundancy state degraded. Socket=%1 "
        "Channel=%2 DIMM=%3 Domain=%4 Rank=%5.",
        "Warning",
        5,

        {
            "number",
            "string",
            "number",
            "string",
            "number",
        },
        "None.",
    },
    Message{
        "SparingRedundancyFull",
        "Indicates the sparing redundancy state is fully redundant.",
        "Sparing redundancy state fully redundant. Socket=%1 "
        "Channel=%2 DIMM=%3 Domain=%4 Rank=%5.",
        "OK",
        5,

        {
            "number",
            "string",
            "number",
            "string",
            "number",
        },
        "None.",
    },
    Message{
        "SsbThermalTrip",
        "Indicates that an SSB Thermal trip has been asserted.",
        "SSB Thermal trip.",
        "Critical",
        0,
        {},
        "None.",
    },
    Message{
        "SystemInterfaceDisabledProvisioned",
        "Indicates that the system interface is in the disabled "
        "provisioned state. All commands are blocked to execute "
        "through the system interface.",
        "The system interface is in the disabled provisioned "
        "state.",
        "OK",
        0,
        {},
        "None.",
    },
    Message{
        "SystemInterfaceUnprovisioned",
        "Indicates that the system interface is in the "
        "unprovisioned state. All commands are permitted to "
        "execute through the system interface.",
        "The system interface is in the unprovisioned state.",
        "Critical",
        0,
        {},
        "None.",
    },
    Message{
        "SystemInterfaceWhitelistProvisioned",
        "Indicates that the system interface is in the whitelist "
        "provisioned state. Only whitelisted commands "
        "are permitted to execute through the system interface.",
        "The system interface is in the whitelist provisioned "
        "state.",
        "Warning",
        0,
        {},
        "None.",
    },
    Message{
        "SystemPowerGoodFailed",
        "Indicates that the system power good signal failed "
        "to assert within the specified time (VR failure).",
        "System power good failed to assert within %1 "
        "milliseconds (VR failure).",
        "Critical",
        1,
        {"number"},
        "None.",
    },
    Message{
        "SystemPowerLost",
        "Indicates that power was lost while the "
        "system was powered on.",
        "System Power Lost.",
        "Critical",
        0,
        {},
        "None.",
    },
    Message{
        "SystemPowerOffFailed",
        "Indicates that the system failed to power off.",
        "System Power-Off Failed.",
        "Critical",
        0,
        {},
        "None.",
    },
    Message{
        "SystemPowerOnFailed",
        "Indicates that the system failed to power on.",
        "System Power-On Failed.",
        "Critical",
        0,
        {},
        "None.",
    },
    Message{
        "VoltageRegulatorOverheated",
        "Indicates that the specified voltage regulator overheated.",
        "%1 Voltage Regulator Overheated.",
        "Critical",
        1,
        {"string"},
        "None.",
    },

};
} // namespace redfish::registries::openbmc
