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
#include <error_messages.hpp>
#include <logging.hpp>

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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.ResourceInUse"},
        {"Message", "The change to the requested resource failed because "
                    "the resource is in use or in transition."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "Warning"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.MalformedJSON"},
        {"Message", "The request body submitted was malformed JSON and "
                    "could not be parsed by the receiving service."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "Critical"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.ResourceMissingAtURI"},
        {"Message", "The resource at the URI " + arg1 + " was not found."},
        {"MessageArgs", {arg1}},
        {"MessageSeverity", "Critical"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.ActionParameterValueFormatError"},
        {"Message",
         "The value " + arg1 + " for the parameter " + arg2 +
             " in the action " + arg3 +
             " is of a different format than the parameter can accept."},
        {"MessageArgs", {arg1, arg2, arg3}},
        {"MessageSeverity", "Warning"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.InternalError"},
        {"Message", "The request failed due to an internal service error.  "
                    "The service is still operational."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "Critical"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.UnrecognizedRequestBody"},
        {"Message", "The service detected a malformed request body that it "
                    "was unable to interpret."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "Warning"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.ResourceAtUriUnauthorized"},
        {"Message", "While accessing the resource at " + arg1 +
                        ", the service received an authorization error " +
                        arg2 + "."},
        {"MessageArgs", {arg1, arg2}},
        {"MessageSeverity", "Critical"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.ActionParameterUnknown"},
        {"Message", "The action " + arg1 +
                        " was submitted with the invalid parameter " + arg2 +
                        "."},
        {"MessageArgs", {arg1, arg2}},
        {"MessageSeverity", "Warning"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.ResourceCannotBeDeleted"},
        {"Message", "The delete request failed because the resource "
                    "requested cannot be deleted."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "Critical"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.PropertyDuplicate"},
        {"Message", "The property " + arg1 + " was duplicated in the request."},
        {"MessageArgs", {arg1}},
        {"MessageSeverity", "Warning"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.ServiceTemporarilyUnavailable"},
        {"Message", "The service is temporarily unavailable.  Retry in " +
                        arg1 + " seconds."},
        {"MessageArgs", {arg1}},
        {"MessageSeverity", "Critical"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.ResourceAlreadyExists"},
        {"Message", "The requested resource of type " + arg1 +
                        " with the property " + arg2 + " with the value " +
                        arg3 + " already exists."},
        {"MessageArgs", {arg1, arg2, arg3}},
        {"MessageSeverity", "Critical"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.AccountForSessionNoLongerExists"},
        {"Message", "The account for the current session has been removed, "
                    "thus the current session has been removed as well."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "OK"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.CreateFailedMissingReqProperties"},
        {"Message",
         "The create operation failed because the required property " + arg1 +
             " was missing from the request."},
        {"MessageArgs", {arg1}},
        {"MessageSeverity", "Critical"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.PropertyValueFormatError"},
        {"Message",
         "The value " + arg1 + " for the property " + arg2 +
             " is of a different format than the property can accept."},
        {"MessageArgs", {arg1, arg2}},
        {"MessageSeverity", "Warning"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.PropertyValueNotInList"},
        {"Message", "The value " + arg1 + " for the property " + arg2 +
                        " is not in the list of acceptable values."},
        {"MessageArgs", {arg1, arg2}},
        {"MessageSeverity", "Warning"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.ResourceAtUriInUnknownFormat"},
        {"Message", "The resource at " + arg1 +
                        " is in a format not recognized by the service."},
        {"MessageArgs", {arg1}},
        {"MessageSeverity", "Critical"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.ServiceInUnknownState"},
        {"Message",
         "The operation failed because the service is in an unknown state "
         "and can no longer take incoming requests."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "Critical"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.EventSubscriptionLimitExceeded"},
        {"Message",
         "The event subscription failed due to the number of simultaneous "
         "subscriptions exceeding the limit of the implementation."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "Critical"},
        {"Resolution",
         "Reduce the number of other subscriptions before trying to "
         "establish the event subscription or increase the limit of "
         "simultaneous subscriptions (if supported)."}};
}

void eventSubscriptionLimitExceeded(crow::Response& res)
{
    res.result(boost::beast::http::status::service_unavailable);
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.ActionParameterMissing"},
        {"Message", "The action " + arg1 + " requires the parameter " + arg2 +
                        " to be present in the request body."},
        {"MessageArgs", {arg1, arg2}},
        {"MessageSeverity", "Critical"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.StringValueTooLong"},
        {"Message", "The string " + arg1 + " exceeds the length limit " +
                        std::to_string(arg2) + "."},
        {"MessageArgs", {arg1, std::to_string(arg2)}},
        {"MessageSeverity", "Warning"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.SessionTerminated"},
        {"Message", "The session was successfully terminated."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "OK"},
        {"Resolution", "No resolution is required."}};
}

void sessionTerminated(crow::Response& res)
{
    res.result(boost::beast::http::status::ok);
    addMessageToJsonRoot(res.jsonValue, sessionTerminated());
}

/**
 * @internal
 * @brief Formats SubscriptionTerminated message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json subscriptionTerminated(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.SubscriptionTerminated"},
        {"Message", "The event subscription has been terminated."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "OK"},
        {"Resolution", "No resolution is required."}};
}

void subscriptionTerminated(crow::Response& res)
{
    res.result(boost::beast::http::status::ok);
    addMessageToJsonRoot(res.jsonValue, subscriptionTerminated());
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.ResourceTypeIncompatible"},
        {"Message", "The @odata.type of the request body " + arg1 +
                        " is incompatible with the @odata.type of the "
                        "resource which is " +
                        arg2 + "."},
        {"MessageArgs", {arg1, arg2}},
        {"MessageSeverity", "Critical"},
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
 * @brief Formats ResetRequired message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resetRequired(const std::string& arg1, const std::string& arg2)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.ResetRequired"},
        {"Message", "In order to complete the operation, a component reset is "
                    "required with the Reset action URI '" +
                        arg1 + "' and ResetType '" + arg2 + "'."},
        {"MessageArgs", {arg1, arg2}},
        {"MessageSeverity", "Warning"},
        {"Resolution",
         "Perform the required Reset action on the specified component."}};
}

void resetRequired(crow::Response& res, const std::string& arg1,
                   const std::string& arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, resetRequired(arg1, arg2));
}

/**
 * @internal
 * @brief Formats ChassisPowerStateOnRequired message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json chassisPowerStateOnRequired(const std::string& arg1)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.ChassisPowerStateOnRequired"},
        {"Message", "The Chassis with Id '" + arg1 +
                        "' requires to be powered on to perform this request."},
        {"MessageArgs", {arg1}},
        {"MessageSeverity", "Warning"},
        {"Resolution",
         "Power on the specified Chassis and resubmit the request."}};
}

void chassisPowerStateOnRequired(crow::Response& res, const std::string& arg1)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, chassisPowerStateOnRequired(arg1));
}

/**
 * @internal
 * @brief Formats ChassisPowerStateOffRequired message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json chassisPowerStateOffRequired(const std::string& arg1)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.ChassisPowerStateOffRequired"},
        {"Message",
         "The Chassis with Id '" + arg1 +
             "' requires to be powered off to perform this request."},
        {"MessageArgs", {arg1}},
        {"MessageSeverity", "Warning"},
        {"Resolution",
         "Power off the specified Chassis and resubmit the request."}};
}

void chassisPowerStateOffRequired(crow::Response& res, const std::string& arg1)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, chassisPowerStateOffRequired(arg1));
}

/**
 * @internal
 * @brief Formats PropertyValueConflict message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json propertyValueConflict(const std::string& arg1,
                                     const std::string& arg2)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.PropertyValueConflict"},
        {"Message", "The property '" + arg1 +
                        "' could not be written because its value would "
                        "conflict with the value of the '" +
                        arg2 + "' property."},
        {"MessageArgs", {arg1, arg2}},
        {"MessageSeverity", "Warning"},
        {"Resolution", "No resolution is required."}};
}

void propertyValueConflict(crow::Response& res, const std::string& arg1,
                           const std::string& arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, propertyValueConflict(arg1, arg2));
}

/**
 * @internal
 * @brief Formats PropertyValueIncorrect message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json propertyValueIncorrect(const std::string& arg1,
                                      const std::string& arg2)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.PropertyValueIncorrect"},
        {"Message", "The property '" + arg1 +
                        "' with the requested value of '" + arg2 +
                        "' could not be written because the value does not "
                        "meet the constraints of the implementation."},
        {"MessageArgs", {arg1, arg2}},
        {"MessageSeverity", "Warning"},
        {"Resolution", "No resolution is required."}};
}

void propertyValueIncorrect(crow::Response& res, const std::string& arg1,
                            const std::string& arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, propertyValueIncorrect(arg1, arg2));
}

/**
 * @internal
 * @brief Formats ResourceCreationConflict message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourceCreationConflict(const std::string& arg1)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.ResourceCreationConflict"},
        {"Message", "The resource could not be created.  The service has a "
                    "resource at URI '" +
                        arg1 + "' that conflicts with the creation request."},
        {"MessageArgs", {arg1}},
        {"MessageSeverity", "Warning"},
        {"Resolution", "No resolution is required."}};
}

void resourceCreationConflict(crow::Response& res, const std::string& arg1)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, resourceCreationConflict(arg1));
}

/**
 * @internal
 * @brief Formats MaximumErrorsExceeded message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json maximumErrorsExceeded(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.MaximumErrorsExceeded"},
        {"Message", "Too many errors have occurred to report them all."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "Critical"},
        {"Resolution",
         "Resolve other reported errors and retry the current operation."}};
}

void maximumErrorsExceeded(crow::Response& res)
{
    res.result(boost::beast::http::status::internal_server_error);
    addMessageToErrorJson(res.jsonValue, maximumErrorsExceeded());
}

/**
 * @internal
 * @brief Formats PreconditionFailed message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json preconditionFailed(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.PreconditionFailed"},
        {"Message", "The ETag supplied did not match the ETag required to "
                    "change this resource."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "Critical"},
        {"Resolution", "Try the operation again using the appropriate ETag."}};
}

void preconditionFailed(crow::Response& res)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, preconditionFailed());
}

/**
 * @internal
 * @brief Formats PreconditionRequired message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json preconditionRequired(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.PreconditionRequired"},
        {"Message", "A precondition header or annotation is required to change "
                    "this resource."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "Critical"},
        {"Resolution", "Try the operation again using an If-Match or "
                       "If-None-Match header and appropriate ETag."}};
}

void preconditionRequired(crow::Response& res)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, preconditionRequired());
}

/**
 * @internal
 * @brief Formats OperationFailed message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json operationFailed(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.OperationFailed"},
        {"Message",
         "An error occurred internal to the service as part of the overall "
         "request.  Partial results may have been returned."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "Warning"},
        {"Resolution", "Resubmit the request.  If the problem persists, "
                       "consider resetting the service or provider."}};
}

void operationFailed(crow::Response& res)
{
    res.result(boost::beast::http::status::internal_server_error);
    addMessageToErrorJson(res.jsonValue, operationFailed());
}

/**
 * @internal
 * @brief Formats OperationTimeout message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json operationTimeout(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.OperationTimeout"},
        {"Message", "A timeout internal to the service occured as part of the "
                    "request.  Partial results may have been returned."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "Warning"},
        {"Resolution", "Resubmit the request.  If the problem persists, "
                       "consider resetting the service or provider."}};
}

void operationTimeout(crow::Response& res)
{
    res.result(boost::beast::http::status::internal_server_error);
    addMessageToErrorJson(res.jsonValue, operationTimeout());
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.PropertyValueTypeError"},
        {"Message",
         "The value " + arg1 + " for the property " + arg2 +
             " is of a different type than the property can accept."},
        {"MessageArgs", {arg1, arg2}},
        {"MessageSeverity", "Warning"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.ResourceNotFound"},
        {"Message", "The requested resource of type " + arg1 + " named " +
                        arg2 + " was not found."},
        {"MessageArgs", {arg1, arg2}},
        {"MessageSeverity", "Critical"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.CouldNotEstablishConnection"},
        {"Message",
         "The service failed to establish a connection with the URI " + arg1 +
             "."},
        {"MessageArgs", {arg1}},
        {"MessageSeverity", "Critical"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.PropertyNotWritable"},
        {"Message", "The property " + arg1 +
                        " is a read only property and cannot be "
                        "assigned a value."},
        {"MessageArgs", {arg1}},
        {"MessageSeverity", "Warning"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.QueryParameterValueTypeError"},
        {"Message",
         "The value " + arg1 + " for the query parameter " + arg2 +
             " is of a different type than the parameter can accept."},
        {"MessageArgs", {arg1, arg2}},
        {"MessageSeverity", "Warning"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.ServiceShuttingDown"},
        {"Message", "The operation failed because the service is shutting "
                    "down and can no longer take incoming requests."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "Critical"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.ActionParameterDuplicate"},
        {"Message",
         "The action " + arg1 +
             " was submitted with more than one value for the parameter " +
             arg2 + "."},
        {"MessageArgs", {arg1, arg2}},
        {"MessageSeverity", "Warning"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.ActionParameterNotSupported"},
        {"Message", "The parameter " + arg1 + " for the action " + arg2 +
                        " is not supported on the target resource."},
        {"MessageArgs", {arg1, arg2}},
        {"MessageSeverity", "Warning"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.SourceDoesNotSupportProtocol"},
        {"Message", "The other end of the connection at " + arg1 +
                        " does not support the specified protocol " + arg2 +
                        "."},
        {"MessageArgs", {arg1, arg2}},
        {"MessageSeverity", "Critical"},
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
    return nlohmann::json{{"@odata.type", "#Message.v1_1_1.Message"},
                          {"MessageId", "Base.1.8.1.AccountRemoved"},
                          {"Message", "The account was successfully removed."},
                          {"MessageArgs", nlohmann::json::array()},
                          {"MessageSeverity", "OK"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.AccessDenied"},
        {"Message", "While attempting to establish a connection to " + arg1 +
                        ", the service denied access."},
        {"MessageArgs", {arg1}},
        {"MessageSeverity", "Critical"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.QueryNotSupported"},
        {"Message", "Querying is not supported by the implementation."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "Warning"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.CreateLimitReachedForResource"},
        {"Message", "The create operation failed because the resource has "
                    "reached the limit of possible resources."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "Critical"},
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
    return nlohmann::json{{"@odata.type", "#Message.v1_1_1.Message"},
                          {"MessageId", "Base.1.8.1.GeneralError"},
                          {"Message",
                           "A general error has occurred. See Resolution for "
                           "information on how to resolve the error."},
                          {"MessageArgs", nlohmann::json::array()},
                          {"MessageSeverity", "Critical"},
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
    return nlohmann::json{{"@odata.type", "#Message.v1_1_1.Message"},
                          {"MessageId", "Base.1.8.1.Success"},
                          {"Message", "Successfully Completed Request"},
                          {"MessageArgs", nlohmann::json::array()},
                          {"MessageSeverity", "OK"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.Created"},
        {"Message", "The resource has been created successfully"},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "OK"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.NoOperation"},
        {"Message", "The request body submitted contain no data to act "
                    "upon and no changes to the resource took place."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "Warning"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.PropertyUnknown"},
        {"Message", "The property " + arg1 +
                        " is not in the list of valid properties for "
                        "the resource."},
        {"MessageArgs", {arg1}},
        {"MessageSeverity", "Warning"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.NoValidSession"},
        {"Message",
         "There is no valid session established with the implementation."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "Critical"},
        {"Resolution",
         "Establish a session before attempting any operations."}};
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.InvalidObject"},
        {"Message", "The object at " + arg1 + " is invalid."},
        {"MessageArgs", {arg1}},
        {"MessageSeverity", "Critical"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.ResourceInStandby"},
        {"Message", "The request could not be performed because the "
                    "resource is in standby."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "Critical"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.ActionParameterValueTypeError"},
        {"Message",
         "The value " + arg1 + " for the parameter " + arg2 +
             " in the action " + arg3 +
             " is of a different type than the parameter can accept."},
        {"MessageArgs", {arg1, arg2, arg3}},
        {"MessageSeverity", "Warning"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.SessionLimitExceeded"},
        {"Message", "The session establishment failed due to the number of "
                    "simultaneous sessions exceeding the limit of the "
                    "implementation."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "Critical"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.ActionNotSupported"},
        {"Message",
         "The action " + arg1 + " is not supported by the resource."},
        {"MessageArgs", {arg1}},
        {"MessageSeverity", "Critical"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.InvalidIndex"},
        {"Message", "The Index " + std::to_string(arg1) +
                        " is not a valid offset into the array."},
        {"MessageArgs", {std::to_string(arg1)}},
        {"MessageSeverity", "Warning"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.EmptyJSON"},
        {"Message", "The request body submitted contained an empty JSON "
                    "object and the service is unable to process it."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "Warning"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.QueryNotSupportedOnResource"},
        {"Message", "Querying is not supported on the requested resource."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "Warning"},
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
 * @brief Formats QueryNotSupportedOnOperation message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json queryNotSupportedOnOperation(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.QueryNotSupportedOnOperation"},
        {"Message", "Querying is not supported with the requested operation."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "Warning"},
        {"Resolution", "Remove the query parameters and resubmit the request "
                       "if the operation failed."}};
}

void queryNotSupportedOnOperation(crow::Response& res)
{
    res.result(boost::beast::http::status::forbidden);
    addMessageToErrorJson(res.jsonValue, queryNotSupportedOnOperation());
}

/**
 * @internal
 * @brief Formats QueryCombinationInvalid message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json queryCombinationInvalid(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.QueryCombinationInvalid"},
        {"Message", "Two or more query parameters in the request cannot be "
                    "used together."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "Warning"},
        {"Resolution", "Remove one or more of the query parameters and "
                       "resubmit the request if the operation failed."}};
}

void queryCombinationInvalid(crow::Response& res)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, queryCombinationInvalid());
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.InsufficientPrivilege"},
        {"Message", "There are insufficient privileges for the account or "
                    "credentials associated with the current session to "
                    "perform the requested operation."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "Critical"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.PropertyValueModified"},
        {"Message", "The property " + arg1 + " was assigned the value " + arg2 +
                        " due to modification by the service."},
        {"MessageArgs", {arg1, arg2}},
        {"MessageSeverity", "Warning"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.AccountNotModified"},
        {"Message", "The account modification request failed."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "Warning"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.QueryParameterValueFormatError"},
        {"Message",
         "The value " + arg1 + " for the parameter " + arg2 +
             " is of a different format than the parameter can accept."},
        {"MessageArgs", {arg1, arg2}},
        {"MessageSeverity", "Warning"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.PropertyMissing"},
        {"Message", "The property " + arg1 +
                        " is a required property and must be included in "
                        "the request."},
        {"MessageArgs", {arg1}},
        {"MessageSeverity", "Warning"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.ResourceExhaustion"},
        {"Message", "The resource " + arg1 +
                        " was unable to satisfy the request due to "
                        "unavailability of resources."},
        {"MessageArgs", {arg1}},
        {"MessageSeverity", "Critical"},
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
    return nlohmann::json{{"@odata.type", "#Message.v1_1_1.Message"},
                          {"MessageId", "Base.1.8.1.AccountModified"},
                          {"Message", "The account was successfully modified."},
                          {"MessageArgs", nlohmann::json::array()},
                          {"MessageSeverity", "OK"},
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
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.QueryParameterOutOfRange"},
        {"Message", "The value " + arg1 + " for the query parameter " + arg2 +
                        " is out of range " + arg3 + "."},
        {"MessageArgs", {arg1, arg2, arg3}},
        {"MessageSeverity", "Warning"},
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
            {"MessageId", "Base.1.8.1.PasswordChangeRequired"},
            {"Message", "The password provided for this account must be "
                        "changed before access is granted.  PATCH the "
                        "'Password' property for this account located at "
                        "the target URI '" +
                            arg1 + "' to complete this process."},
            {"MessageArgs", {arg1}},
            {"MessageSeverity", "Critical"},
            {"Resolution", "Change the password for this account using "
                           "a PATCH to the 'Password' property at the URI "
                           "provided."}});
}

void invalidUpload(crow::Response& res, const std::string& arg1,
                   const std::string& arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, invalidUpload(arg1, arg2));
}

/**
 * @internal
 * @brief Formats Invalid File message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json invalidUpload(const std::string& arg1, const std::string& arg2)
{
    return nlohmann::json{
        {"@odata.type", "/redfish/v1/$metadata#Message.v1_1_1.Message"},
        {"MessageId", "OpenBMC.0.1.0.InvalidUpload"},
        {"Message", "Invalid file uploaded to " + arg1 + ": " + arg2 + "."},
        {"MessageArgs", {arg1, arg2}},
        {"MessageSeverity", "Warning"},
        {"Resolution", "None."}};
}

/**
 * @internal
 * @brief Formats MutualExclusiveProperties into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json mutualExclusiveProperties(const std::string& arg1,
                                         const std::string& arg2)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.5.0.MutualExclusiveProperties"},
        {"Message", "The properties " + arg1 + " and " + arg2 +
                        " are mutually exclusive."},
        {"MessageArgs", {arg1, arg2}},
        {"MessageSeverity", "Warning"},
        {"Resolution",
         "Ensure that the request body doesn't contain mutually exclusive "
         "properties and resubmit the request."}};
}

void mutualExclusiveProperties(crow::Response& res, const std::string& arg1,
                               const std::string& arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, mutualExclusiveProperties(arg1, arg2));
}

} // namespace messages

} // namespace redfish
