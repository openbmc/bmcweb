// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

// Test-only header exposing internal functions for unit testing.
// Do not include in production code.

#include "utils/time_utils.hpp"

#include <chrono>
#include <cstdint>
#include <ctime>
#include <ratio>
#include <string>

namespace redfish::time_utils
{

// Explicit instantiations of resolveOffset for testable duration types
template <typename DurationType>
std::chrono::minutes resolveOffset(DateFormat dateFormat,
                                   std::chrono::sys_time<DurationType> sysTime);

extern template std::chrono::minutes
    resolveOffset<std::chrono::duration<uint64_t>>(
        DateFormat, std::chrono::sys_time<std::chrono::duration<uint64_t>>);
extern template std::chrono::minutes
    resolveOffset<std::chrono::duration<uint64_t, std::milli>>(
        DateFormat,
        std::chrono::sys_time<std::chrono::duration<uint64_t, std::milli>>);
extern template std::chrono::minutes
    resolveOffset<std::chrono::duration<uint64_t, std::micro>>(
        DateFormat,
        std::chrono::sys_time<std::chrono::duration<uint64_t, std::micro>>);
extern template std::chrono::minutes
    resolveOffset<std::chrono::duration<std::time_t>>(
        DateFormat, std::chrono::sys_time<std::chrono::duration<std::time_t>>);

namespace details
{

// Non-template wrappers around toISO8061ExtendedStr for testing
std::string toISO8061ExtendedStrSeconds(uint64_t secondsSinceEpoch,
                                        std::chrono::minutes offset);
std::string toISO8061ExtendedStrMs(uint64_t msSinceEpoch,
                                   std::chrono::minutes offset);
std::string toISO8061ExtendedStrUs(uint64_t usSinceEpoch,
                                   std::chrono::minutes offset);
std::string toISO8061ExtendedStrStdtime(std::time_t secondsSinceEpoch,
                                        std::chrono::minutes offset);

} // namespace details
} // namespace redfish::time_utils
