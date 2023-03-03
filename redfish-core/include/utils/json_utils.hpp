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

#include "error_messages.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "logging.hpp"
#include "nlohmann/json.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

// IWYU pragma: no_include <stdint.h>
// IWYU pragma: no_forward_declare crow::Request

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

template <typename Type>
struct IsOptional : std::false_type
{};

template <typename Type>
struct IsOptional<std::optional<Type>> : std::true_type
{};

template <typename Type>
struct IsVector : std::false_type
{};

template <typename Type>
struct IsVector<std::vector<Type>> : std::true_type
{};

template <typename Type>
struct IsStdArray : std::false_type
{};

template <typename Type, size_t size>
struct IsStdArray<std::array<Type, size>> : std::true_type
{};

enum class UnpackErrorCode
{
    success,
    invalidType,
    outOfRange
};

template <typename ToType, typename FromType>
bool checkRange(const FromType& from, std::string_view key)
{
    if (from > std::numeric_limits<ToType>::max())
    {
        BMCWEB_LOG_DEBUG << "Value for key " << key
                         << " was greater than max: " << __PRETTY_FUNCTION__;
        return false;
    }
    if (from < std::numeric_limits<ToType>::lowest())
    {
        BMCWEB_LOG_DEBUG << "Value for key " << key
                         << " was less than min: " << __PRETTY_FUNCTION__;
        return false;
    }
    if constexpr (std::is_floating_point_v<ToType>)
    {
        if (std::isnan(from))
        {
            BMCWEB_LOG_DEBUG << "Value for key " << key << " was NAN";
            return false;
        }
    }

    return true;
}

template <typename Type>
UnpackErrorCode unpackValueWithErrorCode(nlohmann::json& jsonValue,
                                         std::string_view key, Type& value)
{
    UnpackErrorCode ret = UnpackErrorCode::success;

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
            return UnpackErrorCode::invalidType;
        }
        if (!checkRange<Type>(*jsonPtr, key))
        {
            return UnpackErrorCode::outOfRange;
        }
        value = static_cast<Type>(*jsonPtr);
    }

    else if constexpr (std::is_signed_v<Type>)
    {
        int64_t* jsonPtr = jsonValue.get_ptr<int64_t*>();
        if (jsonPtr == nullptr)
        {
            return UnpackErrorCode::invalidType;
        }
        if (!checkRange<Type>(*jsonPtr, key))
        {
            return UnpackErrorCode::outOfRange;
        }
        value = static_cast<Type>(*jsonPtr);
    }

    else if constexpr ((std::is_unsigned_v<Type>)&&(
                           !std::is_same_v<bool, Type>))
    {
        uint64_t* jsonPtr = jsonValue.get_ptr<uint64_t*>();
        if (jsonPtr == nullptr)
        {
            return UnpackErrorCode::invalidType;
        }
        if (!checkRange<Type>(*jsonPtr, key))
        {
            return UnpackErrorCode::outOfRange;
        }
        value = static_cast<Type>(*jsonPtr);
    }

    else if constexpr (std::is_same_v<nlohmann::json, Type>)
    {
        value = std::move(jsonValue);
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
            return UnpackErrorCode::invalidType;
        }
        value = std::move(*jsonPtr);
    }
    return ret;
}

