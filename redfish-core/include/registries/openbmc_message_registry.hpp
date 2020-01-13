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
    "Copyright 2018 OpenBMC. All rights reserved.",
    "#MessageRegistry.v1_0_0.MessageRegistry",
    "OpenBMC.0.1.0",
    "OpenBMC Message Registry",
    "en",
    "This registry defines the base messages for OpenBMC.",
    "OpenBMC",
    "0.1.0",
    "OpenBMC",
};
constexpr std::array<MessageEntry, 152> registry = {
    MessageEntry{
        "ADDDCCorrectable",
        {
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
        }},
    MessageEntry{
        "AtScaleDebugFeatureEnabledAtHardware",
        {
            "Indicates that At-Scale Debug enable is detected in hardware.",
            "At-Scale Debug Feature is enabled in hardware.",
            "Critical",
            0,
            {},
            "None.",
        }},
    MessageEntry{
        "AtScaleDebugFeatureDisabledAtHardware",
        {
            "Indicates that At-Scale Debug disable is detected in hardware.",
            "At-Scale Debug Feature is disabled in hardware.",
            "OK",
            0,
            {},
            "None.",
        }},
    MessageEntry{"AtScaleDebugFeatureEnabled",
                 {
                     "Indicates that At-Scale Debug service is started.",
                     "At-Scale Debug service is started.",
                     "Critical",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"AtScaleDebugFeatureDisabled",
                 {
                     "Indicates that At-Scale Debug service is stopped.",
                     "At-Scale Debug service is stopped.",
                     "OK",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"AtScaleDebugConnected",
                 {
                     "Indicates At-Scale Debug connection has been established",
                     "At-Scale Debug service is now connected %1",
                     "Critical",
                     1,
                     {"string"},
                     "None.",
                 }},
    MessageEntry{"AtScaleDebugDisconnected",
                 {
                     "Indicates At-Scale Debug connection has ended",
                     "At-Scale Debug service is now disconnected",
                     "OK",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"AtScaleDebugConnectionFailed",
                 {
                     "Indicates At-Scale Debug connection aborted/failed",
                     "At-Scale Debug connection aborted/failed",
                     "Critical",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"AtScaleDebugSpecialUserEnabled",
                 {
                     "Indicates that special user is enabled.",
                     "At-Scale Debug special user is enabled",
                     "Critical",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"AtScaleDebugSpecialUserDisabled",
                 {
                     "Indicates that special user is disabled.",
                     "At-Scale Debug special user is disabled",
                     "OK",
                     0,
                     {},
                     "None.",
                 }},

    MessageEntry{
        "BIOSBoot",
        {
            "Indicates BIOS has transitioned control to the OS Loader.",
            "BIOS System Boot.",
            "OK",
            0,
            {},
            "None.",
        }},
    MessageEntry{"BIOSFirmwarePanicReason",
                 {
                     "Indicates the reason for BIOS firmware panic.",
                     "BIOS firmware panic occurred due to %1.",
                     "Warning",
                     1,
                     {
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"BIOSFirmwareRecoveryReason",
                 {
                     "Indicates the reason for BIOS firmware recovery.",
                     "BIOS firmware recovery occurred due to %1.",
                     "Warning",
                     1,
                     {
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"BIOSFirmwareResiliencyError",
                 {
                     "Indicates BIOS firmware encountered resilience error.",
                     "BIOS firmware resiliency error. Error reason: %1.",
                     "Critical",
                     1,
                     {
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"BIOSPOSTError",
                 {
                     "Indicates BIOS POST has encountered an error.",
                     "BIOS POST Error. Error Code=%1",
                     "Warning",
                     1,
                     {"number"},
                     "None.",
                 }},
    MessageEntry{"BIOSRecoveryComplete",
                 {
                     "Indicates BIOS Recovery has completed.",
                     "BIOS Recovery Complete.",
                     "OK",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"BIOSRecoveryStart",
                 {
                     "Indicates BIOS Recovery has started.",
                     "BIOS Recovery Start.",
                     "Warning",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"BMCFirmwarePanicReason",
                 {
                     "Indicates the reason for last BMC firmware panic.",
                     "BMC firmware panic occurred due to %1.",
                     "Warning",
                     1,
                     {
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"BMCFirmwareRecoveryReason",
                 {
                     "Indicates the reason for last BMC firmware recovery.",
                     "BMC firmware recovery occurred due to %1.",
                     "Warning",
                     1,
                     {
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"BMCFirmwareResiliencyError",
                 {
                     "Indicates BMC firmware encountered resilience error.",
                     "BMC firmware resiliency error. Error reason: %1.",
                     "Critical",
                     1,
                     {
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"ChassisIntrusionDetected",
                 {
                     "Indicates that a physical security event "
                     "of the chassis intrusion has occurred.",
                     "Chassis Intrusion Detected.",
                     "Warning",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"ChassisIntrusionReset",
                 {
                     "Indicates that chassis intrusion status has recovered.",
                     "Chassis Intrusion Reset.",
                     "OK",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"CPLDFirmwarePanicReason",
                 {
                     "Indicates the reason for CPLD firmware panic.",
                     "CPLD firmware panic occurred due to %1.",
                     "Warning",
                     1,
                     {
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"CPLDFirmwareRecoveryReason",
                 {
                     "Indicates the reason for CPLD firmware recovery.",
                     "CPLD firmware recovery occurred due to %1.",
                     "Warning",
                     1,
                     {
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"CPLDFirmwareResiliencyError",
                 {
                     "Indicates CPLD firmware encountered resilience error.",
                     "CPLD firmware resiliency error. Error reason: %1.",
                     "Critical",
                     1,
                     {
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"CPUError",
                 {
                     "Indicates that a CPU Error occurred of "
                     "the specified type or cause.",
                     "CPU Error Occurred: %1.",
                     "Critical",
                     1,
                     {"string"},
                     "None.",
                 }},
    MessageEntry{"CPUThermalTrip",
                 {
                     "Indicates that the specified CPU thermal "
                     "trip has been asserted.",
                     "CPU %1 Thermal Trip.",
                     "Critical",
                     1,
                     {"number"},
                     "None.",
                 }},
    MessageEntry{"DCPowerOff",
                 {
                     "Indicates that the system DC power is off.",
                     "Host system DC power is off",
                     "OK",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"DCPowerOn",
                 {
                     "Indicates that the system DC power is on.",
                     "Host system DC power is on",
                     "OK",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"DriveError",
                 {
                     "Indicates that a Drive Error occurred of "
                     "the specified type or cause.",
                     "Drive Error Occurred: %1.",
                     "Warning",
                     1,
                     {"string"},
                     "None.",
                 }},
    MessageEntry{"EventLogCleared",
                 {
                     "Indicates that the event log has been cleared.",
                     "Event Log Cleared.",
                     "OK",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"FanInserted",
                 {
                     "Indicates that a system fan has been inserted.",
                     "%1 inserted.",
                     "OK",
                     1,
                     {"string"},
                     "None.",
                 }},
    MessageEntry{"FanRedundancyLost",
                 {
                     "Indicates that system fan redundancy has been lost.",
                     "Fan redundancy lost.",
                     "Warning",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"FanRedundancyRegained",
                 {
                     "Indicates that system fan redundancy has been regained.",
                     "Fan redundancy regained.",
                     "OK",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"FanRemoved",
                 {
                     "Indicates that a system fan has been removed.",
                     "%1 removed.",
                     "OK",
                     1,
                     {"string"},
                     "None.",
                 }},
    MessageEntry{"FirmwareUpdateCompleted",
                 {
                     "Indicates a firmware update has completed successfully.",
                     "%1 firmware update to version %2 completed "
                     "successfully.",
                     "OK",
                     2,
                     {"string", "string"},
                     "None.",
                 }},
    MessageEntry{"FirmwareUpdateFailed",
                 {
                     "Indicates a firmware update has failed.",
                     "%1 firmware update to version %2 failed.",
                     "Warning",
                     2,
                     {"string", "string"},
                     "None.",
                 }},
    MessageEntry{"FirmwareUpdateStarted",
                 {
                     "Indicates a firmware update has started.",
                     "%1 firmware update to version %2 started.",
                     "OK",
                     2,
                     {"string", "string"},
                     "None.",
                 }},
    MessageEntry{
        "GeneralFirmwareSecurityViolation",
        {
            "Indicates a general firmware security violation has occurred.",
            "Firmware security violation: %1.",
            "Critical",
            1,
            {"string"},
            "None.",
        }},
    MessageEntry{"InvalidLoginAttempted",
                 {
                     "Indicates that a login was attempted on the specified "
                     "interface with an invalid username or password.",
                     "Invalid username or password attempted on %1.",
                     "Warning",
                     1,
                     {"string"},
                     "None.",
                 }},
    MessageEntry{
        "InventoryAdded",
        {
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
        }},
    MessageEntry{
        "InventoryRemoved",
        {
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
        }},
    MessageEntry{
        "IntelUPILinkWidthReducedToHalf",
        {
            "Indicates Intel UPI link width has reduced to half width.",
            "Intel UPI link width reduced to half. Node=%1.",
            "Warning",
            1,

            {
                "number",
            },
            "None.",
        }},
    MessageEntry{
        "IntelUPILinkWidthReducedToQuarter",
        {
            "Indicates Intel UPI link width has reduced to quarter width.",
            "Intel UPI link width reduced to quarter. Node=%1.",
            "Warning",
            1,

            {
                "number",
            },
            "None.",
        }},

    MessageEntry{"IPMIWatchdog",
                 {
                     "Indicates that there is a host watchdog event.",
                     "Host Watchdog Event: %1",
                     "OK",
                     1,

                     {
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"LanLost",
                 {
                     "Indicates that a physical security event "
                     "of the LAN leash has lost.",
                     "%1 LAN leash lost.",
                     "Warning",
                     1,
                     {
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"LanRegained",
                 {
                     "Indicates that LAN link status has reconnected.",
                     "%1 LAN leash regained.",
                     "OK",
                     1,
                     {
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"LegacyPCIPERR",
                 {
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
                 }},
    MessageEntry{"LegacyPCISERR",
                 {
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
                 }},
    MessageEntry{"ManufacturingModeEntered",
                 {
                     "Indicates that the BMC entered Factory, "
                     "or Manufacturing mode.",
                     "Entered Manufacturing Mode.",
                     "Critical",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"ManufacturingModeExited",
                 {
                     "Indicates that the BMC exited Factory, "
                     "or Manufacturing mode.",
                     "Exited Manufacturing Mode.",
                     "OK",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"MEFirmwarePanicReason",
                 {
                     "Indicates the reason for ME firmware panic.",
                     "ME firmware panic occurred due to %1.",
                     "Warning",
                     1,
                     {
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"MEFirmwareRecoveryReason",
                 {
                     "Indicates the reason for ME firmware recovery.",
                     "ME firmware recovery occurred due to %1.",
                     "Warning",
                     1,
                     {
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"MEFirmwareResiliencyError",
                 {
                     "Indicates ME firmware encountered resilience error.",
                     "ME firmware resiliency error. Error reason: %1.",
                     "Critical",
                     1,
                     {
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"MemoryECCCorrectable",
                 {
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
                 }},
    MessageEntry{"MemoryECCUncorrectable",
                 {
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
                 }},
    MessageEntry{"MemoryParityCommandAndAddress",
                 {
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
                 }},
    MessageEntry{"MemoryParityNotKnown",
                 {
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
                 }},
    MessageEntry{"MemoryRASConfigurationDisabled",
                 {
                     "Indicates Memory RAS Disabled Configuration Status.",
                     "Memory RAS Configuration Disabled. Error=%1 Mode=%2.",
                     "OK",
                     2,

                     {
                         "string",
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"MemoryRASConfigurationEnabled",
                 {
                     "Indicates Memory RAS Enabled Configuration Status.",
                     "Memory RAS Configuration Enabled. Error=%1 Mode=%2.",
                     "OK",
                     2,

                     {
                         "string",
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"MemoryRASModeDisabled",
                 {
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
                 }},
    MessageEntry{"MemoryRASModeEnabled",
                 {
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
                 }},
    MessageEntry{"MemoryThermTrip",
                 {
                     "Indicates that the system memory ThermTrip is asserted "
                     "by the specified component.",
                     "Memory ThermTrip asserted: %1.",
                     "Critical",
                     1,
                     {"string"},
                     "None.",
                 }},
    MessageEntry{"MirroringRedundancyDegraded",
                 {
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
                 }},
    MessageEntry{
        "MirroringRedundancyFull",
        {
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
        }},
    MessageEntry{"NMIButtonPressed",
                 {
                     "Indicates that the NMI button was pressed.",
                     "NMI Button Pressed.",
                     "Critical",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"NMIDiagnosticInterrupt",
                 {
                     "Indicates that an NMI Diagnostic "
                     "Interrupt has been generated.",
                     "NMI Diagnostic Interrupt.",
                     "Critical",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"PCIeCorrectableAdvisoryNonFatal",
                 {
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
                 }},
    MessageEntry{"PCIeCorrectableBadDLLP",
                 {
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
                 }},
    MessageEntry{"PCIeCorrectableBadTLP",
                 {
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
                 }},
    MessageEntry{"PCIeCorrectableHeaderLogOverflow",
                 {
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
                 }},
    MessageEntry{"PCIeCorrectableInternal",
                 {
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
                 }},
    MessageEntry{"PCIeCorrectableLinkBWChanged",
                 {
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
                 }},
    MessageEntry{"PCIeCorrectableReceiverError",
                 {
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
                 }},
    MessageEntry{"PCIeCorrectableReplayNumRollover",
                 {
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
                 }},
    MessageEntry{"PCIeCorrectableReplayTimerTimeout",
                 {
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
                 }},
    MessageEntry{"PCIeCorrectableUnspecifiedAERError",
                 {
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
                 }},
    MessageEntry{"PCIeFatalACSViolation",
                 {
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
                 }},
    MessageEntry{"PCIeFatalAtomicEgressBlocked",
                 {
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
                 }},
    MessageEntry{
        "PCIeFatalCompleterAbort",
        {
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
        }},
    MessageEntry{
        "PCIeFatalCompletionTimeout",
        {
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
        }},
    MessageEntry{
        "PCIeFatalDataLinkLayerProtocol",
        {
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
        }},
    MessageEntry{"PCIeFatalECRCError",
                 {
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
                 }},
    MessageEntry{"PCIeFatalFlowControlProtocol",
                 {
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
                 }},
    MessageEntry{
        "PCIeFatalMalformedTLP",
        {
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
        }},
    MessageEntry{"PCIeFatalMCBlockedTLP",
                 {
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
                 }},
    MessageEntry{
        "PCIeFatalPoisonedTLP",
        {
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
        }},
    MessageEntry{"PCIeFatalReceiverBufferOverflow",
                 {
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
                 }},
    MessageEntry{
        "PCIeFatalReceivedErrNonFatalMessage",
        {
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
        }},
    MessageEntry{"PCIeFatalReceivedFatalMessageFromDownstream",
                 {
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
                 }},
    MessageEntry{"PCIeFatalSurpriseLinkDown",
                 {
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
                 }},
    MessageEntry{"PCIeFatalTLPPrefixBlocked",
                 {
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
                 }},
    MessageEntry{
        "PCIeFatalUncorrectableInternal",
        {
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
        }},
    MessageEntry{"PCIeFatalUnexpectedCompletion",
                 {
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
                 }},
    MessageEntry{"PCIeFatalUnspecifiedNonAERFatalError",
                 {
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
                 }},
    MessageEntry{
        "PCIeFatalUnsupportedRequest",
        {
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
        }},
    MessageEntry{"PowerButtonPressed",
                 {
                     "Indicates that the power button was pressed.",
                     "Power Button Pressed.",
                     "OK",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"PowerRestorePolicyApplied",
                 {
                     "Indicates that power was restored and the "
                     "BMC has applied the restore policy.",
                     "Power restore policy applied.",
                     "OK",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"PowerSupplyConfigurationError",
                 {
                     "Indicates an error in power supply configuration.",
                     "Power supply %1 configuration error.",
                     "Warning",
                     1,
                     {"string"},
                     "None.",
                 }},
    MessageEntry{
        "PowerSupplyConfigurationErrorRecovered",
        {
            "Indicates that power supply configuration error recovered "
            "from a failure.",
            "Power supply %1 configuration error recovered.",
            "OK",
            1,
            {"string"},
            "None.",
        }},
    MessageEntry{
        "PowerSupplyFanFailed",
        {
            "Indicates that the specified power supply fan has failed.",
            "Power supply %1 fan %2 failed.",
            "Warning",
            2,
            {"string", "string"},
            "None.",
        }},
    MessageEntry{
        "PowerSupplyFanRecovered",
        {
            "Indicates that the power supply fan recovered from a failure.",
            "Power supply %1 fan %2 recovered.",
            "OK",
            2,
            {"string", "string"},
            "None.",
        }},
    MessageEntry{"PowerSupplyFailed",
                 {
                     "Indicates that a power supply has failed.",
                     "Power supply %1 failed.",
                     "Warning",
                     1,
                     {"string"},
                     "None.",
                 }},
    MessageEntry{"PowerSupplyFailurePredicted",
                 {
                     "Indicates that a power supply is predicted to fail.",
                     "Power supply %1 failure predicted.",
                     "Warning",
                     1,
                     {"string"},
                     "None.",
                 }},
    MessageEntry{"PowerSupplyInserted",
                 {
                     "Indicates that a power supply has been inserted.",
                     "Power supply %1 inserted.",
                     "OK",
                     1,
                     {"string"},
                     "None.",
                 }},
    MessageEntry{"PowerSupplyPowerGoodFailed",
                 {
                     "Indicates that the power supply power good signal "
                     "failed to assert within the specified time.",
                     "Power supply power good failed to assert within %1 "
                     "milliseconds.",
                     "Critical",
                     1,
                     {"number"},
                     "None.",
                 }},
    MessageEntry{"PowerSupplyPowerLost",
                 {
                     "Indicates that a power supply has lost input power.",
                     "Power supply %1 power lost.",
                     "Warning",
                     1,
                     {"string"},
                     "None.",
                 }},
    MessageEntry{"PowerSupplyPowerRestored",
                 {
                     "Indicates that a power supply input power was restored.",
                     "Power supply %1 power restored.",
                     "OK",
                     1,
                     {"string"},
                     "None.",
                 }},
    MessageEntry{"PowerSupplyPredictedFailureRecovered",
                 {
                     "Indicates that a power supply recovered "
                     "from a predicted failure.",
                     "Power supply %1 predicted failure recovered.",
                     "OK",
                     1,
                     {"string"},
                     "None.",
                 }},
    MessageEntry{"PowerSupplyRecovered",
                 {
                     "Indicates that a power supply recovered from a failure.",
                     "Power supply %1 recovered.",
                     "OK",
                     1,
                     {"string"},
                     "None.",
                 }},
    MessageEntry{"PowerSupplyRemoved",
                 {
                     "Indicates that a power supply has been removed.",
                     "Power supply %1 removed.",
                     "Warning",
                     1,
                     {"string"},
                     "None.",
                 }},
    MessageEntry{"PowerUnitDegradedFromNonRedundant",
                 {
                     "Indicates that power unit is come back to redundant from"
                     "nonredundant but is still not in full redundancy mode.",
                     "Power Unit degraded from nonredundant.",
                     "Warning",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"PowerUnitDegradedFromRedundant",
                 {
                     "Indicates that power unit is degarded from full "
                     "redundancy mode.",
                     "Power Unit degraded from redundant.",
                     "Warning",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"PowerUnitRedundancyDegraded",
                 {
                     "Indicates that power unit redundancy has been degraded.",
                     "Power Unit Redundancy degraded.",
                     "Warning",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{
        "PowerUnitNonRedundantFromInsufficient",
        {
            "Indicates that power unit is not in redundancy mode and get"
            "sufficient power to support redundancy from insufficient"
            "power.",

            "Power Unit NonRedundant from insufficient to sufficient.",
            "Warning",
            0,
            {},
            "None.",
        }},
    MessageEntry{"PowerUnitNonRedundantInsufficient",
                 {
                     "Indicates that power unit do not have sufficient "
                     "power to support redundancy.",
                     "Power Unit NonRedundant and has insufficient resource.",
                     "Critical",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"PowerUnitRedundancyLost",
                 {
                     "Indicates that power unit redundancy has been lost.",
                     "Power Unit Redundancy lost.",
                     "Warning",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{
        "PowerUnitRedundancyRegained",
        {
            "Indicates that power unit full redundancy has been regained.",
            "Power Unit Redundancy regained.",
            "OK",
            0,
            {},
            "None.",
        }},
    MessageEntry{
        "PowerUnitNonRedundantSufficient",
        {
            "Indicates that power unit is not in redundancy mode but still"
            "has sufficient power to support redundancy.",
            "Power Unit Nonredundant but has sufficient resource.",
            "Warning",
            0,
            {},
            "None.",
        }},
    MessageEntry{"ResetButtonPressed",
                 {
                     "Indicates that the reset button was pressed.",
                     "Reset Button Pressed.",
                     "OK",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"SecurityBoot2ndFlashEnabled",
                 {
                     "Indicates that the BMC 2nd boot flash is enabled.",
                     "BMC 2nd boot flash is enabled.",
                     "Critical",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"SecurityP2aBridgeEnabled",
                 {
                     "Indicates that the P2A bridge is enabled.",
                     "P2A(PCIe to AHB) bridge is enabled.",
                     "Critical",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"SecurityUartPortDebugEnabled",
                 {
                     "Indicates that the uart port debug is enabled.",
                     "Uart port debug is enabled.",
                     "Critical",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{
        "SecurityUserStrongHashAlgoRestored",
        {
            "Indicates that password computing hash algorithm changed.",
            "Password computing hash algorithm is changed to sha256/sha512.",
            "OK",
            0,
            {},
            "None.",
        }},

    MessageEntry{"SecurityUserNonRootUidZeroAssigned",
                 {
                     "Indicates that non root user assigned with user ID zero.",
                     "User ID Zero is assigned with non-root user.",
                     "Critical",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"SecurityUserNonRootUidZeroRemoved",
                 {
                     "Indicates that non root user ID is removed",
                     "Non root user assigned with user ID zero is removed.",
                     "OK",
                     0,
                     {},
                     "None.",
                 }},

    MessageEntry{"SecurityUserRootEnabled",
                 {
                     "Indicates that system root user is enabled.",
                     "User root is enabled.",
                     "Critical",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"SecurityUserRootDisabled",
                 {
                     "Indicates that system root user is disabled.",
                     "User root is disabled.",
                     "OK",
                     0,
                     {},
                     "None.",
                 }},

    MessageEntry{"SecurityUserUnsupportedShellEnabled",
                 {
                     "Indicates that unsupported shell is enabled.",
                     "Unsupported shell is enabled.",
                     "Critical",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"SecurityUserUnsupportedShellRemoved",
                 {
                     "Indicates that unsupported shell is removed.",
                     "Unsupported shell is removed.",
                     "OK",
                     0,
                     {},
                     "None.",
                 }},

    MessageEntry{
        "SecurityUserWeakHashAlgoEnabled",
        {
            "Indicates that weak password computing hash algorithm is enabled.",
            "Weak password computing hash algorithm is enabled.",
            "Critical",
            0,
            {},
            "None.",
        }},

    MessageEntry{"SELEntryAdded",
                 {
                     "Indicates a SEL entry was added using the "
                     "Add SEL Entry or Platform Event command.",
                     "SEL Entry Added: %1",
                     "OK",
                     1,

                     {
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"SensorThresholdCriticalHighGoingHigh",
                 {
                     "Indicates that a threshold sensor has crossed a "
                     "critical high threshold going high.",
                     "%1 sensor crossed a critical high threshold going "
                     "high. Reading=%2 Threshold=%3.",
                     "Critical",
                     3,
                     {"string", "number", "number"},
                     "Check the sensor or subsystem for errors.",
                 }},
    MessageEntry{"SensorThresholdCriticalHighGoingLow",
                 {
                     "Indicates that a threshold sensor has crossed a "
                     "critical high threshold going low.",
                     "%1 sensor crossed a critical high threshold going low. "
                     "Reading=%2 Threshold=%3.",
                     "OK",
                     3,
                     {"string", "number", "number"},
                     "None.",
                 }},
    MessageEntry{"SensorThresholdCriticalLowGoingHigh",
                 {
                     "Indicates that a threshold sensor has crossed a "
                     "critical low threshold going high.",
                     "%1 sensor crossed a critical low threshold going high. "
                     "Reading=%2 Threshold=%3.",
                     "OK",
                     3,
                     {"string", "number", "number"},
                     "None.",
                 }},
    MessageEntry{"SensorThresholdCriticalLowGoingLow",
                 {
                     "Indicates that a threshold sensor has crossed a "
                     "critical low threshold going low.",
                     "%1 sensor crossed a critical low threshold going low. "
                     "Reading=%2 Threshold=%3.",
                     "Critical",
                     3,
                     {"string", "number", "number"},
                     "Check the sensor or subsystem for errors.",
                 }},
    MessageEntry{"SensorThresholdWarningHighGoingHigh",
                 {
                     "Indicates that a threshold sensor has crossed a "
                     "warning high threshold going high.",
                     "%1 sensor crossed a warning high threshold going high. "
                     "Reading=%2 Threshold=%3.",
                     "Warning",
                     3,
                     {"string", "number", "number"},
                     "Check the sensor or subsystem for errors.",
                 }},
    MessageEntry{"SensorThresholdWarningHighGoingLow",
                 {
                     "Indicates that a threshold sensor has crossed a "
                     "warning high threshold going low.",
                     "%1 sensor crossed a warning high threshold going low. "
                     "Reading=%2 Threshold=%3.",
                     "OK",
                     3,
                     {"string", "number", "number"},
                     "None.",
                 }},
    MessageEntry{"SensorThresholdWarningLowGoingHigh",
                 {
                     "Indicates that a threshold sensor has crossed a "
                     "warning low threshold going high.",
                     "%1 sensor crossed a warning low threshold going high. "
                     "Reading=%2 Threshold=%3.",
                     "OK",
                     3,
                     {"string", "number", "number"},
                     "None.",
                 }},
    MessageEntry{"SensorThresholdWarningLowGoingLow",
                 {
                     "Indicates that a threshold sensor has crossed a "
                     "warning low threshold going low.",
                     "%1 sensor crossed a warning low threshold going low. "
                     "Reading=%2 Threshold=%3.",
                     "Warning",
                     3,
                     {"string", "number", "number"},
                     "Check the sensor or subsystem for errors.",
                 }},
    MessageEntry{"ServiceFailure",
                 {
                     "Indicates that a service has exited unsuccessfully.",
                     "Service %1 has exited unsuccessfully.",
                     "Warning",
                     1,
                     {"string"},
                     "None.",
                 }},
    MessageEntry{"SparingRedundancyDegraded",
                 {
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
                 }},
    MessageEntry{
        "SparingRedundancyFull",
        {
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
        }},
    MessageEntry{"SsbThermalTrip",
                 {
                     "Indicates that an SSB Thermal trip has been asserted.",
                     "SSB Thermal trip.",
                     "Critical",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"SystemInterfaceDisabledProvisioned",
                 {
                     "Indicates that the system interface is in the disabled "
                     "provisioned state. All commands are blocked to execute "
                     "through the system interface.",
                     "The system interface is in the disabled provisioned "
                     "state.",
                     "OK",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"SystemInterfaceUnprovisioned",
                 {
                     "Indicates that the system interface is in the "
                     "unprovisioned state. All commands are permitted to "
                     "execute through the system interface.",
                     "The system interface is in the unprovisioned state.",
                     "Critical",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"SystemInterfaceWhitelistProvisioned",
                 {
                     "Indicates that the system interface is in the whitelist "
                     "provisioned state. Only whitelisted commands "
                     "are permitted to execute through the system interface.",
                     "The system interface is in the whitelist provisioned "
                     "state.",
                     "Warning",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"SystemPowerGoodFailed",
                 {
                     "Indicates that the system power good signal failed "
                     "to assert within the specified time (VR failure).",
                     "System power good failed to assert within %1 "
                     "milliseconds (VR failure).",
                     "Critical",
                     1,
                     {"number"},
                     "None.",
                 }},
    MessageEntry{"SystemPowerLost",
                 {
                     "Indicates that power was lost while the "
                     "system was powered on.",
                     "System Power Lost.",
                     "Critical",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"SystemPowerOffFailed",
                 {
                     "Indicates that the system failed to power off.",
                     "System Power-Off Failed.",
                     "Critical",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"SystemPowerOnFailed",
                 {
                     "Indicates that the system failed to power on.",
                     "System Power-On Failed.",
                     "Critical",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{
        "VoltageRegulatorOverheated",
        {
            "Indicates that the specified voltage regulator overheated.",
            "%1 Voltage Regulator Overheated.",
            "Critical",
            1,
            {"string"},
            "None.",
        }},
};
} // namespace redfish::message_registries::openbmc
