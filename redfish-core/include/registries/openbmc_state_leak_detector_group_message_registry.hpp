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

namespace redfish::registries::openbmc_state_leak_detector_group
{
const Header header = {
    "Copyright 2024-2025 OpenBMC.",
    "#MessageRegistry.v1_6_3.MessageRegistry",
    1,
    0,
    0,
    "OpenBMC Message Registry for xyz.openbmc_project.State.Leak.DetectorGroup",
    "en",
    "",
    "OpenBMC_StateLeakDetectorGroup",
    "OpenBMC",
};
constexpr const char* url =
    "https://raw.githubusercontent.com/openbmc/bmcweb/refs/heads/master/redfish-core/include/registries/openbmc_state_leak_detector_group.json";

constexpr std::array registry =
{
    MessageEntry{
        "DetectorGroupCritical",
        {
            "Detector group has a critical status.",
            "Detector group %1 is in a critical state.",
            "Critical",
            1,
            {
                "string",
            },
            "Inspect the detectors in the group.",
        }},
    MessageEntry{
        "DetectorGroupNormal",
        {
            "The detector group has returned to its normal operating state.",
            "Detector group {DetectorName} is operating normally.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "DetectorGroupWarning",
        {
            "Detector group has a warning status.",
            "Detector group %1 is in a warning state.",
            "Warning",
            1,
            {
                "string",
            },
            "Inspect the detectors in the group.",
        }},

};

enum class Index
{
    detectorGroupCritical = 0,
    detectorGroupNormal = 1,
    detectorGroupWarning = 2,
};
} // namespace redfish::registries::openbmc_state_leak_detector_group
