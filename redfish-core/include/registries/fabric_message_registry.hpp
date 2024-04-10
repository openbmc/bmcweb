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

namespace redfish::registries::fabric
{
const Header header = {
    "Copyright 2014-2023 DMTF. All rights reserved.",
    "#MessageRegistry.v1_6_2.MessageRegistry",
    "Fabric.1.0.2",
    "Fabric Message Registry",
    "en",
    "This registry defines messages for generic fabrics.",
    "Fabric",
    "1.0.2",
    "DMTF",
};
constexpr const char* url =
    "https://redfish.dmtf.org/registries/Fabric.1.0.2.json";

constexpr std::array registry =
{
    MessageEntry{
        "AddressPoolCreated",
        {
            "Indicates that an address pool was created.",
            "Address pool '%1' was created in fabric '%2'.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "AddressPoolModified",
        {
            "Indicates that an address pool was modified.",
            "Address pool '%1' in fabric '%2' was modified.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "AddressPoolRemoved",
        {
            "Indicates that an address pool was removed.",
            "Address pool '%1' was removed from fabric '%2'.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "CableFailed",
        {
            "Indicates that a cable has failed.",
            "The cable in switch '%1' port '%2' has failed.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "Contact the network administrator for problem resolution.",
        }},
    MessageEntry{
        "CableInserted",
        {
            "Indicates that a cable was inserted into a switch's port.",
            "A cable was inserted into switch '%1' port '%2'.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "CableOK",
        {
            "Indicates that a cable has returned to working condition.",
            "The cable in switch '%1' port '%2' has returned to working condition.",
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
            "Indicates that a cable was removed from a switch's port.",
            "A cable was removed from switch '%1' port '%2'.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "ConnectionCreated",
        {
            "Indicates that a connection was created.",
            "Connection '%1' was created in fabric '%2'.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "ConnectionModified",
        {
            "Indicates that a connection was modified.",
            "Connection '%1' in fabric '%2' was modified.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "ConnectionRemoved",
        {
            "Indicates that a connection was removed.",
            "Connection '%1' was removed from fabric '%2'.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "DegradedDownstreamLinkEstablished",
        {
            "Indicates that a switch's downstream connection is established but is in a degraded state.",
            "Switch '%1' downstream link is established on port '%2', but is running in a degraded state.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "Contact the network administrator for problem resolution.",
        }},
    MessageEntry{
        "DegradedInterswitchLinkEstablished",
        {
            "Indicates that a switch's interswitch connection is established but is in a degraded state.",
            "Switch '%1' interswitch link is established on port '%2', but is running in a degraded state.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "Contact the network administrator for problem resolution.",
        }},
    MessageEntry{
        "DegradedUpstreamLinkEstablished",
        {
            "Indicates that a switch's upstream connection is established but is in a degraded state.",
            "Switch '%1' upstream link is established on port '%2', but is running in a degraded state.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "Contact the network administrator for problem resolution.",
        }},
    MessageEntry{
        "DownstreamLinkDropped",
        {
            "Indicates that a switch's downstream connection has gone down.",
            "Switch '%1' downstream link has gone down on port '%2'.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "Contact the network administrator for problem resolution.",
        }},
    MessageEntry{
        "DownstreamLinkEstablished",
        {
            "Indicates that a switch's downstream connection is established.",
            "Switch '%1' downstream link is established on port '%2'.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "DownstreamLinkFlapDetected",
        {
            "Indicates that a switch's downstream connection is highly unstable.",
            "Switch '%1' downstream link on port '%2' was established and dropped %3 times in the last %4 minutes.",
            "Warning",
            4,
            {
                "string",
                "string",
                "number",
                "number",
            },
            "Contact the network administrator for problem resolution.",
        }},
    MessageEntry{
        "EndpointCreated",
        {
            "Indicates that an endpoint was created or discovered.",
            "Endpoint '%1' was created in fabric '%2'.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "EndpointModified",
        {
            "Indicates that an endpoint was modified.",
            "Endpoint '%1' in fabric '%2' was modified.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "EndpointRemoved",
        {
            "Indicates that an endpoint was removed.",
            "Endpoint '%1' was removed from fabric '%2'.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "InterswitchLinkDropped",
        {
            "Indicates that a switch's interswitch connection has gone down.",
            "Switch '%1' interswitch link has gone down on port '%2'.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "Contact the network administrator for problem resolution.",
        }},
    MessageEntry{
        "InterswitchLinkEstablished",
        {
            "Indicates that a switch's interswitch connection is established.",
            "Switch '%1' interswitch link is established on port '%2'.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "InterswitchLinkFlapDetected",
        {
            "Indicates that a switch's interswitch connection is highly unstable.",
            "Switch '%1' interswitch link on port '%2' was established and dropped %3 times in the last %4 minutes.",
            "Warning",
            4,
            {
                "string",
                "string",
                "number",
                "number",
            },
            "Contact the network administrator for problem resolution.",
        }},
    MessageEntry{
        "MaxFrameSizeExceeded",
        {
            "Indicates that the maximum transmission unit (MTU) for the link was exceeded.",
            "MTU size on switch '%1' port '%2' is set to %3.  One or more packets with a larger size were dropped.",
            "Warning",
            3,
            {
                "string",
                "string",
                "number",
            },
            "Ensure that path MTU discovery is enabled and functioning correctly.",
        }},
    MessageEntry{
        "MediaControllerAdded",
        {
            "Indicates that a media controller was added.",
            "Media controller '%1' was added to chassis '%2'.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "MediaControllerModified",
        {
            "Indicates that a media controller was modified.",
            "Media controller '%1' in chassis '%2' was modified.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "MediaControllerRemoved",
        {
            "Indicates that a media controller was removed.",
            "Media controller '%1' was removed from chassis '%2'.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "PortAutomaticallyDisabled",
        {
            "Indicates that a switch's port was automatically disabled.",
            "Switch '%1' port '%2' was automatically disabled.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "PortAutomaticallyEnabled",
        {
            "Indicates that a switch's port was automatically enabled.",
            "Switch '%1' port '%2' was automatically enabled.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "PortDegraded",
        {
            "Indicates that a switch's port is in a degraded state.",
            "Switch '%1' port '%2' is in a degraded state.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "Contact the network administrator for problem resolution.",
        }},
    MessageEntry{
        "PortFailed",
        {
            "Indicates that a switch's port has become inoperative.",
            "Switch '%1' port '%2' has failed and is inoperative.",
            "Critical",
            2,
            {
                "string",
                "string",
            },
            "Contact the network administrator for problem resolution.",
        }},
    MessageEntry{
        "PortManuallyDisabled",
        {
            "Indicates that a switch's port was manually disabled.",
            "Switch '%1' port '%2' was manually disabled.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "PortManuallyEnabled",
        {
            "Indicates that a switch's port was manually enabled.",
            "Switch '%1' port '%2' was manually enabled.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "PortOK",
        {
            "Indicates that a switch's port has returned to a functional state.",
            "Switch '%1' port '%2' has returned to a functional state.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "SwitchDegraded",
        {
            "Indicates that a switch is in a degraded state.",
            "Switch '%1' is in a degraded state.",
            "Warning",
            1,
            {
                "string",
            },
            "Contact the network administrator for problem resolution.",
        }},
    MessageEntry{
        "SwitchFailed",
        {
            "Indicates that a switch has become inoperative.",
            "Switch '%1' has failed and is inoperative.",
            "Critical",
            1,
            {
                "string",
            },
            "Contact the network administrator for problem resolution.",
        }},
    MessageEntry{
        "SwitchOK",
        {
            "Indicates that a switch has returned to a functional state.",
            "Switch '%1' has returned to a functional state.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "UpstreamLinkDropped",
        {
            "Indicates that a switch's upstream connection has gone down.",
            "Switch '%1' upstream link has gone down on port '%2'.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "Contact the network administrator for problem resolution.",
        }},
    MessageEntry{
        "UpstreamLinkEstablished",
        {
            "Indicates that a switch's upstream connection is established.",
            "Switch '%1' upstream link is established on port '%2'.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "UpstreamLinkFlapDetected",
        {
            "Indicates that a switch's upstream connection is highly unstable.",
            "Switch '%1' upstream link on port '%2' was established and dropped %3 times in the last %4 minutes.",
            "Warning",
            4,
            {
                "string",
                "string",
                "number",
                "number",
            },
            "Contact the network administrator for problem resolution.",
        }},
    MessageEntry{
        "ZoneCreated",
        {
            "Indicates that a zone was created.",
            "Zone '%1' was created in fabric '%2'.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "ZoneModified",
        {
            "Indicates that a zone was modified.",
            "Zone '%1' in fabric '%2' was modified.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "ZoneRemoved",
        {
            "Indicates that a zone was removed.",
            "Zone '%1' was removed from fabric '%2'.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},

};

enum class Index
{
    addressPoolCreated = 0,
    addressPoolModified = 1,
    addressPoolRemoved = 2,
    cableFailed = 3,
    cableInserted = 4,
    cableOK = 5,
    cableRemoved = 6,
    connectionCreated = 7,
    connectionModified = 8,
    connectionRemoved = 9,
    degradedDownstreamLinkEstablished = 10,
    degradedInterswitchLinkEstablished = 11,
    degradedUpstreamLinkEstablished = 12,
    downstreamLinkDropped = 13,
    downstreamLinkEstablished = 14,
    downstreamLinkFlapDetected = 15,
    endpointCreated = 16,
    endpointModified = 17,
    endpointRemoved = 18,
    interswitchLinkDropped = 19,
    interswitchLinkEstablished = 20,
    interswitchLinkFlapDetected = 21,
    maxFrameSizeExceeded = 22,
    mediaControllerAdded = 23,
    mediaControllerModified = 24,
    mediaControllerRemoved = 25,
    portAutomaticallyDisabled = 26,
    portAutomaticallyEnabled = 27,
    portDegraded = 28,
    portFailed = 29,
    portManuallyDisabled = 30,
    portManuallyEnabled = 31,
    portOK = 32,
    switchDegraded = 33,
    switchFailed = 34,
    switchOK = 35,
    upstreamLinkDropped = 36,
    upstreamLinkEstablished = 37,
    upstreamLinkFlapDetected = 38,
    zoneCreated = 39,
    zoneModified = 40,
    zoneRemoved = 41,
};
} // namespace redfish::registries::fabric
