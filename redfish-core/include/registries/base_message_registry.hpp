/*
// Copyright (c) 2019 Intel Corporation
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

namespace redfish::message_registries::base
{
const Header header = {
    "Copyright 2014-2018 DMTF. All rights reserved.",
    "#MessageRegistry.v1_0_0.MessageRegistry",
    "Base.1.4.0",
    "Base Message Registry",
    "en",
    "This registry defines the base messages for Redfish",
    "Base",
    "1.4.0",
    "DMTF",
};
constexpr std::array<MessageEntry, 58> registry = {
    MessageEntry{
        "AccessDenied",
        {
            "Indicates that while attempting to access, connect to or transfer "
            "to/from another resource, the service denied access.",
            "While attempting to establish a connection to %1, the service "
            "denied access.",
            "Critical",
            1,
            {
                "string",
            },
            "Attempt to ensure that the URI is correct and that the service "
            "has the appropriate credentials.",
        }},
    MessageEntry{"AccountForSessionNoLongerExists",
                 {
                     "Indicates that the account for the session has been "
                     "removed, thus the session has been removed as well.",
                     "The account for the current session has been removed, "
                     "thus the current session has been removed as well.",
                     "OK",
                     0,
                     {},
                     "Attempt to connect with a valid account.",
                 }},
    MessageEntry{"AccountModified",
                 {
                     "Indicates that the account was successfully modified.",
                     "The account was successfully modified.",
                     "OK",
                     0,
                     {},
                     "No resolution is required.",
                 }},
    MessageEntry{"AccountNotModified",
                 {
                     "Indicates that the modification requested for the "
                     "account was not successful.",
                     "The account modification request failed.",
                     "Warning",
                     0,
                     {},
                     "The modification may have failed due to permission "
                     "issues or issues with the request body.",
                 }},
    MessageEntry{"AccountRemoved",
                 {
                     "Indicates that the account was successfully removed.",
                     "The account was successfully removed.",
                     "OK",
                     0,
                     {},
                     "No resolution is required.",
                 }},
    MessageEntry{
        "ActionNotSupported",
        {
            "Indicates that the action supplied with the POST operation is not "
            "supported by the resource.",
            "The action %1 is not supported by the resource.",
            "Critical",
            1,
            {
                "string",
            },
            "The action supplied cannot be resubmitted to the implementation.  "
            "Perhaps the action was invalid, the wrong resource was the target "
            "or the implementation documentation may be of assistance.",
        }},
    MessageEntry{"ActionParameterDuplicate",
                 {
                     "Indicates that the action was supplied with a duplicated "
                     "parameter in the request body.",
                     "The action %1 was submitted with more than one value for "
                     "the parameter %2.",
                     "Warning",
                     2,
                     {
                         "string",
                         "string",
                     },
                     "Resubmit the action with only one instance of the "
                     "parameter in the request body if the operation failed.",
                 }},
    MessageEntry{"ActionParameterMissing",
                 {
                     "Indicates that the action requested was missing a "
                     "parameter that is required to process the action.",
                     "The action %1 requires the parameter %2 to be present in "
                     "the request body.",
                     "Critical",
                     2,
                     {
                         "string",
                         "string",
                     },
                     "Supply the action with the required parameter in the "
                     "request body when the request is resubmitted.",
                 }},
    MessageEntry{"ActionParameterNotSupported",
                 {
                     "Indicates that the parameter supplied for the action is "
                     "not supported on the resource.",
                     "The parameter %1 for the action %2 is not supported on "
                     "the target resource.",
                     "Warning",
                     2,
                     {
                         "string",
                         "string",
                     },
                     "Remove the parameter supplied and resubmit the request "
                     "if the operation failed.",
                 }},
    MessageEntry{
        "ActionParameterUnknown",
        {
            "Indicates that an action was submitted but a parameter supplied "
            "did not match any of the known parameters.",
            "The action %1 was submitted with the invalid parameter %2.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "Correct the invalid parameter and resubmit the request if the "
            "operation failed.",
        }},
    MessageEntry{"ActionParameterValueFormatError",
                 {
                     "Indicates that a parameter was given the correct value "
                     "type but the value of that parameter was not supported.  "
                     "This includes value size/length exceeded.",
                     "The value %1 for the parameter %2 in the action %3 is of "
                     "a different format than the parameter can accept.",
                     "Warning",
                     3,
                     {
                         "string",
                         "string",
                         "string",
                     },
                     "Correct the value for the parameter in the request body "
                     "and resubmit the request if the operation failed.",
                 }},
    MessageEntry{"ActionParameterValueTypeError",
                 {
                     "Indicates that a parameter was given the wrong value "
                     "type, such as when a number is supplied for a parameter "
                     "that requires a string.",
                     "The value %1 for the parameter %2 in the action %3 is of "
                     "a different type than the parameter can accept.",
                     "Warning",
                     3,
                     {
                         "string",
                         "string",
                         "string",
                     },
                     "Correct the value for the parameter in the request body "
                     "and resubmit the request if the operation failed.",
                 }},
    MessageEntry{
        "CouldNotEstablishConnection",
        {
            "Indicates that the attempt to access the resource/file/image at "
            "the URI was unsuccessful because a session could not be "
            "established.",
            "The service failed to establish a connection with the URI %1.",
            "Critical",
            1,
            {
                "string",
            },
            "Ensure that the URI contains a valid and reachable node name, "
            "protocol information and other URI components.",
        }},
    MessageEntry{
        "CreateFailedMissingReqProperties",
        {
            "Indicates that a create was attempted on a resource but that "
            "properties that are required for the create operation were "
            "missing from the request.",
            "The create operation failed because the required property %1 was "
            "missing from the request.",
            "Critical",
            1,
            {
                "string",
            },
            "Correct the body to include the required property with a valid "
            "value and resubmit the request if the operation failed.",
        }},
    MessageEntry{"CreateLimitReachedForResource",
                 {
                     "Indicates that no more resources can be created on the "
                     "resource as it has reached its create limit.",
                     "The create operation failed because the resource has "
                     "reached the limit of possible resources.",
                     "Critical",
                     0,
                     {},
                     "Either delete resources and resubmit the request if the "
                     "operation failed or do not resubmit the request.",
                 }},
    MessageEntry{"Created",
                 {
                     "Indicates that all conditions of a successful creation "
                     "operation have been met.",
                     "The resource has been created successfully",
                     "OK",
                     0,
                     {},
                     "None",
                 }},
    MessageEntry{
        "EmptyJSON",
        {
            "Indicates that the request body contained an empty JSON object "
            "when one or more properties are expected in the body.",
            "The request body submitted contained an empty JSON object and the "
            "service is unable to process it.",
            "Warning",
            0,
            {},
            "Add properties in the JSON object and resubmit the request.",
        }},
    MessageEntry{
        "EventSubscriptionLimitExceeded",
        {
            "Indicates that a event subscription establishment has been "
            "requested but the operation failed due to the number of "
            "simultaneous connection exceeding the limit of the "
            "implementation.",
            "The event subscription failed due to the number of simultaneous "
            "subscriptions exceeding the limit of the implementation.",
            "Critical",
            0,
            {},
            "Reduce the number of other subscriptions before trying to "
            "establish the event subscription or increase the limit of "
            "simultaneous subscriptions (if supported).",
        }},
    MessageEntry{
        "GeneralError",
        {
            "Indicates that a general error has occurred.  Use in ExtendedInfo "
            "is discouraged.  When used in ExtendedInfo, implementations are "
            "expected to include a Resolution property with this error to "
            "indicate how to resolve the problem.",
            "A general error has occurred. See Resolution for information on "
            "how to resolve the error.",
            "Critical",
            0,
            {},
            "None.",
        }},
    MessageEntry{
        "InsufficientPrivilege",
        {
            "Indicates that the credentials associated with the established "
            "session do not have sufficient privileges for the requested "
            "operation",
            "There are insufficient privileges for the account or credentials "
            "associated with the current session to perform the requested "
            "operation.",
            "Critical",
            0,
            {},
            "Either abandon the operation or change the associated access "
            "rights and resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "InternalError",
        {
            "Indicates that the request failed for an unknown internal error "
            "but that the service is still operational.",
            "The request failed due to an internal service error.  The service "
            "is still operational.",
            "Critical",
            0,
            {},
            "Resubmit the request.  If the problem persists, consider "
            "resetting the service.",
        }},
    MessageEntry{"InvalidIndex",
                 {
                     "The Index is not valid.",
                     "The Index %1 is not a valid offset into the array.",
                     "Warning",
                     1,
                     {
                         "number",
                     },
                     "Verify the index value provided is within the bounds of "
                     "the array.",
                 }},
    MessageEntry{
        "InvalidObject",
        {
            "Indicates that the object in question is invalid according to the "
            "implementation.  Examples include a firmware update malformed "
            "URI.",
            "The object at %1 is invalid.",
            "Critical",
            1,
            {
                "string",
            },
            "Either the object is malformed or the URI is not correct.  "
            "Correct the condition and resubmit the request if it failed.",
        }},
    MessageEntry{"MalformedJSON",
                 {
                     "Indicates that the request body was malformed JSON.  "
                     "Could be duplicate, syntax error,etc.",
                     "The request body submitted was malformed JSON and could "
                     "not be parsed by the receiving service.",
                     "Critical",
                     0,
                     {},
                     "Ensure that the request body is valid JSON and resubmit "
                     "the request.",
                 }},
    MessageEntry{
        "NoOperation",
        {
            "Indicates that the requested operation will not perform any "
            "changes on the service.",
            "The request body submitted contain no data to act upon and no "
            "changes to the resource took place.",
            "Warning",
            0,
            {},
            "Add properties in the JSON object and resubmit the request.",
        }},
    MessageEntry{
        "NoValidSession",
        {
            "Indicates that the operation failed because a valid session is "
            "required in order to access any resources.",
            "There is no valid session established with the implementation.",
            "Critical",
            0,
            {},
            "Establish as session before attempting any operations.",
        }},
    MessageEntry{"PropertyDuplicate",
                 {
                     "Indicates that a duplicate property was included in the "
                     "request body.",
                     "The property %1 was duplicated in the request.",
                     "Warning",
                     1,
                     {
                         "string",
                     },
                     "Remove the duplicate property from the request body and "
                     "resubmit the request if the operation failed.",
                 }},
    MessageEntry{
        "PropertyMissing",
        {
            "Indicates that a required property was not supplied as part of "
            "the request.",
            "The property %1 is a required property and must be included in "
            "the request.",
            "Warning",
            1,
            {
                "string",
            },
            "Ensure that the property is in the request body and has a valid "
            "value and resubmit the request if the operation failed.",
        }},
    MessageEntry{"PropertyNotWritable",
                 {
                     "Indicates that a property was given a value in the "
                     "request body, but the property is a readonly property.",
                     "The property %1 is a read only property and cannot be "
                     "assigned a value.",
                     "Warning",
                     1,
                     {
                         "string",
                     },
                     "Remove the property from the request body and resubmit "
                     "the request if the operation failed.",
                 }},
    MessageEntry{"PropertyUnknown",
                 {
                     "Indicates that an unknown property was included in the "
                     "request body.",
                     "The property %1 is not in the list of valid properties "
                     "for the resource.",
                     "Warning",
                     1,
                     {
                         "string",
                     },
                     "Remove the unknown property from the request body and "
                     "resubmit the request if the operation failed.",
                 }},
    MessageEntry{"PropertyValueFormatError",
                 {
                     "Indicates that a property was given the correct value "
                     "type but the value of that property was not supported.",
                     "The value %1 for the property %2 is of a different "
                     "format than the property can accept.",
                     "Warning",
                     2,
                     {
                         "string",
                         "string",
                     },
                     "Correct the value for the property in the request body "
                     "and resubmit the request if the operation failed.",
                 }},
    MessageEntry{"PropertyValueModified",
                 {
                     "Indicates that a property was given the correct value "
                     "type but the value of that property was modified.  "
                     "Examples are truncated or rounded values.",
                     "The property %1 was assigned the value %2 due to "
                     "modification by the service.",
                     "Warning",
                     2,
                     {
                         "string",
                         "string",
                     },
                     "No resolution is required.",
                 }},
    MessageEntry{
        "PropertyValueNotInList",
        {
            "Indicates that a property was given the correct value type but "
            "the value of that property was not supported.  This values not in "
            "an enumeration",
            "The value %1 for the property %2 is not in the list of acceptable "
            "values.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "Choose a value from the enumeration list that the implementation "
            "can support and resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "PropertyValueTypeError",
        {
            "Indicates that a property was given the wrong value type, such as "
            "when a number is supplied for a property that requires a string.",
            "The value %1 for the property %2 is of a different type than the "
            "property can accept.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "Correct the value for the property in the request body and "
            "resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "QueryNotSupported",
        {
            "Indicates that query is not supported on the implementation.",
            "Querying is not supported by the implementation.",
            "Warning",
            0,
            {},
            "Remove the query parameters and resubmit the request if the "
            "operation failed.",
        }},
    MessageEntry{"QueryNotSupportedOnResource",
                 {
                     "Indicates that query is not supported on the given "
                     "resource, such as when a start/count query is attempted "
                     "on a resource that is not a collection.",
                     "Querying is not supported on the requested resource.",
                     "Warning",
                     0,
                     {},
                     "Remove the query parameters and resubmit the request if "
                     "the operation failed.",
                 }},
    MessageEntry{
        "QueryParameterOutOfRange",
        {
            "Indicates that a query parameter was supplied that is out of "
            "range for the given resource.  This can happen with values that "
            "are too low or beyond that possible for the supplied resource, "
            "such as when a page is requested that is beyond the last page.",
            "The value %1 for the query parameter %2 is out of range %3.",
            "Warning",
            3,
            {
                "string",
                "string",
                "string",
            },
            "Reduce the value for the query parameter to a value that is "
            "within range, such as a start or count value that is within "
            "bounds of the number of resources in a collection or a page that "
            "is within the range of valid pages.",
        }},
    MessageEntry{"QueryParameterValueFormatError",
                 {
                     "Indicates that a query parameter was given the correct "
                     "value type but the value of that parameter was not "
                     "supported.  This includes value size/length exceeded.",
                     "The value %1 for the parameter %2 is of a different "
                     "format than the parameter can accept.",
                     "Warning",
                     2,
                     {
                         "string",
                         "string",
                     },
                     "Correct the value for the query parameter in the request "
                     "and resubmit the request if the operation failed.",
                 }},
    MessageEntry{"QueryParameterValueTypeError",
                 {
                     "Indicates that a query parameter was given the wrong "
                     "value type, such as when a number is supplied for a "
                     "query parameter that requires a string.",
                     "The value %1 for the query parameter %2 is of a "
                     "different type than the parameter can accept.",
                     "Warning",
                     2,
                     {
                         "string",
                         "string",
                     },
                     "Correct the value for the query parameter in the request "
                     "and resubmit the request if the operation failed.",
                 }},
    MessageEntry{"ResourceAlreadyExists",
                 {
                     "Indicates that a resource change or creation was "
                     "attempted but that the operation cannot proceed because "
                     "the resource already exists.",
                     "The requested resource of type %1 with the property %2 "
                     "with the value %3 already exists.",
                     "Critical",
                     3,
                     {
                         "string",
                         "string",
                         "string",
                     },
                     "Do not repeat the create operation as the resource has "
                     "already been created.",
                 }},
    MessageEntry{
        "ResourceAtUriInUnknownFormat",
        {
            "Indicates that the URI was valid but the resource or image at "
            "that URI was in a format not supported by the service.",
            "The resource at %1 is in a format not recognized by the service.",
            "Critical",
            1,
            {
                "string",
            },
            "Place an image or resource or file that is recognized by the "
            "service at the URI.",
        }},
    MessageEntry{"ResourceAtUriUnauthorized",
                 {
                     "Indicates that the attempt to access the "
                     "resource/file/image at the URI was unauthorized.",
                     "While accessing the resource at %1, the service received "
                     "an authorization error %2.",
                     "Critical",
                     2,
                     {
                         "string",
                         "string",
                     },
                     "Ensure that the appropriate access is provided for the "
                     "service in order for it to access the URI.",
                 }},
    MessageEntry{"ResourceCannotBeDeleted",
                 {
                     "Indicates that a delete operation was attempted on a "
                     "resource that cannot be deleted.",
                     "The delete request failed because the resource requested "
                     "cannot be deleted.",
                     "Critical",
                     0,
                     {},
                     "Do not attempt to delete a non-deletable resource.",
                 }},
    MessageEntry{
        "ResourceExhaustion",
        {
            "Indicates that a resource could not satisfy the request due to "
            "some unavailability of resources.  An example is that available "
            "capacity has been allocated.",
            "The resource %1 was unable to satisfy the request due to "
            "unavailability of resources.",
            "Critical",
            1,
            {
                "string",
            },
            "Ensure that the resources are available and resubmit the request.",
        }},
    MessageEntry{"ResourceInStandby",
                 {
                     "Indicates that the request could not be performed "
                     "because the resource is in standby.",
                     "The request could not be performed because the resource "
                     "is in standby.",
                     "Critical",
                     0,
                     {},
                     "Ensure that the resource is in the correct power state "
                     "and resubmit the request.",
                 }},
    MessageEntry{"ResourceInUse",
                 {
                     "Indicates that a change was requested to a resource but "
                     "the change was rejected due to the resource being in use "
                     "or transition.",
                     "The change to the requested resource failed because the "
                     "resource is in use or in transition.",
                     "Warning",
                     0,
                     {},
                     "Remove the condition and resubmit the request if the "
                     "operation failed.",
                 }},
    MessageEntry{
        "ResourceMissingAtURI",
        {
            "Indicates that the operation expected an image or other resource "
            "at the provided URI but none was found.  Examples of this are in "
            "requests that require URIs like Firmware Update.",
            "The resource at the URI %1 was not found.",
            "Critical",
            1,
            {
                "string",
            },
            "Place a valid resource at the URI or correct the URI and resubmit "
            "the request.",
        }},
    MessageEntry{
        "ResourceNotFound",
        {
            "Indicates that the operation expected a resource identifier that "
            "corresponds to an existing resource but one was not found.",
            "The requested resource of type %1 named %2 was not found.",
            "Critical",
            2,
            {
                "string",
                "string",
            },
            "Provide a valid resource identifier and resubmit the request.",
        }},
    MessageEntry{
        "ResourceTypeIncompatible",
        {
            "Indicates that the resource type of the operation does not match "
            "that for the operation destination.  Examples of when this can "
            "happen include during a POST to a collection using the wrong "
            "resource type, an update where the @odata.types do not match or "
            "on a major version incompatability.",
            "The @odata.type of the request body %1 is incompatible with the "
            "@odata.type of the resource which is %2.",
            "Critical",
            2,
            {
                "string",
                "string",
            },
            "Resubmit the request with a payload compatible with the "
            "resource's schema.",
        }},
    MessageEntry{
        "ServiceInUnknownState",
        {
            "Indicates that the operation failed because the service is in an "
            "unknown state and cannot accept additional requests.",
            "The operation failed because the service is in an unknown state "
            "and can no longer take incoming requests.",
            "Critical",
            0,
            {},
            "Restart the service and resubmit the request if the operation "
            "failed.",
        }},
    MessageEntry{"ServiceShuttingDown",
                 {
                     "Indicates that the operation failed as the service is "
                     "shutting down, such as when the service reboots.",
                     "The operation failed because the service is shutting "
                     "down and can no longer take incoming requests.",
                     "Critical",
                     0,
                     {},
                     "When the service becomes available, resubmit the request "
                     "if the operation failed.",
                 }},
    MessageEntry{
        "ServiceTemporarilyUnavailable",
        {
            "Indicates the service is temporarily unavailable.",
            "The service is temporarily unavailable.  Retry in %1 seconds.",
            "Critical",
            1,
            {
                "string",
            },
            "Wait for the indicated retry duration and retry the operation.",
        }},
    MessageEntry{
        "SessionLimitExceeded",
        {
            "Indicates that a session establishment has been requested but the "
            "operation failed due to the number of simultaneous sessions "
            "exceeding the limit of the implementation.",
            "The session establishment failed due to the number of "
            "simultaneous sessions exceeding the limit of the implementation.",
            "Critical",
            0,
            {},
            "Reduce the number of other sessions before trying to establish "
            "the session or increase the limit of simultaneous sessions (if "
            "supported).",
        }},
    MessageEntry{
        "SessionTerminated",
        {
            "Indicates that the DELETE operation on the Session resource "
            "resulted in the successful termination of the session.",
            "The session was successfully terminated.",
            "OK",
            0,
            {},
            "No resolution is required.",
        }},
    MessageEntry{
        "SourceDoesNotSupportProtocol",
        {
            "Indicates that while attempting to access, connect to or transfer "
            "a resource/file/image from another location that the other end of "
            "the connection did not support the protocol",
            "The other end of the connection at %1 does not support the "
            "specified protocol %2.",
            "Critical",
            2,
            {
                "string",
                "string",
            },
            "Change protocols or URIs. ",
        }},
    MessageEntry{"StringValueTooLong",
                 {
                     "Indicates that a string value passed to the given "
                     "resource exceeded its length limit. An example is when a "
                     "shorter limit is imposed by an implementation than that "
                     "allowed by the specification.",
                     "The string %1 exceeds the length limit %2.",
                     "Warning",
                     2,
                     {
                         "string",
                         "number",
                     },
                     "Resubmit the request with an appropriate string length.",
                 }},
    MessageEntry{"Success",
                 {
                     "Indicates that all conditions of a successful operation "
                     "have been met.",
                     "Successfully Completed Request",
                     "OK",
                     0,
                     {},
                     "None",
                 }},
    MessageEntry{
        "UnrecognizedRequestBody",
        {
            "Indicates that the service encountered an unrecognizable request "
            "body that could not even be interpreted as malformed JSON.",
            "The service detected a malformed request body that it was unable "
            "to interpret.",
            "Warning",
            0,
            {},
            "Correct the request body and resubmit the request if it failed.",
        }},
};
} // namespace redfish::message_registries::base
