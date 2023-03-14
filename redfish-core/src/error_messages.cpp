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

#include "error_message_utils.hpp"
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

// Clang can't seem to decide whether this header needs to be included or not,
// and is inconsistent.  Include it for now
// NOLINTNEXTLINE(misc-include-cleaner)
#include <utility>

namespace redfish
{

namespace messages
{

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
    addMessageToJsonRoot(res.jsonValue, success());
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
    res.result(boost::beast::http::status::internal_server_error);
    addMessageToErrorJson(res.jsonValue, generalError());
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
nlohmann::json noOperation()
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
 * @brief Formats PropertyUnknown message into JSON
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
 * @brief Formats PropertyValueTypeError message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json propertyValueTypeError(const nlohmann::json& arg1,
                                      std::string_view arg2)
{
    std::string arg1Str =
        arg1.dump(-1, ' ', true, nlohmann::json::error_handler_t::replace);
    return getLog(redfish::registries::base::Index::propertyValueTypeError,
                  std::to_array<std::string_view>({arg1Str, arg2}));
}

void propertyValueTypeError(crow::Response& res, const nlohmann::json& arg1,
                            std::string_view arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToJson(res.jsonValue, propertyValueTypeError(arg1, arg2), arg2);
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
        arg1.dump(-1, ' ', true, nlohmann::json::error_handler_t::replace);
    return getLog(redfish::registries::base::Index::propertyValueFormatError,
                  std::to_array<std::string_view>({arg1Str, arg2}));
}

void propertyValueFormatError(crow::Response& res, const nlohmann::json& arg1,
                              std::string_view arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToJson(res.jsonValue, propertyValueFormatError(arg1, arg2), arg2);
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
        arg1.dump(-1, ' ', true, nlohmann::json::error_handler_t::replace);
    return getLog(redfish::registries::base::Index::propertyValueNotInList,
                  std::to_array<std::string_view>({arg1Str, arg2}));
}

void propertyValueNotInList(crow::Response& res, const nlohmann::json& arg1,
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
nlohmann::json propertyValueOutOfRange(const nlohmann::json& arg1,
                                       std::string_view arg2)
{
    std::string arg1Str =
        arg1.dump(-1, ' ', true, nlohmann::json::error_handler_t::replace);
    return getLog(redfish::registries::base::Index::propertyValueOutOfRange,
                  std::to_array<std::string_view>({arg1Str, arg2}));
}

void propertyValueOutOfRange(crow::Response& res, const nlohmann::json& arg1,
                             std::string_view arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, propertyValueOutOfRange(arg1, arg2));
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
    return getLog(redfish::registries::base::Index::propertyValueError,
                  std::to_array({arg1}));
}

void propertyValueError(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToJson(res.jsonValue, propertyValueError(arg1), arg1);
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
 * @brief Formats PropertyNotUpdated message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json propertyNotUpdated(std::string_view arg1)
{
    return getLog(redfish::registries::base::Index::propertyNotUpdated,
                  std::to_array({arg1}));
}

void propertyNotUpdated(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, propertyNotUpdated(arg1));
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
 * @brief Formats InvalidJSON message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json invalidJSON(std::string_view arg1)
{
    return getLog(redfish::registries::base::Index::invalidJSON,
                  std::to_array({arg1}));
}

void invalidJSON(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, invalidJSON(arg1));
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
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, emptyJSON());
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
 * @brief Formats ActionParameterValueTypeError message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json actionParameterValueTypeError(
    const nlohmann::json& arg1, std::string_view arg2, std::string_view arg3)
{
    std::string arg1Str =
        arg1.dump(-1, ' ', true, nlohmann::json::error_handler_t::replace);
    return getLog(
        redfish::registries::base::Index::actionParameterValueTypeError,
        std::to_array<std::string_view>({arg1Str, arg2, arg3}));
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
 * @brief Formats ActionParameterValueFormatError message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json actionParameterValueFormatError(
    const nlohmann::json& arg1, std::string_view arg2, std::string_view arg3)
{
    std::string arg1Str =
        arg1.dump(-1, ' ', true, nlohmann::json::error_handler_t::replace);
    return getLog(
        redfish::registries::base::Index::actionParameterValueFormatError,
        std::to_array<std::string_view>({arg1Str, arg2, arg3}));
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
 * @brief Formats ActionParameterValueOutOfRange message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json actionParameterValueOutOfRange(
    std::string_view arg1, std::string_view arg2, std::string_view arg3)
{
    return getLog(
        redfish::registries::base::Index::actionParameterValueOutOfRange,
        std::to_array({arg1, arg2, arg3}));
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
 * @brief Formats ActionParameterValueError message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json actionParameterValueError(const nlohmann::json& arg1,
                                         std::string_view arg2)
{
    std::string arg1Str =
        arg1.dump(-1, ' ', true, nlohmann::json::error_handler_t::replace);
    return getLog(redfish::registries::base::Index::actionParameterValueError,
                  std::to_array<std::string_view>({arg1Str, arg2}));
}

void actionParameterValueError(crow::Response& res, const nlohmann::json& arg1,
                               std::string_view arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, actionParameterValueError(arg1, arg2));
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
 * @brief Formats ArraySizeTooLong message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json arraySizeTooLong(std::string_view arg1, uint64_t arg2)
{
    std::string arg2Str = std::to_string(arg2);
    return getLog(redfish::registries::base::Index::arraySizeTooLong,
                  std::to_array<std::string_view>({arg1, arg2Str}));
}

void arraySizeTooLong(crow::Response& res, std::string_view arg1, uint64_t arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, arraySizeTooLong(arg1, arg2));
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
    return getLog(redfish::registries::base::Index::arraySizeTooShort,
                  std::to_array({arg1, arg2}));
}

void arraySizeTooShort(crow::Response& res, std::string_view arg1,
                       std::string_view arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, arraySizeTooShort(arg1, arg2));
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
        arg1.dump(-1, ' ', true, nlohmann::json::error_handler_t::replace);
    return getLog(
        redfish::registries::base::Index::queryParameterValueTypeError,
        std::to_array<std::string_view>({arg1Str, arg2}));
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
 * @brief Formats QueryParameterValueFormatError message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json queryParameterValueFormatError(const nlohmann::json& arg1,
                                              std::string_view arg2)
{
    std::string arg1Str =
        arg1.dump(-1, ' ', true, nlohmann::json::error_handler_t::replace);
    return getLog(
        redfish::registries::base::Index::queryParameterValueFormatError,
        std::to_array<std::string_view>({arg1Str, arg2}));
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
 * @brief Formats QueryParameterValueError message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json queryParameterValueError(std::string_view arg1)
{
    return getLog(redfish::registries::base::Index::queryParameterValueError,
                  std::to_array({arg1}));
}

void queryParameterValueError(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, queryParameterValueError(arg1));
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
 * @brief Formats QueryParameterUnsupported message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json queryParameterUnsupported(std::string_view arg1)
{
    return getLog(redfish::registries::base::Index::queryParameterUnsupported,
                  std::to_array({arg1}));
}

void queryParameterUnsupported(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, queryParameterUnsupported(arg1));
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
    res.result(boost::beast::http::status::service_unavailable);
    addMessageToErrorJson(res.jsonValue, sessionLimitExceeded());
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
    res.result(boost::beast::http::status::service_unavailable);
    addMessageToErrorJson(res.jsonValue, eventSubscriptionLimitExceeded());
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
    res.result(boost::beast::http::status::method_not_allowed);
    addMessageToErrorJson(res.jsonValue, resourceCannotBeDeleted());
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
    res.result(boost::beast::http::status::service_unavailable);
    addMessageToErrorJson(res.jsonValue, resourceInUse());
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
 * @brief Formats ResourceNotFound message into JSON
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
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, payloadTooLarge());
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
    return getLog(redfish::registries::base::Index::invalidURI,
                  std::to_array({arg1}));
}

void invalidURI(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, invalidURI(arg1));
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
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, createLimitReachedForResource());
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
    res.result(boost::beast::http::status::service_unavailable);
    addMessageToErrorJson(res.jsonValue, serviceShuttingDown());
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
    res.result(boost::beast::http::status::service_unavailable);
    addMessageToErrorJson(res.jsonValue, serviceInUnknownState());
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
    res.result(boost::beast::http::status::forbidden);
    addMessageToErrorJson(res.jsonValue, noValidSession());
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
    res.result(boost::beast::http::status::forbidden);
    addMessageToErrorJson(res.jsonValue, insufficientPrivilege());
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
    res.result(boost::beast::http::status::ok);
    addMessageToErrorJson(res.jsonValue, accountModified());
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
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, accountNotModified());
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
    res.result(boost::beast::http::status::ok);
    addMessageToJsonRoot(res.jsonValue, accountRemoved());
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
    res.result(boost::beast::http::status::forbidden);
    addMessageToErrorJson(res.jsonValue, accountForSessionNoLongerExists());
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
    return getLog(redfish::registries::base::Index::invalidObject,
                  std::to_array<std::string_view>({arg1.buffer()}));
}

