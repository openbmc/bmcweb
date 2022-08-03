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
    "Copyright 2014-2023 DMTF in cooperation with the Storage Networking Industry Association (SNIA). All rights reserved.",
    "#MessageRegistry.v1_6_0.MessageRegistry",
    "ResourceEvent.1.3.0",
    "Resource Event Message Registry",
    "en",
    "This registry defines the messages to use for resource events.",
    "ResourceEvent",
    "1.3.0",
    "DMTF",
};
constexpr const char* url =
    "https://redfish.dmtf.org/registries/ResourceEvent.1.3.0.json";

constexpr std::array registry =
{
    Message{
        "AggregationSourceDiscovered",
        "Indicates that a new aggregation source has been discovered.",
        "A aggregation source of connection method `%1` located at `%2` has been discovered.",
        "OK",
        2,
        {
            "string",
            "string",
        },
        "The aggregation source is available to the service and can be identified using the identified connection method.",
    },
    Message{
        "LicenseAdded",
        "Indicates that a license has been added.",
        "A license for '%1' has been added.  The following message was returned: '%2'.",
        "OK",
        2,
        {
            "string",
            "string",
        },
        "See vendor specific instructions for specific actions.",
    },
    Message{
        "LicenseChanged",
        "Indicates that a license has changed.",
        "A license for '%1' has changed.  The following message was returned: '%2'.",
        "Warning",
        2,
        {
            "string",
            "string",
        },
        "See vendor specific instructions for specific actions.",
    },
    Message{
        "LicenseExpired",
        "Indicates that a license has expired.",
        "A license for '%1' has expired.  The following message was returned: '%2'.",
        "Warning",
        2,
        {
            "string",
            "string",
        },
        "See vendor specific instructions for specific actions.",
    },
    Message{
        "ResourceChanged",
        "Indicates that one or more resource properties have changed.  This is not used whenever there is another event message for that specific change, such as only the state has changed.",
        "One or more resource properties have changed.",
        "OK",
        0,
        {},
        "None.",
    },
    Message{
        "ResourceCreated",
        "Indicates that all conditions of a successful creation operation have been met.",
        "The resource has been created successfully.",
        "OK",
        0,
        {},
        "None",
    },
    Message{
        "ResourceErrorThresholdCleared",
        "Indicates that a specified resource property has cleared its error threshold.  Examples would be drive I/O errors, or network link errors.",
        "The resource property %1 has cleared the error threshold of value %2.",
        "OK",
        2,
        {
            "string",
            "number",
        },
        "None.",
    },
    Message{
        "ResourceErrorThresholdExceeded",
        "Indicates that a specified resource property has exceeded its error threshold.  Examples would be drive I/O errors, or network link errors.",
        "The resource property %1 has exceeded error threshold of value %2.",
        "Critical",
        2,
        {
            "string",
            "number",
        },
        "None.",
    },
    Message{
        "ResourceErrorsCorrected",
        "Indicates that a specified resource property has corrected errors.  Examples would be drive I/O errors, or network link errors.",
        "The resource property %1 has corrected errors of type '%2'.",
        "OK",
        2,
        {
            "string",
            "string",
        },
        "None.",
    },
    Message{
        "ResourceErrorsDetected",
        "Indicates that a specified resource property has detected errors.  Examples would be drive I/O errors, or network link errors.",
        "The resource property %1 has detected errors of type '%2'.",
        "Warning",
        2,
        {
            "string",
            "string",
        },
        "Resolution dependent upon error type.",
    },
    Message{
        "ResourcePaused",
        "Indicates that the power state of a resource has changed to paused.",
        "The resource `%1` has been paused.",
        "OK",
        1,
        {
            "string",
        },
        "None.",
    },
    Message{
        "ResourcePoweredOff",
        "Indicates that the power state of a resource has changed to powered off.",
        "The resource `%1` has powered off.",
        "OK",
        1,
        {
            "string",
        },
        "None.",
    },
    Message{
        "ResourcePoweredOn",
        "Indicates that the power state of a resource has changed to powered on.",
        "The resource `%1` has powered on.",
        "OK",
        1,
        {
            "string",
        },
        "None.",
    },
    Message{
        "ResourcePoweringOff",
        "Indicates that the power state of a resource has changed to powering off.",
        "The resource `%1` is powering off.",
        "OK",
        1,
        {
            "string",
        },
        "None.",
    },
    Message{
        "ResourcePoweringOn",
        "Indicates that the power state of a resource has changed to powering on.",
        "The resource `%1` is powering on.",
        "OK",
        1,
        {
            "string",
        },
        "None.",
    },
    Message{
        "ResourceRemoved",
        "Indicates that all conditions of a successful remove operation have been met.",
        "The resource has been removed successfully.",
        "OK",
        0,
        {},
        "None",
    },
    Message{
        "ResourceSelfTestCompleted",
        "Indicates that a self-test has completed.",
        "A self-test has completed.",
        "OK",
        0,
        {},
        "None.",
    },
    Message{
        "ResourceSelfTestFailed",
        "Indicates that a self-test has failed.  Suggested resolution may be provided as OEM data.",
        "A self-test has failed.  The following message was returned: '%1'.",
        "Critical",
        1,
        {
            "string",
        },
        "See vendor specific instructions for specific actions.",
    },
    Message{
        "ResourceStateChanged",
        "Indicates that the state of a resource has changed.",
        "The state of resource `%1` has changed to %2.",
        "OK",
        2,
        {
            "string",
            "string",
        },
        "None.",
    },
    Message{
        "ResourceStatusChangedCritical",
        "Indicates that the health of a resource has changed to Critical.",
        "The health of resource `%1` has changed to %2.",
        "Critical",
        2,
        {
            "string",
            "string",
        },
        "None.",
    },
    Message{
        "ResourceStatusChangedOK",
        "Indicates that the health of a resource has changed to OK.",
        "The health of resource '%1' has changed to %2.",
        "OK",
        2,
        {
            "string",
            "string",
        },
        "None.",
    },
    Message{
        "ResourceStatusChangedWarning",
        "Indicates that the health of a resource has changed to Warning.",
        "The health of resource `%1` has changed to %2.",
        "Warning",
        2,
        {
            "string",
            "string",
        },
        "None.",
    },
    Message{
        "ResourceVersionIncompatible",
        "Indicates that an incompatible version of software has been detected.  Examples may be after a component or system level software update.",
        "An incompatible version of software '%1' has been detected.",
        "Warning",
        1,
        {
            "string",
        },
        "Compare the version of the resource with the compatible version of the software.",
    },
    Message{
        "ResourceWarningThresholdCleared",
        "Indicates that a specified resource property has cleared its warning threshold.  Examples would be drive I/O errors, or network link errors.  Suggested resolution may be provided as OEM data.",
        "The resource property %1 has cleared the warning threshold of value %2.",
        "OK",
        2,
        {
            "string",
            "number",
        },
        "None.",
    },
    Message{
        "ResourceWarningThresholdExceeded",
        "Indicates that a specified resource property has exceeded its warning threshold.  Examples would be drive I/O errors, or network link errors.  Suggested resolution may be provided as OEM data.",
        "The resource property %1 has exceeded its warning threshold of value %2.",
        "Warning",
        2,
        {
            "string",
            "number",
        },
        "None.",
    },
    Message{
        "TestMessage",
        "A test message used to validate event delivery mechanisms.",
        "Test message.",
        "OK",
        0,
        {},
        "None.",
    },
    Message{
        "URIForResourceChanged",
        "Indicates that the URI for a resource has changed.  Examples for this would be physical component replacement or redistribution.",
        "The URI for the resource has changed.",
        "OK",
        0,
        {},
        "None.",
    },

};

enum class Index
{
    aggregationSourceDiscovered = 0,
    licenseAdded = 1,
    licenseChanged = 2,
    licenseExpired = 3,
    resourceChanged = 4,
    resourceCreated = 5,
    resourceErrorThresholdCleared = 6,
    resourceErrorThresholdExceeded = 7,
    resourceErrorsCorrected = 8,
    resourceErrorsDetected = 9,
    resourcePaused = 10,
    resourcePoweredOff = 11,
    resourcePoweredOn = 12,
    resourcePoweringOff = 13,
    resourcePoweringOn = 14,
    resourceRemoved = 15,
    resourceSelfTestCompleted = 16,
    resourceSelfTestFailed = 17,
    resourceStateChanged = 18,
    resourceStatusChangedCritical = 19,
    resourceStatusChangedOK = 20,
    resourceStatusChangedWarning = 21,
    resourceVersionIncompatible = 22,
    resourceWarningThresholdCleared = 23,
    resourceWarningThresholdExceeded = 24,
    testMessage = 25,
    uRIForResourceChanged = 26,
};
} // namespace redfish::registries::resource_event
