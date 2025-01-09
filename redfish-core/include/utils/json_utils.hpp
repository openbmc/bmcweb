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
#include "http_connection.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "human_sort.hpp"
#include "logging.hpp"

#include <boost/system/result.hpp>
#include <boost/url/parse.hpp>
#include <boost/url/url_view.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <ranges>
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

template <typename Type, std::size_t size>
struct IsStdArray<std::array<Type, size>> : std::true_type
{};

template <typename Type>
struct IsVariant : std::false_type
{};

template <typename... Types>
struct IsVariant<std::variant<Types...>> : std::true_type
{};

enum class UnpackErrorCode
{
    success,
    invalidType,
    outOfRange
};

template <typename ToType, typename FromType>
bool checkRange(const FromType& from [[maybe_unused]],
                std::string_view key [[maybe_unused]])
{
    if constexpr (std::is_floating_point_v<ToType>)
    {
        if (std::isnan(from))
        {
            BMCWEB_LOG_DEBUG("Value for key {} was NAN", key);
            return false;
        }
    }
    if constexpr (std::numeric_limits<ToType>::max() <
                  std::numeric_limits<FromType>::max())
    {
        if (from > std::numeric_limits<ToType>::max())
        {
            BMCWEB_LOG_DEBUG("Value for key {} was greater than max {}", key,
                             std::numeric_limits<FromType>::max());
            return false;
        }
    }
    if constexpr (std::numeric_limits<ToType>::lowest() >
                  std::numeric_limits<FromType>::lowest())
    {
        if (from < std::numeric_limits<ToType>::lowest())
        {
            BMCWEB_LOG_DEBUG("Value for key {} was less than min {}", key,
                             std::numeric_limits<FromType>::lowest());
            return false;
        }
    }

    return true;
}

template <typename Type>
UnpackErrorCode unpackValueWithErrorCode(nlohmann::json& jsonValue,
                                         std::string_view key, Type& value);

