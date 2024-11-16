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
#include "error_messages.hpp"

#include "http_response.hpp"
#include "logging.hpp"
#include "registries.hpp"
#include "registries/base_message_registry.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/url/url_view_base.hpp>
#include <nlohmann/json.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <source_location>
#include <span>
#include <string>
#include <string_view>
#include <utility>

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
            BMCWEB_LOG_CRITICAL(
                "Attempt to add error message without MessageId");
            return;
        }

        auto messageFieldIterator = message.find("Message");
        if (messageFieldIterator == message.end())
        {
            BMCWEB_LOG_CRITICAL("Attempt to add error message without Message");
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
nlohmann::json resourceInUse()
{
    return getLog(redfish::registries::base::Index::resourceInUse, {});
}

void resourceInUse(crow::Response& res)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, resourceInUse());
}

/**
 * @internal
 * @brief Formats MalformedJSON message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json malformedJSON()
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
nlohmann::json resourceMissingAtURI(const boost::urls::url_view_base& arg1)
{
    std::array<std::string_view, 1> args{arg1.buffer()};
    return getLog(redfish::registries::base::Index::resourceMissingAtURI, args);
}

void resourceMissingAtURI(crow::Response& res,
                          const boost::urls::url_view_base& arg1)
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
nlohmann::json actionParameterValueFormatError(
    const nlohmann::json& arg1, std::string_view arg2, std::string_view arg3)
{
    std::string arg1Str =
        arg1.dump(2, ' ', true, nlohmann::json::error_handler_t::replace);
    std::array<std::string_view, 3> args{arg1Str, arg2, arg3};
    return getLog(
        redfish::registries::base::Index::actionParameterValueFormatError,
        args);
}

void actionParameterValueFormatError(
    crow::Response& res, const nlohmann::json& arg1, std::string_view arg2,
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
nlohmann::json actionParameterValueNotInList(
    std::string_view arg1, std::string_view arg2, std::string_view arg3)
{
    std::array<std::string_view, 3> args{arg1, arg2, arg3};
    return getLog(
        redfish::registries::base::Index::actionParameterValueNotInList, args);
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
nlohmann::json internalError()
{
    return getLog(redfish::registries::base::Index::internalError, {});
}

void internalError(crow::Response& res, std::source_location location =
                                            std::source_location::current())
{
    BMCWEB_LOG_CRITICAL("Internal Error {}({}:{}) `{}`: ", location.file_name(),
                        location.line(), location.column(),
                        location.function_name());
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, internalError(arg1));
}

/**
 * @internal
 * @brief Formats UnrecognizedRequestBody message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json unrecognizedRequestBody()
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
nlohmann::json resourceAtUriUnauthorized(const boost::urls::url_view_base& arg1,
                                         std::string_view arg2)
{
    std::array<std::string_view, 2> args{arg1.buffer(), arg2};
    return getLog(redfish::registries::base::Index::resourceAtUriUnauthorized,
                  args);
}

void resourceAtUriUnauthorized(crow::Response& res,
                               const boost::urls::url_view_base& arg1,
                               std::string_view arg2)
{
    res.result(boost::beast::http::status::unknown?);
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
    std::array<std::string_view, 2> args{arg1, arg2};
    return getLog(redfish::registries::base::Index::actionParameterUnknown,
                  args);
}

void actionParameterUnknown(crow::Response& res, std::string_view arg1,
                            std::string_view arg2)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, actionParameterUnknown(arg1, arg2));
}

/**
 * @internal
 * @brief Formats ResourceCannotBeDeleted message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourceCannotBeDeleted()
{
    return getLog(redfish::registries::base::Index::resourceCannotBeDeleted,
                  {});
}

void resourceCannotBeDeleted(crow::Response& res)
{
    res.result(boost::beast::http::status::unknown?);
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
    std::array<std::string_view, 1> args{arg1};
    return getLog(redfish::registries::base::Index::propertyDuplicate, args);
}

void propertyDuplicate(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, propertyDuplicate(arg1));
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
    std::array<std::string_view, 1> args{arg1};
    return getLog(
        redfish::registries::base::Index::serviceTemporarilyUnavailable, args);
}

void serviceTemporarilyUnavailable(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, serviceTemporarilyUnavailable(arg1));
}

/**
 * @internal
 * @brief Formats ResourceAlreadyExists message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourceAlreadyExists(
    std::string_view arg1, std::string_view arg2, std::string_view arg3)
{
    std::array<std::string_view, 3> args{arg1, arg2, arg3};
    return getLog(redfish::registries::base::Index::resourceAlreadyExists,
                  args);
}

void resourceAlreadyExists(crow::Response& res, std::string_view arg1,
                           std::string_view arg2, std::string_view arg3)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue,
                          resourceAlreadyExists(arg1, arg2, arg3));
}

/**
 * @internal
 * @brief Formats AccountForSessionNoLongerExists message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json accountForSessionNoLongerExists()
{
    return getLog(
        redfish::registries::base::Index::accountForSessionNoLongerExists, {});
}

void accountForSessionNoLongerExists(crow::Response& res)
{
    res.result(boost::beast::http::status::unknown?);
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
    std::array<std::string_view, 1> args{arg1};
    return getLog(
        redfish::registries::base::Index::createFailedMissingReqProperties,
        args);
}

void createFailedMissingReqProperties(crow::Response& res,
                                      std::string_view arg1)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue,
                          createFailedMissingReqProperties(arg1));
}

/**
 * @internal
 * @brief Formats PropertyValueFormatError message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json propertyValueFormatError(const nlohmann::json& arg1,
                                        std::string_view arg2)
{
    std::string arg1Str =
        arg1.dump(2, ' ', true, nlohmann::json::error_handler_t::replace);
    std::array<std::string_view, 2> args{arg1Str, arg2};
    return getLog(redfish::registries::base::Index::propertyValueFormatError,
                  args);
}

void propertyValueFormatError(crow::Response& res, const nlohmann::json& arg1,
                              std::string_view arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, propertyValueFormatError(arg1, arg2));
}

/**
 * @internal
 * @brief Formats PropertyValueNotInList message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json propertyValueNotInList(const nlohmann::json& arg1,
                                      std::string_view arg2)
{
    std::string arg1Str =
        arg1.dump(2, ' ', true, nlohmann::json::error_handler_t::replace);
    std::array<std::string_view, 2> args{arg1Str, arg2};
    return getLog(redfish::registries::base::Index::propertyValueNotInList,
                  args);
}

void propertyValueNotInList(crow::Response& res, const nlohmann::json& arg1,
                            std::string_view arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, propertyValueNotInList(arg1, arg2));
}

/**
 * @internal
 * @brief Formats PropertyValueOutOfRange message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json propertyValueOutOfRange(const nlohmann::json& arg1,
                                       std::string_view arg2)
{
    std::string arg1Str =
        arg1.dump(2, ' ', true, nlohmann::json::error_handler_t::replace);
    std::array<std::string_view, 2> args{arg1Str, arg2};
    return getLog(redfish::registries::base::Index::propertyValueOutOfRange,
                  args);
}

void propertyValueOutOfRange(crow::Response& res, const nlohmann::json& arg1,
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
nlohmann::json
    resourceAtUriInUnknownFormat(const boost::urls::url_view_base& arg1)
{
    std::array<std::string_view, 1> args{arg1.buffer()};
    return getLog(
        redfish::registries::base::Index::resourceAtUriInUnknownFormat, args);
}

void resourceAtUriInUnknownFormat(crow::Response& res,
                                  const boost::urls::url_view_base& arg1)
{
    res.result(boost::beast::http::status::unknown?);
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
    std::array<std::string_view, 1> args{arg1};
    return getLog(redfish::registries::base::Index::serviceDisabled, args);
}

void serviceDisabled(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, serviceDisabled(arg1));
}

/**
 * @internal
 * @brief Formats ServiceInUnknownState message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json serviceInUnknownState()
{
    return getLog(redfish::registries::base::Index::serviceInUnknownState, {});
}

void serviceInUnknownState(crow::Response& res)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, serviceInUnknownState());
}

/**
 * @internal
 * @brief Formats EventSubscriptionLimitExceeded message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json eventSubscriptionLimitExceeded()
{
    return getLog(
        redfish::registries::base::Index::eventSubscriptionLimitExceeded, {});
}

void eventSubscriptionLimitExceeded(crow::Response& res)
{
    res.result(boost::beast::http::status::unknown?);
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
    std::array<std::string_view, 2> args{arg1, arg2};
    return getLog(redfish::registries::base::Index::actionParameterMissing,
                  args);
}

void actionParameterMissing(crow::Response& res, std::string_view arg1,
                            std::string_view arg2)
{
    res.result(boost::beast::http::status::unknown?);
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
    std::array<std::string_view, 2> args{arg1, arg2};
    return getLog(redfish::registries::base::Index::stringValueTooLong, args);
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
nlohmann::json sessionTerminated()
{
    return getLog(redfish::registries::base::Index::sessionTerminated, {});
}

void sessionTerminated(crow::Response& res)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, sessionTerminated());
}

/**
 * @internal
 * @brief Formats SubscriptionTerminated message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json subscriptionTerminated()
{
    return getLog(redfish::registries::base::Index::subscriptionTerminated, {});
}

void subscriptionTerminated(crow::Response& res)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, subscriptionTerminated());
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
    std::array<std::string_view, 2> args{arg1, arg2};
    return getLog(redfish::registries::base::Index::resourceTypeIncompatible,
                  args);
}

void resourceTypeIncompatible(crow::Response& res, std::string_view arg1,
                              std::string_view arg2)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, resourceTypeIncompatible(arg1, arg2));
}

/**
 * @internal
 * @brief Formats ResetRequired message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resetRequired(const boost::urls::url_view_base& arg1,
                             std::string_view arg2)
{
    std::array<std::string_view, 2> args{arg1.buffer(), arg2};
    return getLog(redfish::registries::base::Index::resetRequired, args);
}

void resetRequired(crow::Response& res, const boost::urls::url_view_base& arg1,
                   std::string_view arg2)
{
    res.result(boost::beast::http::status::unknown?);
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
    std::array<std::string_view, 1> args{arg1};
    return getLog(redfish::registries::base::Index::chassisPowerStateOnRequired,
                  args);
}

void chassisPowerStateOnRequired(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::unknown?);
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
    std::array<std::string_view, 1> args{arg1};
    return getLog(
        redfish::registries::base::Index::chassisPowerStateOffRequired, args);
}

void chassisPowerStateOffRequired(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::unknown?);
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
    std::array<std::string_view, 2> args{arg1, arg2};
    return getLog(redfish::registries::base::Index::propertyValueConflict,
                  args);
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
nlohmann::json propertyValueResourceConflict(
    std::string_view arg1, const nlohmann::json& arg2,
    const boost::urls::url_view_base& arg3)
{
    std::string arg2Str =
        arg2.dump(2, ' ', true, nlohmann::json::error_handler_t::replace);
    std::array<std::string_view, 3> args{arg1, arg2Str, arg3.buffer()};
    return getLog(
        redfish::registries::base::Index::propertyValueResourceConflict, args);
}

void propertyValueResourceConflict(crow::Response& res, std::string_view arg1,
                                   const nlohmann::json& arg2,
                                   const boost::urls::url_view_base& arg3)
{
    res.result(boost::beast::http::status::bad_request);
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
                                             const nlohmann::json& arg2)
{
    std::string arg2Str =
        arg2.dump(2, ' ', true, nlohmann::json::error_handler_t::replace);
    std::array<std::string_view, 2> args{arg1, arg2Str};
    return getLog(
        redfish::registries::base::Index::propertyValueExternalConflict, args);
}

void propertyValueExternalConflict(crow::Response& res, std::string_view arg1,
                                   const nlohmann::json& arg2)
{
    res.result(boost::beast::http::status::bad_request);
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
                                      const nlohmann::json& arg2)
{
    std::string arg2Str =
        arg2.dump(2, ' ', true, nlohmann::json::error_handler_t::replace);
    std::array<std::string_view, 2> args{arg1, arg2Str};
    return getLog(redfish::registries::base::Index::propertyValueIncorrect,
                  args);
}

void propertyValueIncorrect(crow::Response& res, std::string_view arg1,
                            const nlohmann::json& arg2)
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
nlohmann::json resourceCreationConflict(const boost::urls::url_view_base& arg1)
{
    std::array<std::string_view, 1> args{arg1.buffer()};
    return getLog(redfish::registries::base::Index::resourceCreationConflict,
                  args);
}

void resourceCreationConflict(crow::Response& res,
                              const boost::urls::url_view_base& arg1)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, resourceCreationConflict(arg1));
}

/**
 * @internal
 * @brief Formats MaximumErrorsExceeded message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json maximumErrorsExceeded()
{
    return getLog(redfish::registries::base::Index::maximumErrorsExceeded, {});
}

void maximumErrorsExceeded(crow::Response& res)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, maximumErrorsExceeded());
}

/**
 * @internal
 * @brief Formats PreconditionFailed message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json preconditionFailed()
{
    return getLog(redfish::registries::base::Index::preconditionFailed, {});
}

void preconditionFailed(crow::Response& res)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, preconditionFailed());
}

/**
 * @internal
 * @brief Formats PreconditionRequired message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json preconditionRequired()
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
nlohmann::json operationFailed()
{
    return getLog(redfish::registries::base::Index::operationFailed, {});
}

void operationFailed(crow::Response& res)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, operationFailed());
}

/**
 * @internal
 * @brief Formats OperationTimeout message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json operationTimeout()
{
    return getLog(redfish::registries::base::Index::operationTimeout, {});
}

void operationTimeout(crow::Response& res)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, operationTimeout());
}

/**
 * @internal
 * @brief Formats PropertyValueTypeError message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json propertyValueTypeError(const nlohmann::json& arg1,
                                      std::string_view arg2)
{
    std::string arg1Str =
        arg1.dump(2, ' ', true, nlohmann::json::error_handler_t::replace);
    std::array<std::string_view, 2> args{arg1Str, arg2};
    return getLog(redfish::registries::base::Index::propertyValueTypeError,
                  args);
}

void propertyValueTypeError(crow::Response& res, const nlohmann::json& arg1,
                            std::string_view arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, propertyValueTypeError(arg1, arg2));
}

/**
 * @internal
 * @brief Formats PropertyValueError message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json propertyValueError(std::string_view arg1)
{
    std::array<std::string_view, 1> args{arg1};
    return getLog(redfish::registries::base::Index::propertyValueError, args);
}

void propertyValueError(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, propertyValueError(arg1));
}

/**
 * @internal
 * @brief Formats ResourceNotFound message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourceNotFound(std::string_view arg1, std::string_view arg2)
{
    std::array<std::string_view, 2> args{arg1, arg2};
    return getLog(redfish::registries::base::Index::resourceNotFound, args);
}

void resourceNotFound(crow::Response& res, std::string_view arg1,
                      std::string_view arg2)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, resourceNotFound(arg1, arg2));
}

/**
 * @internal
 * @brief Formats CouldNotEstablishConnection message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json
    couldNotEstablishConnection(const boost::urls::url_view_base& arg1)
{
    std::array<std::string_view, 1> args{arg1.buffer()};
    return getLog(redfish::registries::base::Index::couldNotEstablishConnection,
                  args);
}

void couldNotEstablishConnection(crow::Response& res,
                                 const boost::urls::url_view_base& arg1)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, couldNotEstablishConnection(arg1));
}

/**
 * @internal
 * @brief Formats PropertyNotWritable message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json propertyNotWritable(std::string_view arg1)
{
    std::array<std::string_view, 1> args{arg1};
    return getLog(redfish::registries::base::Index::propertyNotWritable, args);
}

void propertyNotWritable(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, propertyNotWritable(arg1));
}

/**
 * @internal
 * @brief Formats QueryParameterValueTypeError message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json queryParameterValueTypeError(const nlohmann::json& arg1,
                                            std::string_view arg2)
{
    std::string arg1Str =
        arg1.dump(2, ' ', true, nlohmann::json::error_handler_t::replace);
    std::array<std::string_view, 2> args{arg1Str, arg2};
    return getLog(
        redfish::registries::base::Index::queryParameterValueTypeError, args);
}

void queryParameterValueTypeError(
    crow::Response& res, const nlohmann::json& arg1, std::string_view arg2)
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
nlohmann::json serviceShuttingDown()
{
    return getLog(redfish::registries::base::Index::serviceShuttingDown, {});
}

void serviceShuttingDown(crow::Response& res)
{
    res.result(boost::beast::http::status::unknown?);
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
    std::array<std::string_view, 2> args{arg1, arg2};
    return getLog(redfish::registries::base::Index::actionParameterDuplicate,
                  args);
}

void actionParameterDuplicate(crow::Response& res, std::string_view arg1,
                              std::string_view arg2)
{
    res.result(boost::beast::http::status::unknown?);
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
    std::array<std::string_view, 2> args{arg1, arg2};
    return getLog(redfish::registries::base::Index::actionParameterNotSupported,
                  args);
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
nlohmann::json sourceDoesNotSupportProtocol(
    const boost::urls::url_view_base& arg1, std::string_view arg2)
{
    std::array<std::string_view, 2> args{arg1.buffer(), arg2};
    return getLog(
        redfish::registries::base::Index::sourceDoesNotSupportProtocol, args);
}

void sourceDoesNotSupportProtocol(crow::Response& res,
                                  const boost::urls::url_view_base& arg1,
                                  std::string_view arg2)
{
    res.result(boost::beast::http::status::unknown?);
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
    std::array<std::string_view, 1> args{arg1};
    return getLog(redfish::registries::base::Index::strictAccountTypes, args);
}

void strictAccountTypes(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, strictAccountTypes(arg1));
}

/**
 * @internal
 * @brief Formats AccountRemoved message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json accountRemoved()
{
    return getLog(redfish::registries::base::Index::accountRemoved, {});
}

void accountRemoved(crow::Response& res)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, accountRemoved());
}

/**
 * @internal
 * @brief Formats AccessDenied message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json accessDenied(const boost::urls::url_view_base& arg1)
{
    std::array<std::string_view, 1> args{arg1.buffer()};
    return getLog(redfish::registries::base::Index::accessDenied, args);
}

void accessDenied(crow::Response& res, const boost::urls::url_view_base& arg1)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, accessDenied(arg1));
}

/**
 * @internal
 * @brief Formats QueryNotSupported message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json queryNotSupported()
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
nlohmann::json createLimitReachedForResource()
{
    return getLog(
        redfish::registries::base::Index::createLimitReachedForResource, {});
}

void createLimitReachedForResource(crow::Response& res)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, createLimitReachedForResource());
}

/**
 * @internal
 * @brief Formats GeneralError message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json generalError()
{
    return getLog(redfish::registries::base::Index::generalError, {});
}

void generalError(crow::Response& res)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, generalError());
}

/**
 * @internal
 * @brief Formats Success message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json success()
{
    return getLog(redfish::registries::base::Index::success, {});
}

void success(crow::Response& res)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, success());
}

/**
 * @internal
 * @brief Formats Created message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json created()
{
    return getLog(redfish::registries::base::Index::created, {});
}

void created(crow::Response& res)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, created());
}

/**
 * @internal
 * @brief Formats NoOperation message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json noOperation()
{
    return getLog(redfish::registries::base::Index::noOperation, {});
}

void noOperation(crow::Response& res)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, noOperation());
}

/**
 * @internal
 * @brief Formats PropertyUnknown message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json propertyUnknown(std::string_view arg1)
{
    std::array<std::string_view, 1> args{arg1};
    return getLog(redfish::registries::base::Index::propertyUnknown, args);
}

void propertyUnknown(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, propertyUnknown(arg1));
}

/**
 * @internal
 * @brief Formats NoValidSession message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json noValidSession()
{
    return getLog(redfish::registries::base::Index::noValidSession, {});
}

void noValidSession(crow::Response& res)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, noValidSession());
}

/**
 * @internal
 * @brief Formats InvalidObject message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json invalidObject(const boost::urls::url_view_base& arg1)
{
    std::array<std::string_view, 1> args{arg1.buffer()};
    return getLog(redfish::registries::base::Index::invalidObject, args);
}

void invalidObject(crow::Response& res, const boost::urls::url_view_base& arg1)
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
nlohmann::json resourceInStandby()
{
    return getLog(redfish::registries::base::Index::resourceInStandby, {});
}

void resourceInStandby(crow::Response& res)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, resourceInStandby());
}

/**
 * @internal
 * @brief Formats ActionParameterValueTypeError message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json actionParameterValueTypeError(
    const nlohmann::json& arg1, std::string_view arg2, std::string_view arg3)
{
    std::string arg1Str =
        arg1.dump(2, ' ', true, nlohmann::json::error_handler_t::replace);
    std::array<std::string_view, 3> args{arg1Str, arg2, arg3};
    return getLog(
        redfish::registries::base::Index::actionParameterValueTypeError, args);
}

void actionParameterValueTypeError(crow::Response& res,
                                   const nlohmann::json& arg1,
                                   std::string_view arg2, std::string_view arg3)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue,
                          actionParameterValueTypeError(arg1, arg2, arg3));
}

/**
 * @internal
 * @brief Formats ActionParameterValueError message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json actionParameterValueError(const nlohmann::json& arg1,
                                         std::string_view arg2)
{
    std::string arg1Str =
        arg1.dump(2, ' ', true, nlohmann::json::error_handler_t::replace);
    std::array<std::string_view, 2> args{arg1Str, arg2};
    return getLog(redfish::registries::base::Index::actionParameterValueError,
                  args);
}

void actionParameterValueError(crow::Response& res, const nlohmann::json& arg1,
                               std::string_view arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, actionParameterValueError(arg1, arg2));
}

/**
 * @internal
 * @brief Formats SessionLimitExceeded message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json sessionLimitExceeded()
{
    return getLog(redfish::registries::base::Index::sessionLimitExceeded, {});
}

void sessionLimitExceeded(crow::Response& res)
{
    res.result(boost::beast::http::status::unknown?);
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
    std::array<std::string_view, 1> args{arg1};
    return getLog(redfish::registries::base::Index::actionNotSupported, args);
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
    std::array<std::string_view, 1> args{arg1};
    return getLog(redfish::registries::base::Index::invalidIndex, args);
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
nlohmann::json emptyJSON()
{
    return getLog(redfish::registries::base::Index::emptyJSON, {});
}

void emptyJSON(crow::Response& res)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, emptyJSON());
}

/**
 * @internal
 * @brief Formats QueryNotSupportedOnResource message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json queryNotSupportedOnResource()
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
nlohmann::json queryNotSupportedOnOperation()
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
nlohmann::json queryCombinationInvalid()
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
 * @brief Formats EventBufferExceeded message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json eventBufferExceeded()
{
    return getLog(redfish::registries::base::Index::eventBufferExceeded, {});
}

void eventBufferExceeded(crow::Response& res)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, eventBufferExceeded());
}

/**
 * @internal
 * @brief Formats InsufficientPrivilege message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json insufficientPrivilege()
{
    return getLog(redfish::registries::base::Index::insufficientPrivilege, {});
}

void insufficientPrivilege(crow::Response& res)
{
    res.result(boost::beast::http::status::unknown?);
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
                                     const nlohmann::json& arg2)
{
    std::string arg2Str =
        arg2.dump(2, ' ', true, nlohmann::json::error_handler_t::replace);
    std::array<std::string_view, 2> args{arg1, arg2Str};
    return getLog(redfish::registries::base::Index::propertyValueModified,
                  args);
}

void propertyValueModified(crow::Response& res, std::string_view arg1,
                           const nlohmann::json& arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, propertyValueModified(arg1, arg2));
}

/**
 * @internal
 * @brief Formats AccountNotModified message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json accountNotModified()
{
    return getLog(redfish::registries::base::Index::accountNotModified, {});
}

void accountNotModified(crow::Response& res)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, accountNotModified());
}

/**
 * @internal
 * @brief Formats QueryParameterValueFormatError message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json queryParameterValueFormatError(const nlohmann::json& arg1,
                                              std::string_view arg2)
{
    std::string arg1Str =
        arg1.dump(2, ' ', true, nlohmann::json::error_handler_t::replace);
    std::array<std::string_view, 2> args{arg1Str, arg2};
    return getLog(
        redfish::registries::base::Index::queryParameterValueFormatError, args);
}

void queryParameterValueFormatError(
    crow::Response& res, const nlohmann::json& arg1, std::string_view arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue,
                          queryParameterValueFormatError(arg1, arg2));
}

/**
 * @internal
 * @brief Formats PropertyMissing message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json propertyMissing(std::string_view arg1)
{
    std::array<std::string_view, 1> args{arg1};
    return getLog(redfish::registries::base::Index::propertyMissing, args);
}

void propertyMissing(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, propertyMissing(arg1));
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
    std::array<std::string_view, 1> args{arg1};
    return getLog(redfish::registries::base::Index::resourceExhaustion, args);
}

void resourceExhaustion(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, resourceExhaustion(arg1));
}

/**
 * @internal
 * @brief Formats AccountModified message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json accountModified()
{
    return getLog(redfish::registries::base::Index::accountModified, {});
}

void accountModified(crow::Response& res)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, accountModified());
}

/**
 * @internal
 * @brief Formats QueryParameterOutOfRange message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json queryParameterOutOfRange(
    std::string_view arg1, std::string_view arg2, std::string_view arg3)
{
    std::array<std::string_view, 3> args{arg1, arg2, arg3};
    return getLog(redfish::registries::base::Index::queryParameterOutOfRange,
                  args);
}

void queryParameterOutOfRange(crow::Response& res, std::string_view arg1,
                              std::string_view arg2, std::string_view arg3)
{
    res.result(boost::beast::http::status::unknown?);
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
nlohmann::json passwordChangeRequired(const boost::urls::url_view_base& arg1)
{
    std::array<std::string_view, 1> args{arg1.buffer()};
    return getLog(redfish::registries::base::Index::passwordChangeRequired,
                  args);
}

void passwordChangeRequired(crow::Response& res,
                            const boost::urls::url_view_base& arg1)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, passwordChangeRequired(arg1));
}

/**
 * @internal
 * @brief Formats InvalidUpload message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json invalidUpload(std::string_view arg1, std::string_view arg2)
{
    std::array<std::string_view, 2> args{arg1, arg2};
    return getLog(redfish::registries::base::Index::invalidUpload, args);
}

void invalidUpload(crow::Response& res, std::string_view arg1,
                   std::string_view arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, invalidUpload(arg1, arg2));
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
    res.result(boost::beast::http::status::unknown?);
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
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, operationNotAllowed());
}

/**
 * @internal
 * @brief Formats ArraySizeTooLong message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json arraySizeTooLong(std::string_view arg1, uint64_t arg2)
{
    std::array<std::string_view, 2> args{arg1, arg2};
    return getLog(redfish::registries::base::Index::arraySizeTooLong, args);
}

void arraySizeTooLong(crow::Response& res, std::string_view arg1, uint64_t arg2)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, arraySizeTooLong(arg1, arg2));
}

/**
 * @internal
 * @brief Formats GenerateSecretKeyRequired message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json generateSecretKeyRequired(const boost::urls::url_view_base& arg1)
{
    std::array<std::string_view, 1> args{arg1.buffer()};
    return getLog(redfish::registries::base::Index::generateSecretKeyRequired,
                  args);
}

void generateSecretKeyRequired(crow::Response& res,
                               const boost::urls::url_view_base& arg1)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, generateSecretKeyRequired(arg1));
}

/**
 * @internal
 * @brief Formats PropertyNotUpdated message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json propertyNotUpdated(std::string_view arg1)
{
    std::array<std::string_view, 1> args{arg1};
    return getLog(redfish::registries::base::Index::propertyNotUpdated, args);
}

void propertyNotUpdated(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, propertyNotUpdated(arg1));
}

/**
 * @internal
 * @brief Formats InvalidJSON message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json invalidJSON(std::string_view arg1)
{
    std::array<std::string_view, 1> args{arg1};
    return getLog(redfish::registries::base::Index::invalidJSON, args);
}

void invalidJSON(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, invalidJSON(arg1));
}

/**
 * @internal
 * @brief Formats ActionParameterValueOutOfRange message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json actionParameterValueOutOfRange(
    std::string_view arg1, std::string_view arg2, std::string_view arg3)
{
    std::array<std::string_view, 3> args{arg1, arg2, arg3};
    return getLog(
        redfish::registries::base::Index::actionParameterValueOutOfRange, args);
}

void actionParameterValueOutOfRange(crow::Response& res, std::string_view arg1,
                                    std::string_view arg2,
                                    std::string_view arg3)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue,
                          actionParameterValueOutOfRange(arg1, arg2, arg3));
}

/**
 * @internal
 * @brief Formats ArraySizeTooShort message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json arraySizeTooShort(std::string_view arg1, std::string_view arg2)
{
    std::array<std::string_view, 2> args{arg1, arg2};
    return getLog(redfish::registries::base::Index::arraySizeTooShort, args);
}

void arraySizeTooShort(crow::Response& res, std::string_view arg1,
                       std::string_view arg2)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, arraySizeTooShort(arg1, arg2));
}

/**
 * @internal
 * @brief Formats QueryParameterValueError message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json queryParameterValueError(std::string_view arg1)
{
    std::array<std::string_view, 1> args{arg1};
    return getLog(redfish::registries::base::Index::queryParameterValueError,
                  args);
}

void queryParameterValueError(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, queryParameterValueError(arg1));
}

/**
 * @internal
 * @brief Formats QueryParameterUnsupported message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json queryParameterUnsupported(std::string_view arg1)
{
    std::array<std::string_view, 1> args{arg1};
    return getLog(redfish::registries::base::Index::queryParameterUnsupported,
                  args);
}

void queryParameterUnsupported(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, queryParameterUnsupported(arg1));
}

/**
 * @internal
 * @brief Formats PayloadTooLarge message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json payloadTooLarge()
{
    return getLog(redfish::registries::base::Index::payloadTooLarge, {});
}

void payloadTooLarge(crow::Response& res)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, payloadTooLarge());
}

/**
 * @internal
 * @brief Formats MissingOrMalformedPart message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json missingOrMalformedPart()
{
    return getLog(redfish::registries::base::Index::missingOrMalformedPart, {});
}

void missingOrMalformedPart(crow::Response& res)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, missingOrMalformedPart());
}

/**
 * @internal
 * @brief Formats InvalidURI message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json invalidURI(std::string_view arg1)
{
    std::array<std::string_view, 1> args{arg1};
    return getLog(redfish::registries::base::Index::invalidURI, args);
}

void invalidURI(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, invalidURI(arg1));
}

/**
 * @internal
 * @brief Formats StringValueTooShort message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json stringValueTooShort(std::string_view arg1, std::string_view arg2)
{
    std::array<std::string_view, 2> args{arg1, arg2};
    return getLog(redfish::registries::base::Index::stringValueTooShort, args);
}

void stringValueTooShort(crow::Response& res, std::string_view arg1,
                         std::string_view arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, stringValueTooShort(arg1, arg2));
}

/**
 * @internal
 * @brief Formats ResetRecommended message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resetRecommended(std::string_view arg1, std::string_view arg2)
{
    std::array<std::string_view, 2> args{arg1, arg2};
    return getLog(redfish::registries::base::Index::resetRecommended, args);
}

void resetRecommended(crow::Response& res, std::string_view arg1,
                      std::string_view arg2)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, resetRecommended(arg1, arg2));
}

/**
 * @internal
 * @brief Formats ActionParameterValueConflict message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json
    actionParameterValueConflict(std::string_view arg1, std::string_view arg2)
{
    std::array<std::string_view, 2> args{arg1, arg2};
    return getLog(
        redfish::registries::base::Index::actionParameterValueConflict, args);
}

void actionParameterValueConflict(crow::Response& res, std::string_view arg1,
                                  std::string_view arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue,
                          actionParameterValueConflict(arg1, arg2));
}

/**
 * @internal
 * @brief Formats HeaderMissing message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json headerMissing(std::string_view arg1)
{
    std::array<std::string_view, 1> args{arg1};
    return getLog(redfish::registries::base::Index::headerMissing, args);
}

void headerMissing(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, headerMissing(arg1));
}

/**
 * @internal
 * @brief Formats HeaderInvalid message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json headerInvalid(std::string_view arg1)
{
    std::array<std::string_view, 1> args{arg1};
    return getLog(redfish::registries::base::Index::headerInvalid, args);
}

void headerInvalid(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, headerInvalid(arg1));
}

/**
 * @internal
 * @brief Formats UndeterminedFault message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json undeterminedFault(std::string_view arg1)
{
    std::array<std::string_view, 1> args{arg1};
    return getLog(redfish::registries::base::Index::undeterminedFault, args);
}

void undeterminedFault(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, undeterminedFault(arg1));
}

/**
 * @internal
 * @brief Formats ConditionInRelatedResource message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json conditionInRelatedResource()
{
    return getLog(redfish::registries::base::Index::conditionInRelatedResource,
                  {});
}

void conditionInRelatedResource(crow::Response& res)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, conditionInRelatedResource());
}

/**
 * @internal
 * @brief Formats RestrictedRole message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json restrictedRole(std::string_view arg1)
{
    std::array<std::string_view, 1> args{arg1};
    return getLog(redfish::registries::base::Index::restrictedRole, args);
}

void restrictedRole(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, restrictedRole(arg1));
}

/**
 * @internal
 * @brief Formats RestrictedPrivilege message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json restrictedPrivilege(std::string_view arg1)
{
    std::array<std::string_view, 1> args{arg1};
    return getLog(redfish::registries::base::Index::restrictedPrivilege, args);
}

void restrictedPrivilege(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, restrictedPrivilege(arg1));
}

/**
 * @internal
 * @brief Formats PropertyDeprecated message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json propertyDeprecated(std::string_view arg1)
{
    std::array<std::string_view, 1> args{arg1};
    return getLog(redfish::registries::base::Index::propertyDeprecated, args);
}

void propertyDeprecated(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, propertyDeprecated(arg1));
}

/**
 * @internal
 * @brief Formats ResourceDeprecated message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourceDeprecated(std::string_view arg1)
{
    std::array<std::string_view, 1> args{arg1};
    return getLog(redfish::registries::base::Index::resourceDeprecated, args);
}

void resourceDeprecated(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, resourceDeprecated(arg1));
}

/**
 * @internal
 * @brief Formats PropertyValueDeprecated message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json propertyValueDeprecated(std::string_view arg1,
                                       std::string_view arg2)
{
    std::array<std::string_view, 2> args{arg1, arg2};
    return getLog(redfish::registries::base::Index::propertyValueDeprecated,
                  args);
}

void propertyValueDeprecated(crow::Response& res, std::string_view arg1,
                             std::string_view arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, propertyValueDeprecated(arg1, arg2));
}

/**
 * @internal
 * @brief Formats ActionDeprecated message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json actionDeprecated(std::string_view arg1)
{
    std::array<std::string_view, 1> args{arg1};
    return getLog(redfish::registries::base::Index::actionDeprecated, args);
}

void actionDeprecated(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, actionDeprecated(arg1));
}

/**
 * @internal
 * @brief Formats NetworkNameResolutionNotConfigured message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json networkNameResolutionNotConfigured()
{
    return getLog(
        redfish::registries::base::Index::networkNameResolutionNotConfigured,
        {});
}

void networkNameResolutionNotConfigured(crow::Response& res)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, networkNameResolutionNotConfigured());
}

/**
 * @internal
 * @brief Formats NetworkNameResolutionNotSupported message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json networkNameResolutionNotSupported()
{
    return getLog(
        redfish::registries::base::Index::networkNameResolutionNotSupported,
        {});
}

void networkNameResolutionNotSupported(crow::Response& res)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, networkNameResolutionNotSupported());
}

/**
 * @internal
 * @brief Formats AuthenticationTokenRequired message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json authenticationTokenRequired()
{
    return getLog(redfish::registries::base::Index::authenticationTokenRequired,
                  {});
}

void authenticationTokenRequired(crow::Response& res)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, authenticationTokenRequired());
}

/**
 * @internal
 * @brief Formats OneTimePasscodeSent message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json oneTimePasscodeSent(std::string_view arg1)
{
    std::array<std::string_view, 1> args{arg1};
    return getLog(redfish::registries::base::Index::oneTimePasscodeSent, args);
}

void oneTimePasscodeSent(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, oneTimePasscodeSent(arg1));
}

/**
 * @internal
 * @brief Formats LicenseRequired message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json licenseRequired(std::string_view arg1)
{
    std::array<std::string_view, 1> args{arg1};
    return getLog(redfish::registries::base::Index::licenseRequired, args);
}

void licenseRequired(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, licenseRequired(arg1));
}

/**
 * @internal
 * @brief Formats PropertyModified message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json propertyModified()
{
    return getLog(redfish::registries::base::Index::propertyModified, {});
}

void propertyModified(crow::Response& res)
{
    res.result(boost::beast::http::status::unknown?);
    addMessageToErrorJson(res.jsonValue, propertyModified());
}

} // namespace messages
} // namespace redfish
