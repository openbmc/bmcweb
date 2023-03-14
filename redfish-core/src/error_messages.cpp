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
#include "error_messages.hpp"

#include "http_response.hpp"
#include "logging.hpp"
#include "nlohmann/json.hpp"
#include "registries.hpp"
#include "registries/base_message_registry.hpp"
#include "source_location.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>

#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <utility>

// IWYU pragma: no_include <stddef.h>

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
        error["code"] = *messageIdIterator;
        error["message"] = *messageFieldIterator;
    }
    else
    {
        // More than 1 error occurred, so the message has to be generic
        error["code"] = std::string(messageVersionPrefix) + "GeneralError";
        error["message"] = "A general error has occurred. See Resolution for "
                           "information on how to resolve the error.";
    }

    // This check could technically be done in the default construction
    // branch above, but because we need the pointer to the extended info field
    // anyway, it's more efficient to do it here.
    auto& extendedInfo = error[messages::messageAnnotation];
    if (!extendedInfo.is_array())
    {
        extendedInfo = nlohmann::json::array();
    }

    extendedInfo.push_back(message);
}

void moveErrorsToErrorJson(nlohmann::json& target, nlohmann::json& source)
{
    if (!source.is_object())
    {
        return;
    }
    auto errorIt = source.find("error");
    if (errorIt == source.end())
    {
        // caller puts error message in root
        messages::addMessageToErrorJson(target, source);
        source.clear();
        return;
    }
    auto extendedInfoIt = errorIt->find(messages::messageAnnotation);
    if (extendedInfoIt == errorIt->end())
    {
        return;
    }
    const nlohmann::json::array_t* extendedInfo =
        (*extendedInfoIt).get_ptr<const nlohmann::json::array_t*>();
    if (extendedInfo == nullptr)
    {
        source.erase(errorIt);
        return;
    }
    for (const nlohmann::json& message : *extendedInfo)
    {
        addMessageToErrorJson(target, message);
    }
    source.erase(errorIt);
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
                             std::string_view fieldPath)
{
    std::string extendedInfo(fieldPath);
    extendedInfo += messages::messageAnnotation;

    nlohmann::json& field = target[extendedInfo];
    if (!field.is_array())
    {
        // Force object to be an array
        field = nlohmann::json::array();
    }

    // Object exists and it is an array so we can just push in the message
    field.push_back(message);
}

static nlohmann::json getLog(redfish::registries::base::Index name,
                             std::span<const std::string_view> args)
{
    size_t index = static_cast<size_t>(name);
    if (index >= redfish::registries::base::registry.size())
    {
        return {};
    }
    return getLogFromRegistry(redfish::registries::base::header,
                              redfish::registries::base::registry, index, args);
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
    return getLog(redfish::registries::base::Index::resourceInUse, {});
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
    return getLog(redfish::registries::base::Index::malformedJSON, {});
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
nlohmann::json resourceMissingAtURI(const boost::urls::url_view& arg1)
{
    std::array<std::string_view, 1> args{arg1.buffer()};
    return getLog(redfish::registries::base::Index::resourceMissingAtURI, args);
}

void resourceMissingAtURI(crow::Response& res,
                          const boost::urls::url_view& arg1)
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
nlohmann::json actionParameterValueFormatError(std::string_view arg1,
                                               std::string_view arg2,
                                               std::string_view arg3)
{
    return getLog(
        redfish::registries::base::Index::actionParameterValueFormatError,
        std::to_array({arg1, arg2, arg3}));
}

void actionParameterValueFormatError(crow::Response& res, std::string_view arg1,
                                     std::string_view arg2,
                                     std::string_view arg3)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue,
                          actionParameterValueFormatError(arg1, arg2, arg3));
}

