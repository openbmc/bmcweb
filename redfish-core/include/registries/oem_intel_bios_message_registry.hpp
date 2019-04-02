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

namespace redfish::message_registries::oem::intel::bios
{
const Header header = {
    .copyright = "Copyright 2019 Intel. All rights reserved.",
    .type = "#MessageRegistry.v1_0_0.MessageRegistry",
    .id = "IntelBIOS.0.1.0",
    .name = "Intel BIOS Message Registry",
    .language = "en",
    .description = "This registry defines the Intel BIOS messages.",
    .registryPrefix = "IntelBIOS",
    .registryVersion = "0.1.0",
    .owningEntity = "Intel",
};
const std::array registry = {
    MessageEntry{
        "ADDDCCorrectable",
        {
            .description = "Indicates an ADDDC Correctable Error.",
            .message =
                "ADDDC Correctable Error.Socket=%1 Channel=%2 DIMM=%3 Rank=%4.",
            .severity = "Warning",
            .numberOfArgs = 4,
            .paramTypes =
                {
                    "number",
                    "string",
                    "number",
                    "number",
                },
            .resolution = "None.",
        }},
    MessageEntry{
        "BIOSBoot",
        {
            .description =
                "Indicates BIOS has transitioned control to the OS Loader.",
            .message = "BIOS System Boot.",
            .severity = "OK",
            .numberOfArgs = 0,
            .paramTypes = {},
            .resolution = "None.",
        }},
    MessageEntry{
        "BIOSPOSTError",
        {
            .description = "Indicates BIOS POST has encountered an error.",
            .message = "BIOS POST Error. Error Code=%1",
            .severity = "Warning",
            .numberOfArgs = 1,
            .paramTypes = {"number"},
            .resolution = "None.",
        }},
    MessageEntry{"BIOSRecoveryComplete",
                 {
                     .description = "Indicates BIOS Recovery has completed.",
                     .message = "BIOS Recovery Complete.",
                     .severity = "OK",
                     .numberOfArgs = 0,
                     .paramTypes = {},
                     .resolution = "None.",
                 }},
    MessageEntry{"BIOSRecoveryStart",
                 {
                     .description = "Indicates BIOS Recovery has started.",
                     .message = "BIOS Recovery Start.",
                     .severity = "Warning",
                     .numberOfArgs = 0,
                     .paramTypes = {},
                     .resolution = "None.",
                 }},
    MessageEntry{
        "IntelUPILinkWidthReducedToHalf",
        {
            .description =
                "Indicates Intel UPI link width has reduced to half width.",
            .message = "Intel UPI link width reduced to half. Node=%1.",
            .severity = "Warning",
            .numberOfArgs = 1,
            .paramTypes =
                {
                    "number",
                },
            .resolution = "None.",
        }},
    MessageEntry{
        "IntelUPILinkWidthReducedToQuarter",
        {
            .description =
                "Indicates Intel UPI link width has reduced to quarter width.",
            .message = "Intel UPI link width reduced to quarter. Node=%1.",
            .severity = "Warning",
            .numberOfArgs = 1,
            .paramTypes =
                {
                    "number",
                },
            .resolution = "None.",
        }},
    MessageEntry{
        "LegacyPCIPERR",
        {
            .description = "Indicates a Legacy PCI PERR.",
            .message = "Legacy PCI PERR. Bus=%1 Device=%2 Function=%3.",
            .severity = "Warning",
            .numberOfArgs = 3,
            .paramTypes =
                {
                    "number",
                    "number",
                    "number",
                },
            .resolution = "None.",
        }},
    MessageEntry{
        "LegacyPCISERR",
        {
            .description = "Indicates a Legacy PCI SERR.",
            .message = "Legacy PCI SERR. Bus=%1 Device=%2 Function=%3.",
            .severity = "Critical",
            .numberOfArgs = 3,
            .paramTypes =
                {
                    "number",
                    "number",
                    "number",
                },
            .resolution = "None.",
        }},
    MessageEntry{"MemoryECCCorrectable",
                 {
                     .description = "Indicates a Correctable Memory ECC error.",
                     .message = "Memory ECC correctable error. Socket=%1 "
                                "Channel=%2 DIMM=%3 Rank=%4.",
                     .severity = "Warning",
                     .numberOfArgs = 4,
                     .paramTypes =
                         {
                             "number",
                             "string",
                             "number",
                             "number",
                         },
                     .resolution = "None.",
                 }},
    MessageEntry{
        "MemoryECCUncorrectable",
        {
            .description = "Indicates an Uncorrectable Memory ECC error.",
            .message = "Memory ECC uncorrectable error. Socket=%1 Channel=%2 "
                       "DIMM=%3 Rank=%4.",
            .severity = "Critical",
            .numberOfArgs = 4,
            .paramTypes =
                {
                    "number",
                    "string",
                    "number",
                    "number",
                },
            .resolution = "None.",
        }},
    MessageEntry{
        "MemoryParityCommandAndAddress",
        {
            .description = "Indicates a Command and Address parity error.",
            .message = "Command and Address parity error. Socket=%1 Channel=%2 "
                       "DIMM=%3 ChannelValid=%4 DIMMValid=%5.",
            .severity = "Critical",
            .numberOfArgs = 5,
            .paramTypes =
                {
                    "number",
                    "string",
                    "number",
                    "number",
                    "number",
                },
            .resolution = "None.",
        }},
    MessageEntry{"MemoryParityNotKnown",
                 {
                     .description = "Indicates an unknown parity error.",
                     .message = "Memory parity error. Socket=%1 Channel=%2 "
                                "DIMM=%3 ChannelValid=%4 DIMMValid=%5.",
                     .severity = "Critical",
                     .numberOfArgs = 5,
                     .paramTypes =
                         {
                             "number",
                             "string",
                             "number",
                             "number",
                             "number",
                         },
                     .resolution = "None.",
                 }},
    MessageEntry{
        "MemoryRASConfigurationDisabled",
        {
            .description =
                "Indicates Memory RAS Disabled Configuration Status.",
            .message = "Memory RAS Configuration Disabled. Error=%1 Mode=%2.",
            .severity = "OK",
            .numberOfArgs = 2,
            .paramTypes =
                {
                    "string",
                    "string",
                },
            .resolution = "None.",
        }},
    MessageEntry{
        "MemoryRASConfigurationEnabled",
        {
            .description = "Indicates Memory RAS Enabled Configuration Status.",
            .message = "Memory RAS Configuration Enabled. Error=%1 Mode=%2.",
            .severity = "OK",
            .numberOfArgs = 2,
            .paramTypes =
                {
                    "string",
                    "string",
                },
            .resolution = "None.",
        }},
    MessageEntry{
        "MemoryRASModeDisabled",
        {
            .description = "Indicates Memory RAS Disabled Mode Selection.",
            .message = "Memory RAS Mode Select Disabled. Prior Mode=%1 "
                       "Selected Mode=%2.",
            .severity = "OK",
            .numberOfArgs = 2,
            .paramTypes =
                {
                    "string",
                    "string",
                },
            .resolution = "None.",
        }},
    MessageEntry{
        "MemoryRASModeEnabled",
        {
            .description = "Indicates Memory RAS Enabled Mode Selection.",
            .message = "Memory RAS Mode Select Enabled. Prior Mode=%1 Selected "
                       "Mode=%2.",
            .severity = "OK",
            .numberOfArgs = 2,
            .paramTypes =
                {
                    "string",
                    "string",
                },
            .resolution = "None.",
        }},
    MessageEntry{
        "MirroringRedundancyDegraded",
        {
            .description =
                "Indicates the mirroring redundancy state is degraded.",
            .message = "Mirroring redundancy state degraded. Socket=%1 "
                       "Channel=%2 DIMM=%3 Pair=%4 Rank=%5.",
            .severity = "Warning",
            .numberOfArgs = 5,
            .paramTypes =
                {
                    "number",
                    "string",
                    "number",
                    "number",
                    "number",
                },
            .resolution = "None.",
        }},
    MessageEntry{
        "MirroringRedundancyFull",
        {
            .description =
                "Indicates the mirroring redundancy state is fully redundant.",
            .message = "Mirroring redundancy state fully redundant. Socket=%1 "
                       "Channel=%2 DIMM=%3 Pair=%4 Rank=%5.",
            .severity = "OK",
            .numberOfArgs = 5,
            .paramTypes =
                {
                    "number",
                    "string",
                    "number",
                    "number",
                    "number",
                },
            .resolution = "None.",
        }},
    MessageEntry{
        "PCIeCorrectableAdvisoryNonFatal",
        {
            .description =
                "Indicates a PCIe Correctable Advisory Non-fatal Error.",
            .message = "PCIe Correctable Advisory Non-fatal Error. Bus=%1 "
                       "Device=%2 Function=%3.",
            .severity = "Warning",
            .numberOfArgs = 3,
            .paramTypes =
                {
                    "number",
                    "number",
                    "number",
                },
            .resolution = "None.",
        }},
    MessageEntry{
        "PCIeCorrectableBadDLLP",
        {
            .description = "Indicates a PCIe Correctable Bad DLLP Error.",
            .message =
                "PCIe Correctable Bad DLLP. Bus=%1 Device=%2 Function=%3.",
            .severity = "Warning",
            .numberOfArgs = 3,
            .paramTypes =
                {
                    "number",
                    "number",
                    "number",
                },
            .resolution = "None.",
        }},
    MessageEntry{
        "PCIeCorrectableBadTLP",
        {
            .description = "Indicates a PCIe Correctable Bad TLP Error.",
            .message =
                "PCIe Correctable Bad TLP. Bus=%1 Device=%2 Function=%3.",
            .severity = "Warning",
            .numberOfArgs = 3,
            .paramTypes =
                {
                    "number",
                    "number",
                    "number",
                },
            .resolution = "None.",
        }},
    MessageEntry{
        "PCIeCorrectableHeaderLogOverflow",
        {
            .description =
                "Indicates a PCIe Correctable Header Log Overflow Error.",
            .message = "PCIe Correctable Header Log Overflow. Bus=%1 Device=%2 "
                       "Function=%3.",
            .severity = "Warning",
            .numberOfArgs = 3,
            .paramTypes =
                {
                    "number",
                    "number",
                    "number",
                },
            .resolution = "None.",
        }},
    MessageEntry{
        "PCIeCorrectableInternal",
        {
            .description = "Indicates a PCIe Correctable Internal Error.",
            .message = "PCIe Correctable Internal Error. Bus=%1 Device=%2 "
                       "Function=%3.",
            .severity = "Warning",
            .numberOfArgs = 3,
            .paramTypes =
                {
                    "number",
                    "number",
                    "number",
                },
            .resolution = "None.",
        }},
    MessageEntry{"PCIeCorrectableLinkBWChanged",
                 {
                     .description =
                         "Indicates a PCIe Correctable Link BW Changed Error.",
                     .message = "PCIe Correctable Link BW Changed. Bus=%1 "
                                "Device=%2 Function=%3.",
                     .severity = "Warning",
                     .numberOfArgs = 3,
                     .paramTypes =
                         {
                             "number",
                             "number",
                             "number",
                         },
                     .resolution = "None.",
                 }},
    MessageEntry{
        "PCIeCorrectableReceiverError",
        {
            .description = "Indicates a PCIe Correctable Receiver Error.",
            .message = "PCIe Correctable Receiver Error. Bus=%1 Device=%2 "
                       "Function=%3.",
            .severity = "Warning",
            .numberOfArgs = 3,
            .paramTypes =
                {
                    "number",
                    "number",
                    "number",
                },
            .resolution = "None.",
        }},
    MessageEntry{
        "PCIeCorrectableReplayNumRollover",
        {
            .description = "Indicates a PCIe Correctable Replay Num Rollover.",
            .message = "PCIe Correctable Replay Num Rollover. Bus=%1 Device=%2 "
                       "Function=%3.",
            .severity = "Warning",
            .numberOfArgs = 3,
            .paramTypes =
                {
                    "number",
                    "number",
                    "number",
                },
            .resolution = "None.",
        }},
    MessageEntry{
        "PCIeCorrectableReplayTimerTimeout",
        {
            .description = "Indicates a PCIe Correctable Replay Timer Timeout.",
            .message = "PCIe Correctable Replay Timer Timeout. Bus=%1 "
                       "Device=%2 Function=%3.",
            .severity = "Warning",
            .numberOfArgs = 3,
            .paramTypes =
                {
                    "number",
                    "number",
                    "number",
                },
            .resolution = "None.",
        }},
    MessageEntry{"PCIeCorrectableUnspecifiedAERError",
                 {
                     .description =
                         "Indicates a PCIe Correctable Unspecified AER Error.",
                     .message = "PCIe Correctable Unspecified AER Error. "
                                "Bus=%1 Device=%2 Function=%3.",
                     .severity = "Warning",
                     .numberOfArgs = 3,
                     .paramTypes =
                         {
                             "number",
                             "number",
                             "number",
                         },
                     .resolution = "None.",
                 }},
    MessageEntry{
        "PCIeFatalACSViolation",
        {
            .description = "Indicates a PCIe ACS Violation Error.",
            .message =
                "PCIe Fatal ACS Violation. Bus=%1 Device=%2 Function=%3.",
            .severity = "Error",
            .numberOfArgs = 3,
            .paramTypes =
                {
                    "number",
                    "number",
                    "number",
                },
            .resolution = "None.",
        }},
    MessageEntry{
        "PCIeFatalAtomicEgressBlocked",
        {
            .description = "Indicates a PCIe Atomic Egress Blocked Error.",
            .message = "PCIe Fatal Atomic Egress Blocked. Bus=%1 Device=%2 "
                       "Function=%3.",
            .severity = "Error",
            .numberOfArgs = 3,
            .paramTypes =
                {
                    "number",
                    "number",
                    "number",
                },
            .resolution = "None.",
        }},
    MessageEntry{
        "PCIeFatalCompleterAbort",
        {
            .description = "Indicates a PCIe Completer Abort Error.",
            .message =
                "PCIe Fatal Completer Abort. Bus=%1 Device=%2 Function=%3.",
            .severity = "Error",
            .numberOfArgs = 3,
            .paramTypes =
                {
                    "number",
                    "number",
                    "number",
                },
            .resolution = "None.",
        }},
    MessageEntry{
        "PCIeFatalCompletionTimeout",
        {
            .description = "Indicates a PCIe Completion Timeout Error.",
            .message =
                "PCIe Fatal Completion Timeout. Bus=%1 Device=%2 Function=%3.",
            .severity = "Error",
            .numberOfArgs = 3,
            .paramTypes =
                {
                    "number",
                    "number",
                    "number",
                },
            .resolution = "None.",
        }},
    MessageEntry{
        "PCIeFatalDataLinkLayerProtocol",
        {
            .description = "Indicates a PCIe Data Link Layer Protocol Error.",
            .message =
                "PCIe Fatal Data Link Layer Protocol Error. Bus=%1 Device=%2 "
                "Function=%3.",
            .severity = "Error",
            .numberOfArgs = 3,
            .paramTypes =
                {
                    "number",
                    "number",
                    "number",
                },
            .resolution = "None.",
        }},
    MessageEntry{
        "PCIeFatalECRCError",
        {
            .description = "Indicates a PCIe ECRC Error.",
            .message = "PCIe Fatal ECRC Error. Bus=%1 Device=%2 Function=%3.",
            .severity = "Error",
            .numberOfArgs = 3,
            .paramTypes =
                {
                    "number",
                    "number",
                    "number",
                },
            .resolution = "None.",
        }},
    MessageEntry{
        "PCIeFatalFlowControlProtocol",
        {
            .description = "Indicates a PCIe Flow Control Protocol Error.",
            .message =
                "PCIe Fatal Flow Control Protocol Error. Bus=%1 Device=%2 "
                "Function=%3.",
            .severity = "Error",
            .numberOfArgs = 3,
            .paramTypes =
                {
                    "number",
                    "number",
                    "number",
                },
            .resolution = "None.",
        }},
    MessageEntry{
        "PCIeFatalMalformedTLP",
        {
            .description = "Indicates a PCIe Malformed TLP Error.",
            .message =
                "PCIe Fatal Malformed TLP Error. Bus=%1 Device=%2 Function=%3.",
            .severity = "Error",
            .numberOfArgs = 3,
            .paramTypes =
                {
                    "number",
                    "number",
                    "number",
                },
            .resolution = "None.",
        }},
    MessageEntry{"PCIeFatalMCBlockedTLP",
                 {
                     .description = "Indicates a PCIe MC Blocked TLP Error.",
                     .message = "PCIe Fatal MC Blocked TLP Error. Bus=%1 "
                                "Device=%2 Function=%3.",
                     .severity = "Error",
                     .numberOfArgs = 3,
                     .paramTypes =
                         {
                             "number",
                             "number",
                             "number",
                         },
                     .resolution = "None.",
                 }},
    MessageEntry{
        "PCIeFatalPoisonedTLP",
        {
            .description = "Indicates a PCIe Poisoned TLP Error.",
            .message =
                "PCIe Fatal Poisoned TLP Error. Bus=%1 Device=%2 Function=%3.",
            .severity = "Error",
            .numberOfArgs = 3,
            .paramTypes =
                {
                    "number",
                    "number",
                    "number",
                },
            .resolution = "None.",
        }},
    MessageEntry{
        "PCIeFatalReceiverBufferOverflow",
        {
            .description = "Indicates a PCIe Receiver Buffer Overflow Error.",
            .message = "PCIe Fatal Receiver Buffer Overflow. Bus=%1 Device=%2 "
                       "Function=%3.",
            .severity = "Error",
            .numberOfArgs = 3,
            .paramTypes =
                {
                    "number",
                    "number",
                    "number",
                },
            .resolution = "None.",
        }},
    MessageEntry{
        "PCIeFatalReceivedERR_NONFATALMessage",
        {
            .description =
                "Indicates a PCIe Received ERR_NONFATAL Message Error.",
            .message =
                "PCIe Fatal Received ERR_NONFATAL Message. Bus=%1 Device=%2 "
                "Function=%3.",
            .severity = "Error",
            .numberOfArgs = 3,
            .paramTypes =
                {
                    "number",
                    "number",
                    "number",
                },
            .resolution = "None.",
        }},
    MessageEntry{"PCIeFatalReceivedFatalMessageFromDownstream",
                 {
                     .description = "Indicates a PCIe Received Fatal Message "
                                    "From Downstream Error.",
                     .message =
                         "PCIe Fatal Received Fatal Message From Downstream. "
                         "Bus=%1 Device=%2 Function=%3.",
                     .severity = "Error",
                     .numberOfArgs = 3,
                     .paramTypes =
                         {
                             "number",
                             "number",
                             "number",
                         },
                     .resolution = "None.",
                 }},
    MessageEntry{
        "PCIeFatalSurpriseLinkDown",
        {
            .description = "Indicates a PCIe Surprise Link Down Error.",
            .message = "PCIe Fatal Surprise Link Down Error. Bus=%1 Device=%2 "
                       "Function=%3.",
            .severity = "Error",
            .numberOfArgs = 3,
            .paramTypes =
                {
                    "number",
                    "number",
                    "number",
                },
            .resolution = "None.",
        }},
    MessageEntry{
        "PCIeFatalTLPPrefixBlocked",
        {
            .description = "Indicates a PCIe TLP Prefix Blocked Error.",
            .message = "PCIe Fatal TLP Prefix Blocked Error. Bus=%1 Device=%2 "
                       "Function=%3.",
            .severity = "Error",
            .numberOfArgs = 3,
            .paramTypes =
                {
                    "number",
                    "number",
                    "number",
                },
            .resolution = "None.",
        }},
    MessageEntry{
        "PCIeFatalUncorrectableInternal",
        {
            .description = "Indicates a PCIe Uncorrectable Internal Error.",
            .message =
                "PCIe Fatal Uncorrectable Internal Error. Bus=%1 Device=%2 "
                "Function=%3.",
            .severity = "Error",
            .numberOfArgs = 3,
            .paramTypes =
                {
                    "number",
                    "number",
                    "number",
                },
            .resolution = "None.",
        }},
    MessageEntry{
        "PCIeFatalUnexpectedCompletion",
        {
            .description = "Indicates a PCIe Unexpected Completion Error.",
            .message = "PCIe Fatal Unexpected Completion. Bus=%1 Device=%2 "
                       "Function=%3.",
            .severity = "Error",
            .numberOfArgs = 3,
            .paramTypes =
                {
                    "number",
                    "number",
                    "number",
                },
            .resolution = "None.",
        }},
    MessageEntry{
        "PCIeFatalUnspecifiedNonAERFatalError",
        {
            .description = "Indicates a PCIe Unspecified Non-AER Fatal Error.",
            .message = "PCIe Fatal Unspecified Non-AER Fatal Error. Bus=%1 "
                       "Device=%2 Function=%3.",
            .severity = "Error",
            .numberOfArgs = 3,
            .paramTypes =
                {
                    "number",
                    "number",
                    "number",
                },
            .resolution = "None.",
        }},
    MessageEntry{
        "PCIeFatalUnsupportedRequest",
        {
            .description = "Indicates a PCIe Unsupported Request Error.",
            .message =
                "PCIe Fatal Unsupported Request. Bus=%1 Device=%2 Function=%3.",
            .severity = "Error",
            .numberOfArgs = 3,
            .paramTypes =
                {
                    "number",
                    "number",
                    "number",
                },
            .resolution = "None.",
        }},
    MessageEntry{"SparingRedundancyDegraded",
                 {
                     .description =
                         "Indicates the sparing redundancy state is degraded.",
                     .message = "Sparing redundancy state degraded. Socket=%1 "
                                "Channel=%2 DIMM=%3 Domain=%4 Rank=%5.",
                     .severity = "Warning",
                     .numberOfArgs = 5,
                     .paramTypes =
                         {
                             "number",
                             "string",
                             "number",
                             "string",
                             "number",
                         },
                     .resolution = "None.",
                 }},
    MessageEntry{
        "SparingRedundancyFull",
        {
            .description =
                "Indicates the sparing redundancy state is fully redundant.",
            .message = "Sparing redundancy state fully redundant. Socket=%1 "
                       "Channel=%2 DIMM=%3 Domain=%4 Rank=%5.",
            .severity = "OK",
            .numberOfArgs = 5,
            .paramTypes =
                {
                    "number",
                    "string",
                    "number",
                    "string",
                    "number",
                },
            .resolution = "None.",
        }},
    MessageEntry{"Unknown",
                 {
                     .description = "Indicates an unkown BIOS message.",
                     .message = "Unknown BIOS message: %1",
                     .severity = "Critical",
                     .numberOfArgs = 1,
                     .paramTypes =
                         {
                             "string",
                         },
                     .resolution = "None.",
                 }},
}; // namespace redfish::message_registries::oem::intel::bios
} // namespace redfish::message_registries::oem::intel::bios
