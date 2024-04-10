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

namespace redfish::registries::network_device
{
const Header header = {
    "Copyright 2019-2023 DMTF. All rights reserved.",
    "#MessageRegistry.v1_6_2.MessageRegistry",
    "NetworkDevice.1.0.3",
    "Network Device Message Registry",
    "en",
    "This registry defines the messages for networking devices.",
    "NetworkDevice",
    "1.0.3",
    "DMTF",
};
constexpr const char* url =
    "https://redfish.dmtf.org/registries/NetworkDevice.1.0.3.json";

constexpr std::array registry =
{
    MessageEntry{
        "CableInserted",
        {
            "Indicates that a network cable was inserted.",
            "A network cable was inserted into network adapter '%1' port '%2'.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "CableRemoved",
        {
            "Indicates that a network cable was removed.",
            "A cable was removed from network adapter '%1' port '%2'.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "ConnectionDropped",
        {
            "Indicates that a network connection was dropped.",
            "The connection is no longer active for network adapter '%1' port '%2' function '%3'.",
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
        "ConnectionEstablished",
        {
            "Indicates that a network connection was established.",
            "A network connection was established for network adapter '%1' port '%2' function '%3'.",
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
        "DegradedConnectionEstablished",
        {
            "Indicates that a network connection was established, but at an unexpectedly low link speed.",
            "A degraded network connection was established for network adapter '%1' port '%2' function '%3'.",
            "Warning",
            3,
            {
                "string",
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "LinkFlapDetected",
        {
            "Indicates that a network connection is highly unstable.",
            "The network connection for network adapter '%1' port '%2' function '%3' was established and dropped '%4' times in the last '%5' minutes.",
            "Warning",
            5,
            {
                "string",
                "string",
                "string",
                "number",
                "number",
            },
            "Contact the network administrator for problem resolution.",
        }},

};

enum class Index
{
    cableInserted = 0,
    cableRemoved = 1,
    connectionDropped = 2,
    connectionEstablished = 3,
    degradedConnectionEstablished = 4,
    linkFlapDetected = 5,
};
} // namespace redfish::registries::network_device
