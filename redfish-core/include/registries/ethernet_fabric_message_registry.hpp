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

namespace redfish::registries::ethernet_fabric
{
const Header header = {
    "Copyright 2020-2023 DMTF. All rights reserved.",
    "#MessageRegistry.v1_6_2.MessageRegistry",
    "EthernetFabric.1.0.1",
    "Ethernet Fabric Message Registry",
    "en",
    "This registry defines messages for Ethernet fabrics.",
    "EthernetFabric",
    "1.0.1",
    "DMTF",
};
constexpr const char* url =
    "https://redfish.dmtf.org/registries/EthernetFabric.1.0.1.json";

constexpr std::array registry =
{
    MessageEntry{
        "LLDPInterfaceDisabled",
        {
            "Indicates that an interface has disabled Link Layer Discovery Protocol (LLDP).",
            "LLDP was disabled on switch '%1' port '%2'.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "Check that LLDP is enabled on device endpoints.",
        }},
    MessageEntry{
        "LLDPInterfaceEnabled",
        {
            "Indicates that an interface has enabled Link Layer Discovery Protocol (LLDP).",
            "LLDP was enabled on switch '%1' port '%2'.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "MLAGInterfaceDegraded",
        {
            "Indicates that multi-chassis link aggregation group (MLAG) interfaces were established, but at an unexpectedly low aggregated link speed.",
            "MLAG interface '%1' is degraded on switch '%2'.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "Contact the network administrator for problem resolution.",
        }},
    MessageEntry{
        "MLAGInterfaceDown",
        {
            "Indicates that the multi-chassis link aggregation group (MLAG) interface is down on a switch.",
            "The MLAG interface '%1' on switch '%2' is down.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "Check physical connectivity and that the MLAG system ID matches on switch pairs.",
        }},
    MessageEntry{
        "MLAGInterfacesUp",
        {
            "Indicates that all multi-chassis link aggregation group (MLAG) interfaces are up.",
            "All MLAG interfaces were established for MLAG ID '%1'.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "MLAGPeerDown",
        {
            "Indicates that the multi-chassis link aggregation group (MLAG) peer is down.",
            "MLAG peer switch '%1' with MLAG ID '%2' is down.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "Check physical connectivity and that the port channel ID matches on switch pairs.",
        }},
    MessageEntry{
        "MLAGPeerUp",
        {
            "Indicates that the multi-chassis link aggregation group (MLAG) peer is up.",
            "MLAG peer switch '%1' with MLAG ID '%2' is up.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "RoutingFailureThresholdExceeded",
        {
            "Indicates that a switch has encountered an unusually large number of routing errors.",
            "Switch '%1' has encountered %2 routing errors in the last %3 minutes.",
            "Warning",
            3,
            {
                "string",
                "number",
                "number",
            },
            "Contact the network administrator for problem resolution.",
        }},

};

enum class Index
{
    lLDPInterfaceDisabled = 0,
    lLDPInterfaceEnabled = 1,
    mLAGInterfaceDegraded = 2,
    mLAGInterfaceDown = 3,
    mLAGInterfacesUp = 4,
    mLAGPeerDown = 5,
    mLAGPeerUp = 6,
    routingFailureThresholdExceeded = 7,
};
} // namespace redfish::registries::ethernet_fabric
