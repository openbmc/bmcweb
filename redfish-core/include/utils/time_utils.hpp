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

#include <chrono>
#include <string>

namespace redfish
{

namespace time_utils
{

/**
 * @brief Convert time value into duration format that is based on ISO 8601.
 *        Example output: "P192DT12H01M01.500000S"
 */
std::string toDurationFormat(std::chrono::milliseconds ms)
{
    std::string fmt;
    fmt.reserve(sizeof("PxxxxxxxxxxxxDTxxHxxMxx.xxxxxxS\0"));

    using Days = std::chrono::duration<long long int, std::ratio<24 * 60 * 60>>;
    auto days = std::chrono::floor<Days>(ms);
    ms -= days;

    auto hours = std::chrono::floor<std::chrono::hours>(ms);
    ms -= hours;

    auto minutes = std::chrono::floor<std::chrono::minutes>(ms);
    ms -= minutes;

    auto seconds = std::chrono::floor<std::chrono::duration<double>>(ms);

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
