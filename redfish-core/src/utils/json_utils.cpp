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
#include "utils/json_utils.hpp"

#include <error_messages.hpp>

namespace redfish
{

namespace json_util
{

Result getString(const char* fieldName, const nlohmann::json& json,
                 const std::string*& output)
{
    // Find field
    auto fieldIt = json.find(fieldName);

    // Verify existence
    if (fieldIt == json.end())
    {
        return Result::NOT_EXIST;
    }

    output = fieldIt->get_ptr<const std::string*>();

    // Verify type - we know that it exists, so nullptr means wrong type
    if (output == nullptr)
    {
        return Result::WRONG_TYPE;
    }

    return Result::SUCCESS;
}

Result getObject(const char* fieldName, const nlohmann::json& json,
                 nlohmann::json* output)
{
    // Verify input pointer
    if (output == nullptr)
    {
        return Result::NULL_POINTER;
    }

    // Find field
    auto fieldIt = json.find(fieldName);

    // Verify existence
    if (fieldIt == json.end())
    {
        return Result::NOT_EXIST;
    }

    // Verify type
    if (!fieldIt->is_object())
    {
        return Result::WRONG_TYPE;
    }

    // Extract value
    *output = *fieldIt;

    return Result::SUCCESS;
}

Result getArray(const char* fieldName, const nlohmann::json& json,
                nlohmann::json* output)
{
    // Verify input pointer
    if (output == nullptr)
    {
        return Result::NULL_POINTER;
    }

    // Find field
    auto fieldIt = json.find(fieldName);

    // Verify existence
    if (fieldIt == json.end())
    {
        return Result::NOT_EXIST;
    }

    // Verify type
    if (!fieldIt->is_array())
    {
        return Result::WRONG_TYPE;
    }

    // Extract value
    *output = *fieldIt;

    return Result::SUCCESS;
}

Result getInt(const char* fieldName, const nlohmann::json& json,
              int64_t& output)
{
    // Find field
    auto fieldIt = json.find(fieldName);

    // Verify existence
    if (fieldIt == json.end())
    {
        return Result::NOT_EXIST;
    }

    const int64_t* retVal = fieldIt->get_ptr<const int64_t*>();

    // Verify type - we know that it exists, so nullptr means wrong type
    if (retVal == nullptr)
    {
        return Result::WRONG_TYPE;
    }

    // Extract value
    output = *retVal;

    return Result::SUCCESS;
}

Result getUnsigned(const char* fieldName, const nlohmann::json& json,
                   uint64_t& output)
{
    // Find field
    auto fieldIt = json.find(fieldName);

    // Verify existence
    if (fieldIt == json.end())
    {
        return Result::NOT_EXIST;
    }

    const uint64_t* retVal = fieldIt->get_ptr<const uint64_t*>();

    // Verify type - we know that it exists, so nullptr means wrong type
    if (retVal == nullptr)
    {
        return Result::WRONG_TYPE;
    }

    // Extract value
    output = *retVal;

    return Result::SUCCESS;
}

Result getBool(const char* fieldName, const nlohmann::json& json, bool& output)
{
    // Find field
    auto fieldIt = json.find(fieldName);

    // Verify existence
    if (fieldIt == json.end())
    {
        return Result::NOT_EXIST;
    }

    const bool* retVal = fieldIt->get_ptr<const bool*>();

    // Verify type - we know that it exists, so nullptr means wrong type
    if (retVal == nullptr)
    {
        return Result::WRONG_TYPE;
    }

    // Extract value
    output = *retVal;

    return Result::SUCCESS;
}

Result getDouble(const char* fieldName, const nlohmann::json& json,
                 double& output)
{
    // Find field
    auto fieldIt = json.find(fieldName);

    // Verify existence
    if (fieldIt == json.end())
    {
        return Result::NOT_EXIST;
    }

    const double* retVal = fieldIt->get_ptr<const double*>();

    // Verify type - we know that it exists, so nullptr means wrong type
    if (retVal == nullptr)
    {
        return Result::WRONG_TYPE;
    }

    // Extract value
    output = *retVal;

    return Result::SUCCESS;
}

Result getString(const char* fieldName, const nlohmann::json& json,
                 const std::string*& output, uint8_t msgCfgMap,
                 nlohmann::json& msgJson, const std::string&& fieldPath)
{
    // Find field
    auto fieldIt = json.find(fieldName);

    // Verify existence
    if (fieldIt == json.end())
    {
        if (msgCfgMap & static_cast<int>(MessageSetting::MISSING))
        {
            messages::addMessageToJson(
                msgJson, messages::propertyMissing(fieldName), fieldPath);
        }

        return Result::NOT_EXIST;
    }

    output = fieldIt->get_ptr<const std::string*>();

    // Verify type - we know that it exists, so nullptr means wrong type
    if (output == nullptr)
    {
        if (msgCfgMap & static_cast<int>(MessageSetting::TYPE_ERROR))
        {
            messages::addMessageToJson(
                msgJson,
                messages::propertyValueTypeError(fieldIt->dump(), fieldName),
                fieldPath);
        }

        return Result::WRONG_TYPE;
    }

    return Result::SUCCESS;
}

Result getObject(const char* fieldName, const nlohmann::json& json,
                 nlohmann::json* output, uint8_t msgCfgMap,
                 nlohmann::json& msgJson, const std::string&& fieldPath)
{
    // Verify input pointer
    if (output == nullptr)
    {
        return Result::NULL_POINTER;
    }

    // Find field
    auto fieldIt = json.find(fieldName);

    // Verify existence
    if (fieldIt == json.end())
    {
        if (msgCfgMap & static_cast<int>(MessageSetting::MISSING))
        {
            messages::addMessageToJson(
                msgJson, messages::propertyMissing(fieldName), fieldPath);
        }

        return Result::NOT_EXIST;
    }

    // Verify type
    if (!fieldIt->is_object())
    {
        if (msgCfgMap & static_cast<int>(MessageSetting::TYPE_ERROR))
        {
            messages::addMessageToJson(
                msgJson,
                messages::propertyValueTypeError(fieldIt->dump(), fieldName),
                fieldPath);
        }

        return Result::WRONG_TYPE;
    }

    // Extract value
    *output = *fieldIt;

    return Result::SUCCESS;
}

Result getArray(const char* fieldName, const nlohmann::json& json,
                nlohmann::json* output, uint8_t msgCfgMap,
                nlohmann::json& msgJson, const std::string&& fieldPath)
{
    // Verify input pointer
    if (output == nullptr)
    {
        return Result::NULL_POINTER;
    }

    // Find field
    auto fieldIt = json.find(fieldName);

    // Verify existence
    if (fieldIt == json.end())
    {
        if (msgCfgMap & static_cast<int>(MessageSetting::MISSING))
        {
            messages::addMessageToJson(
                msgJson, messages::propertyMissing(fieldName), fieldPath);
        }

        return Result::NOT_EXIST;
    }

    // Verify type
    if (!fieldIt->is_array())
    {
        if (msgCfgMap & static_cast<int>(MessageSetting::TYPE_ERROR))
        {
            messages::addMessageToJson(
                msgJson,
                messages::propertyValueTypeError(fieldIt->dump(), fieldName),
                fieldPath);
        }

        return Result::WRONG_TYPE;
    }

    // Extract value
    *output = *fieldIt;

    return Result::SUCCESS;
}

Result getInt(const char* fieldName, const nlohmann::json& json,
              int64_t& output, uint8_t msgCfgMap, nlohmann::json& msgJson,
              const std::string&& fieldPath)
{
    // Find field
    auto fieldIt = json.find(fieldName);

    // Verify existence
    if (fieldIt == json.end())
    {
        if (msgCfgMap & static_cast<int>(MessageSetting::MISSING))
        {
            messages::addMessageToJson(
                msgJson, messages::propertyMissing(fieldName), fieldPath);
        }

        return Result::NOT_EXIST;
    }

    const int64_t* retVal = fieldIt->get_ptr<const int64_t*>();

    // Verify type - we know that it exists, so nullptr means wrong type
    if (retVal == nullptr)
    {
        if (msgCfgMap & static_cast<int>(MessageSetting::TYPE_ERROR))
        {
            messages::addMessageToJson(
                msgJson,
                messages::propertyValueTypeError(fieldIt->dump(), fieldName),
                fieldPath);
        }

        return Result::WRONG_TYPE;
    }

    // Extract value
    output = *retVal;

    return Result::SUCCESS;
}

Result getUnsigned(const char* fieldName, const nlohmann::json& json,
                   uint64_t& output, uint8_t msgCfgMap, nlohmann::json& msgJson,
                   const std::string&& fieldPath)
{
    // Find field
    auto fieldIt = json.find(fieldName);

    // Verify existence
    if (fieldIt == json.end())
    {
        if (msgCfgMap & static_cast<int>(MessageSetting::MISSING))
        {
            messages::addMessageToJson(
                msgJson, messages::propertyMissing(fieldName), fieldPath);
        }

        return Result::NOT_EXIST;
    }

    const uint64_t* retVal = fieldIt->get_ptr<const uint64_t*>();

    // Verify type - we know that it exists, so nullptr means wrong type
    if (retVal == nullptr)
    {
        if (msgCfgMap & static_cast<int>(MessageSetting::TYPE_ERROR))
        {
            messages::addMessageToJson(
                msgJson,
                messages::propertyValueTypeError(fieldIt->dump(), fieldName),
                fieldPath);
        }

        return Result::WRONG_TYPE;
    }

    // Extract value
    output = *retVal;

    return Result::SUCCESS;
}

Result getBool(const char* fieldName, const nlohmann::json& json, bool& output,
               uint8_t msgCfgMap, nlohmann::json& msgJson,
               const std::string&& fieldPath)
{
    // Find field
    auto fieldIt = json.find(fieldName);

    // Verify existence
    if (fieldIt == json.end())
    {
        if (msgCfgMap & static_cast<int>(MessageSetting::MISSING))
        {
            messages::addMessageToJson(
                msgJson, messages::propertyMissing(fieldName), fieldPath);
        }

        return Result::NOT_EXIST;
    }

    const bool* retVal = fieldIt->get_ptr<const bool*>();

    // Verify type - we know that it exists, so nullptr means wrong type
    if (retVal == nullptr)
    {
        if (msgCfgMap & static_cast<int>(MessageSetting::TYPE_ERROR))
        {
            messages::addMessageToJson(
                msgJson,
                messages::propertyValueTypeError(fieldIt->dump(), fieldName),
                fieldPath);
        }

        return Result::WRONG_TYPE;
    }

    // Extract value
    output = *retVal;

    return Result::SUCCESS;
}

Result getDouble(const char* fieldName, const nlohmann::json& json,
                 double& output, uint8_t msgCfgMap, nlohmann::json& msgJson,
                 const std::string&& fieldPath)
{
    // Find field
    auto fieldIt = json.find(fieldName);

    // Verify existence
    if (fieldIt == json.end())
    {
        if (msgCfgMap & static_cast<int>(MessageSetting::MISSING))
        {
            messages::addMessageToJson(
                msgJson, messages::propertyMissing(fieldName), fieldPath);
        }

        return Result::NOT_EXIST;
    }

    const double* retVal = fieldIt->get_ptr<const double*>();

    // Verify type - we know that it exists, so nullptr means wrong type
    if (retVal == nullptr)
    {
        if (msgCfgMap & static_cast<int>(MessageSetting::TYPE_ERROR))
        {
            messages::addMessageToJson(
                msgJson,
                messages::propertyValueTypeError(fieldIt->dump(), fieldName),
                fieldPath);
        }

        return Result::WRONG_TYPE;
    }

    // Extract value
    output = *retVal;

    return Result::SUCCESS;
}

bool processJsonFromRequest(crow::Response& res, const crow::Request& req,
                            nlohmann::json& reqJson)
{
    reqJson = nlohmann::json::parse(req.body, nullptr, false);

    if (reqJson.is_discarded())
    {
        messages::addMessageToErrorJson(res.jsonValue,
                                        messages::malformedJSON());

        res.result(boost::beast::http::status::bad_request);
        res.end();

        return false;
    }

    return true;
}

} // namespace json_util

} // namespace redfish
