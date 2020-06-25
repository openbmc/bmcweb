/*
// Copyright (c) 2020 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
/****************************************************************
 * This is an auto-generated header which contains definitions
 * for Redfish DMTF defined messages.
 ***************************************************************/
#pragma once
#include <registries.hpp>

namespace redfish::message_registries::resource_event
{
const Header header = {
    "Copyright 2014-2020 DMTF in cooperation with the Storage Networking "
    "Industry Association (SNIA). All rights reserved.",
    "#MessageRegistry.v1_4_0.MessageRegistry",
    "ResourceEvent.1.0.3",
    "Resource Event Message Registry",
    "en",
    "This registry defines the messages to use for resource events.",
    "ResourceEvent",
    "1.0.3",
    "DMTF",
};
constexpr const char* url =
    "https://redfish.dmtf.org/registries/ResourceEvent.1.0.3.json";

constexpr std::array<MessageEntry, 19> registry = {
    MessageEntry{"LicenseAdded",
                 {
                     "Indicates that a license has been added.",
                     "A license for '%1' has been added.  The following "
                     "message was returned: '%2'.",
                     "OK",
                     "OK",
                     2,
                     {
                         "string",
                         "string",
                     },
                     "See vendor specific instructions for specific actions.",
                 }},
    MessageEntry{"LicenseChanged",
                 {
                     "Indicates that a license has changed.",
                     "A license for '%1' has changed.  The following message "
                     "was returned: '%2'.",
                     "Warning",
                     "Warning",
                     2,
                     {
                         "string",
                         "string",
                     },
                     "See vendor specific instructions for specific actions.",
                 }},
    MessageEntry{"LicenseExpired",
                 {
                     "Indicates that a license has expired.",
                     "A license for '%1' has expired.  The following message "
                     "was returned: '%2'.",
                     "Warning",
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
            "Indicates that one or more resource properties have changed.  "
            "This is not used whenever there is another event message for that "
            "specific change, such as only the state has changed.",
            "One or more resource properties have changed.",
            "OK",
            "OK",
            0,
            {},
            "None.",
        }},
    MessageEntry{"ResourceCreated",
                 {
                     "Indicates that all conditions of a successful creation "
                     "operation have been met.",
                     "The resource has been created successfully.",
                     "OK",
                     "OK",
                     0,
                     {},
                     "None",
                 }},
    MessageEntry{"ResourceErrorThresholdCleared",
                 {
                     "Indicates that a specified resource property has cleared "
                     "its error threshold.  Examples would be drive I/O "
                     "errors, or network link errors.",
                     "The resource property %1 has cleared the error threshold "
                     "of value %2.",
                     "OK",
                     "OK",
                     2,
                     {
                         "string",
                         "number",
                     },
                     "None.",
                 }},
    MessageEntry{"ResourceErrorThresholdExceeded",
                 {
                     "Indicates that a specified resource property has "
                     "exceeded its error threshold.  Examples would be drive "
                     "I/O errors, or network link errors.",
                     "The resource property %1 has exceeded error threshold of "
                     "value %2.",
                     "Critical",
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
            "Indicates that a specified resource property has corrected "
            "errors.  Examples would be drive I/O errors, or network link "
            "errors.",
            "The resource property %1 has corrected errors of type '%2'.",
            "OK",
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
            "Indicates that a specified resource property has detected errors. "
            " Examples would be drive I/O errors, or network link errors.",
            "The resource property %1 has detected errors of type '%2'.",
            "Warning",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "Resolution dependent upon error type.",
        }},
    MessageEntry{"ResourceRemoved",
                 {
                     "Indicates that all conditions of a successful remove "
                     "operation have been met.",
                     "The resource has been removed successfully.",
                     "OK",
                     "OK",
                     0,
                     {},
                     "None",
                 }},
    MessageEntry{"ResourceSelfTestCompleted",
                 {
                     "Indicates that a self-test has completed.",
                     "A self-test has completed.",
                     "OK",
                     "OK",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"ResourceSelfTestFailed",
                 {
                     "Indicates that a self-test has failed.  Suggested "
                     "resolution may be provided as OEM data.",
                     "A self-test has failed.  The following message was "
                     "returned: '%1'.",
                     "Critical",
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
            "Indicates that an incompatible version of software has been "
            "detected.  Examples may be after a component or system level "
            "software update.",
            "An incompatible version of software '%1' has been detected.",
            "Warning",
            "Warning",
            1,
            {
                "string",
            },
            "Compare the version of the resource with the compatible version "
            "of the software.",
        }},
    MessageEntry{"ResourceWarningThresholdCleared",
                 {
                     "Indicates that a specified resource property has cleared "
                     "its warning threshold.  Examples would be drive I/O "
                     "errors, or network link errors.  Suggested resolution "
                     "may be provided as OEM data.",
                     "The resource property %1 has cleared the warning "
                     "threshold of value %2.",
                     "OK",
                     "OK",
                     2,
                     {
                         "string",
                         "number",
                     },
                     "None.",
                 }},
    MessageEntry{"ResourceWarningThresholdExceeded",
                 {
                     "Indicates that a specified resource property has "
                     "exceeded its warning threshold.  Examples would be drive "
                     "I/O errors, or network link errors.  Suggested "
                     "resolution may be provided as OEM data.",
                     "The resource property %1 has exceeded its warning "
                     "threshold of value %2.",
                     "Warning",
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
            "Indicates that the URI for a resource has changed.  Examples for "
            "this would be physical component replacement or redistribution.",
            "The URI for the resource has changed.",
            "OK",
            "OK",
            0,
            {},
            "None.",
        }},
};
} // namespace redfish::message_registries::resource_event
