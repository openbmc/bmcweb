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

#include <boost/algorithm/string/trim.hpp>
#include <chrono>
#include <cstdint>
#include <string>
#include <type_traits>

namespace redfish
{

namespace time_utils
{

namespace details
{

template <typename T>
std::string toDurationFormatItem(std::chrono::milliseconds& duration,
                                 const char* postfix)
{
    const auto t = std::chrono::duration_cast<T>(duration);
    if (t.count() != 0)
    {
        std::string fmt = std::to_string(t.count());

        if constexpr (std::is_same<T, std::chrono::milliseconds>::value)
        {
            fmt = std::to_string(
                static_cast<double>(t.count()) /
                static_cast<double>(std::chrono::milliseconds::period::den));
            boost::algorithm::trim_right_if(
                fmt, [](char x) { return x == '0' || x == '.'; });
        }

        duration -= t;
        return fmt + postfix;
    }

    return "";
}

template <typename T>
static uint32_t fromDurationFormatItem(std::string_view& fmt,
                                       const char* postfix)
{
    auto pos = fmt.find(postfix);
    if (pos == std::string::npos)
    {
        return 0;
    }

    long out;
    if constexpr (std::is_same<T, std::chrono::milliseconds>::value)
    {
        out = static_cast<long>(std::strtod(fmt.data(), nullptr) * 1000);
    }
    else
    {
        out = std::strtol(fmt.data(), nullptr, 10);
    }

    fmt.remove_prefix(pos + 1);
    return static_cast<uint32_t>(std::chrono::milliseconds(T(out)).count());
}
} // namespace details

/**
 * @brief Convert time value into duration format that is based on ISO 8601.
 *        Pattern: "-?P(\\d+D)?(T(\\d+H)?(\\d+M)?(\\d+(.\\d+)?S)?)?"
 *        Reference: "Redfish Telemetry White Paper".
 */
std::string toDurationFormat(const uint32_t ms)
{
    std::chrono::milliseconds duration(ms);
    std::string fmt;
    fmt.reserve(sizeof("PxxxDTxxHxxMxx.xxxxxxS"));

    using Days = std::chrono::duration<int, std::ratio<24 * 60 * 60>>;

    fmt += "P";
    fmt += details::toDurationFormatItem<Days>(duration, "D");
    if (duration.count() == 0)
    {
        return fmt;
    }

    fmt += "T";
    fmt += details::toDurationFormatItem<std::chrono::hours>(duration, "H");
    fmt += details::toDurationFormatItem<std::chrono::minutes>(duration, "M");
    fmt +=
        details::toDurationFormatItem<std::chrono::milliseconds>(duration, "S");

    return fmt;
}

static uint32_t fromDurationFormat(std::string_view fmt)
{
    uint32_t out = 0;

    using Days = std::chrono::duration<int, std::ratio<24 * 60 * 60>>;

    if (fmt[0] != 'P')
    {
        return out;
    }
    fmt.remove_prefix(1);
    out += details::fromDurationFormatItem<Days>(fmt, "D");

    if (fmt[0] != 'T')
    {
        return out;
    }
    fmt.remove_prefix(1);
    out += details::fromDurationFormatItem<std::chrono::hours>(fmt, "H");
    out += details::fromDurationFormatItem<std::chrono::minutes>(fmt, "M");
    out += details::fromDurationFormatItem<std::chrono::milliseconds>(fmt, "S");

    return out;
}
} // namespace time_utils
} // namespace redfish