template <typename Type>
bool unpackValue(nlohmann::json& jsonValue, std::string_view key,
                 crow::Response& res, Type& value)
{
    bool ret = true;

    if constexpr (IsOptional<Type>::value)
    {
        value.emplace();
        ret = unpackValue<typename Type::value_type>(jsonValue, key, res,
                                                     *value) &&
              ret;
    }
    else if constexpr (IsStdArray<Type>::value)
    {
        if (!jsonValue.is_array())
        {
            messages::propertyValueTypeError(
                res,
                res.jsonValue.dump(2, ' ', true,
                                   nlohmann::json::error_handler_t::replace),
                key);
            return false;
        }
        if (jsonValue.size() != value.size())
        {
            messages::propertyValueTypeError(
                res,
                res.jsonValue.dump(2, ' ', true,
                                   nlohmann::json::error_handler_t::replace),
                key);
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
    else if constexpr (IsVector<Type>::value)
    {
        if (!jsonValue.is_array())
        {
            messages::propertyValueTypeError(
                res,
                res.jsonValue.dump(2, ' ', true,
                                   nlohmann::json::error_handler_t::replace),
                key);
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
        UnpackErrorCode ec = unpackValueWithErrorCode(jsonValue, key, value);
        if (ec != UnpackErrorCode::success)
        {
            if (ec == UnpackErrorCode::invalidType)
            {
                messages::propertyValueTypeError(
                    res,
                    jsonValue.dump(2, ' ', true,
                                   nlohmann::json::error_handler_t::replace),
                    key);
            }
            else if (ec == UnpackErrorCode::outOfRange)
            {
                messages::propertyValueNotInList(
                    res,
                    jsonValue.dump(2, ' ', true,
                                   nlohmann::json::error_handler_t::replace),
                    key);
            }
            return false;
        }
    }

    return ret;
}

template <typename Type>
bool unpackValue(nlohmann::json& jsonValue, std::string_view key, Type& value)
{
    bool ret = true;
    if constexpr (IsOptional<Type>::value)
    {
        value.emplace();
        ret = unpackValue<typename Type::value_type>(jsonValue, key, *value) &&
              ret;
    }
    else if constexpr (IsStdArray<Type>::value)
    {
        if (!jsonValue.is_array())
        {
            return false;
        }
        if (jsonValue.size() != value.size())
        {
            return false;
        }
        size_t index = 0;
        for (const auto& val : jsonValue.items())
        {
            ret = unpackValue<typename Type::value_type>(val.value(), key,
                                                         value[index++]) &&
                  ret;
        }
    }
    else if constexpr (IsVector<Type>::value)
    {
        if (!jsonValue.is_array())
        {
            return false;
        }

        for (const auto& val : jsonValue.items())
        {
            value.emplace_back();
            ret = unpackValue<typename Type::value_type>(val.value(), key,
                                                         value.back()) &&
                  ret;
        }
    }
    else
    {
        UnpackErrorCode ec = unpackValueWithErrorCode(jsonValue, key, value);
        if (ec != UnpackErrorCode::success)
        {
            return false;
        }
    }

    return ret;
}
} // namespace details

// clang-format off
using UnpackVariant = std::variant<
    uint8_t*,
    uint16_t*,
    int16_t*,
    uint32_t*,
    int32_t*,
    uint64_t*,
    int64_t*,
    bool*,
    double*,
    std::string*,
    nlohmann::json*,
    std::vector<uint8_t>*,
    std::vector<uint16_t>*,
    std::vector<int16_t>*,
    std::vector<uint32_t>*,
    std::vector<int32_t>*,
    std::vector<uint64_t>*,
    std::vector<int64_t>*,
    //std::vector<bool>*,
    std::vector<double>*,
    std::vector<std::string>*,
    std::vector<nlohmann::json>*,
    std::optional<uint8_t>*,
    std::optional<uint16_t>*,
    std::optional<int16_t>*,
    std::optional<uint32_t>*,
    std::optional<int32_t>*,
    std::optional<uint64_t>*,
    std::optional<int64_t>*,
    std::optional<bool>*,
    std::optional<double>*,
    std::optional<std::string>*,
    std::optional<nlohmann::json>*,
    std::optional<std::vector<uint8_t>>*,
    std::optional<std::vector<uint16_t>>*,
    std::optional<std::vector<int16_t>>*,
    std::optional<std::vector<uint32_t>>*,
    std::optional<std::vector<int32_t>>*,
    std::optional<std::vector<uint64_t>>*,
    std::optional<std::vector<int64_t>>*,
    //std::optional<std::vector<bool>>*,
    std::optional<std::vector<double>>*,
    std::optional<std::vector<std::string>>*,
    std::optional<std::vector<nlohmann::json>>*
>;
// clang-format on

struct PerUnpack
{
    std::string_view key;
    UnpackVariant value;
    bool complete = false;
};

inline bool readJsonHelper(nlohmann::json& jsonRequest, crow::Response& res,
                           std::span<PerUnpack> toUnpack)
{
    bool result = true;
    nlohmann::json::object_t* obj =
        jsonRequest.get_ptr<nlohmann::json::object_t*>();
    if (obj == nullptr)
    {
        BMCWEB_LOG_DEBUG << "Json value is not an object";
        messages::unrecognizedRequestBody(res);
        return false;
    }
    for (auto& item : *obj)
    {
        size_t unpackIndex = 0;
        for (; unpackIndex < toUnpack.size(); unpackIndex++)
        {
            PerUnpack& unpackSpec = toUnpack[unpackIndex];
            std::string_view key = unpackSpec.key;
            size_t keysplitIndex = key.find('/');
            std::string_view leftover;
            if (keysplitIndex != std::string_view::npos)
            {
                leftover = key.substr(keysplitIndex + 1);
                key = key.substr(0, keysplitIndex);
            }

            if (key != item.first || unpackSpec.complete)
            {
                continue;
            }

            // Sublevel key
            if (!leftover.empty())
            {
                // Include the slash in the key so we can compare later
                key = unpackSpec.key.substr(0, keysplitIndex + 1);
                nlohmann::json j;
                result = details::unpackValue<nlohmann::json>(item.second, key,
                                                              res, j) &&
                         result;
                if (!result)
                {
                    return result;
                }

                std::vector<PerUnpack> nextLevel;
                for (PerUnpack& p : toUnpack)
                {
                    if (!p.key.starts_with(key))
                    {
                        continue;
                    }
                    std::string_view thisLeftover = p.key.substr(key.size());
                    nextLevel.push_back({thisLeftover, p.value, false});
                    p.complete = true;
                }

                result = readJsonHelper(j, res, nextLevel) && result;
                break;
            }

            result = std::visit(
                         [&item, &unpackSpec, &res](auto&& val) {
                using ContainedT =
                    std::remove_pointer_t<std::decay_t<decltype(val)>>;
                return details::unpackValue<ContainedT>(
                    item.second, unpackSpec.key, res, *val);
                         },
                         unpackSpec.value) &&
                result;

            unpackSpec.complete = true;
            break;
        }

        if (unpackIndex == toUnpack.size())
        {
            messages::propertyUnknown(res, item.first);
            result = false;
        }
    }

    for (PerUnpack& perUnpack : toUnpack)
    {
        if (!perUnpack.complete)
        {
            bool isOptional = std::visit(
                [](auto&& val) {
                using ContainedType =
                    std::remove_pointer_t<std::decay_t<decltype(val)>>;
                return details::IsOptional<ContainedType>::value;
                },
                perUnpack.value);
            if (isOptional)
            {
                continue;
            }
            messages::propertyMissing(res, perUnpack.key);
            result = false;
        }
    }
    return result;
}

inline void packVariant(std::span<PerUnpack> /*toPack*/)
{}

template <typename FirstType, typename... UnpackTypes>
void packVariant(std::span<PerUnpack> toPack, std::string_view key,
                 FirstType& first, UnpackTypes&&... in)
{
    if (toPack.empty())
    {
        return;
    }
    toPack[0].key = key;
    toPack[0].value = &first;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    packVariant(toPack.subspan(1), std::forward<UnpackTypes&&>(in)...);
}

template <typename FirstType, typename... UnpackTypes>
bool readJson(nlohmann::json& jsonRequest, crow::Response& res,
              std::string_view key, FirstType&& first, UnpackTypes&&... in)
{
    size_t n = sizeof...(UnpackTypes) + 2;
    std::array<PerUnpack, n / 2> toUnpack2;
    packVariant(toUnpack2, key, first, std::forward<UnpackTypes&&>(in)...);
    return readJsonHelper(jsonRequest, res, toUnpack2);
}

inline std::optional<nlohmann::json>
    readJsonPatchHelper(const crow::Request& req, crow::Response& res)
{
    nlohmann::json jsonRequest;
    if (!json_util::processJsonFromRequest(res, req, jsonRequest))
    {
        BMCWEB_LOG_DEBUG << "Json value not readable";
        return std::nullopt;
    }
    nlohmann::json::object_t* object =
        jsonRequest.get_ptr<nlohmann::json::object_t*>();
    if (object == nullptr || object->empty())
    {
        BMCWEB_LOG_DEBUG << "Json value is empty";
        messages::emptyJSON(res);
        return std::nullopt;
    }
    std::erase_if(*object,
                  [](const std::pair<std::string, nlohmann::json>& item) {
        return item.first.starts_with("@odata.");
    });
    if (object->empty())
    {
        //  If the update request only contains OData annotations, the service
        //  should return the HTTP 400 Bad Request status code with the
        //  NoOperation message from the Base Message Registry, ...
        messages::noOperation(res);
        return std::nullopt;
    }

    return {std::move(jsonRequest)};
}

template <typename... UnpackTypes>
bool readJsonPatch(const crow::Request& req, crow::Response& res,
                   std::string_view key, UnpackTypes&&... in)
{
    std::optional<nlohmann::json> jsonRequest = readJsonPatchHelper(req, res);
    if (jsonRequest == std::nullopt)
    {
        return false;
    }

    return readJson(*jsonRequest, res, key, std::forward<UnpackTypes&&>(in)...);
}

template <typename... UnpackTypes>
bool readJsonAction(const crow::Request& req, crow::Response& res,
                    const char* key, UnpackTypes&&... in)
{
    nlohmann::json jsonRequest;
    if (!json_util::processJsonFromRequest(res, req, jsonRequest))
    {
        BMCWEB_LOG_DEBUG << "Json value not readable";
        return false;
    }
    return readJson(jsonRequest, res, key, std::forward<UnpackTypes&&>(in)...);
}

} // namespace json_util
} // namespace redfish