void invalidObject(crow::Response& res, const boost::urls::url_view_base& arg1)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, invalidObject(arg1));
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

void internalError(crow::Response& res, const std::source_location location)
{
    BMCWEB_LOG_CRITICAL("Internal Error {}({}:{}) `{}`: ", location.file_name(),
                        location.line(), location.column(),
                        location.function_name());
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
 * @brief Formats ResourceMissingAtURI message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourceMissingAtURI(const boost::urls::url_view_base& arg1)
{
    return getLog(redfish::registries::base::Index::resourceMissingAtURI,
                  std::to_array<std::string_view>({arg1.buffer()}));
}

void resourceMissingAtURI(crow::Response& res,
                          const boost::urls::url_view_base& arg1)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, resourceMissingAtURI(arg1));
}

/**
 * @internal
 * @brief Formats ResourceAtUriInUnknownFormat message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourceAtUriInUnknownFormat(
    const boost::urls::url_view_base& arg1)
{
    return getLog(
        redfish::registries::base::Index::resourceAtUriInUnknownFormat,
        std::to_array<std::string_view>({arg1.buffer()}));
}

void resourceAtUriInUnknownFormat(crow::Response& res,
                                  const boost::urls::url_view_base& arg1)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, resourceAtUriInUnknownFormat(arg1));
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
    return getLog(redfish::registries::base::Index::resourceAtUriUnauthorized,
                  std::to_array<std::string_view>({arg1.buffer(), arg2}));
}

void resourceAtUriUnauthorized(crow::Response& res,
                               const boost::urls::url_view_base& arg1,
                               std::string_view arg2)
{
    res.result(boost::beast::http::status::unauthorized);
    addMessageToErrorJson(res.jsonValue, resourceAtUriUnauthorized(arg1, arg2));
}

/**
 * @internal
 * @brief Formats CouldNotEstablishConnection message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json couldNotEstablishConnection(
    const boost::urls::url_view_base& arg1)
{
    return getLog(redfish::registries::base::Index::couldNotEstablishConnection,
                  std::to_array<std::string_view>({arg1.buffer()}));
}

void couldNotEstablishConnection(crow::Response& res,
                                 const boost::urls::url_view_base& arg1)
{
    res.result(boost::beast::http::status::not_found);
    addMessageToErrorJson(res.jsonValue, couldNotEstablishConnection(arg1));
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
    return getLog(
        redfish::registries::base::Index::sourceDoesNotSupportProtocol,
        std::to_array<std::string_view>({arg1.buffer(), arg2}));
}

void sourceDoesNotSupportProtocol(crow::Response& res,
                                  const boost::urls::url_view_base& arg1,
                                  std::string_view arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue,
                          sourceDoesNotSupportProtocol(arg1, arg2));
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
    return getLog(redfish::registries::base::Index::accessDenied,
                  std::to_array<std::string_view>({arg1.buffer()}));
}

void accessDenied(crow::Response& res, const boost::urls::url_view_base& arg1)
{
    res.result(boost::beast::http::status::forbidden);
    addMessageToErrorJson(res.jsonValue, accessDenied(arg1));
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
 * @brief Formats InvalidIndex message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json invalidIndex(uint64_t arg1)
{
    std::string arg1Str = std::to_string(arg1);
    return getLog(redfish::registries::base::Index::invalidIndex,
                  std::to_array<std::string_view>({arg1Str}));
}

void invalidIndex(crow::Response& res, uint64_t arg1)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, invalidIndex(arg1));
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
        arg2.dump(-1, ' ', true, nlohmann::json::error_handler_t::replace);
    return getLog(redfish::registries::base::Index::propertyValueModified,
                  std::to_array<std::string_view>({arg1, arg2Str}));
}

void propertyValueModified(crow::Response& res, std::string_view arg1,
                           const nlohmann::json& arg2)
{
    res.result(boost::beast::http::status::ok);
    addMessageToJson(res.jsonValue, propertyValueModified(arg1, arg2), arg1);
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
    res.result(boost::beast::http::status::service_unavailable);
    addMessageToErrorJson(res.jsonValue, resourceInStandby());
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
 * @brief Formats StringValueTooLong message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json stringValueTooLong(std::string_view arg1, uint64_t arg2)
{
    std::string arg2Str = std::to_string(arg2);
    return getLog(redfish::registries::base::Index::stringValueTooLong,
                  std::to_array<std::string_view>({arg1, arg2Str}));
}

void stringValueTooLong(crow::Response& res, std::string_view arg1,
                        uint64_t arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, stringValueTooLong(arg1, arg2));
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
    return getLog(redfish::registries::base::Index::stringValueTooShort,
                  std::to_array({arg1, arg2}));
}

void stringValueTooShort(crow::Response& res, std::string_view arg1,
                         std::string_view arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, stringValueTooShort(arg1, arg2));
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
nlohmann::json subscriptionTerminated()
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
 * @brief Formats PasswordChangeRequired message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json passwordChangeRequired(const boost::urls::url_view_base& arg1)
{
    return getLog(redfish::registries::base::Index::passwordChangeRequired,
                  std::to_array<std::string_view>({arg1.buffer()}));
}

void passwordChangeRequired(crow::Response& res,
                            const boost::urls::url_view_base& arg1)
{
    addMessageToJsonRoot(res.jsonValue, passwordChangeRequired(arg1));
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
    return getLog(redfish::registries::base::Index::resetRequired,
                  std::to_array<std::string_view>({arg1.buffer(), arg2}));
}

void resetRequired(crow::Response& res, const boost::urls::url_view_base& arg1,
                   std::string_view arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, resetRequired(arg1, arg2));
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
    return getLog(redfish::registries::base::Index::resetRecommended,
                  std::to_array({arg1, arg2}));
}

void resetRecommended(crow::Response& res, std::string_view arg1,
                      std::string_view arg2)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, resetRecommended(arg1, arg2));
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
    return getLog(redfish::registries::base::Index::chassisPowerStateOnRequired,
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
nlohmann::json propertyValueResourceConflict(
    std::string_view arg1, const nlohmann::json& arg2,
    const boost::urls::url_view_base& arg3)
{
    std::string arg2Str =
        arg2.dump(-1, ' ', true, nlohmann::json::error_handler_t::replace);
    return getLog(
        redfish::registries::base::Index::propertyValueResourceConflict,
        std::to_array<std::string_view>({arg1, arg2Str, arg3.buffer()}));
}

void propertyValueResourceConflict(crow::Response& res, std::string_view arg1,
                                   const nlohmann::json& arg2,
                                   const boost::urls::url_view_base& arg3)
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
                                             const nlohmann::json& arg2)
{
    std::string arg2Str =
        arg2.dump(-1, ' ', true, nlohmann::json::error_handler_t::replace);
    return getLog(
        redfish::registries::base::Index::propertyValueExternalConflict,
        std::to_array<std::string_view>({arg1, arg2Str}));
}

void propertyValueExternalConflict(crow::Response& res, std::string_view arg1,
                                   const nlohmann::json& arg2)
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
                                      const nlohmann::json& arg2)
{
    std::string arg2Str =
        arg2.dump(-1, ' ', true, nlohmann::json::error_handler_t::replace);
    return getLog(redfish::registries::base::Index::propertyValueIncorrect,
                  std::to_array<std::string_view>({arg1, arg2Str}));
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
    return getLog(redfish::registries::base::Index::resourceCreationConflict,
                  std::to_array<std::string_view>({arg1.buffer()}));
}

void resourceCreationConflict(crow::Response& res,
                              const boost::urls::url_view_base& arg1)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, resourceCreationConflict(arg1));
}

/**
 * @internal
 * @brief Formats ActionParameterValueConflict message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json actionParameterValueConflict(std::string_view arg1,
                                            std::string_view arg2)
{
    return getLog(
        redfish::registries::base::Index::actionParameterValueConflict,
        std::to_array({arg1, arg2}));
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
nlohmann::json preconditionFailed()
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
 * @brief Formats HeaderMissing message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json headerMissing(std::string_view arg1)
{
    return getLog(redfish::registries::base::Index::headerMissing,
                  std::to_array({arg1}));
}

void headerMissing(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::bad_request);
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
    return getLog(redfish::registries::base::Index::headerInvalid,
                  std::to_array({arg1}));
}

void headerInvalid(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, headerInvalid(arg1));
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
nlohmann::json operationTimeout()
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

/**
 * @internal
 * @brief Formats UndeterminedFault message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json undeterminedFault(std::string_view arg1)
{
    return getLog(redfish::registries::base::Index::undeterminedFault,
                  std::to_array({arg1}));
}

void undeterminedFault(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::bad_request);
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
    res.result(boost::beast::http::status::bad_request);
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
    return getLog(redfish::registries::base::Index::restrictedRole,
                  std::to_array({arg1}));
}

void restrictedRole(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::forbidden);
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
    return getLog(redfish::registries::base::Index::restrictedPrivilege,
                  std::to_array({arg1}));
}

void restrictedPrivilege(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, restrictedPrivilege(arg1));
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
 * @brief Formats PropertyDeprecated message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json propertyDeprecated(std::string_view arg1)
{
    return getLog(redfish::registries::base::Index::propertyDeprecated,
                  std::to_array({arg1}));
}

void propertyDeprecated(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::bad_request);
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
    return getLog(redfish::registries::base::Index::resourceDeprecated,
                  std::to_array({arg1}));
}

void resourceDeprecated(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::bad_request);
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
    return getLog(redfish::registries::base::Index::propertyValueDeprecated,
                  std::to_array({arg1, arg2}));
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
    return getLog(redfish::registries::base::Index::actionDeprecated,
                  std::to_array({arg1}));
}

void actionDeprecated(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::bad_request);
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
    res.result(boost::beast::http::status::bad_request);
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
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, eventBufferExceeded());
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
    res.result(boost::beast::http::status::bad_request);
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
    return getLog(redfish::registries::base::Index::oneTimePasscodeSent,
                  std::to_array({arg1}));
}

void oneTimePasscodeSent(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::bad_request);
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
    return getLog(redfish::registries::base::Index::licenseRequired,
                  std::to_array({arg1}));
}

void licenseRequired(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::bad_request);
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
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, propertyModified());
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
    return getLog(redfish::registries::base::Index::generateSecretKeyRequired,
                  std::to_array<std::string_view>({arg1.buffer()}));
}

void generateSecretKeyRequired(crow::Response& res,
                               const boost::urls::url_view_base& arg1)
{
    res.result(boost::beast::http::status::forbidden);
    addMessageToErrorJson(res.jsonValue, generateSecretKeyRequired(arg1));
}

} // namespace messages
} // namespace redfish