/**
 * @internal
 * @brief Formats ActionParameterValueNotInList message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json actionParameterValueNotInList(std::string_view arg1,
                                             std::string_view arg2,
                                             std::string_view arg3)
{
    return getLog(
        redfish::registries::base::Index::actionParameterValueNotInList,
        std::to_array({arg1, arg2, arg3}));
}

void actionParameterValueNotInList(crow::Response& res, std::string_view arg1,
                                   std::string_view arg2, std::string_view arg3)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue,
                          actionParameterValueNotInList(arg1, arg2, arg3));
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
    return getLog(redfish::registries::base::Index::internalError, {});
}

void internalError(crow::Response& res, const bmcweb::source_location location)
{
    BMCWEB_LOG_CRITICAL << "Internal Error " << location.file_name() << "("
                        << location.line() << ":" << location.column() << ") `"
                        << location.function_name() << "`: ";
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
    return getLog(redfish::registries::base::Index::unrecognizedRequestBody,
                  {});
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
nlohmann::json resourceAtUriUnauthorized(const boost::urls::url_view& arg1,
                                         std::string_view arg2)
{
    return getLog(redfish::registries::base::Index::resourceAtUriUnauthorized,
                  std::to_array<std::string_view>({arg1.buffer(), arg2}));
}

void resourceAtUriUnauthorized(crow::Response& res,
                               const boost::urls::url_view& arg1,
                               std::string_view arg2)
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
nlohmann::json actionParameterUnknown(std::string_view arg1,
                                      std::string_view arg2)
{
    return getLog(redfish::registries::base::Index::actionParameterUnknown,
                  std::to_array({arg1, arg2}));
}

void actionParameterUnknown(crow::Response& res, std::string_view arg1,
                            std::string_view arg2)
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
    return getLog(redfish::registries::base::Index::resourceCannotBeDeleted,
                  {});
}

void resourceCannotBeDeleted(crow::Response& res)
{
    res.result(boost::beast::http::status::method_not_allowed);
    addMessageToErrorJson(res.jsonValue, resourceCannotBeDeleted());
}

/**
 * @internal
 * @brief Formats PropertyDuplicate message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json propertyDuplicate(std::string_view arg1)
{
    return getLog(redfish::registries::base::Index::propertyDuplicate,
                  std::to_array({arg1}));
}

void propertyDuplicate(crow::Response& res, std::string_view arg1)
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
nlohmann::json serviceTemporarilyUnavailable(std::string_view arg1)
{
    return getLog(
        redfish::registries::base::Index::serviceTemporarilyUnavailable,
        std::to_array({arg1}));
}

void serviceTemporarilyUnavailable(crow::Response& res, std::string_view arg1)
{
    res.addHeader(boost::beast::http::field::retry_after, arg1);
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
nlohmann::json resourceAlreadyExists(std::string_view arg1,
                                     std::string_view arg2,
                                     std::string_view arg3)
{
    return getLog(redfish::registries::base::Index::resourceAlreadyExists,
                  std::to_array({arg1, arg2, arg3}));
}

void resourceAlreadyExists(crow::Response& res, std::string_view arg1,
                           std::string_view arg2, std::string_view arg3)
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
    return getLog(
        redfish::registries::base::Index::accountForSessionNoLongerExists, {});
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
nlohmann::json createFailedMissingReqProperties(std::string_view arg1)
{
    return getLog(
        redfish::registries::base::Index::createFailedMissingReqProperties,
        std::to_array({arg1}));
}

void createFailedMissingReqProperties(crow::Response& res,
                                      std::string_view arg1)
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
nlohmann::json propertyValueFormatError(std::string_view arg1,
                                        std::string_view arg2)
{
    return getLog(redfish::registries::base::Index::propertyValueFormatError,
                  std::to_array({arg1, arg2}));
}

void propertyValueFormatError(crow::Response& res, std::string_view arg1,
                              std::string_view arg2)
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
nlohmann::json propertyValueNotInList(std::string_view arg1,
                                      std::string_view arg2)
{
    return getLog(redfish::registries::base::Index::propertyValueNotInList,
                  std::to_array({arg1, arg2}));
}

void propertyValueNotInList(crow::Response& res, std::string_view arg1,
                            std::string_view arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToJson(res.jsonValue, propertyValueNotInList(arg1, arg2), arg2);
}

/**
 * @internal
 * @brief Formats PropertyValueOutOfRange message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json propertyValueOutOfRange(std::string_view arg1,
                                       std::string_view arg2)
{
    return getLog(redfish::registries::base::Index::propertyValueOutOfRange,
                  std::to_array({arg1, arg2}));
}

void propertyValueOutOfRange(crow::Response& res, std::string_view arg1,
                             std::string_view arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, propertyValueOutOfRange(arg1, arg2));
}

/**
 * @internal
 * @brief Formats ResourceAtUriInUnknownFormat message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourceAtUriInUnknownFormat(const boost::urls::url_view& arg1)
{
    return getLog(
        redfish::registries::base::Index::resourceAtUriInUnknownFormat,
        std::to_array<std::string_view>({arg1.buffer()}));
}

void resourceAtUriInUnknownFormat(crow::Response& res,
                                  const boost::urls::url_view& arg1)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, resourceAtUriInUnknownFormat(arg1));
}

/**
 * @internal
 * @brief Formats ServiceDisabled message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json serviceDisabled(std::string_view arg1)
{
    return getLog(redfish::registries::base::Index::serviceDisabled,
                  std::to_array({arg1}));
}

void serviceDisabled(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::service_unavailable);
    addMessageToErrorJson(res.jsonValue, serviceDisabled(arg1));
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
    return getLog(redfish::registries::base::Index::serviceInUnknownState, {});
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
    return getLog(
        redfish::registries::base::Index::eventSubscriptionLimitExceeded, {});
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
nlohmann::json actionParameterMissing(std::string_view arg1,
                                      std::string_view arg2)
{
    return getLog(redfish::registries::base::Index::actionParameterMissing,
                  std::to_array({arg1, arg2}));
}

void actionParameterMissing(crow::Response& res, std::string_view arg1,
                            std::string_view arg2)
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
nlohmann::json stringValueTooLong(std::string_view arg1, int arg2)
{
    std::string arg2String = std::to_string(arg2);
    return getLog(redfish::registries::base::Index::stringValueTooLong,
                  std::to_array({arg1, std::string_view(arg2String)}));
}

void stringValueTooLong(crow::Response& res, std::string_view arg1, int arg2)
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
    return getLog(redfish::registries::base::Index::sessionTerminated, {});
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
    return getLog(redfish::registries::base::Index::subscriptionTerminated, {});
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
nlohmann::json resourceTypeIncompatible(std::string_view arg1,
                                        std::string_view arg2)
{
    return getLog(redfish::registries::base::Index::resourceTypeIncompatible,
                  std::to_array({arg1, arg2}));
}

void resourceTypeIncompatible(crow::Response& res, std::string_view arg1,
                              std::string_view arg2)
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
nlohmann::json resetRequired(const boost::urls::url_view& arg1,
                             std::string_view arg2)
{
    return getLog(redfish::registries::base::Index::resetRequired,
                  std::to_array<std::string_view>({arg1.buffer(), arg2}));
}

void resetRequired(crow::Response& res, const boost::urls::url_view& arg1,
                   std::string_view arg2)
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
nlohmann::json chassisPowerStateOnRequired(std::string_view arg1)
{
    return getLog(redfish::registries::base::Index::resetRequired,
                  std::to_array({arg1}));
}

void chassisPowerStateOnRequired(crow::Response& res, std::string_view arg1)
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
nlohmann::json chassisPowerStateOffRequired(std::string_view arg1)
{
    return getLog(
        redfish::registries::base::Index::chassisPowerStateOffRequired,
        std::to_array({arg1}));
}

void chassisPowerStateOffRequired(crow::Response& res, std::string_view arg1)
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
nlohmann::json propertyValueConflict(std::string_view arg1,
                                     std::string_view arg2)
{
    return getLog(redfish::registries::base::Index::propertyValueConflict,
                  std::to_array({arg1, arg2}));
}

void propertyValueConflict(crow::Response& res, std::string_view arg1,
                           std::string_view arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, propertyValueConflict(arg1, arg2));
}

/**
 * @internal
 * @brief Formats PropertyValueResourceConflict message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json propertyValueResourceConflict(std::string_view arg1,
                                             std::string_view arg2,
                                             const boost::urls::url_view& arg3)
{
    return getLog(
        redfish::registries::base::Index::propertyValueResourceConflict,
        std::to_array<std::string_view>({arg1, arg2, arg3.buffer()}));
}

void propertyValueResourceConflict(crow::Response& res, std::string_view arg1,
                                   std::string_view arg2,
                                   const boost::urls::url_view& arg3)
{
    res.result(boost::beast::http::status::conflict);
    addMessageToErrorJson(res.jsonValue,
                          propertyValueResourceConflict(arg1, arg2, arg3));
}

/**
 * @internal
 * @brief Formats PropertyValueExternalConflict message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json propertyValueExternalConflict(std::string_view arg1,
                                             std::string_view arg2)
{
    return getLog(
        redfish::registries::base::Index::propertyValueExternalConflict,
        std::to_array({arg1, arg2}));
}

void propertyValueExternalConflict(crow::Response& res, std::string_view arg1,
                                   std::string_view arg2)
{
    res.result(boost::beast::http::status::conflict);
    addMessageToErrorJson(res.jsonValue,
                          propertyValueExternalConflict(arg1, arg2));
}

/**
 * @internal
 * @brief Formats PropertyValueIncorrect message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json propertyValueIncorrect(std::string_view arg1,
                                      std::string_view arg2)
{
    return getLog(redfish::registries::base::Index::propertyValueIncorrect,
                  std::to_array({arg1, arg2}));
}

void propertyValueIncorrect(crow::Response& res, std::string_view arg1,
                            std::string_view arg2)
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
nlohmann::json resourceCreationConflict(const boost::urls::url_view& arg1)
{
    return getLog(redfish::registries::base::Index::resourceCreationConflict,
                  std::to_array<std::string_view>({arg1.buffer()}));
}

void resourceCreationConflict(crow::Response& res,
                              const boost::urls::url_view& arg1)
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
    return getLog(redfish::registries::base::Index::maximumErrorsExceeded, {});
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
    return getLog(redfish::registries::base::Index::preconditionFailed, {});
}

void preconditionFailed(crow::Response& res)
{
    res.result(boost::beast::http::status::precondition_failed);
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
    return getLog(redfish::registries::base::Index::preconditionRequired, {});
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
    return getLog(redfish::registries::base::Index::operationFailed, {});
}

void operationFailed(crow::Response& res)
{
    res.result(boost::beast::http::status::bad_gateway);
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
    return getLog(redfish::registries::base::Index::operationTimeout, {});
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
nlohmann::json propertyValueTypeError(std::string_view arg1,
                                      std::string_view arg2)
{
    return getLog(redfish::registries::base::Index::propertyValueTypeError,
                  std::to_array({arg1, arg2}));
}

void propertyValueTypeError(crow::Response& res, std::string_view arg1,
                            std::string_view arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToJson(res.jsonValue, propertyValueTypeError(arg1, arg2), arg2);
}

/**
 * @internal
 * @brief Formats ResourceNotFound message into JSONd
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourceNotFound(std::string_view arg1, std::string_view arg2)
{
    return getLog(redfish::registries::base::Index::resourceNotFound,
                  std::to_array({arg1, arg2}));
}

void resourceNotFound(crow::Response& res, std::string_view arg1,
                      std::string_view arg2)
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
nlohmann::json couldNotEstablishConnection(const boost::urls::url_view& arg1)
{
    return getLog(redfish::registries::base::Index::couldNotEstablishConnection,
                  std::to_array<std::string_view>({arg1.buffer()}));
}

void couldNotEstablishConnection(crow::Response& res,
                                 const boost::urls::url_view& arg1)
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
nlohmann::json propertyNotWritable(std::string_view arg1)
{
    return getLog(redfish::registries::base::Index::propertyNotWritable,
                  std::to_array({arg1}));
}

void propertyNotWritable(crow::Response& res, std::string_view arg1)
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
nlohmann::json queryParameterValueTypeError(std::string_view arg1,
                                            std::string_view arg2)
{
    return getLog(
        redfish::registries::base::Index::queryParameterValueTypeError,
        std::to_array({arg1, arg2}));
}

void queryParameterValueTypeError(crow::Response& res, std::string_view arg1,
                                  std::string_view arg2)
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
    return getLog(redfish::registries::base::Index::serviceShuttingDown, {});
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
nlohmann::json actionParameterDuplicate(std::string_view arg1,
                                        std::string_view arg2)
{
    return getLog(redfish::registries::base::Index::actionParameterDuplicate,
                  std::to_array({arg1, arg2}));
}

void actionParameterDuplicate(crow::Response& res, std::string_view arg1,
                              std::string_view arg2)
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
nlohmann::json actionParameterNotSupported(std::string_view arg1,
                                           std::string_view arg2)
{
    return getLog(redfish::registries::base::Index::actionParameterNotSupported,
                  std::to_array({arg1, arg2}));
}

void actionParameterNotSupported(crow::Response& res, std::string_view arg1,
                                 std::string_view arg2)
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
nlohmann::json sourceDoesNotSupportProtocol(const boost::urls::url_view& arg1,
                                            std::string_view arg2)
{
    return getLog(
        redfish::registries::base::Index::sourceDoesNotSupportProtocol,
        std::to_array<std::string_view>({arg1.buffer(), arg2}));
}

void sourceDoesNotSupportProtocol(crow::Response& res,
                                  const boost::urls::url_view& arg1,
                                  std::string_view arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue,
                          sourceDoesNotSupportProtocol(arg1, arg2));
}

/**
 * @internal
 * @brief Formats StrictAccountTypes message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json strictAccountTypes(std::string_view arg1)
{
    return getLog(redfish::registries::base::Index::strictAccountTypes,
                  std::to_array({arg1}));
}

void strictAccountTypes(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, strictAccountTypes(arg1));
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
    return getLog(redfish::registries::base::Index::accountRemoved, {});
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
nlohmann::json accessDenied(const boost::urls::url_view& arg1)
{
    return getLog(redfish::registries::base::Index::accessDenied,
                  std::to_array<std::string_view>({arg1.buffer()}));
}

void accessDenied(crow::Response& res, const boost::urls::url_view& arg1)
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
    return getLog(redfish::registries::base::Index::queryNotSupported, {});
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
    return getLog(
        redfish::registries::base::Index::createLimitReachedForResource, {});
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
    return getLog(redfish::registries::base::Index::generalError, {});
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
    return getLog(redfish::registries::base::Index::success, {});
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
    return getLog(redfish::registries::base::Index::created, {});
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
    return getLog(redfish::registries::base::Index::noOperation, {});
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
nlohmann::json propertyUnknown(std::string_view arg1)
{
    return getLog(redfish::registries::base::Index::propertyUnknown,
                  std::to_array({arg1}));
}

void propertyUnknown(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, propertyUnknown(arg1));
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
    return getLog(redfish::registries::base::Index::noValidSession, {});
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
nlohmann::json invalidObject(const boost::urls::url_view& arg1)
{
    return getLog(redfish::registries::base::Index::invalidObject,
                  std::to_array<std::string_view>({arg1.buffer()}));
}

void invalidObject(crow::Response& res, const boost::urls::url_view& arg1)
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
    return getLog(redfish::registries::base::Index::resourceInStandby, {});
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
nlohmann::json actionParameterValueTypeError(std::string_view arg1,
                                             std::string_view arg2,
                                             std::string_view arg3)
{
    return getLog(
        redfish::registries::base::Index::actionParameterValueTypeError,
        std::to_array({arg1, arg2, arg3}));
}

void actionParameterValueTypeError(crow::Response& res, std::string_view arg1,
                                   std::string_view arg2, std::string_view arg3)
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
    return getLog(redfish::registries::base::Index::sessionLimitExceeded, {});
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
nlohmann::json actionNotSupported(std::string_view arg1)
{
    return getLog(redfish::registries::base::Index::actionNotSupported,
                  std::to_array({arg1}));
}

void actionNotSupported(crow::Response& res, std::string_view arg1)
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
nlohmann::json invalidIndex(int64_t arg1)
{
    std::string arg1Str = std::to_string(arg1);
    return getLog(redfish::registries::base::Index::invalidIndex,
                  std::to_array<std::string_view>({arg1Str}));
}

void invalidIndex(crow::Response& res, int64_t arg1)
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
    return getLog(redfish::registries::base::Index::emptyJSON, {});
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
    return getLog(redfish::registries::base::Index::queryNotSupportedOnResource,
                  {});
}

void queryNotSupportedOnResource(crow::Response& res)
{
    res.result(boost::beast::http::status::bad_request);
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
    return getLog(
        redfish::registries::base::Index::queryNotSupportedOnOperation, {});
}

void queryNotSupportedOnOperation(crow::Response& res)
{
    res.result(boost::beast::http::status::bad_request);
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
    return getLog(redfish::registries::base::Index::queryCombinationInvalid,
                  {});
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
    return getLog(redfish::registries::base::Index::insufficientPrivilege, {});
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
nlohmann::json propertyValueModified(std::string_view arg1,
                                     std::string_view arg2)
{
    return getLog(redfish::registries::base::Index::propertyValueModified,
                  std::to_array({arg1, arg2}));
}

void propertyValueModified(crow::Response& res, std::string_view arg1,
                           std::string_view arg2)
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
    return getLog(redfish::registries::base::Index::accountNotModified, {});
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
nlohmann::json queryParameterValueFormatError(std::string_view arg1,
                                              std::string_view arg2)
{
    return getLog(
        redfish::registries::base::Index::queryParameterValueFormatError,
        std::to_array({arg1, arg2}));
}

void queryParameterValueFormatError(crow::Response& res, std::string_view arg1,
                                    std::string_view arg2)
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
nlohmann::json propertyMissing(std::string_view arg1)
{
    return getLog(redfish::registries::base::Index::propertyMissing,
                  std::to_array({arg1}));
}

void propertyMissing(crow::Response& res, std::string_view arg1)
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
nlohmann::json resourceExhaustion(std::string_view arg1)
{
    return getLog(redfish::registries::base::Index::resourceExhaustion,
                  std::to_array({arg1}));
}

void resourceExhaustion(crow::Response& res, std::string_view arg1)
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
    return getLog(redfish::registries::base::Index::accountModified, {});
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
nlohmann::json queryParameterOutOfRange(std::string_view arg1,
                                        std::string_view arg2,
                                        std::string_view arg3)
{
    return getLog(redfish::registries::base::Index::queryParameterOutOfRange,
                  std::to_array({arg1, arg2, arg3}));
}

void queryParameterOutOfRange(crow::Response& res, std::string_view arg1,
                              std::string_view arg2, std::string_view arg3)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue,
                          queryParameterOutOfRange(arg1, arg2, arg3));
}

nlohmann::json passwordChangeRequired(const boost::urls::url_view& arg1)
{
    return getLog(redfish::registries::base::Index::passwordChangeRequired,
                  std::to_array<std::string_view>({arg1.buffer()}));
}

/**
 * @internal
 * @brief Formats PasswordChangeRequired message into JSON
 *
 * See header file for more information
 * @endinternal
 */
