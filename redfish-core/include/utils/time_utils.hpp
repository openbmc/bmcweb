/*
// Copyright (c) 2020 Intel Corporation
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

#include <algorithm>
#include <charconv>
#include <chrono>
#include <optional>
#include <string>

namespace redfish
{

namespace time_utils
{

namespace details
{

using Days = std::chrono::duration<long, std::ratio<24 * 60 * 60>>;

inline void leftZeroPadding(std::string& str, const std::size_t padding)
{
    if (str.size() < padding)
    {
        str.insert(0, padding - str.size(), '0');
    }
}

inline bool fromChars(const char* start, const char* end,
                      std::chrono::milliseconds::rep& val)
{
    auto [ptr, ec] = std::from_chars(start, end, val);
    if (ptr != end)
    {
        BMCWEB_LOG_ERROR
            << "Failed to convert string to decimal because of unexpected sign";
        return false;
    }
    if (ec != std::errc())
    {
        BMCWEB_LOG_ERROR << "Failed to convert string to decimal with err: "
                         << static_cast<int>(ec) << "("
                         << std::make_error_code(ec).message() << ")";
        return false;
    }
    return true;
}

template <typename T>
bool fromDurationItem(std::string_view& fmt, const char postfix,
                      std::chrono::milliseconds& out)
{
    const size_t pos = fmt.find(postfix);
    if (pos != std::string::npos)
    {
        if ((pos + 1U) > fmt.size())
        {
            return false;
        }

        std::chrono::milliseconds::rep v = 0;
        if constexpr (std::is_same_v<T, std::chrono::milliseconds>)
        {
            std::string str(fmt.data(), std::min<size_t>(pos, 3U));
            while (str.size() < 3U)
            {
                str += '0';
            }
            fromChars(str.data(), str.data() + str.size(), v);
        }
        else
        {
            fromChars(fmt.data(), fmt.data() + pos, v);
        }

        fmt.remove_prefix(pos + 1U);
        out += T(v);
        if (out < T(v))
        {
            return false;
        }
    }

    return true;
}
} // namespace details

/**
 * @brief Convert string that represents value in Duration Format to its numeric
 *        equivalent.
 */
std::optional<std::chrono::milliseconds>
    fromDurationString(const std::string& str)
{
    std::chrono::milliseconds out = std::chrono::milliseconds::zero();
    std::string_view fmt = str;

    if (!fmt.empty() && fmt.front() == 'P')
    {
        fmt.remove_prefix(1);
        if (!details::fromDurationItem<details::Days>(fmt, 'D', out))
        {
            return std::nullopt;
        }

        if (!fmt.empty() && fmt.front() == 'T')
        {
            fmt.remove_prefix(1);
            if (!details::fromDurationItem<std::chrono::hours>(fmt, 'H', out) ||
                !details::fromDurationItem<std::chrono::minutes>(fmt, 'M', out))
            {
                return std::nullopt;
            }

            if ((fmt.find('.') != std::string::npos &&
                 fmt.find('S') != std::string::npos) &&
                (!details::fromDurationItem<std::chrono::seconds>(fmt, '.',
                                                                  out) ||
                 !details::fromDurationItem<std::chrono::milliseconds>(fmt, 'S',
                                                                       out)))
            {
                return std::nullopt;
            }
            else if (!details::fromDurationItem<std::chrono::seconds>(fmt, 'S',
                                                                      out))
            {
                return std::nullopt;
            }
        }
    }
    if (!fmt.empty())
    {
        BMCWEB_LOG_ERROR << "Invalid duration format: " << str;
        return std::nullopt;
    }
    return out;
}

/**
 * @brief Convert time value into duration format that is based on ISO 8601.
 *        Example output: "P12DT1M5.5S"
 *        Ref: Redfish Specification, Section 9.4.4. Duration values
 */
std::string toDurationString(std::chrono::milliseconds ms)
{
    if (ms < std::chrono::milliseconds::zero())
    {
        return "";
    }

    std::string fmt;
    fmt.reserve(sizeof("PxxxxxxxxxxxxDTxxHxxMxx.xxxxxxS"));

    details::Days days = std::chrono::floor<details::Days>(ms);
    ms -= days;

    std::chrono::hours hours = std::chrono::floor<std::chrono::hours>(ms);
    ms -= hours;

    std::chrono::minutes minutes = std::chrono::floor<std::chrono::minutes>(ms);
    ms -= minutes;

    std::chrono::seconds seconds = std::chrono::floor<std::chrono::seconds>(ms);
    ms -= seconds;

    fmt = "P";
    if (days.count() > 0)
    {
        fmt += std::to_string(days.count()) + "D";
    }
    fmt += "T";
    if (hours.count() > 0)
    {
        fmt += std::to_string(hours.count()) + "H";
    }
    if (minutes.count() > 0)
    {
        fmt += std::to_string(minutes.count()) + "M";
    }
    if (seconds.count() != 0 || ms.count() != 0)
    {
        fmt += std::to_string(seconds.count()) + ".";
        std::string msStr = std::to_string(ms.count());
        details::leftZeroPadding(msStr, 3);
        fmt += msStr + "S";
    }

    return fmt;
}

} // namespace time_utils
} // namespace redfish
