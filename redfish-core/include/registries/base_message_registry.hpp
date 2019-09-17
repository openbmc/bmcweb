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
    .copyright = "Copyright 2014-2019 DMTF. All rights reserved.",
    .type = "#MessageRegistry.v1_0_0.MessageRegistry",
    .id = "Base.1.5.0",
    .name = "Base Message Registry",
    .language = "en",
    .description = "This registry defines the base messages for Redfish",
    .registryPrefix = "Base",
    .registryVersion = "1.5.0",
    .owningEntity = "DMTF",
};
const std::array registry = {
    MessageEntry{
        "AccessDenied",
        {
            .description =
                "Indicates that while attempting to access, connect to or "
                "transfer to/from another resource, the service denied access.",
            .message = "While attempting to establish a connection to %1, the "
                       "service denied access.",
            .severity = "Critical",
            .numberOfArgs = 1,
            .paramTypes =
                {
                    "string",
                },
            .resolution = "Attempt to ensure that the URI is correct and that "
                          "the service has the appropriate credentials.",
        }},
    MessageEntry{
        "AccountForSessionNoLongerExists",
        {
            .description =
                "Indicates that the account for the session has been removed, "
                "thus the session has been removed as well.",
            .message = "The account for the current session has been removed, "
                       "thus the current session has been removed as well.",
            .severity = "OK",
            .numberOfArgs = 0,
            .paramTypes = {},
            .resolution = "Attempt to connect with a valid account.",
        }},
    MessageEntry{
        "AccountModified",
        {
            .description =
                "Indicates that the account was successfully modified.",
            .message = "The account was successfully modified.",
            .severity = "OK",
            .numberOfArgs = 0,
            .paramTypes = {},
            .resolution = "No resolution is required.",
        }},
    MessageEntry{
        "AccountNotModified",
        {
            .description = "Indicates that the modification requested for the "
                           "account was not successful.",
            .message = "The account modification request failed.",
            .severity = "Warning",
            .numberOfArgs = 0,
            .paramTypes = {},
            .resolution = "The modification may have failed due to permission "
                          "issues or issues with the request body.",
        }},
    MessageEntry{"AccountRemoved",
                 {
                     .description =
                         "Indicates that the account was successfully removed.",
                     .message = "The account was successfully removed.",
                     .severity = "OK",
                     .numberOfArgs = 0,
                     .paramTypes = {},
                     .resolution = "No resolution is required.",
                 }},
    MessageEntry{
        "ActionNotSupported",
        {
            .description = "Indicates that the action supplied with the POST "
                           "operation is not supported by the resource.",
            .message = "The action %1 is not supported by the resource.",
            .severity = "Critical",
            .numberOfArgs = 1,
            .paramTypes =
                {
                    "string",
                },
            .resolution = "The action supplied cannot be resubmitted to the "
                          "implementation.  Perhaps the action was invalid, "
                          "the wrong resource was the target or the "
                          "implementation documentation may be of assistance.",
        }},
    MessageEntry{
        "ActionParameterDuplicate",
        {
            .description = "Indicates that the action was supplied with a "
                           "duplicated parameter in the request body.",
            .message = "The action %1 was submitted with more than one value "
                       "for the parameter %2.",
            .severity = "Warning",
            .numberOfArgs = 2,
            .paramTypes =
                {
                    "string",
                    "string",
                },
            .resolution =
                "Resubmit the action with only one instance of the parameter "
                "in the request body if the operation failed.",
        }},
    MessageEntry{
        "ActionParameterMissing",
        {
            .description = "Indicates that the action requested was missing a "
                           "parameter that is required to process the action.",
            .message = "The action %1 requires the parameter %2 to be present "
                       "in the request body.",
            .severity = "Critical",
            .numberOfArgs = 2,
            .paramTypes =
                {
                    "string",
                    "string",
                },
            .resolution = "Supply the action with the required parameter in "
                          "the request body when the request is resubmitted.",
        }},
    MessageEntry{
        "ActionParameterNotSupported",
        {
            .description = "Indicates that the parameter supplied for the "
                           "action is not supported on the resource.",
            .message = "The parameter %1 for the action %2 is not supported on "
                       "the target resource.",
            .severity = "Warning",
            .numberOfArgs = 2,
            .paramTypes =
                {
                    "string",
                    "string",
                },
            .resolution = "Remove the parameter supplied and resubmit the "
                          "request if the operation failed.",
        }},
    MessageEntry{
        "ActionParameterUnknown",
        {
            .description =
                "Indicates that an action was submitted but a parameter "
                "supplied did not match any of the known parameters.",
            .message =
                "The action %1 was submitted with the invalid parameter %2.",
            .severity = "Warning",
            .numberOfArgs = 2,
            .paramTypes =
                {
                    "string",
                    "string",
                },
            .resolution = "Correct the invalid parameter and resubmit the "
                          "request if the operation failed.",
        }},
    MessageEntry{
        "ActionParameterValueFormatError",
        {
            .description =
                "Indicates that a parameter was given the correct value type "
                "but the value of that parameter was not supported.  This "
                "includes value size/length exceeded.",
            .message = "The value %1 for the parameter %2 in the action %3 is "
                       "of a different format than the parameter can accept.",
            .severity = "Warning",
            .numberOfArgs = 3,
            .paramTypes =
                {
                    "string",
                    "string",
                    "string",
                },
            .resolution =
                "Correct the value for the parameter in the request body and "
                "resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "ActionParameterValueTypeError",
        {
            .description = "Indicates that a parameter was given the wrong "
                           "value type, such as when a number is supplied for "
                           "a parameter that requires a string.",
            .message = "The value %1 for the parameter %2 in the action %3 is "
                       "of a different type than the parameter can accept.",
            .severity = "Warning",
            .numberOfArgs = 3,
            .paramTypes =
                {
                    "string",
                    "string",
                    "string",
                },
            .resolution =
                "Correct the value for the parameter in the request body and "
                "resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "CouldNotEstablishConnection",
        {
            .description = "Indicates that the attempt to access the "
                           "resource/file/image at the URI was unsuccessful "
                           "because a session could not be established.",
            .message =
                "The service failed to establish a connection with the URI %1.",
            .severity = "Critical",
            .numberOfArgs = 1,
            .paramTypes =
                {
                    "string",
                },
            .resolution =
                "Ensure that the URI contains a valid and reachable node name, "
                "protocol information and other URI components.",
        }},
    MessageEntry{
        "CreateFailedMissingReqProperties",
        {
            .description =
                "Indicates that a create was attempted on a resource but that "
                "properties that are required for the create operation were "
                "missing from the request.",
            .message = "The create operation failed because the required "
                       "property %1 was missing from the request.",
            .severity = "Critical",
            .numberOfArgs = 1,
            .paramTypes =
                {
                    "string",
                },
            .resolution =
                "Correct the body to include the required property with a "
                "valid value and resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "CreateLimitReachedForResource",
        {
            .description = "Indicates that no more resources can be created on "
                           "the resource as it has reached its create limit.",
            .message = "The create operation failed because the resource has "
                       "reached the limit of possible resources.",
            .severity = "Critical",
            .numberOfArgs = 0,
            .paramTypes = {},
            .resolution =
                "Either delete resources and resubmit the request if the "
                "operation failed or do not resubmit the request.",
        }},
    MessageEntry{
        "Created",
        {
            .description = "Indicates that all conditions of a successful "
                           "creation operation have been met.",
            .message = "The resource has been created successfully",
            .severity = "OK",
            .numberOfArgs = 0,
            .paramTypes = {},
            .resolution = "None",
        }},
    MessageEntry{
        "EmptyJSON",
        {
            .description =
                "Indicates that the request body contained an empty JSON "
                "object when one or more properties are expected in the body.",
            .message = "The request body submitted contained an empty JSON "
                       "object and the service is unable to process it.",
            .severity = "Warning",
            .numberOfArgs = 0,
            .paramTypes = {},
            .resolution =
                "Add properties in the JSON object and resubmit the request.",
        }},
    MessageEntry{
        "EventSubscriptionLimitExceeded",
        {
            .description = "Indicates that a event subscription establishment "
                           "has been requested but the operation failed due to "
                           "the number of simultaneous connection exceeding "
                           "the limit of the implementation.",
            .message = "The event subscription failed due to the number of "
                       "simultaneous subscriptions exceeding the limit of the "
                       "implementation.",
            .severity = "Critical",
            .numberOfArgs = 0,
            .paramTypes = {},
            .resolution =
                "Reduce the number of other subscriptions before trying to "
                "establish the event subscription or increase the limit of "
                "simultaneous subscriptions (if supported).",
        }},
    MessageEntry{
        "GeneralError",
        {
            .description =
                "Indicates that a general error has occurred.  Use in "
                "ExtendedInfo is discouraged.  When used in ExtendedInfo, "
                "implementations are expected to include a Resolution property "
                "with this error to indicate how to resolve the problem.",
            .message = "A general error has occurred. See Resolution for "
                       "information on how to resolve the error.",
            .severity = "Critical",
            .numberOfArgs = 0,
            .paramTypes = {},
            .resolution = "None.",
        }},
    MessageEntry{
        "InsufficientPrivilege",
        {
            .description = "Indicates that the credentials associated with the "
                           "established session do not have sufficient "
                           "privileges for the requested operation",
            .message = "There are insufficient privileges for the account or "
                       "credentials associated with the current session to "
                       "perform the requested operation.",
            .severity = "Critical",
            .numberOfArgs = 0,
            .paramTypes = {},
            .resolution =
                "Either abandon the operation or change the associated access "
                "rights and resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "InternalError",
        {
            .description =
                "Indicates that the request failed for an unknown internal "
                "error but that the service is still operational.",
            .message = "The request failed due to an internal service error.  "
                       "The service is still operational.",
            .severity = "Critical",
            .numberOfArgs = 0,
            .paramTypes = {},
            .resolution = "Resubmit the request.  If the problem persists, "
                          "consider resetting the service.",
        }},
    MessageEntry{
        "InvalidIndex",
        {
            .description = "The Index is not valid.",
            .message = "The Index %1 is not a valid offset into the array.",
            .severity = "Warning",
            .numberOfArgs = 1,
            .paramTypes =
                {
                    "number",
                },
            .resolution = "Verify the index value provided is within the "
                          "bounds of the array.",
        }},
    MessageEntry{
        "InvalidObject",
        {
            .description = "Indicates that the object in question is invalid "
                           "according to the implementation.  Examples include "
                           "a firmware update malformed URI.",
            .message = "The object at %1 is invalid.",
            .severity = "Critical",
            .numberOfArgs = 1,
            .paramTypes =
                {
                    "string",
                },
            .resolution =
                "Either the object is malformed or the URI is not correct.  "
                "Correct the condition and resubmit the request if it failed.",
        }},
    MessageEntry{
        "MalformedJSON",
        {
            .description = "Indicates that the request body was malformed "
                           "JSON.  Could be duplicate, syntax error,etc.",
            .message = "The request body submitted was malformed JSON and "
                       "could not be parsed by the receiving service.",
            .severity = "Critical",
            .numberOfArgs = 0,
            .paramTypes = {},
            .resolution = "Ensure that the request body is valid JSON and "
                          "resubmit the request.",
        }},
    MessageEntry{
        "NoOperation",
        {
            .description = "Indicates that the requested operation will not "
                           "perform any changes on the service.",
            .message = "The request body submitted contain no data to act upon "
                       "and no changes to the resource took place.",
            .severity = "Warning",
            .numberOfArgs = 0,
            .paramTypes = {},
            .resolution =
                "Add properties in the JSON object and resubmit the request.",
        }},
    MessageEntry{
        "NoValidSession",
        {
            .description =
                "Indicates that the operation failed because a valid session "
                "is required in order to access any resources.",
            .message = "There is no valid session established with the "
                       "implementation.",
            .severity = "Critical",
            .numberOfArgs = 0,
            .paramTypes = {},
            .resolution =
                "Establish as session before attempting any operations.",
        }},
    MessageEntry{
        "PasswordChangeRequired",
        {
            .description =
                "Indicates that the password for the account provided must be "
                "changed before accessing the service.  The password can be "
                "changed with a PATCH to the 'Password' property in the "
                "ManagerAccount resource instance.  Implementations that "
                "provide a default password for an account may require a "
                "password change prior to first access to the service.",
            .message = "The password provided for this account must be changed "
                       "before access is granted.  PATCH the 'Password' "
                       "property for this account located at the target URI "
                       "'%1' to complete this process.",
            .severity = "Critical",
            .numberOfArgs = 1,
            .paramTypes =
                {
                    "string",
                },
            .resolution = "Change the password for this account using a PATCH "
                          "to the 'Password' property at the URI provided.",
        }},
    MessageEntry{
        "PropertyDuplicate",
        {
            .description = "Indicates that a duplicate property was included "
                           "in the request body.",
            .message = "The property %1 was duplicated in the request.",
            .severity = "Warning",
            .numberOfArgs = 1,
            .paramTypes =
                {
                    "string",
                },
            .resolution = "Remove the duplicate property from the request body "
                          "and resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "PropertyMissing",
        {
            .description = "Indicates that a required property was not "
                           "supplied as part of the request.",
            .message = "The property %1 is a required property and must be "
                       "included in the request.",
            .severity = "Warning",
            .numberOfArgs = 1,
            .paramTypes =
                {
                    "string",
                },
            .resolution =
                "Ensure that the property is in the request body and has a "
                "valid value and resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "PropertyNotWritable",
        {
            .description =
                "Indicates that a property was given a value in the request "
                "body, but the property is a readonly property.",
            .message = "The property %1 is a read only property and cannot be "
                       "assigned a value.",
            .severity = "Warning",
            .numberOfArgs = 1,
            .paramTypes =
                {
                    "string",
                },
            .resolution = "Remove the property from the request body and "
                          "resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "PropertyUnknown",
        {
            .description = "Indicates that an unknown property was included in "
                           "the request body.",
            .message = "The property %1 is not in the list of valid properties "
                       "for the resource.",
            .severity = "Warning",
            .numberOfArgs = 1,
            .paramTypes =
                {
                    "string",
                },
            .resolution = "Remove the unknown property from the request body "
                          "and resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "PropertyValueFormatError",
        {
            .description =
                "Indicates that a property was given the correct value type "
                "but the value of that property was not supported.",
            .message = "The value %1 for the property %2 is of a different "
                       "format than the property can accept.",
            .severity = "Warning",
            .numberOfArgs = 2,
            .paramTypes =
                {
                    "string",
                    "string",
                },
            .resolution =
                "Correct the value for the property in the request body and "
                "resubmit the request if the operation failed.",
        }},
    MessageEntry{"PropertyValueModified",
                 {
                     .description =
                         "Indicates that a property was given the correct "
                         "value type but the value of that property was "
                         "modified.  Examples are truncated or rounded values.",
                     .message = "The property %1 was assigned the value %2 due "
                                "to modification by the service.",
                     .severity = "Warning",
                     .numberOfArgs = 2,
                     .paramTypes =
                         {
                             "string",
                             "string",
                         },
                     .resolution = "No resolution is required.",
                 }},
    MessageEntry{
        "PropertyValueNotInList",
        {
            .description = "Indicates that a property was given the correct "
                           "value type but the value of that property was not "
                           "supported.  This values not in an enumeration",
            .message = "The value %1 for the property %2 is not in the list of "
                       "acceptable values.",
            .severity = "Warning",
            .numberOfArgs = 2,
            .paramTypes =
                {
                    "string",
                    "string",
                },
            .resolution = "Choose a value from the enumeration list that the "
                          "implementation can support and resubmit the request "
                          "if the operation failed.",
        }},
    MessageEntry{
        "PropertyValueTypeError",
        {
            .description = "Indicates that a property was given the wrong "
                           "value type, such as when a number is supplied for "
                           "a property that requires a string.",
            .message = "The value %1 for the property %2 is of a different "
                       "type than the property can accept.",
            .severity = "Warning",
            .numberOfArgs = 2,
            .paramTypes =
                {
                    "string",
                    "string",
                },
            .resolution =
                "Correct the value for the property in the request body and "
                "resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "QueryNotSupported",
        {
            .description =
                "Indicates that query is not supported on the implementation.",
            .message = "Querying is not supported by the implementation.",
            .severity = "Warning",
            .numberOfArgs = 0,
            .paramTypes = {},
            .resolution = "Remove the query parameters and resubmit the "
                          "request if the operation failed.",
        }},
    MessageEntry{
        "QueryNotSupportedOnResource",
        {
            .description = "Indicates that query is not supported on the given "
                           "resource, such as when a start/count query is "
                           "attempted on a resource that is not a collection.",
            .message = "Querying is not supported on the requested resource.",
            .severity = "Warning",
            .numberOfArgs = 0,
            .paramTypes = {},
            .resolution = "Remove the query parameters and resubmit the "
                          "request if the operation failed.",
        }},
    MessageEntry{
        "QueryParameterOutOfRange",
        {
            .description = "Indicates that a query parameter was supplied that "
                           "is out of range for the given resource.  This can "
                           "happen with values that are too low or beyond that "
                           "possible for the supplied resource, such as when a "
                           "page is requested that is beyond the last page.",
            .message =
                "The value %1 for the query parameter %2 is out of range %3.",
            .severity = "Warning",
            .numberOfArgs = 3,
            .paramTypes =
                {
                    "string",
                    "string",
                    "string",
                },
            .resolution =
                "Reduce the value for the query parameter to a value that is "
                "within range, such as a start or count value that is within "
                "bounds of the number of resources in a collection or a page "
                "that is within the range of valid pages.",
        }},
    MessageEntry{
        "QueryParameterValueFormatError",
        {
            .description =
                "Indicates that a query parameter was given the correct value "
                "type but the value of that parameter was not supported.  This "
                "includes value size/length exceeded.",
            .message = "The value %1 for the parameter %2 is of a different "
                       "format than the parameter can accept.",
            .severity = "Warning",
            .numberOfArgs = 2,
            .paramTypes =
                {
                    "string",
                    "string",
                },
            .resolution =
                "Correct the value for the query parameter in the request and "
                "resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "QueryParameterValueTypeError",
        {
            .description =
                "Indicates that a query parameter was given the wrong value "
                "type, such as when a number is supplied for a query parameter "
                "that requires a string.",
            .message = "The value %1 for the query parameter %2 is of a "
                       "different type than the parameter can accept.",
            .severity = "Warning",
            .numberOfArgs = 2,
            .paramTypes =
                {
                    "string",
                    "string",
                },
            .resolution =
                "Correct the value for the query parameter in the request and "
                "resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "ResourceAlreadyExists",
        {
            .description = "Indicates that a resource change or creation was "
                           "attempted but that the operation cannot proceed "
                           "because the resource already exists.",
            .message = "The requested resource of type %1 with the property %2 "
                       "with the value %3 already exists.",
            .severity = "Critical",
            .numberOfArgs = 3,
            .paramTypes =
                {
                    "string",
                    "string",
                    "string",
                },
            .resolution = "Do not repeat the create operation as the resource "
                          "has already been created.",
        }},
    MessageEntry{
        "ResourceAtUriInUnknownFormat",
        {
            .description =
                "Indicates that the URI was valid but the resource or image at "
                "that URI was in a format not supported by the service.",
            .message = "The resource at %1 is in a format not recognized by "
                       "the service.",
            .severity = "Critical",
            .numberOfArgs = 1,
            .paramTypes =
                {
                    "string",
                },
            .resolution = "Place an image or resource or file that is "
                          "recognized by the service at the URI.",
        }},
    MessageEntry{
        "ResourceAtUriUnauthorized",
        {
            .description = "Indicates that the attempt to access the "
                           "resource/file/image at the URI was unauthorized.",
            .message = "While accessing the resource at %1, the service "
                       "received an authorization error %2.",
            .severity = "Critical",
            .numberOfArgs = 2,
            .paramTypes =
                {
                    "string",
                    "string",
                },
            .resolution = "Ensure that the appropriate access is provided for "
                          "the service in order for it to access the URI.",
        }},
    MessageEntry{
        "ResourceCannotBeDeleted",
        {
            .description = "Indicates that a delete operation was attempted on "
                           "a resource that cannot be deleted.",
            .message = "The delete request failed because the resource "
                       "requested cannot be deleted.",
            .severity = "Critical",
            .numberOfArgs = 0,
            .paramTypes = {},
            .resolution = "Do not attempt to delete a non-deletable resource.",
        }},
    MessageEntry{
        "ResourceExhaustion",
        {
            .description =
                "Indicates that a resource could not satisfy the request due "
                "to some unavailability of resources.  An example is that "
                "available capacity has been allocated.",
            .message = "The resource %1 was unable to satisfy the request due "
                       "to unavailability of resources.",
            .severity = "Critical",
            .numberOfArgs = 1,
            .paramTypes =
                {
                    "string",
                },
            .resolution = "Ensure that the resources are available and "
                          "resubmit the request.",
        }},
    MessageEntry{
        "ResourceInStandby",
        {
            .description = "Indicates that the request could not be performed "
                           "because the resource is in standby.",
            .message = "The request could not be performed because the "
                       "resource is in standby.",
            .severity = "Critical",
            .numberOfArgs = 0,
            .paramTypes = {},
            .resolution = "Ensure that the resource is in the correct power "
                          "state and resubmit the request.",
        }},
    MessageEntry{
        "ResourceInUse",
        {
            .description = "Indicates that a change was requested to a "
                           "resource but the change was rejected due to the "
                           "resource being in use or transition.",
            .message = "The change to the requested resource failed because "
                       "the resource is in use or in transition.",
            .severity = "Warning",
            .numberOfArgs = 0,
            .paramTypes = {},
            .resolution = "Remove the condition and resubmit the request if "
                          "the operation failed.",
        }},
    MessageEntry{
        "ResourceMissingAtURI",
        {
            .description =
                "Indicates that the operation expected an image or other "
                "resource at the provided URI but none was found.  Examples of "
                "this are in requests that require URIs like Firmware Update.",
            .message = "The resource at the URI %1 was not found.",
            .severity = "Critical",
            .numberOfArgs = 1,
            .paramTypes =
                {
                    "string",
                },
            .resolution = "Place a valid resource at the URI or correct the "
                          "URI and resubmit the request.",
        }},
    MessageEntry{
        "ResourceNotFound",
        {
            .description = "Indicates that the operation expected a resource "
                           "identifier that corresponds to an existing "
                           "resource but one was not found.",
            .message =
                "The requested resource of type %1 named %2 was not found.",
            .severity = "Critical",
            .numberOfArgs = 2,
            .paramTypes =
                {
                    "string",
                    "string",
                },
            .resolution =
                "Provide a valid resource identifier and resubmit the request.",
        }},
    MessageEntry{
        "ResourceTypeIncompatible",
        {
            .description =
                "Indicates that the resource type of the operation does not "
                "match that for the operation destination.  Examples of when "
                "this can happen include during a POST to a collection using "
                "the wrong resource type, an update where the @odata.types do "
                "not match or on a major version incompatability.",
            .message = "The @odata.type of the request body %1 is incompatible "
                       "with the @odata.type of the resource which is %2.",
            .severity = "Critical",
            .numberOfArgs = 2,
            .paramTypes =
                {
                    "string",
                    "string",
                },
            .resolution = "Resubmit the request with a payload compatible with "
                          "the resource's schema.",
        }},
    MessageEntry{
        "ServiceInUnknownState",
        {
            .description =
                "Indicates that the operation failed because the service is in "
                "an unknown state and cannot accept additional requests.",
            .message =
                "The operation failed because the service is in an unknown "
                "state and can no longer take incoming requests.",
            .severity = "Critical",
            .numberOfArgs = 0,
            .paramTypes = {},
            .resolution = "Restart the service and resubmit the request if the "
                          "operation failed.",
        }},
    MessageEntry{
        "ServiceShuttingDown",
        {
            .description =
                "Indicates that the operation failed as the service is "
                "shutting down, such as when the service reboots.",
            .message = "The operation failed because the service is shutting "
                       "down and can no longer take incoming requests.",
            .severity = "Critical",
            .numberOfArgs = 0,
            .paramTypes = {},
            .resolution = "When the service becomes available, resubmit the "
                          "request if the operation failed.",
        }},
    MessageEntry{
        "ServiceTemporarilyUnavailable",
        {
            .description = "Indicates the service is temporarily unavailable.",
            .message =
                "The service is temporarily unavailable.  Retry in %1 seconds.",
            .severity = "Critical",
            .numberOfArgs = 1,
            .paramTypes =
                {
                    "string",
                },
            .resolution = "Wait for the indicated retry duration and retry the "
                          "operation.",
        }},
    MessageEntry{
        "SessionLimitExceeded",
        {
            .description =
                "Indicates that a session establishment has been requested but "
                "the operation failed due to the number of simultaneous "
                "sessions exceeding the limit of the implementation.",
            .message = "The session establishment failed due to the number of "
                       "simultaneous sessions exceeding the limit of the "
                       "implementation.",
            .severity = "Critical",
            .numberOfArgs = 0,
            .paramTypes = {},
            .resolution = "Reduce the number of other sessions before trying "
                          "to establish the session or increase the limit of "
                          "simultaneous sessions (if supported).",
        }},
    MessageEntry{
        "SessionTerminated",
        {
            .description =
                "Indicates that the DELETE operation on the Session resource "
                "resulted in the successful termination of the session.",
            .message = "The session was successfully terminated.",
            .severity = "OK",
            .numberOfArgs = 0,
            .paramTypes = {},
            .resolution = "No resolution is required.",
        }},
    MessageEntry{
        "SourceDoesNotSupportProtocol",
        {
            .description =
                "Indicates that while attempting to access, connect to or "
                "transfer a resource/file/image from another location that the "
                "other end of the connection did not support the protocol",
            .message = "The other end of the connection at %1 does not support "
                       "the specified protocol %2.",
            .severity = "Critical",
            .numberOfArgs = 2,
            .paramTypes =
                {
                    "string",
                    "string",
                },
            .resolution = "Change protocols or URIs. ",
        }},
    MessageEntry{
        "StringValueTooLong",
        {
            .description =
                "Indicates that a string value passed to the given resource "
                "exceeded its length limit. An example is when a shorter limit "
                "is imposed by an implementation than that allowed by the "
                "specification.",
            .message = "The string %1 exceeds the length limit %2.",
            .severity = "Warning",
            .numberOfArgs = 2,
            .paramTypes =
                {
                    "string",
                    "number",
                },
            .resolution =
                "Resubmit the request with an appropriate string length.",
        }},
    MessageEntry{
        "SubscriptionTerminated",
        {
            .description = "An event subscription has been terminated by the "
                           "Service. No further events will be delivered.",
            .message = "The event subscription has been terminated.",
            .severity = "OK",
            .numberOfArgs = 0,
            .paramTypes = {},
            .resolution = "No resolution is required.",
        }},
    MessageEntry{"Success",
                 {
                     .description = "Indicates that all conditions of a "
                                    "successful operation have been met.",
                     .message = "Successfully Completed Request",
                     .severity = "OK",
                     .numberOfArgs = 0,
                     .paramTypes = {},
                     .resolution = "None",
                 }},
    MessageEntry{
        "UnrecognizedRequestBody",
        {
            .description = "Indicates that the service encountered an "
                           "unrecognizable request body that could not even be "
                           "interpreted as malformed JSON.",
            .message = "The service detected a malformed request body that it "
                       "was unable to interpret.",
            .severity = "Warning",
            .numberOfArgs = 0,
            .paramTypes = {},
            .resolution = "Correct the request body and resubmit the request "
                          "if it failed.",
        }},
};
} // namespace redfish::message_registries::base
