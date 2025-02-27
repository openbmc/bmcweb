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
struct heartbeat_event
{
static constexpr Header header = {
    "Copyright 2021-2023 DMTF. All rights reserved.",
    "#MessageRegistry.v1_6_2.MessageRegistry",
    1,
    0,
    1,
    "Heartbeat Event Message Registry",
    "en",
    "This registry defines the messages to use for periodic heartbeat, also known as 'keep alive', events.",
    "HeartbeatEvent",
    "DMTF",
};

static constexpr const char* url =
    "https://redfish.dmtf.org/registries/HeartbeatEvent.1.0.1.json";

static constexpr std::array registry =
{
    MessageEntry{
        "RedfishServiceFunctional",
        {
            "An event sent periodically upon request to indicates that the Redfish service is functional.",
            "Redfish service is functional.",
            "OK",
            0,
            {},
            "None.",
        }},

};

enum class Index
{
    redfishServiceFunctional = 0,
};
}; // struct heartbeat_event

[[gnu::constructor]] inline void register_heartbeat_event()
{ registerRegistry<heartbeat_event>(); }

} // namespace redfish::registries
