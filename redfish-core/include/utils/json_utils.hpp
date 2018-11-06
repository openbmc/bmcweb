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

#include <crow/http_request.h>
#include <crow/http_response.h>

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
template <typename Type> struct unpackValue
{
    using isRequired = std::true_type;
    using JsonType = std::add_const_t<std::add_pointer_t<Type>>;
    Type value;
    bool valid = false;
    unpackValue(nlohmann::json& jsonValue)
    {
        if constexpr (std::is_arithmetic_v<Type>)
        {
            uint64_t* jsonPtr = jsonValue.get_ptr<uint64_t*>();
            if (jsonPtr == nullptr)
            {
                return;
            }
            if (*jsonPtr > std::numeric_limits<Type>::max())
            {
                return;
            }
            value = static_cast<Type>(*jsonPtr);
        }
        else
        {
            JsonType jsonPtr = jsonValue.get_ptr<JsonType>();
            if (jsonPtr == nullptr)
            {
                return;
            }
            value = std::move(*jsonPtr);
        }

        valid = true;
    }
};

template <typename OptionalType>
struct unpackValue<boost::optional<OptionalType>>
{
    using isRequired = std::false_type;
    OptionalType value;
    bool valid = false;
    unpackValue(nlohmann::json& jsonValue)
    {
        unpackValue<OptionalType> optionalValue(jsonValue);
        value = std::move(optionalValue.value);
        valid = optionalValue.valid;
    }
};

template <size_t Count, size_t Index>
void readJsonValues(const std::string& key, nlohmann::json& jsonValue,
                    crow::Response& res, std::bitset<Count>& handled)
{
    BMCWEB_LOG_DEBUG << "Unable to find variable for key" << key;
    messages::propertyUnknown(res, key, key);
}

template <size_t Count, size_t Index, typename ValueType,
          typename... UnpackTypes>
void readJsonValues(const std::string& key, nlohmann::json& jsonValue,
                    crow::Response& res, std::bitset<Count>& handled,
                    const char* keyToCheck, ValueType& valueToFill,
                    UnpackTypes&... in)
{
    if (key != keyToCheck)
    {
        readJsonValues<Count, Index + 1>(key, jsonValue, res, handled, in...);
        return;
    }

    handled.set(Index);

    unpackValue<ValueType> value(jsonValue);
    if (!value.valid)
    {
        BMCWEB_LOG_DEBUG << "Value for key " << key
                         << " was incorrect type: " << jsonValue.type_name();
        messages::propertyValueTypeError(res, jsonValue.dump(), key, key);
        return;
    }

    valueToFill = std::move(value.value);
}

template <size_t Index = 0, size_t Count>
void handleMissing(std::bitset<Count>& handled, crow::Response& res)
{
}

template <size_t Index = 0, size_t Count, typename ValueType,
          typename... UnpackTypes>
void handleMissing(std::bitset<Count>& handled, crow::Response& res,
                   const char* key, ValueType& unused, UnpackTypes&... in)
{
    if (!handled.test(Index) && unpackValue<ValueType>::isRequired::value)
    {
        messages::propertyMissing(res, key, key);
    }
    details::handleMissing<Index + 1, Count>(handled, res, in...);
}
} // namespace details

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
        details::readJsonValues<(sizeof...(in) + 1) / 2, 0, UnpackTypes...>(
            item.key(), item.value(), res, handled, key, in...);
    }

    details::handleMissing(handled, res, key, in...);

    return res.result() == boost::beast::http::status::ok;
}

} // namespace json_util
} // namespace redfish
