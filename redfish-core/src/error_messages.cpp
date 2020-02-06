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
#include <logging.h>

#include <error_messages.hpp>

namespace redfish
{

namespace messages
{

static void addMessageToErrorJson(nlohmann::json& target,
                                  const nlohmann::json& message)
{
    auto& error = target["error"];

    // If this is the first error message, fill in the information from the
    // first error message to the top level struct
    if (!error.is_object())
    {
        auto messageIdIterator = message.find("MessageId");
        if (messageIdIterator == message.end())
        {
            BMCWEB_LOG_CRITICAL
                << "Attempt to add error message without MessageId";
            return;
        }

        auto messageFieldIterator = message.find("Message");
        if (messageFieldIterator == message.end())
        {
            BMCWEB_LOG_CRITICAL
                << "Attempt to add error message without Message";
            return;
        }
        error = {{"code", *messageIdIterator},
                 {"message", *messageFieldIterator}};
    }
    else
    {
        // More than 1 error occurred, so the message has to be generic
        error["code"] = std::string(messageVersionPrefix) + "GeneralError";
        error["message"] = "A general error has occurred. See Resolution for "
                           "information on how to resolve the error.";
    }

    // This check could technically be done in in the default construction
    // branch above, but because we need the pointer to the extended info field
    // anyway, it's more efficient to do it here.
    auto& extendedInfo = error[messages::messageAnnotation];
    if (!extendedInfo.is_array())
    {
        extendedInfo = nlohmann::json::array();
    }

    extendedInfo.push_back(message);
}

static void addMessageToJsonRoot(nlohmann::json& target,
                                 const nlohmann::json& message)
{
    if (!target[messages::messageAnnotation].is_array())
    {
        // Force object to be an array
        target[messages::messageAnnotation] = nlohmann::json::array();
    }

    target[messages::messageAnnotation].push_back(message);
}

static void addMessageToJson(nlohmann::json& target,
                             const nlohmann::json& message,
                             const std::string& fieldPath)
{
    std::string extendedInfo(fieldPath + messages::messageAnnotation);

    if (!target[extendedInfo].is_array())
    {
        // Force object to be an array
        target[extendedInfo] = nlohmann::json::array();
    }

    // Object exists and it is an array so we can just push in the message
    target[extendedInfo].push_back(message);
}

/**
 * @internal
 * @brief Formats ResourceInUse message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourceInUse(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.ResourceInUse"},
        {"Message", "The change to the requested resource failed because "
                    "the resource is in use or in transition."},
        {"MessageArgs", nlohmann::json::array()},
        {"Severity", "Warning"},
        {"Resolution", "Remove the condition and resubmit the request if "
                       "the operation failed."}};
}

void resourceInUse(crow::Response& res)
{
    res.result(boost::beast::http::status::service_unavailable);
    addMessageToErrorJson(res.jsonValue, resourceInUse());
}

/**
 * @internal
 * @brief Formats MalformedJSON message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json malformedJSON(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.MalformedJSON"},
        {"Message", "The request body submitted was malformed JSON and "
                    "could not be parsed by the receiving service."},
        {"MessageArgs", nlohmann::json::array()},
        {"Severity", "Critical"},
        {"Resolution", "Ensure that the request body is valid JSON and "
                       "resubmit the request."}};
}

void malformedJSON(crow::Response& res)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, malformedJSON());
}

/**
 * @internal
 * @brief Formats ResourceMissingAtURI message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourceMissingAtURI(const std::string& arg1)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.ResourceMissingAtURI"},
        {"Message", "The resource at the URI " + arg1 + " was not found."},
        {"MessageArgs", {arg1}},
        {"Severity", "Critical"},
        {"Resolution", "Place a valid resource at the URI or correct the "
                       "URI and resubmit the request."}};
}

void resourceMissingAtURI(crow::Response& res, const std::string& arg1)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, resourceMissingAtURI(arg1));
}

/**
 * @internal
 * @brief Formats ActionParameterValueFormatError message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json actionParameterValueFormatError(const std::string& arg1,
                                               const std::string& arg2,
                                               const std::string& arg3)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.ActionParameterValueFormatError"},
        {"Message",
         "The value " + arg1 + " for the parameter " + arg2 +
             " in the action " + arg3 +
             " is of a different format than the parameter can accept."},
        {"MessageArgs", {arg1, arg2, arg3}},
        {"Severity", "Warning"},
        {"Resolution",
         "Correct the value for the parameter in the request body and "
         "resubmit the request if the operation failed."}};
}

void actionParameterValueFormatError(crow::Response& res,
                                     const std::string& arg1,
                                     const std::string& arg2,
                                     const std::string& arg3)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue,
                          actionParameterValueFormatError(arg1, arg2, arg3));
}

/**
 * @internal
 * @brief Formats InternalError message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json internalError(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.InternalError"},
        {"Message", "The request failed due to an internal service error.  "
                    "The service is still operational."},
        {"MessageArgs", nlohmann::json::array()},
        {"Severity", "Critical"},
        {"Resolution", "Resubmit the request.  If the problem persists, "
                       "consider resetting the service."}};
}

void internalError(crow::Response& res)
{
    res.result(boost::beast::http::status::internal_server_error);
    addMessageToErrorJson(res.jsonValue, internalError());
}

/**
 * @internal
 * @brief Formats UnrecognizedRequestBody message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json unrecognizedRequestBody(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.UnrecognizedRequestBody"},
        {"Message", "The service detected a malformed request body that it "
                    "was unable to interpret."},
        {"MessageArgs", nlohmann::json::array()},
        {"Severity", "Warning"},
        {"Resolution", "Correct the request body and resubmit the request "
                       "if it failed."}};
}

void unrecognizedRequestBody(crow::Response& res)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, unrecognizedRequestBody());
}

/**
 * @internal
 * @brief Formats ResourceAtUriUnauthorized message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourceAtUriUnauthorized(const std::string& arg1,
                                         const std::string& arg2)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.ResourceAtUriUnauthorized"},
        {"Message", "While accessing the resource at " + arg1 +
                        ", the service received an authorization error " +
                        arg2 + "."},
        {"MessageArgs", {arg1, arg2}},
        {"Severity", "Critical"},
        {"Resolution", "Ensure that the appropriate access is provided for "
                       "the service in order for it to access the URI."}};
}

void resourceAtUriUnauthorized(crow::Response& res, const std::string& arg1,
                               const std::string& arg2)
{
    res.result(boost::beast::http::status::unauthorized);
    addMessageToErrorJson(res.jsonValue, resourceAtUriUnauthorized(arg1, arg2));
}

/**
 * @internal
 * @brief Formats ActionParameterUnknown message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json actionParameterUnknown(const std::string& arg1,
                                      const std::string& arg2)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.ActionParameterUnknown"},
        {"Message", "The action " + arg1 +
                        " was submitted with the invalid parameter " + arg2 +
                        "."},
        {"MessageArgs", {arg1, arg2}},
        {"Severity", "Warning"},
        {"Resolution", "Correct the invalid parameter and resubmit the "
                       "request if the operation failed."}};
}

void actionParameterUnknown(crow::Response& res, const std::string& arg1,
                            const std::string& arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, actionParameterUnknown(arg1, arg2));
}

/**
 * @internal
 * @brief Formats ResourceCannotBeDeleted message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourceCannotBeDeleted(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.ResourceCannotBeDeleted"},
        {"Message", "The delete request failed because the resource "
                    "requested cannot be deleted."},
        {"MessageArgs", nlohmann::json::array()},
        {"Severity", "Critical"},
        {"Resolution", "Do not attempt to delete a non-deletable resource."}};
}

void resourceCannotBeDeleted(crow::Response& res)
{
    res.result(boost::beast::http::status::forbidden);
    addMessageToErrorJson(res.jsonValue, resourceCannotBeDeleted());
}

/**
 * @internal
 * @brief Formats PropertyDuplicate message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json propertyDuplicate(const std::string& arg1)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.PropertyDuplicate"},
        {"Message", "The property " + arg1 + " was duplicated in the request."},
        {"MessageArgs", {arg1}},
        {"Severity", "Warning"},
        {"Resolution",
         "Remove the duplicate property from the request body and resubmit "
         "the request if the operation failed."}};
}

void propertyDuplicate(crow::Response& res, const std::string& arg1)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToJson(res.jsonValue, propertyDuplicate(arg1), arg1);
}

/**
 * @internal
 * @brief Formats ServiceTemporarilyUnavailable message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json serviceTemporarilyUnavailable(const std::string& arg1)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.ServiceTemporarilyUnavailable"},
        {"Message", "The service is temporarily unavailable.  Retry in " +
                        arg1 + " seconds."},
        {"MessageArgs", {arg1}},
        {"Severity", "Critical"},
        {"Resolution", "Wait for the indicated retry duration and retry "
                       "the operation."}};
}

void serviceTemporarilyUnavailable(crow::Response& res, const std::string& arg1)
{
    res.addHeader("Retry-After", arg1);
    res.result(boost::beast::http::status::service_unavailable);
    addMessageToErrorJson(res.jsonValue, serviceTemporarilyUnavailable(arg1));
}

/**
 * @internal
 * @brief Formats ResourceAlreadyExists message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourceAlreadyExists(const std::string& arg1,
                                     const std::string& arg2,
                                     const std::string& arg3)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.ResourceAlreadyExists"},
        {"Message", "The requested resource of type " + arg1 +
                        " with the property " + arg2 + " with the value " +
                        arg3 + " already exists."},
        {"MessageArgs", {arg1, arg2, arg3}},
        {"Severity", "Critical"},
        {"Resolution", "Do not repeat the create operation as the resource "
                       "has already been created."}};
}

void resourceAlreadyExists(crow::Response& res, const std::string& arg1,
                           const std::string& arg2, const std::string& arg3)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToJson(res.jsonValue, resourceAlreadyExists(arg1, arg2, arg3),
                     arg2);
}

/**
 * @internal
 * @brief Formats AccountForSessionNoLongerExists message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json accountForSessionNoLongerExists(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.AccountForSessionNoLongerExists"},
        {"Message", "The account for the current session has been removed, "
                    "thus the current session has been removed as well."},
        {"MessageArgs", nlohmann::json::array()},
        {"Severity", "OK"},
        {"Resolution", "Attempt to connect with a valid account."}};
}

void accountForSessionNoLongerExists(crow::Response& res)
{
    res.result(boost::beast::http::status::forbidden);
    addMessageToErrorJson(res.jsonValue, accountForSessionNoLongerExists());
}

/**
 * @internal
 * @brief Formats CreateFailedMissingReqProperties message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json createFailedMissingReqProperties(const std::string& arg1)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.CreateFailedMissingReqProperties"},
        {"Message",
         "The create operation failed because the required property " + arg1 +
             " was missing from the request."},
        {"MessageArgs", {arg1}},
        {"Severity", "Critical"},
        {"Resolution",
         "Correct the body to include the required property with a valid "
         "value and resubmit the request if the operation failed."}};
}

void createFailedMissingReqProperties(crow::Response& res,
                                      const std::string& arg1)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToJson(res.jsonValue, createFailedMissingReqProperties(arg1),
                     arg1);
}

/**
 * @internal
 * @brief Formats PropertyValueFormatError message into JSON for the specified
 * property
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json propertyValueFormatError(const std::string& arg1,
                                        const std::string& arg2)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.PropertyValueFormatError"},
        {"Message",
         "The value " + arg1 + " for the property " + arg2 +
             " is of a different format than the property can accept."},
        {"MessageArgs", {arg1, arg2}},
        {"Severity", "Warning"},
        {"Resolution",
         "Correct the value for the property in the request body and "
         "resubmit the request if the operation failed."}};
}

void propertyValueFormatError(crow::Response& res, const std::string& arg1,
                              const std::string& arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToJson(res.jsonValue, propertyValueFormatError(arg1, arg2), arg2);
}

/**
 * @internal
 * @brief Formats PropertyValueNotInList message into JSON for the specified
 * property
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json propertyValueNotInList(const std::string& arg1,
                                      const std::string& arg2)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.PropertyValueNotInList"},
        {"Message", "The value " + arg1 + " for the property " + arg2 +
                        " is not in the list of acceptable values."},
        {"MessageArgs", {arg1, arg2}},
        {"Severity", "Warning"},
        {"Resolution", "Choose a value from the enumeration list that "
                       "the implementation "
                       "can support and resubmit the request if the "
                       "operation failed."}};
}

void propertyValueNotInList(crow::Response& res, const std::string& arg1,
                            const std::string& arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToJson(res.jsonValue, propertyValueNotInList(arg1, arg2), arg2);
}

/**
 * @internal
 * @brief Formats ResourceAtUriInUnknownFormat message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourceAtUriInUnknownFormat(const std::string& arg1)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.ResourceAtUriInUnknownFormat"},
        {"Message", "The resource at " + arg1 +
                        " is in a format not recognized by the service."},
        {"MessageArgs", {arg1}},
        {"Severity", "Critical"},
        {"Resolution", "Place an image or resource or file that is "
                       "recognized by the service at the URI."}};
}

void resourceAtUriInUnknownFormat(crow::Response& res, const std::string& arg1)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, resourceAtUriInUnknownFormat(arg1));
}

/**
 * @internal
 * @brief Formats ServiceInUnknownState message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json serviceInUnknownState(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.ServiceInUnknownState"},
        {"Message",
         "The operation failed because the service is in an unknown state "
         "and can no longer take incoming requests."},
        {"MessageArgs", nlohmann::json::array()},
        {"Severity", "Critical"},
        {"Resolution", "Restart the service and resubmit the request if "
                       "the operation failed."}};
}

void serviceInUnknownState(crow::Response& res)
{
    res.result(boost::beast::http::status::service_unavailable);
    addMessageToErrorJson(res.jsonValue, serviceInUnknownState());
}

/**
 * @internal
 * @brief Formats EventSubscriptionLimitExceeded message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json eventSubscriptionLimitExceeded(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.EventSubscriptionLimitExceeded"},
        {"Message",
         "The event subscription failed due to the number of simultaneous "
         "subscriptions exceeding the limit of the implementation."},
        {"MessageArgs", nlohmann::json::array()},
        {"Severity", "Critical"},
        {"Resolution",
         "Reduce the number of other subscriptions before trying to "
         "establish the event subscription or increase the limit of "
         "simultaneous subscriptions (if supported)."}};
}

void eventSubscriptionLimitExceeded(crow::Response& res)
{
    res.result(boost::beast::http::status::forbidden);
    addMessageToErrorJson(res.jsonValue, eventSubscriptionLimitExceeded());
}

/**
 * @internal
 * @brief Formats ActionParameterMissing message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json actionParameterMissing(const std::string& arg1,
                                      const std::string& arg2)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.ActionParameterMissing"},
        {"Message", "The action " + arg1 + " requires the parameter " + arg2 +
                        " to be present in the request body."},
        {"MessageArgs", {arg1, arg2}},
        {"Severity", "Critical"},
        {"Resolution",
         "Supply the action with the required parameter in the request "
         "body when the request is resubmitted."}};
}

void actionParameterMissing(crow::Response& res, const std::string& arg1,
                            const std::string& arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, actionParameterMissing(arg1, arg2));
}

/**
 * @internal
 * @brief Formats StringValueTooLong message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json stringValueTooLong(const std::string& arg1, const int& arg2)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.StringValueTooLong"},
        {"Message", "The string " + arg1 + " exceeds the length limit " +
                        std::to_string(arg2) + "."},
        {"MessageArgs", {arg1, std::to_string(arg2)}},
        {"Severity", "Warning"},
        {"Resolution",
         "Resubmit the request with an appropriate string length."}};
}

void stringValueTooLong(crow::Response& res, const std::string& arg1,
                        const int& arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, stringValueTooLong(arg1, arg2));
}

/**
 * @internal
 * @brief Formats SessionTerminated message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json sessionTerminated(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.SessionTerminated"},
        {"Message", "The session was successfully terminated."},
        {"MessageArgs", nlohmann::json::array()},
        {"Severity", "OK"},
        {"Resolution", "No resolution is required."}};
}

void sessionTerminated(crow::Response& res)
{
    res.result(boost::beast::http::status::ok);
    addMessageToJsonRoot(res.jsonValue, sessionTerminated());
}

/**
 * @internal
 * @brief Formats ResourceTypeIncompatible message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourceTypeIncompatible(const std::string& arg1,
                                        const std::string& arg2)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.ResourceTypeIncompatible"},
        {"Message", "The @odata.type of the request body " + arg1 +
                        " is incompatible with the @odata.type of the "
                        "resource which is " +
                        arg2 + "."},
        {"MessageArgs", {arg1, arg2}},
        {"Severity", "Critical"},
        {"Resolution", "Resubmit the request with a payload compatible "
                       "with the resource's schema."}};
}

void resourceTypeIncompatible(crow::Response& res, const std::string& arg1,
                              const std::string& arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, resourceTypeIncompatible(arg1, arg2));
}

/**
 * @internal
 * @brief Formats PropertyValueTypeError message into JSON for the specified
 * property
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json propertyValueTypeError(const std::string& arg1,
                                      const std::string& arg2)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.PropertyValueTypeError"},
        {"Message",
         "The value " + arg1 + " for the property " + arg2 +
             " is of a different type than the property can accept."},
        {"MessageArgs", {arg1, arg2}},
        {"Severity", "Warning"},
        {"Resolution",
         "Correct the value for the property in the request body and "
         "resubmit the request if the operation failed."}};
}

void propertyValueTypeError(crow::Response& res, const std::string& arg1,
                            const std::string& arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToJson(res.jsonValue, propertyValueTypeError(arg1, arg2), arg2);
}

/**
 * @internal
 * @brief Formats ResourceNotFound message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourceNotFound(const std::string& arg1,
                                const std::string& arg2)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.ResourceNotFound"},
        {"Message", "The requested resource of type " + arg1 + " named " +
                        arg2 + " was not found."},
        {"MessageArgs", {arg1, arg2}},
        {"Severity", "Critical"},
        {"Resolution",
         "Provide a valid resource identifier and resubmit the request."}};
}

void resourceNotFound(crow::Response& res, const std::string& arg1,
                      const std::string& arg2)
{
    res.result(boost::beast::http::status::not_found);
    addMessageToErrorJson(res.jsonValue, resourceNotFound(arg1, arg2));
}

/**
 * @internal
 * @brief Formats CouldNotEstablishConnection message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json couldNotEstablishConnection(const std::string& arg1)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.CouldNotEstablishConnection"},
        {"Message",
         "The service failed to establish a Connection with the URI " + arg1 +
             "."},
        {"MessageArgs", {arg1}},
        {"Severity", "Critical"},
        {"Resolution",
         "Ensure that the URI contains a valid and reachable node name, "
         "protocol information and other URI components."}};
}

void couldNotEstablishConnection(crow::Response& res, const std::string& arg1)
{
    res.result(boost::beast::http::status::not_found);
    addMessageToErrorJson(res.jsonValue, couldNotEstablishConnection(arg1));
}

/**
 * @internal
 * @brief Formats PropertyNotWritable message into JSON for the specified
 * property
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json propertyNotWritable(const std::string& arg1)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.PropertyNotWritable"},
        {"Message", "The property " + arg1 +
                        " is a read only property and cannot be "
                        "assigned a value."},
        {"MessageArgs", {arg1}},
        {"Severity", "Warning"},
        {"Resolution", "Remove the property from the request body and "
                       "resubmit the request if the operation failed."}};
}

void propertyNotWritable(crow::Response& res, const std::string& arg1)
{
    res.result(boost::beast::http::status::forbidden);
    addMessageToJson(res.jsonValue, propertyNotWritable(arg1), arg1);
}

/**
 * @internal
 * @brief Formats QueryParameterValueTypeError message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json queryParameterValueTypeError(const std::string& arg1,
                                            const std::string& arg2)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.QueryParameterValueTypeError"},
        {"Message",
         "The value " + arg1 + " for the query parameter " + arg2 +
             " is of a different type than the parameter can accept."},
        {"MessageArgs", {arg1, arg2}},
        {"Severity", "Warning"},
        {"Resolution",
         "Correct the value for the query parameter in the request and "
         "resubmit the request if the operation failed."}};
}

void queryParameterValueTypeError(crow::Response& res, const std::string& arg1,
                                  const std::string& arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue,
                          queryParameterValueTypeError(arg1, arg2));
}

/**
 * @internal
 * @brief Formats ServiceShuttingDown message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json serviceShuttingDown(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.ServiceShuttingDown"},
        {"Message", "The operation failed because the service is shutting "
                    "down and can no longer take incoming requests."},
        {"MessageArgs", nlohmann::json::array()},
        {"Severity", "Critical"},
        {"Resolution", "When the service becomes available, resubmit the "
                       "request if the operation failed."}};
}

void serviceShuttingDown(crow::Response& res)
{
    res.result(boost::beast::http::status::service_unavailable);
    addMessageToErrorJson(res.jsonValue, serviceShuttingDown());
}

/**
 * @internal
 * @brief Formats ActionParameterDuplicate message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json actionParameterDuplicate(const std::string& arg1,
                                        const std::string& arg2)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.ActionParameterDuplicate"},
        {"Message",
         "The action " + arg1 +
             " was submitted with more than one value for the parameter " +
             arg2 + "."},
        {"MessageArgs", {arg1, arg2}},
        {"Severity", "Warning"},
        {"Resolution",
         "Resubmit the action with only one instance of the parameter in "
         "the request body if the operation failed."}};
}

void actionParameterDuplicate(crow::Response& res, const std::string& arg1,
                              const std::string& arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, actionParameterDuplicate(arg1, arg2));
}

/**
 * @internal
 * @brief Formats ActionParameterNotSupported message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json actionParameterNotSupported(const std::string& arg1,
                                           const std::string& arg2)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.ActionParameterNotSupported"},
        {"Message", "The parameter " + arg1 + " for the action " + arg2 +
                        " is not supported on the target resource."},
        {"MessageArgs", {arg1, arg2}},
        {"Severity", "Warning"},
        {"Resolution", "Remove the parameter supplied and resubmit the "
                       "request if the operation failed."}};
}

void actionParameterNotSupported(crow::Response& res, const std::string& arg1,
                                 const std::string& arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue,
                          actionParameterNotSupported(arg1, arg2));
}

/**
 * @internal
 * @brief Formats SourceDoesNotSupportProtocol message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json sourceDoesNotSupportProtocol(const std::string& arg1,
                                            const std::string& arg2)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.SourceDoesNotSupportProtocol"},
        {"Message", "The other end of the Connection at " + arg1 +
                        " does not support the specified protocol " + arg2 +
                        "."},
        {"MessageArgs", {arg1, arg2}},
        {"Severity", "Critical"},
        {"Resolution", "Change protocols or URIs. "}};
}

void sourceDoesNotSupportProtocol(crow::Response& res, const std::string& arg1,
                                  const std::string& arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue,
                          sourceDoesNotSupportProtocol(arg1, arg2));
}

/**
 * @internal
 * @brief Formats AccountRemoved message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json accountRemoved(void)
{
    return nlohmann::json{{"@odata.type", "#Message.v1_0_0.Message"},
                          {"MessageId", "Base.1.4.0.AccountRemoved"},
                          {"Message", "The account was successfully removed."},
                          {"MessageArgs", nlohmann::json::array()},
                          {"Severity", "OK"},
                          {"Resolution", "No resolution is required."}};
}

void accountRemoved(crow::Response& res)
{
    res.result(boost::beast::http::status::ok);
    addMessageToJsonRoot(res.jsonValue, accountRemoved());
}

/**
 * @internal
 * @brief Formats AccessDenied message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json accessDenied(const std::string& arg1)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.AccessDenied"},
        {"Message", "While attempting to establish a Connection to " + arg1 +
                        ", the service denied access."},
        {"MessageArgs", {arg1}},
        {"Severity", "Critical"},
        {"Resolution", "Attempt to ensure that the URI is correct and that "
                       "the service has the appropriate credentials."}};
}

void accessDenied(crow::Response& res, const std::string& arg1)
{
    res.result(boost::beast::http::status::forbidden);
    addMessageToErrorJson(res.jsonValue, accessDenied(arg1));
}

/**
 * @internal
 * @brief Formats QueryNotSupported message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json queryNotSupported(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.QueryNotSupported"},
        {"Message", "Querying is not supported by the implementation."},
        {"MessageArgs", nlohmann::json::array()},
        {"Severity", "Warning"},
        {"Resolution", "Remove the query parameters and resubmit the "
                       "request if the operation failed."}};
}

void queryNotSupported(crow::Response& res)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, queryNotSupported());
}

/**
 * @internal
 * @brief Formats CreateLimitReachedForResource message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json createLimitReachedForResource(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.CreateLimitReachedForResource"},
        {"Message", "The create operation failed because the resource has "
                    "reached the limit of possible resources."},
        {"MessageArgs", nlohmann::json::array()},
        {"Severity", "Critical"},
        {"Resolution",
         "Either delete resources and resubmit the request if the "
         "operation failed or do not resubmit the request."}};
}

void createLimitReachedForResource(crow::Response& res)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, createLimitReachedForResource());
}

/**
 * @internal
 * @brief Formats GeneralError message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json generalError(void)
{
    return nlohmann::json{{"@odata.type", "#Message.v1_0_0.Message"},
                          {"MessageId", "Base.1.4.0.GeneralError"},
                          {"Message",
                           "A general error has occurred. See Resolution for "
                           "information on how to resolve the error."},
                          {"MessageArgs", nlohmann::json::array()},
                          {"Severity", "Critical"},
                          {"Resolution", "None."}};
}

void generalError(crow::Response& res)
{
    res.result(boost::beast::http::status::internal_server_error);
    addMessageToErrorJson(res.jsonValue, generalError());
}

/**
 * @internal
 * @brief Formats Success message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json success(void)
{
    return nlohmann::json{{"@odata.type", "#Message.v1_0_0.Message"},
                          {"MessageId", "Base.1.4.0.Success"},
                          {"Message", "Successfully Completed Request"},
                          {"MessageArgs", nlohmann::json::array()},
                          {"Severity", "OK"},
                          {"Resolution", "None"}};
}

void success(crow::Response& res)
{
    // don't set res.result here because success is the default and any
    // error should overwrite the default
    addMessageToJsonRoot(res.jsonValue, success());
}

/**
 * @internal
 * @brief Formats Created message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json created(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.Created"},
        {"Message", "The resource has been created successfully"},
        {"MessageArgs", nlohmann::json::array()},
        {"Severity", "OK"},
        {"Resolution", "None"}};
}

void created(crow::Response& res)
{
    res.result(boost::beast::http::status::created);
    addMessageToJsonRoot(res.jsonValue, created());
}

/**
 * @internal
 * @brief Formats NoOperation message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json noOperation(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.NoOperation"},
        {"Message", "The request body submitted contain no data to act "
                    "upon and no changes to the resource took place."},
        {"MessageArgs", nlohmann::json::array()},
        {"Severity", "Warning"},
        {"Resolution",
         "Add properties in the JSON object and resubmit the request."}};
}

void noOperation(crow::Response& res)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, noOperation());
}

/**
 * @internal
 * @brief Formats PropertyUnknown message into JSON for the specified
 * property
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json propertyUnknown(const std::string& arg1)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.PropertyUnknown"},
        {"Message", "The property " + arg1 +
                        " is not in the list of valid properties for "
                        "the resource."},
        {"MessageArgs", {arg1}},
        {"Severity", "Warning"},
        {"Resolution", "Remove the unknown property from the request "
                       "body and resubmit "
                       "the request if the operation failed."}};
}

void propertyUnknown(crow::Response& res, const std::string& arg1)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToJson(res.jsonValue, propertyUnknown(arg1), arg1);
}

/**
 * @internal
 * @brief Formats NoValidSession message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json noValidSession(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.NoValidSession"},
        {"Message",
         "There is no valid session established with the implementation."},
        {"MessageArgs", nlohmann::json::array()},
        {"Severity", "Critical"},
        {"Resolution",
         "Establish as session before attempting any operations."}};
}

void noValidSession(crow::Response& res)
{
    res.result(boost::beast::http::status::forbidden);
    addMessageToErrorJson(res.jsonValue, noValidSession());
}

/**
 * @internal
 * @brief Formats InvalidObject message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json invalidObject(const std::string& arg1)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.InvalidObject"},
        {"Message", "The object at " + arg1 + " is invalid."},
        {"MessageArgs", {arg1}},
        {"Severity", "Critical"},
        {"Resolution",
         "Either the object is malformed or the URI is not correct.  "
         "Correct the condition and resubmit the request if it failed."}};
}

void invalidObject(crow::Response& res, const std::string& arg1)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, invalidObject(arg1));
}

/**
 * @internal
 * @brief Formats ResourceInStandby message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourceInStandby(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.ResourceInStandby"},
        {"Message", "The request could not be performed because the "
                    "resource is in standby."},
        {"MessageArgs", nlohmann::json::array()},
        {"Severity", "Critical"},
        {"Resolution", "Ensure that the resource is in the correct power "
                       "state and resubmit the request."}};
}

void resourceInStandby(crow::Response& res)
{
    res.result(boost::beast::http::status::service_unavailable);
    addMessageToErrorJson(res.jsonValue, resourceInStandby());
}

/**
 * @internal
 * @brief Formats ActionParameterValueTypeError message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json actionParameterValueTypeError(const std::string& arg1,
                                             const std::string& arg2,
                                             const std::string& arg3)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.ActionParameterValueTypeError"},
        {"Message",
         "The value " + arg1 + " for the parameter " + arg2 +
             " in the action " + arg3 +
             " is of a different type than the parameter can accept."},
        {"MessageArgs", {arg1, arg2, arg3}},
        {"Severity", "Warning"},
        {"Resolution",
         "Correct the value for the parameter in the request body and "
         "resubmit the request if the operation failed."}};
}

void actionParameterValueTypeError(crow::Response& res, const std::string& arg1,
                                   const std::string& arg2,
                                   const std::string& arg3)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue,
                          actionParameterValueTypeError(arg1, arg2, arg3));
}

/**
 * @internal
 * @brief Formats SessionLimitExceeded message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json sessionLimitExceeded(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.SessionLimitExceeded"},
        {"Message", "The session establishment failed due to the number of "
                    "simultaneous sessions exceeding the limit of the "
                    "implementation."},
        {"MessageArgs", nlohmann::json::array()},
        {"Severity", "Critical"},
        {"Resolution", "Reduce the number of other sessions before trying "
                       "to establish the session or increase the limit of "
                       "simultaneous sessions (if supported)."}};
}

void sessionLimitExceeded(crow::Response& res)
{
    res.result(boost::beast::http::status::service_unavailable);
    addMessageToErrorJson(res.jsonValue, sessionLimitExceeded());
}

/**
 * @internal
 * @brief Formats ActionNotSupported message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json actionNotSupported(const std::string& arg1)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.ActionNotSupported"},
        {"Message",
         "The action " + arg1 + " is not supported by the resource."},
        {"MessageArgs", {arg1}},
        {"Severity", "Critical"},
        {"Resolution",
         "The action supplied cannot be resubmitted to the implementation. "
         " Perhaps the action was invalid, the wrong resource was the "
         "target or the implementation documentation may be of "
         "assistance."}};
}

void actionNotSupported(crow::Response& res, const std::string& arg1)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, actionNotSupported(arg1));
}

/**
 * @internal
 * @brief Formats InvalidIndex message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json invalidIndex(const int& arg1)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.InvalidIndex"},
        {"Message", "The index " + std::to_string(arg1) +
                        " is not a valid offset into the array."},
        {"MessageArgs", {std::to_string(arg1)}},
        {"Severity", "Warning"},
        {"Resolution", "Verify the index value provided is within the "
                       "bounds of the array."}};
}

void invalidIndex(crow::Response& res, const int& arg1)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, invalidIndex(arg1));
}

/**
 * @internal
 * @brief Formats EmptyJSON message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json emptyJSON(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.EmptyJSON"},
        {"Message", "The request body submitted contained an empty JSON "
                    "object and the service is unable to process it."},
        {"MessageArgs", nlohmann::json::array()},
        {"Severity", "Warning"},
        {"Resolution",
         "Add properties in the JSON object and resubmit the request."}};
}

void emptyJSON(crow::Response& res)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, emptyJSON());
}

/**
 * @internal
 * @brief Formats QueryNotSupportedOnResource message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json queryNotSupportedOnResource(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.QueryNotSupportedOnResource"},
        {"Message", "Querying is not supported on the requested resource."},
        {"MessageArgs", nlohmann::json::array()},
        {"Severity", "Warning"},
        {"Resolution", "Remove the query parameters and resubmit the "
                       "request if the operation failed."}};
}

void queryNotSupportedOnResource(crow::Response& res)
{
    res.result(boost::beast::http::status::forbidden);
    addMessageToErrorJson(res.jsonValue, queryNotSupportedOnResource());
}

/**
 * @internal
 * @brief Formats InsufficientPrivilege message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json insufficientPrivilege(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.InsufficientPrivilege"},
        {"Message", "There are insufficient privileges for the account or "
                    "credentials associated with the current session to "
                    "perform the requested operation."},
        {"MessageArgs", nlohmann::json::array()},
        {"Severity", "Critical"},
        {"Resolution",
         "Either abandon the operation or change the associated access "
         "rights and resubmit the request if the operation failed."}};
}

void insufficientPrivilege(crow::Response& res)
{
    res.result(boost::beast::http::status::forbidden);
    addMessageToErrorJson(res.jsonValue, insufficientPrivilege());
}

/**
 * @internal
 * @brief Formats PropertyValueModified message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json propertyValueModified(const std::string& arg1,
                                     const std::string& arg2)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.PropertyValueModified"},
        {"Message", "The property " + arg1 + " was assigned the value " + arg2 +
                        " due to modification by the service."},
        {"MessageArgs", {arg1, arg2}},
        {"Severity", "Warning"},
        {"Resolution", "No resolution is required."}};
}

void propertyValueModified(crow::Response& res, const std::string& arg1,
                           const std::string& arg2)
{
    res.result(boost::beast::http::status::ok);
    addMessageToJson(res.jsonValue, propertyValueModified(arg1, arg2), arg1);
}

/**
 * @internal
 * @brief Formats AccountNotModified message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json accountNotModified(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.AccountNotModified"},
        {"Message", "The account modification request failed."},
        {"MessageArgs", nlohmann::json::array()},
        {"Severity", "Warning"},
        {"Resolution", "The modification may have failed due to permission "
                       "issues or issues with the request body."}};
}

void accountNotModified(crow::Response& res)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, accountNotModified());
}

/**
 * @internal
 * @brief Formats QueryParameterValueFormatError message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json queryParameterValueFormatError(const std::string& arg1,
                                              const std::string& arg2)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.QueryParameterValueFormatError"},
        {"Message",
         "The value " + arg1 + " for the parameter " + arg2 +
             " is of a different format than the parameter can accept."},
        {"MessageArgs", {arg1, arg2}},
        {"Severity", "Warning"},
        {"Resolution",
         "Correct the value for the query parameter in the request and "
         "resubmit the request if the operation failed."}};
}

void queryParameterValueFormatError(crow::Response& res,
                                    const std::string& arg1,
                                    const std::string& arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue,
                          queryParameterValueFormatError(arg1, arg2));
}

/**
 * @internal
 * @brief Formats PropertyMissing message into JSON for the specified
 * property
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json propertyMissing(const std::string& arg1)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.PropertyMissing"},
        {"Message", "The property " + arg1 +
                        " is a required property and must be included in "
                        "the request."},
        {"MessageArgs", {arg1}},
        {"Severity", "Warning"},
        {"Resolution",
         "Ensure that the property is in the request body and has a "
         "valid "
         "value and resubmit the request if the operation failed."}};
}

void propertyMissing(crow::Response& res, const std::string& arg1)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToJson(res.jsonValue, propertyMissing(arg1), arg1);
}

/**
 * @internal
 * @brief Formats ResourceExhaustion message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourceExhaustion(const std::string& arg1)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.ResourceExhaustion"},
        {"Message", "The resource " + arg1 +
                        " was unable to satisfy the request due to "
                        "unavailability of resources."},
        {"MessageArgs", {arg1}},
        {"Severity", "Critical"},
        {"Resolution", "Ensure that the resources are available and "
                       "resubmit the request."}};
}

void resourceExhaustion(crow::Response& res, const std::string& arg1)
{
    res.result(boost::beast::http::status::service_unavailable);
    addMessageToErrorJson(res.jsonValue, resourceExhaustion(arg1));
}

/**
 * @internal
 * @brief Formats AccountModified message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json accountModified(void)
{
    return nlohmann::json{{"@odata.type", "#Message.v1_0_0.Message"},
                          {"MessageId", "Base.1.4.0.AccountModified"},
                          {"Message", "The account was successfully modified."},
                          {"MessageArgs", nlohmann::json::array()},
                          {"Severity", "OK"},
                          {"Resolution", "No resolution is required."}};
}

void accountModified(crow::Response& res)
{
    res.result(boost::beast::http::status::ok);
    addMessageToErrorJson(res.jsonValue, accountModified());
}

/**
 * @internal
 * @brief Formats QueryParameterOutOfRange message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json queryParameterOutOfRange(const std::string& arg1,
                                        const std::string& arg2,
                                        const std::string& arg3)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "Base.1.4.0.QueryParameterOutOfRange"},
        {"Message", "The value " + arg1 + " for the query parameter " + arg2 +
                        " is out of range " + arg3 + "."},
        {"MessageArgs", {arg1, arg2, arg3}},
        {"Severity", "Warning"},
        {"Resolution",
         "Reduce the value for the query parameter to a value that is "
         "within range, such as a start or count value that is within "
         "bounds of the number of resources in a collection or a page that "
         "is within the range of valid pages."}};
}

void queryParameterOutOfRange(crow::Response& res, const std::string& arg1,
                              const std::string& arg2, const std::string& arg3)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue,
                          queryParameterOutOfRange(arg1, arg2, arg3));
}

/**
 * @internal
 * @brief Formats PasswordChangeRequired message into JSON
 *
 * See header file for more information
 * @endinternal
 */
void passwordChangeRequired(crow::Response& res, const std::string& arg1)
{
    messages::addMessageToJsonRoot(
        res.jsonValue,
        nlohmann::json{
            {"@odata.type", "/redfish/v1/$metadata#Message.v1_5_0.Message"},
            {"MessageId", "Base.1.5.0.PasswordChangeRequired"},
            {"Message", "The password provided for this account must be "
                        "changed before access is granted.  PATCH the "
                        "'Password' property for this account located at "
                        "the target URI '" +
                            arg1 + "' to complete this process."},
            {"MessageArgs", {arg1}},
            {"Severity", "Critical"},
            {"Resolution", "Change the password for this account using "
                           "a PATCH to the 'Password' property at the URI "
                           "provided."}});
}

} // namespace messages

} // namespace redfish
