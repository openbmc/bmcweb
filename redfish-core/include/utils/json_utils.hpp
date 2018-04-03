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
#pragma once
#include <nlohmann/json.hpp>
#include <crow/http_request.h>
#include <crow/http_response.h>

namespace redfish {

namespace json_util {

/**
 * @brief Defines JSON utils operation status
 */
enum class Result { SUCCESS, NOT_EXIST, WRONG_TYPE, NULL_POINTER };

/**
 * @brief Describes JSON utils messages requirement
 */
enum class MessageSetting {
  NONE = 0x0,       ///< No messages will be added
  MISSING = 0x1,    ///< PropertyMissing message will be added
  TYPE_ERROR = 0x2  ///< PropertyValueTypeError message will be added
};

/**
 * @brief Wrapper function for extracting string from JSON object without
 *        throwing exceptions
 *
 * @param[in]  fieldName   Name of requested field
 * @param[in]  json        JSON object from which field should be extracted
 * @param[out] output      Variable to which extracted will be written in case
 *                         of success
 *
 * @return Result informing about operation status, output will be
 *         written only in case of Result::SUCCESS
 */
Result getString(const char* fieldName, const nlohmann::json& json,
                 const std::string*& output);

/**
 * @brief Wrapper function for extracting object from JSON object without
 *        throwing exceptions
 *
 * @param[in]  fieldName   Name of requested field
 * @param[in]  json        JSON object from which field should be extracted
 * @param[out] output      Variable to which extracted will be written in case
 *                         of success
 *
 * @return Result informing about operation status, output will be
 *         written only in case of Result::SUCCESS
 */
Result getObject(const char* fieldName, const nlohmann::json& json,
                 nlohmann::json* output);

/**
 * @brief Wrapper function for extracting array from JSON object without
 *        throwing exceptions
 *
 * @param[in]  fieldName   Name of requested field
 * @param[in]  json        JSON object from which field should be extracted
 * @param[out] output      Variable to which extracted will be written in case
 *                         of success
 *
 * @return Result informing about operation status, output will be
 *         written only in case of Result::SUCCESS
 */
Result getArray(const char* fieldName, const nlohmann::json& json,
                nlohmann::json* output);

/**
 * @brief Wrapper function for extracting int from JSON object without
 *        throwing exceptions
 *
 * @param[in]  fieldName   Name of requested field
 * @param[in]  json        JSON object from which field should be extracted
 * @param[out] output      Variable to which extracted will be written in case
 *                         of success
 *
 * @return Result informing about operation status, output will be
 *         written only in case of Result::SUCCESS
 */
Result getInt(const char* fieldName, const nlohmann::json& json,
              int64_t& output);

/**
 * @brief Wrapper function for extracting uint from JSON object without
 *        throwing exceptions
 *
 * @param[in]  fieldName   Name of requested field
 * @param[in]  json        JSON object from which field should be extracted
 * @param[out] output      Variable to which extracted will be written in case
 *                         of success
 *
 * @return Result informing about operation status, output will be
 *         written only in case of Result::SUCCESS
 */
Result getUnsigned(const char* fieldName, const nlohmann::json& json,
                   uint64_t& output);

/**
 * @brief Wrapper function for extracting bool from JSON object without
 *        throwing exceptions
 *
 * @param[in]  fieldName   Name of requested field
 * @param[in]  json        JSON object from which field should be extracted
 * @param[out] output      Variable to which extracted will be written in case
 *                         of success
 *
 * @return Result informing about operation status, output will be
 *         written only in case of Result::SUCCESS
 */
Result getBool(const char* fieldName, const nlohmann::json& json, bool& output);

/**
 * @brief Wrapper function for extracting float from JSON object without
 *        throwing exceptions (nlohmann stores JSON floats as C++ doubles)
 *
 * @param[in]  fieldName   Name of requested field
 * @param[in]  json        JSON object from which field should be extracted
 * @param[out] output      Variable to which extracted will be written in case
 *                         of success
 *
 * @return Result informing about operation status, output will be
 *         written only in case of Result::SUCCESS
 */
Result getDouble(const char* fieldName, const nlohmann::json& json,
                 double& output);

/**
 * @brief Wrapper function for extracting string from JSON object without
 *        throwing exceptions
 *
 * @param[in]  fieldName   Name of requested field
 * @param[in]  json        JSON object from which field should be extracted
 * @param[out] output      Variable to which extracted will be written in case
 *                         of success
 * @param[in]  msgCfgMap   Map for message addition settings
 * @param[out] msgJson     JSON to which error messages will be added
 * @param[in]  fieldPath   Field path in JSON
 *
 * @return Result informing about operation status, output will be
 *         written only in case of Result::SUCCESS
 */
Result getString(const char* fieldName, const nlohmann::json& json,
                 const std::string*& output, uint8_t msgCfgMap,
                 nlohmann::json& msgJson, const std::string&& fieldPath);

/**
 * @brief Wrapper function for extracting object from JSON object without
 *        throwing exceptions
 *
 * @param[in]  fieldName   Name of requested field
 * @param[in]  json        JSON object from which field should be extracted
 * @param[out] output      Variable to which extracted will be written in case
 *                         of success
 * @param[in]  msgCfgMap   Map for message addition settings
 * @param[out] msgJson     JSON to which error messages will be added
 * @param[in]  fieldPath   Field path in JSON
 *
 * @return Result informing about operation status, output will be
 *         written only in case of Result::SUCCESS
 */
Result getObject(const char* fieldName, const nlohmann::json& json,
                 nlohmann::json* output, uint8_t msgCfgMap,
                 nlohmann::json& msgJson, const std::string&& fieldPath);

/**
 * @brief Wrapper function for extracting array from JSON object without
 *        throwing exceptions
 *
 * @param[in]  fieldName   Name of requested field
 * @param[in]  json        JSON object from which field should be extracted
 * @param[out] output      Variable to which extracted will be written in case
 *                         of success
 * @param[in]  msgCfgMap   Map for message addition settings
 * @param[out] msgJson     JSON to which error messages will be added
 * @param[in]  fieldPath   Field path in JSON
 *
 * @return Result informing about operation status, output will be
 *         written only in case of Result::SUCCESS
 */
Result getArray(const char* fieldName, const nlohmann::json& json,
                nlohmann::json* output, uint8_t msgCfgMap,
                nlohmann::json& msgJson, const std::string&& fieldPath);

/**
 * @brief Wrapper function for extracting int from JSON object without
 *        throwing exceptions
 *
 * @param[in]  fieldName   Name of requested field
 * @param[in]  json        JSON object from which field should be extracted
 * @param[out] output      Variable to which extracted will be written in case
 *                         of success
 * @param[in]  msgCfgMap   Map for message addition settings
 * @param[out] msgJson     JSON to which error messages will be added
 * @param[in]  fieldPath   Field path in JSON
 *
 * @return Result informing about operation status, output will be
 *         written only in case of Result::SUCCESS
 */
Result getInt(const char* fieldName, const nlohmann::json& json,
              int64_t& output, uint8_t msgCfgMap, nlohmann::json& msgJson,
              const std::string&& fieldPath);

/**
 * @brief Wrapper function for extracting uint from JSON object without
 *        throwing exceptions
 *
 * @param[in]  fieldName   Name of requested field
 * @param[in]  json        JSON object from which field should be extracted
 * @param[out] output      Variable to which extracted will be written in case
 *                         of success
 * @param[in]  msgCfgMap   Map for message addition settings
 * @param[out] msgJson     JSON to which error messages will be added
 * @param[in]  fieldPath   Field path in JSON
 *
 * @return Result informing about operation status, output will be
 *         written only in case of Result::SUCCESS
 */
Result getUnsigned(const char* fieldName, const nlohmann::json& json,
                   uint64_t& output, uint8_t msgCfgMap, nlohmann::json& msgJson,
                   const std::string&& fieldPath);

/**
 * @brief Wrapper function for extracting bool from JSON object without
 *        throwing exceptions
 *
 * @param[in]  fieldName   Name of requested field
 * @param[in]  json        JSON object from which field should be extracted
 * @param[out] output      Variable to which extracted will be written in case
 *                         of success
 * @param[in]  msgCfgMap   Map for message addition settings
 * @param[out] msgJson     JSON to which error messages will be added
 * @param[in]  fieldPath   Field path in JSON
 *
 * @return Result informing about operation status, output will be
 *         written only in case of Result::SUCCESS
 */
Result getBool(const char* fieldName, const nlohmann::json& json, bool& output,
               uint8_t msgCfgMap, nlohmann::json& msgJson,
               const std::string&& fieldPath);

/**
 * @brief Wrapper function for extracting float from JSON object without
 *        throwing exceptions (nlohmann stores JSON floats as C++ doubles)
 *
 * @param[in]  fieldName   Name of requested field
 * @param[in]  json        JSON object from which field should be extracted
 * @param[out] output      Variable to which extracted will be written in case
 * of success
 * @param[in]  msgCfgMap   Map for message addition settings
 * @param[out] msgJson     JSON to which error messages will be added
 * @param[in]  fieldPath   Field path in JSON
 *
 * @return Result informing about operation status, output will be
 *         written only in case of Result::SUCCESS
 */
Result getDouble(const char* fieldName, const nlohmann::json& json,
                 double& output, uint8_t msgCfgMap, nlohmann::json& msgJson,
                 const std::string&& fieldPath);

/**
 * @brief Processes request to extract JSON from its body. If it fails, adds
 *       MalformedJSON message to response and ends it.
 *
 * @param[io]  res       Response object
 * @param[in]  req       Request object
 * @param[out] reqJson   JSON object extracted from request's body
 *
 * @return true if JSON is valid, false when JSON is invalid and response has
 *         been filled with message and ended.
 */
bool processJsonFromRequest(crow::response& res, const crow::request& req,
                            nlohmann::json& reqJson);

}  // namespace json_util

}  // namespace redfish
