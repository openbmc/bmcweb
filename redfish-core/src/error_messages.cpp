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
#include <source_location>
#include <span>
#include <string_view>

// Clang can't seem to decide whether this header needs to be included or not,
// and is inconsistent.  Include it for now
// NOLINTNEXTLINE(misc-include-cleaner)
#include <cstdint>
// NOLINTNEXTLINE(misc-include-cleaner)
#include <string>
// NOLINTNEXTLINE(misc-include-cleaner)
#include <utility>

namespace redfish
{

namespace messages
{

static nlohmann::json::object_t getLog(redfish::registries::Base::Index name,
                                       std::span<const std::string_view> args)
{
    size_t index = static_cast<size_t>(name);
    if (index >= redfish::registries::Base::registry.size())
    {
        return {};
    }
    return getLogFromRegistry(redfish::registries::Base::header,
                              redfish::registries::Base::registry, index, args);
}

/**
 * @internal
 * @brief Formats Success message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json::object_t success()
{
    return getLog(redfish::registries::Base::Index::success, {});
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
nlohmann::json::object_t generalError()
{
    return getLog(redfish::registries::Base::Index::generalError, {});
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
nlohmann::json::object_t created()
{
    return getLog(redfish::registries::Base::Index::created, {});
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
nlohmann::json::object_t noOperation()
{
    return getLog(redfish::registries::Base::Index::noOperation, {});
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
nlohmann::json::object_t propertyDuplicate(std::string_view arg1)
{
    return getLog(redfish::registries::Base::Index::propertyDuplicate,
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
nlohmann::json::object_t propertyUnknown(std::string_view arg1)
{
    return getLog(redfish::registries::Base::Index::propertyUnknown,
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
nlohmann::json::object_t propertyValueTypeError(const nlohmann::json& arg1,
                                                std::string_view arg2)
{
    std::string arg1Str =
        arg1.dump(-1, ' ', true, nlohmann::json::error_handler_t::replace);
    return getLog(redfish::registries::Base::Index::propertyValueTypeError,
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
nlohmann::json::object_t propertyValueFormatError(const nlohmann::json& arg1,
                                                  std::string_view arg2)
{
    std::string arg1Str =
        arg1.dump(-1, ' ', true, nlohmann::json::error_handler_t::replace);
    return getLog(redfish::registries::Base::Index::propertyValueFormatError,
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
nlohmann::json::object_t propertyValueNotInList(const nlohmann::json& arg1,
                                                std::string_view arg2)
{
    std::string arg1Str =
        arg1.dump(-1, ' ', true, nlohmann::json::error_handler_t::replace);
    return getLog(redfish::registries::Base::Index::propertyValueNotInList,
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
nlohmann::json::object_t propertyValueOutOfRange(const nlohmann::json& arg1,
                                                 std::string_view arg2)
{
    std::string arg1Str =
        arg1.dump(-1, ' ', true, nlohmann::json::error_handler_t::replace);
    return getLog(redfish::registries::Base::Index::propertyValueOutOfRange,
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
nlohmann::json::object_t propertyValueError(std::string_view arg1)
{
    return getLog(redfish::registries::Base::Index::propertyValueError,
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
nlohmann::json::object_t propertyNotWritable(std::string_view arg1)
{
    return getLog(redfish::registries::Base::Index::propertyNotWritable,
                  std::to_array({arg1}));
}

void propertyNotWritable(crow::Response& res, std::string_view arg1)
{
    res.result(boost::beast::http::status::method_not_allowed);
    addMessageToJson(res.jsonValue, propertyNotWritable(arg1), arg1);
}

/**
 * @internal
 * @brief Formats PropertyNotUpdated message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json::object_t propertyNotUpdated(std::string_view arg1)
{
    return getLog(redfish::registries::Base::Index::propertyNotUpdated,
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
nlohmann::json::object_t propertyMissing(std::string_view arg1)
{
    return getLog(redfish::registries::Base::Index::propertyMissing,
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
nlohmann::json::object_t malformedJSON()
{
    return getLog(redfish::registries::Base::Index::malformedJSON, {});
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
nlohmann::json::object_t invalidJSON(uint64_t arg1)
{
    std::string arg1Str = std::to_string(arg1);
    return getLog(redfish::registries::Base::Index::invalidJSON,
                  std::to_array<std::string_view>({arg1Str}));
}

void invalidJSON(crow::Response& res, uint64_t arg1)
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
nlohmann::json::object_t emptyJSON()
{
    return getLog(redfish::registries::Base::Index::emptyJSON, {});
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
nlohmann::json::object_t actionNotSupported(std::string_view arg1)
{
    return getLog(redfish::registries::Base::Index::actionNotSupported,
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
nlohmann::json::object_t actionParameterMissing(std::string_view arg1,
                                                std::string_view arg2)
{
    return getLog(redfish::registries::Base::Index::actionParameterMissing,
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
nlohmann::json::object_t actionParameterDuplicate(std::string_view arg1,
                                                  std::string_view arg2)
{
    return getLog(redfish::registries::Base::Index::actionParameterDuplicate,
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
nlohmann::json::object_t actionParameterUnknown(std::string_view arg1,
                                                std::string_view arg2)
{
    return getLog(redfish::registries::Base::Index::actionParameterUnknown,
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
nlohmann::json::object_t actionParameterValueTypeError(
    const nlohmann::json& arg1, std::string_view arg2, std::string_view arg3)
{
    std::string arg1Str =
        arg1.dump(-1, ' ', true, nlohmann::json::error_handler_t::replace);
    return getLog(
        redfish::registries::Base::Index::actionParameterValueTypeError,
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
nlohmann::json::object_t actionParameterValueFormatError(
    const nlohmann::json& arg1, std::string_view arg2, std::string_view arg3)
{
    std::string arg1Str =
        arg1.dump(-1, ' ', true, nlohmann::json::error_handler_t::replace);
    return getLog(
        redfish::registries::Base::Index::actionParameterValueFormatError,
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
nlohmann::json::object_t actionParameterValueNotInList(
    std::string_view arg1, std::string_view arg2, std::string_view arg3)
{
    return getLog(
        redfish::registries::Base::Index::actionParameterValueNotInList,
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
nlohmann::json::object_t actionParameterValueOutOfRange(
    std::string_view arg1, std::string_view arg2, std::string_view arg3)
{
    return getLog(
        redfish::registries::Base::Index::actionParameterValueOutOfRange,
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
nlohmann::json::object_t actionParameterValueError(const nlohmann::json& arg1,
                                                   std::string_view arg2)
{
    std::string arg1Str =
        arg1.dump(-1, ' ', true, nlohmann::json::error_handler_t::replace);
    return getLog(redfish::registries::Base::Index::actionParameterValueError,
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
nlohmann::json::object_t actionParameterNotSupported(std::string_view arg1,
                                                     std::string_view arg2)
{
    return getLog(redfish::registries::Base::Index::actionParameterNotSupported,
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
nlohmann::json::object_t arraySizeTooLong(std::string_view arg1, uint64_t arg2)
{
    std::string arg2Str = std::to_string(arg2);
    return getLog(redfish::registries::Base::Index::arraySizeTooLong,
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
nlohmann::json::object_t arraySizeTooShort(std::string_view arg1, uint64_t arg2)
{
    std::string arg2Str = std::to_string(arg2);
    return getLog(redfish::registries::Base::Index::arraySizeTooShort,
                  std::to_array<std::string_view>({arg1, arg2Str}));
}

void arraySizeTooShort(crow::Response& res, std::string_view arg1,
                       uint64_t arg2)
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
nlohmann::json::object_t queryParameterValueTypeError(
    const nlohmann::json& arg1, std::string_view arg2)
{
    std::string arg1Str =
        arg1.dump(-1, ' ', true, nlohmann::json::error_handler_t::replace);
    return getLog(
        redfish::registries::Base::Index::queryParameterValueTypeError,
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
nlohmann::json::object_t queryParameterValueFormatError(
    const nlohmann::json& arg1, std::string_view arg2)
{
    std::string arg1Str =
        arg1.dump(-1, ' ', true, nlohmann::json::error_handler_t::replace);
    return getLog(
        redfish::registries::Base::Index::queryParameterValueFormatError,
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
nlohmann::json::object_t queryParameterValueError(std::string_view arg1)
{
    return getLog(redfish::registries::Base::Index::queryParameterValueError,
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
nlohmann::json::object_t queryParameterOutOfRange(
    std::string_view arg1, std::string_view arg2, std::string_view arg3)
{
    return getLog(redfish::registries::Base::Index::queryParameterOutOfRange,
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
nlohmann::json::object_t queryNotSupportedOnResource()
{
    return getLog(redfish::registries::Base::Index::queryNotSupportedOnResource,
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
nlohmann::json::object_t queryNotSupportedOnOperation()
{
    return getLog(
        redfish::registries::Base::Index::queryNotSupportedOnOperation, {});
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
nlohmann::json::object_t queryNotSupported()
{
    return getLog(redfish::registries::Base::Index::queryNotSupported, {});
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
nlohmann::json::object_t queryCombinationInvalid()
{
    return getLog(redfish::registries::Base::Index::queryCombinationInvalid,
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
nlohmann::json::object_t queryParameterUnsupported(std::string_view arg1)
{
    return getLog(redfish::registries::Base::Index::queryParameterUnsupported,
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
nlohmann::json::object_t sessionLimitExceeded()
{
    return getLog(redfish::registries::Base::Index::sessionLimitExceeded, {});
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
nlohmann::json::object_t eventSubscriptionLimitExceeded()
{
    return getLog(
        redfish::registries::Base::Index::eventSubscriptionLimitExceeded, {});
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
nlohmann::json::object_t resourceCannotBeDeleted()
{
    return getLog(redfish::registries::Base::Index::resourceCannotBeDeleted,
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
nlohmann::json::object_t resourceInUse()
{
    return getLog(redfish::registries::Base::Index::resourceInUse, {});
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
nlohmann::json::object_t resourceAlreadyExists(
    std::string_view arg1, std::string_view arg2, std::string_view arg3)
{
    return getLog(redfish::registries::Base::Index::resourceAlreadyExists,
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
nlohmann::json::object_t resourceNotFound(std::string_view arg1,
                                          std::string_view arg2)
{
    return getLog(redfish::registries::Base::Index::resourceNotFound,
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
nlohmann::json::object_t payloadTooLarge()
{
    return getLog(redfish::registries::Base::Index::payloadTooLarge, {});
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
nlohmann::json::object_t insufficientStorage()
{
    return getLog(redfish::registries::Base::Index::insufficientStorage, {});
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
nlohmann::json::object_t missingOrMalformedPart()
{
    return getLog(redfish::registries::Base::Index::missingOrMalformedPart, {});
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
nlohmann::json::object_t invalidURI(std::string_view arg1)
{
    return getLog(redfish::registries::Base::Index::invalidURI,
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
nlohmann::json::object_t createFailedMissingReqProperties(std::string_view arg1)
{
    return getLog(
        redfish::registries::Base::Index::createFailedMissingReqProperties,
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
nlohmann::json::object_t createLimitReachedForResource()
{
    return getLog(
        redfish::registries::Base::Index::createLimitReachedForResource, {});
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
nlohmann::json::object_t serviceShuttingDown()
{
    return getLog(redfish::registries::Base::Index::serviceShuttingDown, {});
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
nlohmann::json::object_t serviceInUnknownState()
{
    return getLog(redfish::registries::Base::Index::serviceInUnknownState, {});
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
nlohmann::json::object_t noValidSession()
{
    return getLog(redfish::registries::Base::Index::noValidSession, {});
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
nlohmann::json::object_t insufficientPrivilege()
{
    return getLog(redfish::registries::Base::Index::insufficientPrivilege, {});
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
nlohmann::json::object_t accountModified()
{
    return getLog(redfish::registries::Base::Index::accountModified, {});
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
nlohmann::json::object_t accountNotModified()
{
    return getLog(redfish::registries::Base::Index::accountNotModified, {});
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
nlohmann::json::object_t accountRemoved()
{
    return getLog(redfish::registries::Base::Index::accountRemoved, {});
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
nlohmann::json::object_t accountForSessionNoLongerExists()
{
    return getLog(
        redfish::registries::Base::Index::accountForSessionNoLongerExists, {});
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
nlohmann::json::object_t invalidObject(const boost::urls::url_view_base& arg1)
{
    return getLog(redfish::registries::Base::Index::invalidObject,
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
nlohmann::json::object_t internalError()
{
    return getLog(redfish::registries::Base::Index::internalError, {});
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
nlohmann::json::object_t unrecognizedRequestBody()
{
    return getLog(redfish::registries::Base::Index::unrecognizedRequestBody,
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
nlohmann::json::object_t resourceMissingAtURI(
    const boost::urls::url_view_base& arg1)
{
    return getLog(redfish::registries::Base::Index::resourceMissingAtURI,
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
nlohmann::json::object_t resourceAtUriInUnknownFormat(
    const boost::urls::url_view_base& arg1)
{
    return getLog(
        redfish::registries::Base::Index::resourceAtUriInUnknownFormat,
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
nlohmann::json::object_t resourceAtUriUnauthorized(
    const boost::urls::url_view_base& arg1, std::string_view arg2)
{
    return getLog(redfish::registries::Base::Index::resourceAtUriUnauthorized,
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
nlohmann::json::object_t couldNotEstablishConnection(
    const boost::urls::url_view_base& arg1)
{
    return getLog(redfish::registries::Base::Index::couldNotEstablishConnection,
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
nlohmann::json::object_t sourceDoesNotSupportProtocol(
    const boost::urls::url_view_base& arg1, std::string_view arg2)
{
    return getLog(
        redfish::registries::Base::Index::sourceDoesNotSupportProtocol,
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
nlohmann::json::object_t accessDenied(const boost::urls::url_view_base& arg1)
{
    return getLog(redfish::registries::Base::Index::accessDenied,
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
nlohmann::json::object_t serviceTemporarilyUnavailable(std::string_view arg1)
{
    return getLog(
        redfish::registries::Base::Index::serviceTemporarilyUnavailable,
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
nlohmann::json::object_t invalidIndex(uint64_t arg1)
{
    std::string arg1Str = std::to_string(arg1);
    return getLog(redfish::registries::Base::Index::invalidIndex,
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
nlohmann::json::object_t propertyValueModified(std::string_view arg1,
                                               const nlohmann::json& arg2)
{
    std::string arg2Str =
        arg2.dump(-1, ' ', true, nlohmann::json::error_handler_t::replace);
    return getLog(redfish::registries::Base::Index::propertyValueModified,
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
nlohmann::json::object_t resourceInStandby()
{
    return getLog(redfish::registries::Base::Index::resourceInStandby, {});
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
nlohmann::json::object_t resourceExhaustion(std::string_view arg1)
{
    return getLog(redfish::registries::Base::Index::resourceExhaustion,
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
nlohmann::json::object_t stringValueTooLong(std::string_view arg1,
                                            uint64_t arg2)
{
    std::string arg2Str = std::to_string(arg2);
    return getLog(redfish::registries::Base::Index::stringValueTooLong,
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
nlohmann::json::object_t stringValueTooShort(std::string_view arg1,
                                             uint64_t arg2)
{
    std::string arg2Str = std::to_string(arg2);
    return getLog(redfish::registries::Base::Index::stringValueTooShort,
                  std::to_array<std::string_view>({arg1, arg2Str}));
}

void stringValueTooShort(crow::Response& res, std::string_view arg1,
                         uint64_t arg2)
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
nlohmann::json::object_t sessionTerminated()
{
    return getLog(redfish::registries::Base::Index::sessionTerminated, {});
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
nlohmann::json::object_t subscriptionTerminated()
{
    return getLog(redfish::registries::Base::Index::subscriptionTerminated, {});
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
nlohmann::json::object_t resourceTypeIncompatible(std::string_view arg1,
                                                  std::string_view arg2)
{
    return getLog(redfish::registries::Base::Index::resourceTypeIncompatible,
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
nlohmann::json::object_t passwordChangeRequired(
    const boost::urls::url_view_base& arg1)
{
    return getLog(redfish::registries::Base::Index::passwordChangeRequired,
                  std::to_array<std::string_view>({arg1.buffer()}));
}

void passwordChangeRequired(crow::Response& res,
                            const boost::urls::url_view_base& arg1)
{
    addMessageToErrorJson(res.jsonValue, passwordChangeRequired(arg1));
}

/**
 * @internal
 * @brief Formats ResetRequired message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json::object_t resetRequired(const boost::urls::url_view_base& arg1,
                                       std::string_view arg2)
{
    return getLog(redfish::registries::Base::Index::resetRequired,
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
nlohmann::json::object_t resetRecommended(std::string_view arg1,
                                          std::string_view arg2)
{
    return getLog(redfish::registries::Base::Index::resetRecommended,
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
nlohmann::json::object_t chassisPowerStateOnRequired(std::string_view arg1)
{
    return getLog(redfish::registries::Base::Index::chassisPowerStateOnRequired,
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
nlohmann::json::object_t chassisPowerStateOffRequired(std::string_view arg1)
{
    return getLog(
        redfish::registries::Base::Index::chassisPowerStateOffRequired,
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
nlohmann::json::object_t propertyValueConflict(std::string_view arg1,
                                               std::string_view arg2)
{
    return getLog(redfish::registries::Base::Index::propertyValueConflict,
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
nlohmann::json::object_t propertyValueResourceConflict(
    std::string_view arg1, const nlohmann::json& arg2,
    const boost::urls::url_view_base& arg3)
{
    std::string arg2Str =
        arg2.dump(-1, ' ', true, nlohmann::json::error_handler_t::replace);
    return getLog(
        redfish::registries::Base::Index::propertyValueResourceConflict,
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
nlohmann::json::object_t propertyValueExternalConflict(
    std::string_view arg1, const nlohmann::json& arg2)
{
    std::string arg2Str =
        arg2.dump(-1, ' ', true, nlohmann::json::error_handler_t::replace);
    return getLog(
        redfish::registries::Base::Index::propertyValueExternalConflict,
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
nlohmann::json::object_t propertyValueIncorrect(std::string_view arg1,
                                                const nlohmann::json& arg2)
{
    std::string arg2Str =
        arg2.dump(-1, ' ', true, nlohmann::json::error_handler_t::replace);
    return getLog(redfish::registries::Base::Index::propertyValueIncorrect,
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
nlohmann::json::object_t resourceCreationConflict(
    const boost::urls::url_view_base& arg1)
{
    return getLog(redfish::registries::Base::Index::resourceCreationConflict,
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
nlohmann::json::object_t actionParameterValueConflict(std::string_view arg1,
                                                      std::string_view arg2)
{
    return getLog(
        redfish::registries::Base::Index::actionParameterValueConflict,
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
nlohmann::json::object_t maximumErrorsExceeded()
{
    return getLog(redfish::registries::Base::Index::maximumErrorsExceeded, {});
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
nlohmann::json::object_t preconditionFailed()
{
    return getLog(redfish::registries::Base::Index::preconditionFailed, {});
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
nlohmann::json::object_t preconditionRequired()
{
    return getLog(redfish::registries::Base::Index::preconditionRequired, {});
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
nlohmann::json::object_t headerMissing(std::string_view arg1)
{
    return getLog(redfish::registries::Base::Index::headerMissing,
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
nlohmann::json::object_t headerInvalid(std::string_view arg1)
{
    return getLog(redfish::registries::Base::Index::headerInvalid,
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
nlohmann::json::object_t operationFailed()
{
    return getLog(redfish::registries::Base::Index::operationFailed, {});
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
nlohmann::json::object_t operationTimeout()
{
    return getLog(redfish::registries::Base::Index::operationTimeout, {});
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
nlohmann::json::object_t operationNotAllowed()
{
    return getLog(redfish::registries::Base::Index::operationNotAllowed, {});
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
nlohmann::json::object_t undeterminedFault(std::string_view arg1)
{
    return getLog(redfish::registries::Base::Index::undeterminedFault,
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
nlohmann::json::object_t conditionInRelatedResource()
{
    return getLog(redfish::registries::Base::Index::conditionInRelatedResource,
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
nlohmann::json::object_t restrictedRole(std::string_view arg1)
{
    return getLog(redfish::registries::Base::Index::restrictedRole,
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
nlohmann::json::object_t restrictedPrivilege(std::string_view arg1)
{
    return getLog(redfish::registries::Base::Index::restrictedPrivilege,
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
nlohmann::json::object_t strictAccountTypes(std::string_view arg1)
{
    return getLog(redfish::registries::Base::Index::strictAccountTypes,
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
nlohmann::json::object_t propertyDeprecated(std::string_view arg1)
{
    return getLog(redfish::registries::Base::Index::propertyDeprecated,
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
nlohmann::json::object_t resourceDeprecated(std::string_view arg1)
{
    return getLog(redfish::registries::Base::Index::resourceDeprecated,
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
nlohmann::json::object_t propertyValueDeprecated(std::string_view arg1,
                                                 std::string_view arg2)
{
    return getLog(redfish::registries::Base::Index::propertyValueDeprecated,
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
nlohmann::json::object_t actionDeprecated(std::string_view arg1)
{
    return getLog(redfish::registries::Base::Index::actionDeprecated,
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
nlohmann::json::object_t networkNameResolutionNotConfigured()
{
    return getLog(
        redfish::registries::Base::Index::networkNameResolutionNotConfigured,
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
nlohmann::json::object_t networkNameResolutionNotSupported()
{
    return getLog(
        redfish::registries::Base::Index::networkNameResolutionNotSupported,
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
nlohmann::json::object_t serviceDisabled(std::string_view arg1)
{
    return getLog(redfish::registries::Base::Index::serviceDisabled,
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
nlohmann::json::object_t eventBufferExceeded()
{
    return getLog(redfish::registries::Base::Index::eventBufferExceeded, {});
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
nlohmann::json::object_t authenticationTokenRequired()
{
    return getLog(redfish::registries::Base::Index::authenticationTokenRequired,
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
nlohmann::json::object_t oneTimePasscodeSent(std::string_view arg1)
{
    return getLog(redfish::registries::Base::Index::oneTimePasscodeSent,
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
nlohmann::json::object_t licenseRequired(std::string_view arg1)
{
    return getLog(redfish::registries::Base::Index::licenseRequired,
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
nlohmann::json::object_t propertyModified()
{
    return getLog(redfish::registries::Base::Index::propertyModified, {});
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
nlohmann::json::object_t generateSecretKeyRequired(
    const boost::urls::url_view_base& arg1)
{
    return getLog(redfish::registries::Base::Index::generateSecretKeyRequired,
                  std::to_array<std::string_view>({arg1.buffer()}));
}

void generateSecretKeyRequired(crow::Response& res,
                               const boost::urls::url_view_base& arg1)
{
    res.result(boost::beast::http::status::forbidden);
    addMessageToErrorJson(res.jsonValue, generateSecretKeyRequired(arg1));
}

nlohmann::json firmwareImageUploadFailed()
{
    return getLog(redfish::registries::Base::Index::firmwareImageUploadFailed, {});
}

void firmwareImageUploadFailed(crow::Response& res)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, firmwareImageUploadFailed());
}
} // namespace messages
} // namespace redfish
