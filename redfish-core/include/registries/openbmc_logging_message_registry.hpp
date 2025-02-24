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

namespace redfish::registries::openbmc_logging
{
const Header header = {
    "Copyright 2024-2025 OpenBMC.",
    "#MessageRegistry.v1_6_3.MessageRegistry",
    1,
    0,
    1,
    "OpenBMC Message Registry for xyz.openbmc_project.Logging",
    "en",
    "",
    "OpenBMC_Logging",
    "OpenBMC",
};
constexpr const char* url =
    "https://raw.githubusercontent.com/openbmc/bmcweb/refs/heads/master/redfish-core/include/registries/openbmc_logging.json";

constexpr std::array registry =
{
    MessageEntry{
        "Cleared",
        {
            "Indicate the user cleared all logs.",
            "Event log cleared by user.",
            "OK",
            0,
            {},
            "None.",
        }},

};

enum class Index
{
    cleared = 0,
};
} // namespace redfish::registries::openbmc_logging