template <std::size_t Index = 0, typename... Args>
UnpackErrorCode unpackValueVariant(nlohmann::json& j, std::string_view key,
                                   std::variant<Args...>& v)
{
    if constexpr (Index < std::variant_size_v<std::variant<Args...>>)
    {
        std::variant_alternative_t<Index, std::variant<Args...>> type{};
        UnpackErrorCode unpack = unpackValueWithErrorCode(j, key, type);
        if (unpack == UnpackErrorCode::success)
        {
            v = std::move(type);
            return unpack;
        }

        return unpackValueVariant<Index + 1, Args...>(j, key, v);
    }
    return UnpackErrorCode::invalidType;
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

    else if constexpr ((std::is_unsigned_v<Type>) &&
                       (!std::is_same_v<bool, Type>))
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
    else if constexpr (std::is_same_v<std::nullptr_t, Type>)
    {
        if (!jsonValue.is_null())
        {
            return UnpackErrorCode::invalidType;
        }
    }
    else
    {
        using JsonType = std::add_const_t<std::add_pointer_t<Type>>;
        JsonType jsonPtr = jsonValue.get_ptr<JsonType>();
        if (jsonPtr == nullptr)
        {
            BMCWEB_LOG_DEBUG("Value for key {} was incorrect type: {}", key,
                             jsonValue.type_name());
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
        nlohmann::json::array_t* arr =
            jsonValue.get_ptr<nlohmann::json::array_t*>();
        if (arr == nullptr)
        {
            messages::propertyValueTypeError(res, res.jsonValue, key);
            return false;
        }
        if (jsonValue.size() != value.size())
        {
            messages::propertyValueTypeError(res, res.jsonValue, key);
            return false;
        }
        size_t index = 0;
        for (auto& val : *arr)
        {
            ret = unpackValue<typename Type::value_type>(val, key, res,
                                                         value[index++]) &&
                  ret;
        }
    }
    else if constexpr (IsVector<Type>::value)
    {
        nlohmann::json::array_t* arr =
            jsonValue.get_ptr<nlohmann::json::array_t*>();
        if (arr == nullptr)
        {
            messages::propertyValueTypeError(res, res.jsonValue, key);
            return false;
        }

        for (auto& val : *arr)
        {
            value.emplace_back();
            ret = unpackValue<typename Type::value_type>(val, key, res,
                                                         value.back()) &&
                  ret;
        }
    }
    else if constexpr (IsVariant<Type>::value)
    {
        UnpackErrorCode ec = unpackValueVariant(jsonValue, key, value);
        if (ec != UnpackErrorCode::success)
        {
            if (ec == UnpackErrorCode::invalidType)
            {
                messages::propertyValueTypeError(res, jsonValue, key);
            }
            else if (ec == UnpackErrorCode::outOfRange)
            {
                messages::propertyValueOutOfRange(res, jsonValue, key);
            }
            return false;
        }
    }
    else
    {
        UnpackErrorCode ec = unpackValueWithErrorCode(jsonValue, key, value);
        if (ec != UnpackErrorCode::success)
        {
            if (ec == UnpackErrorCode::invalidType)
            {
                messages::propertyValueTypeError(res, jsonValue, key);
            }
            else if (ec == UnpackErrorCode::outOfRange)
            {
                messages::propertyValueOutOfRange(res, jsonValue, key);
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
        nlohmann::json::array_t* arr =
            jsonValue.get_ptr<nlohmann::json::array_t*>();
        if (arr == nullptr)
        {
            return false;
        }
        if (jsonValue.size() != value.size())
        {
            return false;
        }
        size_t index = 0;
        for (const auto& val : *arr)
        {
            ret = unpackValue<typename Type::value_type>(val, key,
                                                         value[index++]) &&
                  ret;
        }
    }
    else if constexpr (IsVector<Type>::value)
    {
        nlohmann::json::array_t* arr =
            jsonValue.get_ptr<nlohmann::json::array_t*>();
        if (arr == nullptr)
        {
            return false;
        }

        for (const auto& val : *arr)
        {
            value.emplace_back();
            ret = unpackValue<typename Type::value_type>(val, key,
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
    nlohmann::json::object_t*,
    std::variant<std::string, std::nullptr_t>*,
    std::variant<uint8_t, std::nullptr_t>*,
    std::variant<int16_t, std::nullptr_t>*,
    std::variant<uint16_t, std::nullptr_t>*,
    std::variant<int32_t, std::nullptr_t>*,
    std::variant<uint32_t, std::nullptr_t>*,
    std::variant<int64_t, std::nullptr_t>*,
    std::variant<uint64_t, std::nullptr_t>*,
    std::variant<double, std::nullptr_t>*,
    std::variant<bool, std::nullptr_t>*,
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
    std::vector<nlohmann::json::object_t>*,
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
    std::optional<nlohmann::json::object_t>*,
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
    std::optional<std::vector<nlohmann::json>>*,
    std::optional<std::vector<nlohmann::json::object_t>>*,
    std::optional<std::variant<std::string, std::nullptr_t>>*,
    std::optional<std::variant<uint8_t, std::nullptr_t>>*,
    std::optional<std::variant<int16_t, std::nullptr_t>>*,
    std::optional<std::variant<uint16_t, std::nullptr_t>>*,
    std::optional<std::variant<int32_t, std::nullptr_t>>*,
    std::optional<std::variant<uint32_t, std::nullptr_t>>*,
    std::optional<std::variant<int64_t, std::nullptr_t>>*,
    std::optional<std::variant<uint64_t, std::nullptr_t>>*,
    std::optional<std::variant<double, std::nullptr_t>>*,
    std::optional<std::variant<bool, std::nullptr_t>>*,
    std::optional<std::vector<std::variant<nlohmann::json::object_t, std::nullptr_t>>>*,
    std::optional<std::variant<nlohmann::json::object_t, std::nullptr_t>>*
>;
// clang-format on

struct PerUnpack
{
    std::string_view key;
    UnpackVariant value;
    bool complete = false;
};

inline bool readJsonHelper(nlohmann::json& jsonRequest, crow::Response& res,
                           std::span<PerUnpack> toUnpack);

inline bool readJsonHelperObject(nlohmann::json::object_t& obj,
                                 crow::Response& res,
                                 std::span<PerUnpack> toUnpack)
{
    bool result = true;
    for (auto& item : obj)
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

            result =
                std::visit(
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

inline bool readJsonHelper(nlohmann::json& jsonRequest, crow::Response& res,
                           std::span<PerUnpack> toUnpack)
{
    nlohmann::json::object_t* obj =
        jsonRequest.get_ptr<nlohmann::json::object_t*>();
    if (obj == nullptr)
    {
        BMCWEB_LOG_DEBUG("Json value is not an object");
        messages::unrecognizedRequestBody(res);
        return false;
    }
    return readJsonHelperObject(*obj, res, toUnpack);
}

inline void packVariant(std::span<PerUnpack> /*toPack*/) {}

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
bool readJsonObject(nlohmann::json::object_t& jsonRequest, crow::Response& res,
                    std::string_view key, FirstType&& first,
                    UnpackTypes&&... in)
{
    const std::size_t n = sizeof...(UnpackTypes) + 2;
    std::array<PerUnpack, n / 2> toUnpack2;
    packVariant(toUnpack2, key, first, std::forward<UnpackTypes&&>(in)...);
    return readJsonHelperObject(jsonRequest, res, toUnpack2);
}

template <typename FirstType, typename... UnpackTypes>
bool readJson(nlohmann::json& jsonRequest, crow::Response& res,
              std::string_view key, FirstType&& first, UnpackTypes&&... in)
{
    nlohmann::json::object_t* obj =
        jsonRequest.get_ptr<nlohmann::json::object_t*>();
    if (obj == nullptr)
    {
        BMCWEB_LOG_DEBUG("Json value is not an object");
        messages::unrecognizedRequestBody(res);
        return false;
    }
    return readJsonObject(*obj, res, key, std::forward<FirstType>(first),
                          std::forward<UnpackTypes&&>(in)...);
}

inline std::optional<nlohmann::json::object_t>
    readJsonPatchHelper(const crow::Request& req, crow::Response& res)
{
    nlohmann::json jsonRequest;
    if (!json_util::processJsonFromRequest(res, req, jsonRequest))
    {
        BMCWEB_LOG_DEBUG("Json value not readable");
        return std::nullopt;
    }
    nlohmann::json::object_t* object =
        jsonRequest.get_ptr<nlohmann::json::object_t*>();
    if (object == nullptr || object->empty())
    {
        BMCWEB_LOG_DEBUG("Json value is empty");
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

    return {std::move(*object)};
}

template <typename... UnpackTypes>
bool readJsonPatch(const crow::Request& req, crow::Response& res,
                   std::string_view key, UnpackTypes&&... in)
{
    std::optional<nlohmann::json::object_t> jsonRequest =
        readJsonPatchHelper(req, res);
    if (!jsonRequest)
    {
        return false;
    }
    if (jsonRequest->empty())
    {
        messages::emptyJSON(res);
        return false;
    }

    return readJsonObject(*jsonRequest, res, key,
                          std::forward<UnpackTypes&&>(in)...);
}

template <typename... UnpackTypes>
bool readJsonAction(const crow::Request& req, crow::Response& res,
                    const char* key, UnpackTypes&&... in)
{
    nlohmann::json jsonRequest;
    if (!json_util::processJsonFromRequest(res, req, jsonRequest))
    {
        BMCWEB_LOG_DEBUG("Json value not readable");
        return false;
    }
    nlohmann::json::object_t* object =
        jsonRequest.get_ptr<nlohmann::json::object_t*>();
    if (object == nullptr)
    {
        BMCWEB_LOG_DEBUG("Json value is empty");
        messages::emptyJSON(res);
        return false;
    }
    return readJsonObject(*object, res, key,
                          std::forward<UnpackTypes&&>(in)...);
}

// Determines if two json objects are less, based on the presence of the
// @odata.id key
inline int objectKeyCmp(std::string_view key, const nlohmann::json& a,
                        const nlohmann::json& b)
{
    using object_t = nlohmann::json::object_t;
    const object_t* aObj = a.get_ptr<const object_t*>();
    const object_t* bObj = b.get_ptr<const object_t*>();

    if (aObj == nullptr)
    {
        if (bObj == nullptr)
        {
            return 0;
        }
        return -1;
    }
    if (bObj == nullptr)
    {
        return 1;
    }
    object_t::const_iterator aIt = aObj->find(key);
    object_t::const_iterator bIt = bObj->find(key);
    // If either object doesn't have the key, they get "sorted" to the
    // beginning.
    if (aIt == aObj->end())
    {
        if (bIt == bObj->end())
        {
            return 0;
        }
        return -1;
    }
    if (bIt == bObj->end())
    {
        return 1;
    }
    const nlohmann::json::string_t* nameA =
        aIt->second.get_ptr<const std::string*>();
    const nlohmann::json::string_t* nameB =
        bIt->second.get_ptr<const std::string*>();
    // If either object doesn't have a string as the key, they get "sorted" to
    // the beginning.
    if (nameA == nullptr)
    {
        if (nameB == nullptr)
        {
            return 0;
        }
        return -1;
    }
    if (nameB == nullptr)
    {
        return 1;
    }
    if (key != "@odata.id")
    {
        return alphanumComp(*nameA, *nameB);
    }

    boost::system::result<boost::urls::url_view> aUrl =
        boost::urls::parse_relative_ref(*nameA);
    boost::system::result<boost::urls::url_view> bUrl =
        boost::urls::parse_relative_ref(*nameB);
    if (!aUrl)
    {
        if (!bUrl)
        {
            return 0;
        }
        return -1;
    }
    if (!bUrl)
    {
        return 1;
    }

    auto segmentsAIt = aUrl->segments().begin();
    auto segmentsBIt = bUrl->segments().begin();

    while (true)
    {
        if (segmentsAIt == aUrl->segments().end())
        {
            if (segmentsBIt == bUrl->segments().end())
            {
                return 0;
            }
            return -1;
        }
        if (segmentsBIt == bUrl->segments().end())
        {
            return 1;
        }
        int res = alphanumComp(*segmentsAIt, *segmentsBIt);
        if (res != 0)
        {
            return res;
        }

        segmentsAIt++;
        segmentsBIt++;
    }
    return 0;
};

// kept for backward compatibility
inline int odataObjectCmp(const nlohmann::json& left,
                          const nlohmann::json& right)
{
    return objectKeyCmp("@odata.id", left, right);
}

struct ODataObjectLess
{
    std::string_view key;

    explicit ODataObjectLess(std::string_view keyIn) : key(keyIn) {}

    bool operator()(const nlohmann::json& left,
                    const nlohmann::json& right) const
    {
        return objectKeyCmp(key, left, right) < 0;
    }
};

// Sort the JSON array by |element[key]|.
// Elements without |key| or type of |element[key]| is not string are smaller
// those whose |element[key]| is string.
inline void sortJsonArrayByKey(nlohmann::json::array_t& array,
                               std::string_view key)
{
    std::ranges::sort(array, ODataObjectLess(key));
}

// Sort the JSON array by |element[key]|.
// Elements without |key| or type of |element[key]| is not string are smaller
// those whose |element[key]| is string.
inline void sortJsonArrayByOData(nlohmann::json::array_t& array)
{
    std::ranges::sort(array, ODataObjectLess("@odata.id"));
}

// Returns the estimated size of the JSON value
// The implementation walks through every key and every value, accumulates the
//  total size of keys and values.
// Ideally, we should use a custom allocator that nlohmann JSON supports.

// Assumption made:
//  1. number: 8 characters
//  2. boolean: 5 characters (False)
//  3. string: len(str) + 2 characters (quote)
//  4. bytes: len(bytes) characters
//  5. null: 4 characters (null)
uint64_t getEstimatedJsonSize(const nlohmann::json& root);

} // namespace json_util
} // namespace redfish