void passwordChangeRequired(crow::Response& res,
                            const boost::urls::url_view& arg1)
{
    messages::addMessageToJsonRoot(res.jsonValue, passwordChangeRequired(arg1));
}

/**
 * @internal
 * @brief Formats InsufficientStorage message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json insufficientStorage()
{
    return getLog(redfish::registries::base::Index::insufficientStorage, {});
}

void insufficientStorage(crow::Response& res)
{
    res.result(boost::beast::http::status::insufficient_storage);
    addMessageToErrorJson(res.jsonValue, insufficientStorage());
}

/**
 * @internal
 * @brief Formats OperationNotAllowed message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json operationNotAllowed()
{
    return getLog(redfish::registries::base::Index::operationNotAllowed, {});
}

void operationNotAllowed(crow::Response& res)
{
    res.result(boost::beast::http::status::method_not_allowed);
    addMessageToErrorJson(res.jsonValue, operationNotAllowed());
}

void invalidUpload(crow::Response& res, std::string_view arg1,
                   std::string_view arg2)
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
nlohmann::json invalidUpload(std::string_view arg1, std::string_view arg2)
{
    std::string msg = "Invalid file uploaded to ";
    msg += arg1;
    msg += ": ";
    msg += arg2;
    msg += ".";

    nlohmann::json::object_t ret;
    ret["@odata.type"] = "/redfish/v1/$metadata#Message.v1_1_1.Message";
    ret["MessageId"] = "OpenBMC.0.2.InvalidUpload";
    ret["Message"] = std::move(msg);
    nlohmann::json::array_t args;
    args.push_back(arg1);
    args.push_back(arg2);
    ret["MessageArgs"] = std::move(args);
    ret["MessageSeverity"] = "Warning";
    ret["Resolution"] = "None.";
    return ret;
}

/**
 * @internal
 * @brief Formats RestrictedRole into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json restrictedRole(const std::string& arg1)
{
    return getLog(redfish::registries::base::Index::restrictedRole,
                  std::to_array<std::string_view>({arg1}));
}

void restrictedRole(crow::Response& res, const std::string& arg1)
{
    res.result(boost::beast::http::status::forbidden);
    addMessageToErrorJson(res.jsonValue, restrictedRole(arg1));
}
} // namespace messages

} // namespace redfish
