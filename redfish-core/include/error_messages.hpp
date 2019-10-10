/*
// Copyright (c) 2018 Intel Corporation
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
 * to use Redfish DMTF defined messages.
 *
 * This header contains preprocessor defines which wrap
 * preparation functions for message with given id. The message
 * ids can be retrieved from Base.__ver__.json file.
 ***************************************************************/
#pragma once
#include <nlohmann/json.hpp>

#include "http_response.h"

namespace redfish
{

namespace messages
{

constexpr const char* messageVersionPrefix = "Base.1.2.0.";
constexpr const char* messageAnnotation = "@Message.ExtendedInfo";

/**
 * @brief Formats ResourceInUse message into JSON
 * Message body: "The change to the requested resource failed because the
 * resource is in use or in transition."
 *
 *
 * @returns Message ResourceInUse formatted to JSON */
void resourceInUse(crow::Response& res);

/**
 * @brief Formats MalformedJSON message into JSON
 * Message body: "The request body submitted was malformed JSON and could not be
 * parsed by the receiving service."
 *
 *
 * @returns Message MalformedJSON formatted to JSON */
void malformedJSON(crow::Response& res);

/**
 * @brief Formats ResourceMissingAtURI message into JSON
 * Message body: "The resource at the URI <arg1> was not found."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 *
 * @returns Message ResourceMissingAtURI formatted to JSON */
void resourceMissingAtURI(crow::Response& res, const std::string& arg1);

/**
 * @brief Formats ActionParameterValueFormatError message into JSON
 * Message body: "The value <arg1> for the parameter <arg2> in the action <arg3>
 * is of a different format than the parameter can accept."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 * @param[in] arg3 Parameter of message that will replace %3 in its body.
 *
 * @returns Message ActionParameterValueFormatError formatted to JSON */
void actionParameterValueFormatError(crow::Response& res,
                                     const std::string& arg1,
                                     const std::string& arg2,
                                     const std::string& arg3);

/**
 * @brief Formats InternalError message into JSON
 * Message body: "The request failed due to an internal service error.  The
 * service is still operational."
 *
 *
 * @returns Message InternalError formatted to JSON */
void internalError(crow::Response& res);

/**
 * @brief Formats UnrecognizedRequestBody message into JSON
 * Message body: "The service detected a malformed request body that it was
 * unable to interpret."
 *
 *
 * @returns Message UnrecognizedRequestBody formatted to JSON */
void unrecognizedRequestBody(crow::Response& res);

/**
 * @brief Formats ResourceAtUriUnauthorized message into JSON
 * Message body: "While accessing the resource at <arg1>, the service received
 * an authorization error <arg2>."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 *
 * @returns Message ResourceAtUriUnauthorized formatted to JSON */
void resourceAtUriUnauthorized(crow::Response& res, const std::string& arg1,
                               const std::string& arg2);

/**
 * @brief Formats ActionParameterUnknown message into JSON
 * Message body: "The action <arg1> was submitted with the invalid parameter
 * <arg2>."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 *
 * @returns Message ActionParameterUnknown formatted to JSON */
void actionParameterUnknown(crow::Response& res, const std::string& arg1,
                            const std::string& arg2);

/**
 * @brief Formats ResourceCannotBeDeleted message into JSON
 * Message body: "The delete request failed because the resource requested
 * cannot be deleted."
 *
 *
 * @returns Message ResourceCannotBeDeleted formatted to JSON */
void resourceCannotBeDeleted(crow::Response& res);

/**
 * @brief Formats PropertyDuplicate message into JSON
 * Message body: "The property <arg1> was duplicated in the request."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 *
 * @returns Message PropertyDuplicate formatted to JSON */
void propertyDuplicate(crow::Response& res, const std::string& arg1);

/**
 * @brief Formats ServiceTemporarilyUnavailable message into JSON
 * Message body: "The service is temporarily unavailable.  Retry in <arg1>
 * seconds."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 *
 * @returns Message ServiceTemporarilyUnavailable formatted to JSON */
void serviceTemporarilyUnavailable(crow::Response& res,
                                   const std::string& arg1);

/**
 * @brief Formats ResourceAlreadyExists message into JSON
 * Message body: "The requested resource of type <arg1> with the property <arg2>
 * with the value <arg3> already exists."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 * @param[in] arg3 Parameter of message that will replace %3 in its body.
 *
 * @returns Message ResourceAlreadyExists formatted to JSON */
void resourceAlreadyExists(crow::Response& res, const std::string& arg1,
                           const std::string& arg2, const std::string& arg3);

/**
 * @brief Formats AccountForSessionNoLongerExists message into JSON
 * Message body: "The account for the current session has been removed, thus the
 * current session has been removed as well."
 *
 *
 * @returns Message AccountForSessionNoLongerExists formatted to JSON */
void accountForSessionNoLongerExists(crow::Response& res);

/**
 * @brief Formats CreateFailedMissingReqProperties message into JSON
 * Message body: "The create operation failed because the required property
 * <arg1> was missing from the request."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 *
 * @returns Message CreateFailedMissingReqProperties formatted to JSON */
void createFailedMissingReqProperties(crow::Response& res,
                                      const std::string& arg1);

/**
 * @brief Formats PropertyValueFormatError message into JSON
 * Message body: "The value <arg1> for the property <arg2> is of a different
 * format than the property can accept."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 *
 * @returns Message PropertyValueFormatError formatted to JSON */
void propertyValueFormatError(crow::Response& res, const std::string& arg1,
                              const std::string& arg2);

/**
 * @brief Formats PropertyValueNotInList message into JSON
 * Message body: "The value <arg1> for the property <arg2> is not in the list of
 * acceptable values."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 *
 * @returns Message PropertyValueNotInList formatted to JSON */
void propertyValueNotInList(crow::Response& res, const std::string& arg1,
                            const std::string& arg2);

/**
 * @brief Formats ResourceAtUriInUnknownFormat message into JSON
 * Message body: "The resource at <arg1> is in a format not recognized by the
 * service."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 *
 * @returns Message ResourceAtUriInUnknownFormat formatted to JSON */
void resourceAtUriInUnknownFormat(crow::Response& res, const std::string& arg1);

/**
 * @brief Formats ServiceInUnknownState message into JSON
 * Message body: "The operation failed because the service is in an unknown
 * state and can no longer take incoming requests."
 *
 *
 * @returns Message ServiceInUnknownState formatted to JSON */
void serviceInUnknownState(crow::Response& res);

/**
 * @brief Formats EventSubscriptionLimitExceeded message into JSON
 * Message body: "The event subscription failed due to the number of
 * simultaneous subscriptions exceeding the limit of the implementation."
 *
 *
 * @returns Message EventSubscriptionLimitExceeded formatted to JSON */
void eventSubscriptionLimitExceeded(crow::Response& res);

/**
 * @brief Formats ActionParameterMissing message into JSON
 * Message body: "The action <arg1> requires the parameter <arg2> to be present
 * in the request body."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 *
 * @returns Message ActionParameterMissing formatted to JSON */
void actionParameterMissing(crow::Response& res, const std::string& arg1,
                            const std::string& arg2);

/**
 * @brief Formats StringValueTooLong message into JSON
 * Message body: "The string <arg1> exceeds the length limit <arg2>."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 *
 * @returns Message StringValueTooLong formatted to JSON */
void stringValueTooLong(crow::Response& res, const std::string& arg1,
                        const int& arg2);

/**
 * @brief Formats SessionTerminated message into JSON
 * Message body: "The session was successfully terminated."
 *
 *
 * @returns Message SessionTerminated formatted to JSON */
void sessionTerminated(crow::Response& res);

/**
 * @brief Formats ResourceTypeIncompatible message into JSON
 * Message body: "The @odata.type of the request body <arg1> is incompatible
 * with the @odata.type of the resource which is <arg2>."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 *
 * @returns Message ResourceTypeIncompatible formatted to JSON */
void resourceTypeIncompatible(crow::Response& res, const std::string& arg1,
                              const std::string& arg2);

/**
 * @brief Formats PropertyValueTypeError message into JSON
 * Message body: "The value <arg1> for the property <arg2> is of a different
 * type than the property can accept."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 *
 * @returns Message PropertyValueTypeError formatted to JSON */
void propertyValueTypeError(crow::Response& res, const std::string& arg1,
                            const std::string& arg2);

/**
 * @brief Formats ResourceNotFound message into JSON
 * Message body: "The requested resource of type <arg1> named <arg2> was not
 * found."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 *
 * @returns Message ResourceNotFound formatted to JSON */
void resourceNotFound(crow::Response& res, const std::string& arg1,
                      const std::string& arg2);

/**
 * @brief Formats CouldNotEstablishConnection message into JSON
 * Message body: "The service failed to establish a Connection with the URI
 * <arg1>."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 *
 * @returns Message CouldNotEstablishConnection formatted to JSON */
void couldNotEstablishConnection(crow::Response& res, const std::string& arg1);

/**
 * @brief Formats PropertyNotWritable message into JSON
 * Message body: "The property <arg1> is a read only property and cannot be
 * assigned a value."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 *
 * @returns Message PropertyNotWritable formatted to JSON */
void propertyNotWritable(crow::Response& res, const std::string& arg1);

/**
 * @brief Formats QueryParameterValueTypeError message into JSON
 * Message body: "The value <arg1> for the query parameter <arg2> is of a
 * different type than the parameter can accept."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 *
 * @returns Message QueryParameterValueTypeError formatted to JSON */
void queryParameterValueTypeError(crow::Response& res, const std::string& arg1,
                                  const std::string& arg2);

/**
 * @brief Formats ServiceShuttingDown message into JSON
 * Message body: "The operation failed because the service is shutting down and
 * can no longer take incoming requests."
 *
 *
 * @returns Message ServiceShuttingDown formatted to JSON */
void serviceShuttingDown(crow::Response& res);

/**
 * @brief Formats ActionParameterDuplicate message into JSON
 * Message body: "The action <arg1> was submitted with more than one value for
 * the parameter <arg2>."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 *
 * @returns Message ActionParameterDuplicate formatted to JSON */
void actionParameterDuplicate(crow::Response& res, const std::string& arg1,
                              const std::string& arg2);

/**
 * @brief Formats ActionParameterNotSupported message into JSON
 * Message body: "The parameter <arg1> for the action <arg2> is not supported on
 * the target resource."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 *
 * @returns Message ActionParameterNotSupported formatted to JSON */
void actionParameterNotSupported(crow::Response& res, const std::string& arg1,
                                 const std::string& arg2);

/**
 * @brief Formats SourceDoesNotSupportProtocol message into JSON
 * Message body: "The other end of the Connection at <arg1> does not support the
 * specified protocol <arg2>."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 *
 * @returns Message SourceDoesNotSupportProtocol formatted to JSON */
void sourceDoesNotSupportProtocol(crow::Response& res, const std::string& arg1,
                                  const std::string& arg2);

/**
 * @brief Formats AccountRemoved message into JSON
 * Message body: "The account was successfully removed."
 *
 *
 * @returns Message AccountRemoved formatted to JSON */
void accountRemoved(crow::Response& res);

/**
 * @brief Formats AccessDenied message into JSON
 * Message body: "While attempting to establish a Connection to <arg1>, the
 * service denied access."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 *
 * @returns Message AccessDenied formatted to JSON */
void accessDenied(crow::Response& res, const std::string& arg1);

/**
 * @brief Formats QueryNotSupported message into JSON
 * Message body: "Querying is not supported by the implementation."
 *
 *
 * @returns Message QueryNotSupported formatted to JSON */
void queryNotSupported(crow::Response& res);

/**
 * @brief Formats CreateLimitReachedForResource message into JSON
 * Message body: "The create operation failed because the resource has reached
 * the limit of possible resources."
 *
 *
 * @returns Message CreateLimitReachedForResource formatted to JSON */
void createLimitReachedForResource(crow::Response& res);

/**
 * @brief Formats GeneralError message into JSON
 * Message body: "A general error has occurred. See ExtendedInfo for more
 * information."
 *
 *
 * @returns Message GeneralError formatted to JSON */
void generalError(crow::Response& res);

/**
 * @brief Formats Success message into JSON
 * Message body: "Successfully Completed Request"
 *
 *
 * @returns Message Success formatted to JSON */
void success(crow::Response& res);

/**
 * @brief Formats Created message into JSON
 * Message body: "The resource has been created successfully"
 *
 *
 * @returns Message Created formatted to JSON */
void created(crow::Response& res);

/**
 * @brief Formats NoOperation message into JSON
 * Message body: "The request body submitted contain no data to act upon and
 * no changes to the resource took place."
 *
 *
 * @returns Message NoOperation formatted to JSON */
void noOperation(crow::Response& res);

/**
 * @brief Formats PropertyUnknown message into JSON
 * Message body: "The property <arg1> is not in the list of valid properties for
 * the resource."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 *
 * @returns Message PropertyUnknown formatted to JSON */
void propertyUnknown(crow::Response& res, const std::string& arg1);

/**
 * @brief Formats NoValidSession message into JSON
 * Message body: "There is no valid session established with the
 * implementation."
 *
 *
 * @returns Message NoValidSession formatted to JSON */
void noValidSession(crow::Response& res);

/**
 * @brief Formats InvalidObject message into JSON
 * Message body: "The object at <arg1> is invalid."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 *
 * @returns Message InvalidObject formatted to JSON */
void invalidObject(crow::Response& res, const std::string& arg1);

/**
 * @brief Formats ResourceInStandby message into JSON
 * Message body: "The request could not be performed because the resource is in
 * standby."
 *
 *
 * @returns Message ResourceInStandby formatted to JSON */
void resourceInStandby(crow::Response& res);

/**
 * @brief Formats ActionParameterValueTypeError message into JSON
 * Message body: "The value <arg1> for the parameter <arg2> in the action <arg3>
 * is of a different type than the parameter can accept."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 * @param[in] arg3 Parameter of message that will replace %3 in its body.
 *
 * @returns Message ActionParameterValueTypeError formatted to JSON */
void actionParameterValueTypeError(crow::Response& res, const std::string& arg1,
                                   const std::string& arg2,
                                   const std::string& arg3);

/**
 * @brief Formats SessionLimitExceeded message into JSON
 * Message body: "The session establishment failed due to the number of
 * simultaneous sessions exceeding the limit of the implementation."
 *
 *
 * @returns Message SessionLimitExceeded formatted to JSON */
void sessionLimitExceeded(crow::Response& res);

/**
 * @brief Formats ActionNotSupported message into JSON
 * Message body: "The action <arg1> is not supported by the resource."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 *
 * @returns Message ActionNotSupported formatted to JSON */
void actionNotSupported(crow::Response& res, const std::string& arg1);

/**
 * @brief Formats InvalidIndex message into JSON
 * Message body: "The index <arg1> is not a valid offset into the array."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 *
 * @returns Message InvalidIndex formatted to JSON */
void invalidIndex(crow::Response& res, const int& arg1);

/**
 * @brief Formats EmptyJSON message into JSON
 * Message body: "The request body submitted contained an empty JSON object and
 * the service is unable to process it."
 *
 *
 * @returns Message EmptyJSON formatted to JSON */
void emptyJSON(crow::Response& res);

/**
 * @brief Formats QueryNotSupportedOnResource message into JSON
 * Message body: "Querying is not supported on the requested resource."
 *
 *
 * @returns Message QueryNotSupportedOnResource formatted to JSON */
void queryNotSupportedOnResource(crow::Response& res);

/**
 * @brief Formats InsufficientPrivilege message into JSON
 * Message body: "There are insufficient privileges for the account or
 * credentials associated with the current session to perform the requested
 * operation."
 *
 *
 * @returns Message InsufficientPrivilege formatted to JSON */
void insufficientPrivilege(crow::Response& res);

/**
 * @brief Formats PropertyValueModified message into JSON
 * Message body: "The property <arg1> was assigned the value <arg2> due to
 * modification by the service."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 *
 * @returns Message PropertyValueModified formatted to JSON */
void propertyValueModified(crow::Response& res, const std::string& arg1,
                           const std::string& arg2);

/**
 * @brief Formats AccountNotModified message into JSON
 * Message body: "The account modification request failed."
 *
 *
 * @returns Message AccountNotModified formatted to JSON */
void accountNotModified(crow::Response& res);

/**
 * @brief Formats QueryParameterValueFormatError message into JSON
 * Message body: "The value <arg1> for the parameter <arg2> is of a different
 * format than the parameter can accept."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 *
 * @returns Message QueryParameterValueFormatError formatted to JSON */
void queryParameterValueFormatError(crow::Response& res,
                                    const std::string& arg1,
                                    const std::string& arg2);

/**
 * @brief Formats PropertyMissing message into JSON
 * Message body: "The property <arg1> is a required property and must be
 * included in the request."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 *
 * @returns Message PropertyMissing formatted to JSON */
void propertyMissing(crow::Response& res, const std::string& arg1);

/**
 * @brief Formats ResourceExhaustion message into JSON
 * Message body: "The resource <arg1> was unable to satisfy the request due to
 * unavailability of resources."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 *
 * @returns Message ResourceExhaustion formatted to JSON */
void resourceExhaustion(crow::Response& res, const std::string& arg1);

/**
 * @brief Formats AccountModified message into JSON
 * Message body: "The account was successfully modified."
 *
 *
 * @returns Message AccountModified formatted to JSON */
void accountModified(crow::Response& res);

/**
 * @brief Formats QueryParameterOutOfRange message into JSON
 * Message body: "The value <arg1> for the query parameter <arg2> is out of
 * range <arg3>."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 * @param[in] arg3 Parameter of message that will replace %3 in its body.
 *
 * @returns Message QueryParameterOutOfRange formatted to JSON */
void queryParameterOutOfRange(crow::Response& res, const std::string& arg1,
                              const std::string& arg2, const std::string& arg3);

} // namespace messages

} // namespace redfish
