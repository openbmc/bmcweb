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

namespace redfish::registries::base
{
const Header header = {
    "Copyright 2014-2022 DMTF. All rights reserved.",
    "#MessageRegistry.v1_5_0.MessageRegistry",
    "Base.1.13.0",
    "Base Message Registry",
    "en",
    "This registry defines the base messages for Redfish",
    "Base",
    "1.13.0",
    "DMTF",
};
constexpr std::string_view url =
    "https://redfish.dmtf.org/registries/Base.1.13.0.json";

constexpr std::array registry =
{
    MessageEntry{
        "AccessDenied",
        {
            "Indicates that while attempting to access, connect to, or transfer to or from another resource, the service denied access.",
            "While attempting to establish a connection to '%1', the service denied access.",
            "Critical",
            1,
            {
                "string",
            },
            "Attempt to ensure that the URI is correct and that the service has the appropriate credentials.",
        }},
    MessageEntry{
        "AccountForSessionNoLongerExists",
        {
            "Indicates that the account for the session has been removed, thus the session has been removed as well.",
            "The account for the current session has been removed, thus the current session has been removed as well.",
            "OK",
            0,
            {},
            "Attempt to connect with a valid account.",
        }},
    MessageEntry{
        "AccountModified",
        {
            "Indicates that the account was successfully modified.",
            "The account was successfully modified.",
            "OK",
            0,
            {},
            "No resolution is required.",
        }},
    MessageEntry{
        "AccountNotModified",
        {
            "Indicates that the modification requested for the account was not successful.",
            "The account modification request failed.",
            "Warning",
            0,
            {},
            "The modification may have failed due to permission issues or issues with the request body.",
        }},
    MessageEntry{
        "AccountRemoved",
        {
            "Indicates that the account was successfully removed.",
            "The account was successfully removed.",
            "OK",
            0,
            {},
            "No resolution is required.",
        }},
    MessageEntry{
        "ActionDeprecated",
        {
            "Indicates the action is deprecated.",
            "The action %1 is deprecated.",
            "Warning",
            1,
            {
                "string",
            },
            "Refer to the schema guide for more information.",
        }},
    MessageEntry{
        "ActionNotSupported",
        {
            "Indicates that the action supplied with the POST operation is not supported by the resource.",
            "The action %1 is not supported by the resource.",
            "Critical",
            1,
            {
                "string",
            },
            "The action supplied cannot be resubmitted to the implementation.  Perhaps the action was invalid, the wrong resource was the target or the implementation documentation may be of assistance.",
        }},
    MessageEntry{
        "ActionParameterDuplicate",
        {
            "Indicates that the action was supplied with a duplicated action parameter in the request body.",
            "The action %1 was submitted with more than one value for the parameter %2.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "Resubmit the action with only one instance of the action parameter in the request body if the operation failed.",
        }},
    MessageEntry{
        "ActionParameterMissing",
        {
            "Indicates that the action requested was missing an action parameter that is required to process the action.",
            "The action %1 requires the parameter %2 to be present in the request body.",
            "Critical",
            2,
            {
                "string",
                "string",
            },
            "Supply the action with the required parameter in the request body when the request is resubmitted.",
        }},
    MessageEntry{
        "ActionParameterNotSupported",
        {
            "Indicates that the parameter supplied for the action is not supported on the resource.",
            "The parameter %1 for the action %2 is not supported on the target resource.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "Remove the parameter supplied and resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "ActionParameterUnknown",
        {
            "Indicates that an action was submitted but an action parameter supplied did not match any of the known parameters.",
            "The action %1 was submitted with the invalid parameter %2.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "Correct the invalid action parameter and resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "ActionParameterValueConflict",
        {
            "Indicates that the requested parameter value could not be completed, because of a mismatch with other parameters or properties in the resource.",
            "The parameter '%1' with the requested value of '%2' does not meet the constraints of the implementation.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "No resolution is required.",
        }},
    MessageEntry{
        "ActionParameterValueError",
        {
            "Indicates that a parameter was given an invalid value.",
            "The value for the parameter %1 in the action %2 is invalid.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "Correct the value for the parameter in the request body and resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "ActionParameterValueFormatError",
        {
            "Indicates that a parameter was given the correct value type but the value of that parameter was not supported.  This includes the value size or length has been exceeded.",
            "The value '%1' for the parameter %2 in the action %3 is of a different format than the parameter can accept.",
            "Warning",
            3,
            {
                "string",
                "string",
                "string",
            },
            "Correct the value for the parameter in the request body and resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "ActionParameterValueNotInList",
        {
            "Indicates that a parameter was given the correct value type but the value of that parameter was not supported.  The value is not in an enumeration.",
            "The value '%1' for the parameter %2 in the action %3 is not in the list of acceptable values.",
            "Warning",
            3,
            {
                "string",
                "string",
                "string",
            },
            "Choose a value from the enumeration list that the implementation can support and resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "ActionParameterValueTypeError",
        {
            "Indicates that a parameter was given the wrong value type, such as when a number is supplied for a parameter that requires a string.",
            "The value '%1' for the parameter %2 in the action %3 is of a different type than the parameter can accept.",
            "Warning",
            3,
            {
                "string",
                "string",
                "string",
            },
            "Correct the value for the parameter in the request body and resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "ChassisPowerStateOffRequired",
        {
            "Indicates that the request requires a specified chassis to be powered off.",
            "The Chassis with Id '%1' requires to be powered off to perform this request.",
            "Warning",
            1,
            {
                "string",
            },
            "Power off the specified chassis and resubmit the request.",
        }},
    MessageEntry{
        "ChassisPowerStateOnRequired",
        {
            "Indicates that the request requires a specified chassis to be powered on.",
            "The chassis with Id '%1' requires to be powered on to perform this request.",
            "Warning",
            1,
            {
                "string",
            },
            "Power on the specified chassis and resubmit the request.",
        }},
    MessageEntry{
        "ConditionInRelatedResource",
        {
            "Indicates that one or more fault or error conditions exist in a related resource.",
            "One or more conditions exist in a related resource.  See the OriginOfCondition property.",
            "Warning",
            0,
            {},
            "Check the Conditions array in the resource shown in the OriginOfCondition property to determine the conditions that need attention.",
        }},
    MessageEntry{
        "CouldNotEstablishConnection",
        {
            "Indicates that the attempt to access the resource, file, or image at the URI was unsuccessful because a session could not be established.",
            "The service failed to establish a connection with the URI '%1'.",
            "Critical",
            1,
            {
                "string",
            },
            "Ensure that the URI contains a valid and reachable node name, protocol information and other URI components.",
        }},
    MessageEntry{
        "CreateFailedMissingReqProperties",
        {
            "Indicates that a create was attempted on a resource but that properties that are required for the create operation were missing from the request.",
            "The create operation failed because the required property %1 was missing from the request.",
            "Critical",
            1,
            {
                "string",
            },
            "Correct the body to include the required property with a valid value and resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "CreateLimitReachedForResource",
        {
            "Indicates that no more resources can be created on the resource as it has reached its create limit.",
            "The create operation failed because the resource has reached the limit of possible resources.",
            "Critical",
            0,
            {},
            "Either delete resources and resubmit the request if the operation failed or do not resubmit the request.",
        }},
    MessageEntry{
        "Created",
        {
            "Indicates that all conditions of a successful create operation have been met.",
            "The resource has been created successfully.",
            "OK",
            0,
            {},
            "None.",
        }},
    MessageEntry{
        "EmptyJSON",
        {
            "Indicates that the request body contained an empty JSON object when one or more properties are expected in the body.",
            "The request body submitted contained an empty JSON object and the service is unable to process it.",
            "Warning",
            0,
            {},
            "Add properties in the JSON object and resubmit the request.",
        }},
    MessageEntry{
        "EventBufferExceeded",
        {
            "Indicates undelivered events may have been lost due to a lack of buffer space in the service.",
            "Undelivered events may have been lost due to exceeding the event buffer.",
            "Warning",
            0,
            {},
            "None.",
        }},
    MessageEntry{
        "EventSubscriptionLimitExceeded",
        {
            "Indicates that a event subscription establishment has been requested but the operation failed due to the number of simultaneous connection exceeding the limit of the implementation.",
            "The event subscription failed due to the number of simultaneous subscriptions exceeding the limit of the implementation.",
            "Critical",
            0,
            {},
            "Reduce the number of other subscriptions before trying to establish the event subscription or increase the limit of simultaneous subscriptions, if supported.",
        }},
    MessageEntry{
        "GeneralError",
        {
            "Indicates that a general error has occurred.  Use in `@Message.ExtendedInfo` is discouraged.  When used in `@Message.ExtendedInfo`, implementations are expected to include a `Resolution` property with this message and provide a service-defined resolution to indicate how to resolve the error.",
            "A general error has occurred.  See Resolution for information on how to resolve the error, or @Message.ExtendedInfo if Resolution is not provided.",
            "Critical",
            0,
            {},
            "None.",
        }},
    MessageEntry{
        "HeaderInvalid",
        {
            "Indicates that a request header is invalid.",
            "Header '%1' is invalid.",
            "Critical",
            1,
            {
                "string",
            },
            "Resubmit the request with a valid request header.",
        }},
    MessageEntry{
        "HeaderMissing",
        {
            "Indicates that a required request header is missing.",
            "Required header '%1' is missing in the request.",
            "Critical",
            1,
            {
                "string",
            },
            "Resubmit the request with the required request header.",
        }},
    MessageEntry{
        "InsufficientPrivilege",
        {
            "Indicates that the credentials associated with the established session do not have sufficient privileges for the requested operation.",
            "There are insufficient privileges for the account or credentials associated with the current session to perform the requested operation.",
            "Critical",
            0,
            {},
            "Either abandon the operation or change the associated access rights and resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "InsufficientStorage",
        {
            "Indicates that the operation could not be completed due to a lack of storage or memory avaiable to the service.",
            "Insufficent storage or memory available to complete the request.",
            "Critical",
            0,
            {},
            "Increase the free storage space available to the service and resubmit the request.",
        }},
    MessageEntry{
        "InternalError",
        {
            "Indicates that the request failed for an unknown internal error but that the service is still operational.",
            "The request failed due to an internal service error.  The service is still operational.",
            "Critical",
            0,
            {},
            "Resubmit the request.  If the problem persists, consider resetting the service.",
        }},
    MessageEntry{
        "InvalidIndex",
        {
            "The index is not valid.",
            "The index %1 is not a valid offset into the array.",
            "Warning",
            1,
            {
                "number",
            },
            "Verify the index value provided is within the bounds of the array.",
        }},
    MessageEntry{
        "InvalidJSON",
        {
            "Indicates that the request body contains invalid JSON.",
            "The request body submitted is invalid JSON starting at line %1 and could not be parsed by the receiving service.",
            "Critical",
            1,
            {
                "number",
            },
            "Ensure that the request body is valid JSON and resubmit the request.",
        }},
    MessageEntry{
        "InvalidObject",
        {
            "Indicates that the object in question is invalid according to the implementation.  Examples include a firmware update malformed URI.",
            "The object at '%1' is invalid.",
            "Critical",
            1,
            {
                "string",
            },
            "Either the object is malformed or the URI is not correct.  Correct the condition and resubmit the request if it failed.",
        }},
    MessageEntry{
        "InvalidURI",
        {
            "Indicates that the operation encountered a URI that does not correspond to a valid resource.",
            "The URI %1 was not found.",
            "Critical",
            1,
            {
                "string",
            },
            "Provide a valid URI and resubmit the request.",
        }},
    MessageEntry{
        "MalformedJSON",
        {
            "Indicates that the request body was malformed JSON.",
            "The request body submitted was malformed JSON and could not be parsed by the receiving service.",
            "Critical",
            0,
            {},
            "Ensure that the request body is valid JSON and resubmit the request.",
        }},
    MessageEntry{
        "MaximumErrorsExceeded",
        {
            "Indicates that sufficient errors have occurred that the reporting service cannot return them all.",
            "Too many errors have occurred to report them all.",
            "Critical",
            0,
            {},
            "Resolve other reported errors and retry the current operation.",
        }},
    MessageEntry{
        "MissingOrMalformedPart",
        {
            "Indicates that a multipart request is missing a required part or contains malformed parts.",
            "The multipart request contains malformed parts or is missing required parts.",
            "Critical",
            0,
            {},
            "Add any missing required parts or correct the malformed parts and resubmit the request.",
        }},
    MessageEntry{
        "NetworkNameResolutionNotConfigured",
        {
            "Indicates that network-based name resolution has not been configured on the service.",
            "Network name resolution has not been configured on this service.",
            "Warning",
            0,
            {},
            "Configure the network name resolution protocol support on this service, or update any URI values to include an IP address instead of a network name and resubmit the request.",
        }},
    MessageEntry{
        "NetworkNameResolutionNotSupported",
        {
            "Indicates the service does not support network-based name resolution.",
            "Resolution of network-based names is not supported by this service.",
            "Warning",
            0,
            {},
            "Update any URI values to include an IP address instead of a network name and resubmit the request.",
        }},
    MessageEntry{
        "NoOperation",
        {
            "Indicates that the requested operation will not perform any changes on the service.",
            "The request body submitted contain no data to act upon and no changes to the resource took place.",
            "Warning",
            0,
            {},
            "Add properties in the JSON object and resubmit the request.",
        }},
    MessageEntry{
        "NoValidSession",
        {
            "Indicates that the operation failed because a valid session is required in order to access any resources.",
            "There is no valid session established with the implementation.",
            "Critical",
            0,
            {},
            "Establish a session before attempting any operations.",
        }},
    MessageEntry{
        "OperationFailed",
        {
            "Indicates that one of the internal operations necessary to complete the request failed.  Examples of this are when an internal service provider is unable to complete the request, such as in aggregation or RDE.",
            "An error occurred internal to the service as part of the overall request.  Partial results may have been returned.",
            "Warning",
            0,
            {},
            "Resubmit the request.  If the problem persists, consider resetting the service or provider.",
        }},
    MessageEntry{
        "OperationNotAllowed",
        {
            "Indicates that the HTTP method in the request is not allowed on this resource.",
            "The HTTP method is not allowed on this resource.",
            "Critical",
            0,
            {},
            "None.",
        }},
    MessageEntry{
        "OperationTimeout",
        {
            "Indicates that one of the internal operations necessary to complete the request timed out.  Examples of this are when an internal service provider is unable to complete the request, such as in aggregation or RDE.",
            "A timeout internal to the service occured as part of the request.  Partial results may have been returned.",
            "Warning",
            0,
            {},
            "Resubmit the request.  If the problem persists, consider resetting the service or provider.",
        }},
    MessageEntry{
        "PasswordChangeRequired",
        {
            "Indicates that the password for the account provided must be changed before accessing the service.  The password can be changed with a PATCH to the `Password` property in the manager account resource instance.  Implementations that provide a default password for an account may require a password change prior to first access to the service.",
            "The password provided for this account must be changed before access is granted.  PATCH the Password property for this account located at the target URI '%1' to complete this process.",
            "Critical",
            1,
            {
                "string",
            },
            "Change the password for this account using a PATCH to the Password property at the URI provided.",
        }},
    MessageEntry{
        "PayloadTooLarge",
        {
            "Indicates that the supplied payload is too large to be accepted by the service.",
            "The supplied payload exceeds the maximum size supported by the service.",
            "Critical",
            0,
            {},
            "Check that the supplied payload is correct and supported by this service.",
        }},
    MessageEntry{
        "PreconditionFailed",
        {
            "Indicates that the ETag supplied did not match the current ETag of the resource.",
            "The ETag supplied did not match the ETag required to change this resource.",
            "Critical",
            0,
            {},
            "Try the operation again using the appropriate ETag.",
        }},
    MessageEntry{
        "PreconditionRequired",
        {
            "Indicates that the request did not provide the required precondition such as an `If-Match` or `If-None-Match` header, or `@odata.etag` annotations.",
            "A precondition header or annotation is required to change this resource.",
            "Critical",
            0,
            {},
            "Try the operation again using an If-Match or If-None-Match header and appropriate ETag.",
        }},
    MessageEntry{
        "PropertyDeprecated",
        {
            "Indicates the property is deprecated.",
            "The deprecated property %1 was included in the request body.",
            "Warning",
            1,
            {
                "string",
            },
            "Refer to the schema guide for more information.",
        }},
    MessageEntry{
        "PropertyDuplicate",
        {
            "Indicates that a duplicate property was included in the request body.",
            "The property %1 was duplicated in the request.",
            "Warning",
            1,
            {
                "string",
            },
            "Remove the duplicate property from the request body and resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "PropertyMissing",
        {
            "Indicates that a required property was not supplied as part of the request.",
            "The property %1 is a required property and must be included in the request.",
            "Warning",
            1,
            {
                "string",
            },
            "Ensure that the property is in the request body and has a valid value and resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "PropertyNotUpdated",
        {
            "Indicates that a property was not updated due to an internal service error, but the service is still functional.",
            "The property %1 was not updated due to an internal service error.  The service is still operational.",
            "Critical",
            1,
            {
                "string",
            },
            "Resubmit the request.  If the problem persists, check for additional messages and consider resetting the service.",
        }},
    MessageEntry{
        "PropertyNotWritable",
        {
            "Indicates that a property was given a value in the request body, but the property is a readonly property.",
            "The property %1 is a read only property and cannot be assigned a value.",
            "Warning",
            1,
            {
                "string",
            },
            "Remove the property from the request body and resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "PropertyUnknown",
        {
            "Indicates that an unknown property was included in the request body.",
            "The property %1 is not in the list of valid properties for the resource.",
            "Warning",
            1,
            {
                "string",
            },
            "Remove the unknown property from the request body and resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "PropertyValueConflict",
        {
            "Indicates that the requested write of a property value could not be completed, because of a conflict with another property value.",
            "The property '%1' could not be written because its value would conflict with the value of the '%2' property.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "No resolution is required.",
        }},
    MessageEntry{
        "PropertyValueDeprecated",
        {
            "Indicates that a property was given a deprecated value.",
            "The value '%1' for the property %2 is deprecated.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "Refer to the schema guide for more information.",
        }},
    MessageEntry{
        "PropertyValueError",
        {
            "Indicates that a property was given an invalid value.",
            "The value provided for the property %1 is not valid.",
            "Warning",
            1,
            {
                "string",
            },
            "Correct the value for the property in the request body and resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "PropertyValueExternalConflict",
        {
            "Indicates that the requested write of a property value could not be completed, due to the current state or configuration of the resource.  This can include configuration conflicts with other resources or parameters that are not exposed by this interface.",
            "The property '%1' with the requested value of '%2' could not be written because the value is not available due to a configuration conflict.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "No resolution is required.",
        }},
    MessageEntry{
        "PropertyValueFormatError",
        {
            "Indicates that a property was given the correct value type but the value of that property was not supported.",
            "The value '%1' for the property %2 is of a different format than the property can accept.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "Correct the value for the property in the request body and resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "PropertyValueIncorrect",
        {
            "Indicates that the requested write of a property value could not be completed, because of an incorrect value of the property.  Examples include values that do not match a regular expression requirement or passwords that do not match the implementation constraints.",
            "The property '%1' with the requested value of '%2' could not be written because the value does not meet the constraints of the implementation.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "No resolution is required.",
        }},
    MessageEntry{
        "PropertyValueModified",
        {
            "Indicates that a property was given the correct value type but the value of that property was modified.  Examples are truncated or rounded values.",
            "The property %1 was assigned the value '%2' due to modification by the service.",
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
            "Indicates that a property was given the correct value type but the value of that property was not supported.  The value is not in an enumeration.",
            "The value '%1' for the property %2 is not in the list of acceptable values.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "Choose a value from the enumeration list that the implementation can support and resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "PropertyValueOutOfRange",
        {
            "Indicates that a property was given the correct value type but the value of that property is outside the supported range.",
            "The value '%1' for the property %2 is not in the supported range of acceptable values.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "Correct the value for the property in the request body and resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "PropertyValueResourceConflict",
        {
            "Indicates that the requested write of a property value could not be completed, due to the current state or configuration of another resource.",
            "The property '%1' with the requested value of '%2' could not be written because the value conflicts with the state or configuration of the resource at '%3'.",
            "Warning",
            3,
            {
                "string",
                "string",
                "string",
            },
            "No resolution is required.",
        }},
    MessageEntry{
        "PropertyValueTypeError",
        {
            "Indicates that a property was given the wrong value type, such as when a number is supplied for a property that requires a string.",
            "The value '%1' for the property %2 is of a different type than the property can accept.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "Correct the value for the property in the request body and resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "QueryCombinationInvalid",
        {
            "Indicates the request contains multiple query parameters, and that two or more of them cannot be used together.",
            "Two or more query parameters in the request cannot be used together.",
            "Warning",
            0,
            {},
            "Remove one or more of the query parameters and resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "QueryNotSupported",
        {
            "Indicates that query is not supported on the implementation.",
            "Querying is not supported by the implementation.",
            "Warning",
            0,
            {},
            "Remove the query parameters and resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "QueryNotSupportedOnOperation",
        {
            "Indicates that query is not supported with the given operation, such as when the `$expand` query is attempted with a PATCH operation.",
            "Querying is not supported with the requested operation.",
            "Warning",
            0,
            {},
            "Remove the query parameters and resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "QueryNotSupportedOnResource",
        {
            "Indicates that query is not supported on the given resource, such as when the `$skip` query is attempted on a resource that is not a collection.",
            "Querying is not supported on the requested resource.",
            "Warning",
            0,
            {},
            "Remove the query parameters and resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "QueryParameterOutOfRange",
        {
            "Indicates that a query parameter was provided that is out of range for the given resource.  This can happen with values that are too low or beyond that possible for the supplied resource, such as when a page is requested that is beyond the last page.",
            "The value '%1' for the query parameter %2 is out of range %3.",
            "Warning",
            3,
            {
                "string",
                "string",
                "string",
            },
            "Reduce the value for the query parameter to a value that is within range, such as a start or count value that is within bounds of the number of resources in a collection or a page that is within the range of valid pages.",
        }},
    MessageEntry{
        "QueryParameterValueError",
        {
            "Indicates that a query parameter was given an invalid value.",
            "The value for the parameter %1 is invalid.",
            "Warning",
            1,
            {
                "string",
            },
            "Correct the value for the query parameter in the request and resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "QueryParameterValueFormatError",
        {
            "Indicates that a query parameter was given the correct value type but the value of that parameter was not supported.  This includes the value size or length has been exceeded.",
            "The value '%1' for the parameter %2 is of a different format than the parameter can accept.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "Correct the value for the query parameter in the request and resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "QueryParameterValueTypeError",
        {
            "Indicates that a query parameter was given the wrong value type, such as when a number is supplied for a query parameter that requires a string.",
            "The value '%1' for the query parameter %2 is of a different type than the parameter can accept.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "Correct the value for the query parameter in the request and resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "ResetRecommended",
        {
            "Indicates that a component reset is recommended for error recovery while unaffected applications can continue running without any effects on accuracy and performance.",
            "In order to recover from errors, a component reset is recommended with the Reset action URI '%1' and ResetType '%2'.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "Perform the recommended reset action on the specified component.",
        }},
    MessageEntry{
        "ResetRequired",
        {
            "Indicates that a component reset is required for changes, error recovery, or operations to complete.",
            "In order to apply changes, recover from errors, or complete the operation, a component reset is required with the Reset action URI '%1' and ResetType '%2'.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "Perform the required reset action on the specified component.",
        }},
    MessageEntry{
        "ResourceAlreadyExists",
        {
            "Indicates that a resource change or creation was attempted but that the operation cannot proceed because the resource already exists.",
            "The requested resource of type %1 with the property %2 with the value '%3' already exists.",
            "Critical",
            3,
            {
                "string",
                "string",
                "string",
            },
            "Do not repeat the create operation as the resource has already been created.",
        }},
    MessageEntry{
        "ResourceAtUriInUnknownFormat",
        {
            "Indicates that the URI was valid but the resource or image at that URI was in a format not supported by the service.",
            "The resource at '%1' is in a format not recognized by the service.",
            "Critical",
            1,
            {
                "string",
            },
            "Place an image or resource or file that is recognized by the service at the URI.",
        }},
    MessageEntry{
        "ResourceAtUriUnauthorized",
        {
            "Indicates that the attempt to access the resource, file, or image at the URI was unauthorized.",
            "While accessing the resource at '%1', the service received an authorization error '%2'.",
            "Critical",
            2,
            {
                "string",
                "string",
            },
            "Ensure that the appropriate access is provided for the service in order for it to access the URI.",
        }},
    MessageEntry{
        "ResourceCannotBeDeleted",
        {
            "Indicates that a delete operation was attempted on a resource that cannot be deleted.",
            "The delete request failed because the resource requested cannot be deleted.",
            "Critical",
            0,
            {},
            "Do not attempt to delete a non-deletable resource.",
        }},
    MessageEntry{
        "ResourceCreationConflict",
        {
            "Indicates that the requested resource creation could not be completed because the service has a resource that conflicts with the request.",
            "The resource could not be created.  The service has a resource at URI '%1' that conflicts with the creation request.",
            "Warning",
            1,
            {
                "string",
            },
            "No resolution is required.",
        }},
    MessageEntry{
        "ResourceDeprecated",
        {
            "Indicates the resource is deprecated.",
            "The operation was performed on a deprecated resource '%1'.",
            "Warning",
            1,
            {
                "string",
            },
            "Refer to the schema guide for more information.",
        }},
    MessageEntry{
        "ResourceExhaustion",
        {
            "Indicates that a resource could not satisfy the request due to some unavailability of resources.  An example is that available capacity has been allocated.",
            "The resource '%1' was unable to satisfy the request due to unavailability of resources.",
            "Critical",
            1,
            {
                "string",
            },
            "Ensure that the resources are available and resubmit the request.",
        }},
    MessageEntry{
        "ResourceInStandby",
        {
            "Indicates that the request could not be performed because the resource is in standby.",
            "The request could not be performed because the resource is in standby.",
            "Critical",
            0,
            {},
            "Ensure that the resource is in the correct power state and resubmit the request.",
        }},
    MessageEntry{
        "ResourceInUse",
        {
            "Indicates that a change was requested to a resource but the change was rejected due to the resource being in use or transition.",
            "The change to the requested resource failed because the resource is in use or in transition.",
            "Warning",
            0,
            {},
            "Remove the condition and resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "ResourceMissingAtURI",
        {
            "Indicates that the operation expected an image or other resource at the provided URI but none was found.  Examples of this are in requests that require URIs like firmware update.",
            "The resource at the URI '%1' was not found.",
            "Critical",
            1,
            {
                "string",
            },
            "Place a valid resource at the URI or correct the URI and resubmit the request.",
        }},
    MessageEntry{
        "ResourceNotFound",
        {
            "Indicates that the operation expected a resource identifier that corresponds to an existing resource but one was not found.",
            "The requested resource of type %1 named '%2' was not found.",
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
            "Indicates that the resource type of the operation does not match that for the operation destination.  Examples of when this can happen include during a POST to a resource collection using the wrong resource type, an update where the `@odata.type` properties do not match, or on a major version incompatibility.",
            "The @odata.type of the request body %1 is incompatible with the @odata.type of the resource, which is %2.",
            "Critical",
            2,
            {
                "string",
                "string",
            },
            "Resubmit the request with a payload compatible with the resource's schema.",
        }},
    MessageEntry{
        "RestrictedPrivilege",
        {
            "Indicates that the operation was not successful because a privilege is restricted.",
            "The operation was not successful because the privilege '%1' is restricted.",
            "Warning",
            1,
            {
                "string",
            },
            "Remove restricted privileges from the request body and resubmit the request.",
        }},
    MessageEntry{
        "RestrictedRole",
        {
            "Indicates that the operation was not successful because the role is restricted.",
            "The operation was not successful because the role '%1' is restricted.",
            "Warning",
            1,
            {
                "string",
            },
            "No resolution is required.  For standard roles, consider using the role specified in the AlternateRoleId property in the Role resource.",
        }},
    MessageEntry{
        "ServiceDisabled",
        {
            "Indicates that the operation failed because the service, such as the account service, is disabled and cannot accept requests.",
            "The operation failed because the service at %1 is disabled and cannot accept requests.",
            "Warning",
            1,
            {
                "string",
            },
            "Enable the service and resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "ServiceInUnknownState",
        {
            "Indicates that the operation failed because the service is in an unknown state and cannot accept additional requests.",
            "The operation failed because the service is in an unknown state and can no longer take incoming requests.",
            "Critical",
            0,
            {},
            "Restart the service and resubmit the request if the operation failed.",
        }},
    MessageEntry{
        "ServiceShuttingDown",
        {
            "Indicates that the operation failed as the service is shutting down, such as when the service reboots.",
            "The operation failed because the service is shutting down and can no longer take incoming requests.",
            "Critical",
            0,
            {},
            "When the service becomes available, resubmit the request if the operation failed.",
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
            "Indicates that a session establishment has been requested but the operation failed due to the number of simultaneous sessions exceeding the limit of the implementation.",
            "The session establishment failed due to the number of simultaneous sessions exceeding the limit of the implementation.",
            "Critical",
            0,
            {},
            "Reduce the number of other sessions before trying to establish the session or increase the limit of simultaneous sessions, if supported.",
        }},
    MessageEntry{
        "SessionTerminated",
        {
            "Indicates that the DELETE operation on the session resource resulted in the successful termination of the session.",
            "The session was successfully terminated.",
            "OK",
            0,
            {},
            "No resolution is required.",
        }},
    MessageEntry{
        "SourceDoesNotSupportProtocol",
        {
            "Indicates that while attempting to access, connect to or transfer a resource, file, or image from another location that the other end of the connection did not support the protocol.",
            "The other end of the connection at '%1' does not support the specified protocol %2.",
            "Critical",
            2,
            {
                "string",
                "string",
            },
            "Change protocols or URIs.",
        }},
    MessageEntry{
        "StrictAccountTypes",
        {
            "Indicates the request failed because a set of `AccountTypes` or `OEMAccountTypes` was not accepted while `StrictAccountTypes` is set to `true`.",
            "The request was not possible to fulfill with the account types included in property '%1' and property StrictAccountTypes set to true.",
            "Warning",
            1,
            {
                "string",
            },
            "Resubmit the request either with an acceptable set of AccountTypes and OEMAccountTypes or with StrictAccountTypes set to false.",
        }},
    MessageEntry{
        "StringValueTooLong",
        {
            "Indicates that a string value passed to the given resource exceeded its length limit.  An example is when a shorter limit is imposed by an implementation than that allowed by the specification.",
            "The string '%1' exceeds the length limit %2.",
            "Warning",
            2,
            {
                "string",
                "number",
            },
            "Resubmit the request with an appropriate string length.",
        }},
    MessageEntry{
        "SubscriptionTerminated",
        {
            "An event subscription has been terminated by the service.  No further events will be delivered.",
            "The event subscription has been terminated.",
            "OK",
            0,
            {},
            "No resolution is required.",
        }},
    MessageEntry{
        "Success",
        {
            "Indicates that all conditions of a successful operation have been met.",
            "The request completed successfully.",
            "OK",
            0,
            {},
            "None",
        }},
    MessageEntry{
        "UndeterminedFault",
        {
            "Indicates that a fault or error condition exists but the source of the fault cannot be determined or is unknown to the service.",
            "A undetermined fault condition has been reported by '%1'.",
            "Critical",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "UnrecognizedRequestBody",
        {
            "Indicates that the service encountered an unrecognizable request body that could not even be interpreted as malformed JSON.",
            "The service detected a malformed request body that it was unable to interpret.",
            "Warning",
            0,
            {},
            "Correct the request body and resubmit the request if it failed.",
        }},

};

enum class Index
{
    accessDenied = 0,
    accountForSessionNoLongerExists = 1,
    accountModified = 2,
    accountNotModified = 3,
    accountRemoved = 4,
    actionDeprecated = 5,
    actionNotSupported = 6,
    actionParameterDuplicate = 7,
    actionParameterMissing = 8,
    actionParameterNotSupported = 9,
    actionParameterUnknown = 10,
    actionParameterValueConflict = 11,
    actionParameterValueError = 12,
    actionParameterValueFormatError = 13,
    actionParameterValueNotInList = 14,
    actionParameterValueTypeError = 15,
    chassisPowerStateOffRequired = 16,
    chassisPowerStateOnRequired = 17,
    conditionInRelatedResource = 18,
    couldNotEstablishConnection = 19,
    createFailedMissingReqProperties = 20,
    createLimitReachedForResource = 21,
    created = 22,
    emptyJSON = 23,
    eventBufferExceeded = 24,
    eventSubscriptionLimitExceeded = 25,
    generalError = 26,
    headerInvalid = 27,
    headerMissing = 28,
    insufficientPrivilege = 29,
    insufficientStorage = 30,
    internalError = 31,
    invalidIndex = 32,
    invalidJSON = 33,
    invalidObject = 34,
    invalidURI = 35,
    malformedJSON = 36,
    maximumErrorsExceeded = 37,
    missingOrMalformedPart = 38,
    networkNameResolutionNotConfigured = 39,
    networkNameResolutionNotSupported = 40,
    noOperation = 41,
    noValidSession = 42,
    operationFailed = 43,
    operationNotAllowed = 44,
    operationTimeout = 45,
    passwordChangeRequired = 46,
    payloadTooLarge = 47,
    preconditionFailed = 48,
    preconditionRequired = 49,
    propertyDeprecated = 50,
    propertyDuplicate = 51,
    propertyMissing = 52,
    propertyNotUpdated = 53,
    propertyNotWritable = 54,
    propertyUnknown = 55,
    propertyValueConflict = 56,
    propertyValueDeprecated = 57,
    propertyValueError = 58,
    propertyValueExternalConflict = 59,
    propertyValueFormatError = 60,
    propertyValueIncorrect = 61,
    propertyValueModified = 62,
    propertyValueNotInList = 63,
    propertyValueOutOfRange = 64,
    propertyValueResourceConflict = 65,
    propertyValueTypeError = 66,
    queryCombinationInvalid = 67,
    queryNotSupported = 68,
    queryNotSupportedOnOperation = 69,
    queryNotSupportedOnResource = 70,
    queryParameterOutOfRange = 71,
    queryParameterValueError = 72,
    queryParameterValueFormatError = 73,
    queryParameterValueTypeError = 74,
    resetRecommended = 75,
    resetRequired = 76,
    resourceAlreadyExists = 77,
    resourceAtUriInUnknownFormat = 78,
    resourceAtUriUnauthorized = 79,
    resourceCannotBeDeleted = 80,
    resourceCreationConflict = 81,
    resourceDeprecated = 82,
    resourceExhaustion = 83,
    resourceInStandby = 84,
    resourceInUse = 85,
    resourceMissingAtURI = 86,
    resourceNotFound = 87,
    resourceTypeIncompatible = 88,
    restrictedPrivilege = 89,
    restrictedRole = 90,
    serviceDisabled = 91,
    serviceInUnknownState = 92,
    serviceShuttingDown = 93,
    serviceTemporarilyUnavailable = 94,
    sessionLimitExceeded = 95,
    sessionTerminated = 96,
    sourceDoesNotSupportProtocol = 97,
    strictAccountTypes = 98,
    stringValueTooLong = 99,
    subscriptionTerminated = 100,
    success = 101,
    undeterminedFault = 102,
    unrecognizedRequestBody = 103,
};
} // namespace redfish::registries::base
