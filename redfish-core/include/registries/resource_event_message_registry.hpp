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

namespace redfish::registries::resource_event
{
const Header header = {
    "Copyright 2014-2020 DMTF in cooperation with the Storage Networking Industry Association (SNIA). All rights reserved.",
    "#MessageRegistry.v1_4_0.MessageRegistry",
    "ResourceEvent.1.0.3",
    "Resource Event Message Registry",
    "en",
    "This registry defines the messages to use for resource events.",
    "ResourceEvent",
    "1.0.3",
    "DMTF",
};
constexpr std::string_view url =
    "https://redfish.dmtf.org/registries/ResourceEvent.1.0.3.json";

constexpr std::array registry =
{
    MessageEntry{
        "LicenseAdded",
        {
            "Indicates that a license has been added.",
            "A license for '%1' has been added.  The following message was returned: '%2'.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "See vendor specific instructions for specific actions.",
        }},
    MessageEntry{
        "LicenseChanged",
        {
            "Indicates that a license has changed.",
            "A license for '%1' has changed.  The following message was returned: '%2'.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "See vendor specific instructions for specific actions.",
        }},
    MessageEntry{
        "LicenseExpired",
        {
            "Indicates that a license has expired.",
            "A license for '%1' has expired.  The following message was returned: '%2'.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "See vendor specific instructions for specific actions.",
        }},
    MessageEntry{
        "ResourceChanged",
        {
            "Indicates that one or more resource properties have changed.  This is not used whenever there is another event message for that specific change, such as only the state has changed.",
            "One or more resource properties have changed.",
            "OK",
            0,
            {},
            "None.",
        }},
    MessageEntry{
        "ResourceCreated",
        {
            "Indicates that all conditions of a successful creation operation have been met.",
            "The resource has been created successfully.",
            "OK",
            0,
            {},
            "None",
        }},
    MessageEntry{
        "ResourceErrorThresholdCleared",
        {
            "Indicates that a specified resource property has cleared its error threshold.  Examples would be drive I/O errors, or network link errors.",
            "The resource property %1 has cleared the error threshold of value %2.",
            "OK",
            2,
            {
                "string",
                "number",
            },
            "None.",
        }},
    MessageEntry{
        "ResourceErrorThresholdExceeded",
        {
            "Indicates that a specified resource property has exceeded its error threshold.  Examples would be drive I/O errors, or network link errors.",
            "The resource property %1 has exceeded error threshold of value %2.",
            "Critical",
            2,
            {
                "string",
                "number",
            },
            "None.",
        }},
    MessageEntry{
        "ResourceErrorsCorrected",
        {
            "Indicates that a specified resource property has corrected errors.  Examples would be drive I/O errors, or network link errors.",
            "The resource property %1 has corrected errors of type '%2'.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "ResourceErrorsDetected",
        {
            "Indicates that a specified resource property has detected errors.  Examples would be drive I/O errors, or network link errors.",
            "The resource property %1 has detected errors of type '%2'.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "Resolution dependent upon error type.",
        }},
    MessageEntry{
        "ResourceRemoved",
        {
            "Indicates that all conditions of a successful remove operation have been met.",
            "The resource has been removed successfully.",
            "OK",
            0,
            {},
            "None",
        }},
    MessageEntry{
        "ResourceSelfTestCompleted",
        {
            "Indicates that a self-test has completed.",
            "A self-test has completed.",
            "OK",
            0,
            {},
            "None.",
        }},
    MessageEntry{
        "ResourceSelfTestFailed",
        {
            "Indicates that a self-test has failed.  Suggested resolution may be provided as OEM data.",
            "A self-test has failed.  The following message was returned: '%1'.",
            "Critical",
            1,
            {
                "string",
            },
            "See vendor specific instructions for specific actions.",
        }},
    MessageEntry{
        "ResourceStatusChangedCritical",
        {
            "Indicates that the health of a resource has changed to Critical.",
            "The health of resource `%1` has changed to %2.",
            "Critical",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "ResourceStatusChangedOK",
        {
            "Indicates that the health of a resource has changed to OK.",
            "The health of resource '%1' has changed to %2.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "ResourceStatusChangedWarning",
        {
            "Indicates that the health of a resource has changed to Warning.",
            "The health of resource `%1` has changed to %2.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "ResourceVersionIncompatible",
        {
            "Indicates that an incompatible version of software has been detected.  Examples may be after a component or system level software update.",
            "An incompatible version of software '%1' has been detected.",
            "Warning",
            1,
            {
                "string",
            },
            "Compare the version of the resource with the compatible version of the software.",
        }},
    MessageEntry{
        "ResourceWarningThresholdCleared",
        {
            "Indicates that a specified resource property has cleared its warning threshold.  Examples would be drive I/O errors, or network link errors.  Suggested resolution may be provided as OEM data.",
            "The resource property %1 has cleared the warning threshold of value %2.",
            "OK",
            2,
            {
                "string",
                "number",
            },
            "None.",
        }},
    MessageEntry{
        "ResourceWarningThresholdExceeded",
        {
            "Indicates that a specified resource property has exceeded its warning threshold.  Examples would be drive I/O errors, or network link errors.  Suggested resolution may be provided as OEM data.",
            "The resource property %1 has exceeded its warning threshold of value %2.",
            "Warning",
            2,
            {
                "string",
                "number",
            },
            "None.",
        }},
    MessageEntry{
        "URIForResourceChanged",
        {
            "Indicates that the URI for a resource has changed.  Examples for this would be physical component replacement or redistribution.",
            "The URI for the resource has changed.",
            "OK",
            0,
            {},
            "None.",
        }},

};

enum class Index
{
    licenseAdded = 0,
    licenseChanged = 1,
    licenseExpired = 2,
    resourceChanged = 3,
    resourceCreated = 4,
    resourceErrorThresholdCleared = 5,
    resourceErrorThresholdExceeded = 6,
    resourceErrorsCorrected = 7,
    resourceErrorsDetected = 8,
    resourceRemoved = 9,
    resourceSelfTestCompleted = 10,
    resourceSelfTestFailed = 11,
    resourceStatusChangedCritical = 12,
    resourceStatusChangedOK = 13,
    resourceStatusChangedWarning = 14,
    resourceVersionIncompatible = 15,
    resourceWarningThresholdCleared = 16,
    resourceWarningThresholdExceeded = 17,
    uRIForResourceChanged = 18,
};
} // namespace redfish::registries::resource_event
