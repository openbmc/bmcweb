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

namespace redfish::registries::openbmc_state_cable
{
const Header header = {
    "Copyright 2024-2025 OpenBMC.",
    "#MessageRegistry.v1_6_3.MessageRegistry",
    1,
    0,
    0,
    "OpenBMC Message Registry for xyz.openbmc_project.State.Cable",
    "en",
    "OpenBMC Message Registry for xyz.openbmc_project.State.Cable",
    "OpenBMC_StateCable",
    "OpenBMC",
};
constexpr const char* url =
    nullptr;

constexpr std::array registry =
{
    MessageEntry{
        "CableConnected",
        {
            "An expected cable is connected.",
            "An expected cable on port %1 is connected.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "CableDisconnected",
        {
            "An expected cable is not connected.",
            "An expected cable on port %1 is not connected.",
            "Warning",
            1,
            {
                "string",
            },
            "Check and fix cable connections.",
        }},

};

enum class Index
{
    cableConnected = 0,
    cableDisconnected = 1,
};
} // namespace redfish::registries::openbmc_state_cable
