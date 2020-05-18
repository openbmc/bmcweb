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

#include <charconv>
#include <chrono>
#include <string>

namespace redfish
{

namespace time_utils
{

namespace details
{

using Days = std::chrono::duration<long, std::ratio<24 * 60 * 60>>;

std::string& leftZeroPadding(std::string&& str, const std::size_t padding)
{
    if (str.size() < padding)
    {
        str.insert(0, padding - str.size(), '0');
    }
    return str;
}

long pow10(size_t exp)
{
    if (exp <= 0)
    {
        return 1;
    }
    return pow10(--exp) * 10;
}

template <typename T>
bool fromDurationItem(std::string_view& fmt, const char postfix,
                      std::chrono::milliseconds& out)
{
    size_t pos = fmt.find(postfix);
    if (pos == std::string::npos || fmt.size() < (pos + 1))
    {
        return true;
    }

    long v = 0;
    auto [p, ec] = std::from_chars(fmt.data(), fmt.data() + pos, v);
    if (p != (fmt.data() + pos) || ec != std::errc())
    {
        BMCWEB_LOG_ERROR << "Failed to parse duration string with err: "
                         << static_cast<int>(ec) << "\n";
        return false;
    }

    if constexpr (std::is_same_v<T, std::chrono::milliseconds>)
    {
        if (pos < 3)
        {
            v *= pow10(3 - pos);
        }
        else if (pos > 3)
        {
            v /= pow10(pos - 3);
        }
    }

    fmt.remove_prefix(pos + 1);
    out += T(v);
    if (out < std::chrono::milliseconds::zero())
    {
        return false;
    }

    return true;
}
} // namespace details

/**
 * @brief Convert string that represents value in Duration Format to its numeric
 *        equivalent.
 */
std::chrono::milliseconds fromDurationString(const std::string& str)
{
    std::chrono::milliseconds out = std::chrono::milliseconds::zero();
    std::string_view fmt = str;
    if (fmt.empty() || fmt.front() != 'P')
    {
        return out;
    }

    fmt.remove_prefix(1);
    if (!details::fromDurationItem<details::Days>(fmt, 'D', out))
    {
        return std::chrono::milliseconds::min();
    }
    if (fmt.empty() || fmt.front() != 'T')
    {
        return out;
    }

    fmt.remove_prefix(1);
    if (!details::fromDurationItem<std::chrono::hours>(fmt, 'H', out) ||
        !details::fromDurationItem<std::chrono::minutes>(fmt, 'M', out))
    {
        return std::chrono::milliseconds::min();
    }

    if (fmt.find('.') != std::string::npos)
    {
        if (!details::fromDurationItem<std::chrono::seconds>(fmt, '.', out) ||
            !details::fromDurationItem<std::chrono::milliseconds>(fmt, 'S',
                                                                  out))
        {
            return std::chrono::milliseconds::min();
        }
    }
    else
    {
        if (!details::fromDurationItem<std::chrono::seconds>(fmt, 'S', out))
        {
            return std::chrono::milliseconds::min();
        }
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
    if (ms <= std::chrono::milliseconds::zero())
    {
        return "";
    }

    std::string fmt;
    fmt.reserve(sizeof("PxxxxxxxxxxxxDTxxHxxMxx.xxxxxxS\0"));

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
        fmt += std::to_string(seconds.count()) + "." +
               details::leftZeroPadding(std::to_string(ms.count()), 3) + "S";
    }

    return fmt;
}

} // namespace time_utils
} // namespace redfish
