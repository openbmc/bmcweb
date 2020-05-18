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

using Days = std::chrono::duration<long long int, std::ratio<24 * 60 * 60>>;

template <typename T>
std::chrono::milliseconds fromDurationFormatItem(std::string& fmt,
                                                 const char postfix)
{
    auto pos = fmt.find(postfix);
    if (pos == std::string::npos || fmt.size() < (pos + 1))
    {
        return std::chrono::milliseconds();
    }

    std::chrono::milliseconds out;
    if constexpr (std::is_same<T, std::chrono::milliseconds>::value)
    {
        /* std::from_chars for floating types is not implemented in gcc */
        const float v = std::stof(fmt.c_str(), nullptr);
        out = T(static_cast<long>(
            std::roundf(v * std::chrono::milliseconds::period::den)));
    }
    else
    {
        long v = 0;
        std::from_chars(fmt.c_str(), fmt.c_str() + pos, v);
        out = T(v);
    }

    fmt.erase(0, pos + 1);
    return out;
}

} // namespace details

std::time_t fromDurationFormat(std::string fmt)
{
    if (fmt.empty() || *fmt.begin() != 'P')
    {
        return 0;
    }

    fmt.erase(fmt.begin());
    std::chrono::milliseconds out =
        details::fromDurationFormatItem<details::Days>(fmt, 'D');
    if (fmt.empty() || *fmt.begin() != 'T')
    {
        return static_cast<std::time_t>(out.count());
    }

    fmt.erase(fmt.begin());
    out += details::fromDurationFormatItem<std::chrono::hours>(fmt, 'H');
    out += details::fromDurationFormatItem<std::chrono::minutes>(fmt, 'M');
    out += details::fromDurationFormatItem<std::chrono::milliseconds>(fmt, 'S');

    return static_cast<std::time_t>(out.count());
}

/**
 * @brief Convert time value into duration format that is based on ISO 8601.
 *        Example output: "P192DT12H1M1.500000S"
 */
std::string toDurationFormat(const std::time_t ms_time)
{
    std::string fmt;
    fmt.reserve(sizeof("PxxxxxxxxxxxxDTxxHxxMxx.xxxxxxS\0"));
    std::chrono::milliseconds ms(ms_time);

    auto days = std::chrono::duration_cast<details::Days>(ms);
    ms -= days;

    auto hours = std::chrono::duration_cast<std::chrono::hours>(ms);
    ms -= hours;

    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(ms);
    ms -= minutes;

    auto seconds =
        std::chrono::duration_cast<std::chrono::duration<double>>(ms);

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
    if (seconds.count() != 0.l)
    {
        fmt += std::to_string(seconds.count()) + "S";
    }

    return fmt;
}

} // namespace time_utils
} // namespace redfish
