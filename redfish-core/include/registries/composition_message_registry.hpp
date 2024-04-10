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

namespace redfish::registries::composition
{
const Header header = {
    "Copyright 2019-2023 DMTF. All rights reserved.",
    "#MessageRegistry.v1_6_2.MessageRegistry",
    "Composition.1.1.2",
    "Composition Message Registry",
    "en",
    "This registry defines the messages for composition related events.",
    "Composition",
    "1.1.2",
    "DMTF",
};
constexpr const char* url =
    "https://redfish.dmtf.org/registries/Composition.1.1.2.json";

constexpr std::array registry =
{
    MessageEntry{
        "ConstrainedResourceAlreadyReserved",
        {
            "Indicates that the requested resources are already reserved in response to a constrained composition request.",
            "The requested resources are reserved under reservation '%1'.",
            "Critical",
            1,
            {
                "string",
            },
            "Delete the reservation containing the resources and resubmit the request.",
        }},
    MessageEntry{
        "EmptyManifest",
        {
            "Indicates that the manifest contains no stanzas or that a stanza in the manifest contains no request.",
            "The provided manifest is empty or a stanza in the manifest contains no request.",
            "Warning",
            0,
            {},
            "Provide a request content for the manifest and resubmit.",
        }},
    MessageEntry{
        "IncompatibleZone",
        {
            "Indicates that not all referenced resource blocks are in the same resource zone.",
            "The requested resource blocks span multiple resource zones.",
            "Critical",
            0,
            {},
            "Request resource blocks from the same resource zone.",
        }},
    MessageEntry{
        "NoResourceMatch",
        {
            "Indicates that the service could not find a matching resource based on the given parameters.",
            "The requested resources of type '%1' are not available for allocation.",
            "Critical",
            1,
            {
                "string",
            },
            "Change parameters associated with the resource, such as quantity or performance, and resubmit the request.",
        }},
    MessageEntry{
        "ResourceBlockChanged",
        {
            "Indicates that a resource block has changed.  This is not used whenever there is another event message for that specific change, such as when only the state has changed.",
            "Resource block '%1' has changed on the service.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "ResourceBlockCompositionStateChanged",
        {
            "Indicates that the composition state of a resource block has changed, specifically the value of the `CompositionState` property within `CompositionStatus`.",
            "The composition status of the resource block '%1' has changed.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "ResourceBlockInUse",
        {
            "Indicates that the composition request contains a resource block that is unable to participate in more compositions.",
            "Resource block '%1' cannot be part of any new compositions.",
            "Warning",
            1,
            {
                "string",
            },
            "Remove the resource block from the request and resubmit the request.",
        }},
    MessageEntry{
        "ResourceBlockInvalid",
        {
            "Indicates that the `Id` of a referenced resource block is no longer valid.",
            "Resource block '%1' is not valid.",
            "Critical",
            1,
            {
                "string",
            },
            "Remove the resource block and resubmit the request.",
        }},
    MessageEntry{
        "ResourceBlockNotFound",
        {
            "Indicates that the referenced resource block was not found.",
            "Resource block '%1' was not found.",
            "Critical",
            1,
            {
                "string",
            },
            "Remove the resource block and resubmit the request.",
        }},
    MessageEntry{
        "ResourceBlockStateChanged",
        {
            "Indicates that the state of a resource block has changed, specifically the value of the `State` property within `Status`.",
            "The state of resource block '%1' has changed.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "ResourceZoneMembershipChanged",
        {
            "Indicates that the membership of a resource zone has changed due to resource blocks being added or removed from the resource zone.",
            "The membership of resource zone '%1' has been changed.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "SpecifiedResourceAlreadyReserved",
        {
            "Indicates that a resource block is already reserved in response to a specific composition request.",
            "Resource block '%1' is already reserved under reservation '%2'.",
            "Critical",
            2,
            {
                "string",
                "string",
            },
            "Delete the reservation containing the resource block or select a different resource block and resubmit the request.",
        }},
    MessageEntry{
        "UnableToProcessStanzaRequest",
        {
            "Indicates that the manifest provided for the `Compose` action contains a stanza with `Content` that could not be processed.",
            "The provided manifest for the Compose action of type %1 contains a stanza with Id of value '%2' with a Content parameter that could not be processed.",
            "Critical",
            2,
            {
                "string",
                "string",
            },
            "Add the Content parameter to the stanza or remove the stanza, and resubmit the request.",
        }},

};

enum class Index
{
    constrainedResourceAlreadyReserved = 0,
    emptyManifest = 1,
    incompatibleZone = 2,
    noResourceMatch = 3,
    resourceBlockChanged = 4,
    resourceBlockCompositionStateChanged = 5,
    resourceBlockInUse = 6,
    resourceBlockInvalid = 7,
    resourceBlockNotFound = 8,
    resourceBlockStateChanged = 9,
    resourceZoneMembershipChanged = 10,
    specifiedResourceAlreadyReserved = 11,
    unableToProcessStanzaRequest = 12,
};
} // namespace redfish::registries::composition
