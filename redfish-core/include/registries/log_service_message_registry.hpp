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
struct log_service
{
static constexpr Header header = {
    "Copyright 2020-2023 DMTF. All rights reserved.",
    "#MessageRegistry.v1_6_2.MessageRegistry",
    1,
    0,
    1,
    "Log Service Message Registry",
    "en",
    "This registry defines the messages for log service related events.",
    "LogService",
    "DMTF",
};

static constexpr const char* url =
    "https://redfish.dmtf.org/registries/LogService.1.0.1.json";

static constexpr std::array registry =
{
    MessageEntry{
        "DiagnosticDataCollected",
        {
            "Indicates that diagnostic data was collected due to a client invoking the `CollectDiagnosticData` action.",
            "'%1' diagnostic data collected.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},

};

enum class Index
{
    diagnosticDataCollected = 0,
};
}; // struct log_service

[[gnu::constructor]] inline void register_log_service()
{ registerRegistry<log_service>(); }

} // namespace redfish::registries
