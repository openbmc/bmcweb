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

#include <http_request.h>
#include <http_response.h>

#include <bitset>
#include <error_messages.hpp>
#include <nlohmann/json.hpp>

namespace redfish
{

namespace json_util
{

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
bool processJsonFromRequest(crow::Response& res, const crow::Request& req,
                            nlohmann::json& reqJson);
namespace details
{

template <typename Type> struct is_optional : std::false_type
{
};

template <typename Type>
struct is_optional<std::optional<Type>> : std::true_type
{
};

template <typename Type>
constexpr bool is_optional_v = is_optional<Type>::value;

template <typename Type> struct is_vector : std::false_type
{
};

template <typename Type> struct is_vector<std::vector<Type>> : std::true_type
{
};

template <typename Type> constexpr bool is_vector_v = is_vector<Type>::value;

template <typename Type> struct is_std_array : std::false_type
{
};

template <typename Type, std::size_t size>
struct is_std_array<std::array<Type, size>> : std::true_type
{
};

template <typename Type>
constexpr bool is_std_array_v = is_std_array<Type>::value;

template <typename ToType, typename FromType>
bool checkRange(const FromType& from, const std::string& key,
                nlohmann::json& jsonValue, crow::Response& res)
{
    if (from > std::numeric_limits<ToType>::max())
    {
        BMCWEB_LOG_DEBUG << "Value for key " << key
                         << " was greater than max: " << __PRETTY_FUNCTION__;
        messages::propertyValueNotInList(res, jsonValue.dump(), key);
        return false;
    }
    if (from < std::numeric_limits<ToType>::lowest())
    {
        BMCWEB_LOG_DEBUG << "Value for key " << key
                         << " was less than min: " << __PRETTY_FUNCTION__;
        messages::propertyValueNotInList(res, jsonValue.dump(), key);
        return false;
    }
    if constexpr (std::is_floating_point_v<ToType>)
    {
        if (std::isnan(from))
        {
            BMCWEB_LOG_DEBUG << "Value for key " << key << " was NAN";
            messages::propertyValueNotInList(res, jsonValue.dump(), key);
            return false;
        }
    }

    return true;
}

template <typename Type>
bool unpackValue(nlohmann::json& jsonValue, const std::string& key,
                 crow::Response& res, Type& value)
{
    bool ret = true;

    if constexpr (std::is_floating_point_v<Type>)
    {
        double helper = 0;
        double* jsonPtr = jsonValue.get_ptr<double*>();

        if (jsonPtr == nullptr)
        {
            int64_t* intPtr = jsonValue.get_ptr<int64_t*>();
            if (intPtr != nullptr)
            {
                helper = static_cast<double>(*intPtr);
                jsonPtr = &helper;
            }
        }
        if (jsonPtr == nullptr)
        {
            messages::propertyValueTypeError(res, jsonValue.dump(), key);
            return false;
        }
        if (!checkRange<Type>(*jsonPtr, key, jsonValue, res))
        {
            return false;
        }
        value = static_cast<Type>(*jsonPtr);
    }

    else if constexpr (std::is_signed_v<Type>)
    {
        int64_t* jsonPtr = jsonValue.get_ptr<int64_t*>();
        if (jsonPtr == nullptr)
        {
            messages::propertyValueTypeError(res, jsonValue.dump(), key);
            return false;
        }
        if (!checkRange<Type>(*jsonPtr, key, jsonValue, res))
        {
            return false;
        }
        value = static_cast<Type>(*jsonPtr);
    }

    else if constexpr ((std::is_unsigned_v<Type>)&&(
                           !std::is_same_v<bool, Type>))
    {
        uint64_t* jsonPtr = jsonValue.get_ptr<uint64_t*>();
        if (jsonPtr == nullptr)
        {
            messages::propertyValueTypeError(res, jsonValue.dump(), key);
            return false;
        }
        if (!checkRange<Type>(*jsonPtr, key, jsonValue, res))
        {
            return false;
        }
        value = static_cast<Type>(*jsonPtr);
    }

    else if constexpr (is_optional_v<Type>)
    {
        value.emplace();
        ret = unpackValue<typename Type::value_type>(jsonValue, key, res,
                                                     *value) &&
              ret;
    }
    else if constexpr (std::is_same_v<nlohmann::json, Type>)
    {
        // Must be a complex type.  Simple types (int string etc) should be
        // unpacked directly
        if (!jsonValue.is_object() && !jsonValue.is_array() &&
            !jsonValue.is_null())
        {
            messages::propertyValueTypeError(res, jsonValue.dump(), key);
            return false;
        }

        value = std::move(jsonValue);
    }
    else if constexpr (is_std_array_v<Type>)
    {
        if (!jsonValue.is_array())
        {
            messages::propertyValueTypeError(res, res.jsonValue.dump(), key);
            return false;
        }
        if (jsonValue.size() != value.size())
        {
            messages::propertyValueTypeError(res, res.jsonValue.dump(), key);
            return false;
        }
        size_t index = 0;
        for (const auto& val : jsonValue.items())
        {
            ret = unpackValue<typename Type::value_type>(val.value(), key, res,
                                                         value[index++]) &&
                  ret;
        }
    }
    else if constexpr (is_vector_v<Type>)
    {
        if (!jsonValue.is_array())
        {
            messages::propertyValueTypeError(res, res.jsonValue.dump(), key);
            return false;
        }

        for (const auto& val : jsonValue.items())
        {
            value.emplace_back();
            ret = unpackValue<typename Type::value_type>(val.value(), key, res,
                                                         value.back()) &&
                  ret;
        }
    }
    else
    {
        using JsonType = std::add_const_t<std::add_pointer_t<Type>>;
        JsonType jsonPtr = jsonValue.get_ptr<JsonType>();
        if (jsonPtr == nullptr)
        {
            BMCWEB_LOG_DEBUG
                << "Value for key " << key
                << " was incorrect type: " << jsonValue.type_name();
            messages::propertyValueTypeError(res, jsonValue.dump(), key);
            return false;
        }
        value = std::move(*jsonPtr);
    }
    return ret;
}

template <size_t Count, size_t Index>
bool readJsonValues(const std::string& key, nlohmann::json& jsonValue,
                    crow::Response& res, std::bitset<Count>& handled)
{
    BMCWEB_LOG_DEBUG << "Unable to find variable for key" << key;
    messages::propertyUnknown(res, key);
    return false;
}

template <size_t Count, size_t Index, typename ValueType,
          typename... UnpackTypes>
bool readJsonValues(const std::string& key, nlohmann::json& jsonValue,
                    crow::Response& res, std::bitset<Count>& handled,
                    const char* keyToCheck, ValueType& valueToFill,
                    UnpackTypes&... in)
{
    bool ret = true;
    if (key != keyToCheck)
    {
        ret = readJsonValues<Count, Index + 1>(key, jsonValue, res, handled,
                                               in...) &&
              ret;
        return ret;
    }

    handled.set(Index);

    return unpackValue<ValueType>(jsonValue, key, res, valueToFill) && ret;
}

template <size_t Index = 0, size_t Count>
bool handleMissing(std::bitset<Count>& handled, crow::Response& res)
{
    return true;
}

template <size_t Index = 0, size_t Count, typename ValueType,
          typename... UnpackTypes>
bool handleMissing(std::bitset<Count>& handled, crow::Response& res,
                   const char* key, ValueType& unused, UnpackTypes&... in)
{
    bool ret = true;
    if (!handled.test(Index) && !is_optional_v<ValueType>)
    {
        ret = false;
        messages::propertyMissing(res, key);
    }
    return details::handleMissing<Index + 1, Count>(handled, res, in...) && ret;
}
} // namespace details

template <typename... UnpackTypes>
bool readJson(nlohmann::json& jsonRequest, crow::Response& res, const char* key,
              UnpackTypes&... in)
{
    bool result = true;
    if (!jsonRequest.is_object())
    {
        BMCWEB_LOG_DEBUG << "Json value is not an object";
        messages::unrecognizedRequestBody(res);
        return false;
    }

    if (jsonRequest.empty())
    {
        BMCWEB_LOG_DEBUG << "Json value is empty";
        messages::emptyJSON(res);
        return false;
    }

    std::bitset<(sizeof...(in) + 1) / 2> handled(0);
    for (const auto& item : jsonRequest.items())
    {
        result =
            details::readJsonValues<(sizeof...(in) + 1) / 2, 0, UnpackTypes...>(
                item.key(), item.value(), res, handled, key, in...) &&
            result;
    }

    BMCWEB_LOG_DEBUG << "JSON result is: " << result;

    return details::handleMissing(handled, res, key, in...) && result;
}

template <typename... UnpackTypes>
bool readJson(const crow::Request& req, crow::Response& res, const char* key,
              UnpackTypes&... in)
{
    nlohmann::json jsonRequest;
    if (!json_util::processJsonFromRequest(res, req, jsonRequest))
    {
        BMCWEB_LOG_DEBUG << "Json value not readable";
        return false;
    }
    return readJson(jsonRequest, res, key, in...);
}

} // namespace json_util
} // namespace redfish
