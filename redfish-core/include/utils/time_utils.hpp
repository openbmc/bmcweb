// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "http_response.hpp"

#include <chrono>
#include <cstdint>
#include <ctime>
#include <optional>
#include <ratio>
#include <string>
#include <string_view>
#include <utility>

namespace redfish
{

namespace time_utils
{

enum class DateFormat
{
    UTC,
    LocalTimezone,
};

// Apply timezone offset to a duration with overflow/underflow protection.
// Uses offset (signed std::chrono::seconds) for the sign check to avoid
// unsigned wraparound when casting negative offsets to SubType.
template <typename SubType>
SubType applyOffsetClamped(SubType val, std::chrono::seconds offset)
{
    bool pos = offset >= std::chrono::seconds::zero();
    auto absDur = std::chrono::duration_cast<SubType>(pos ? offset : -offset);
    auto clamp = pos ? SubType::max() : SubType::min();
    bool overflow = pos ? val.count() > (clamp.count() - absDur.count())
                        : val.count() < (clamp.count() + absDur.count());
    if (overflow)
    {
        return clamp;
    }
    return pos ? val + absDur : val - absDur;
}

/**
 * @brief Convert string that represents value in Duration Format to its numeric
 *        equivalent.
 */
std::optional<std::chrono::milliseconds> fromDurationString(std::string_view v);

/**
 * @brief Convert time value into duration format that is based on ISO 8601.
 *        Example output: "P12DT1M5.5S"
 *        Ref: Redfish Specification, Section 9.4.4. Duration values
 */
std::string toDurationString(std::chrono::milliseconds ms);

std::optional<std::string> toDurationStringFromUint(uint64_t timeMs);

// Returns the formatted date time string.
// If tz is not provided, the UTC time is used.
// Note that the maximum supported date is 9999-12-31T23:59:59+00:00, if
// the given |secondsSinceEpoch| is too large, we return the maximum supported
// date.
std::string getDateTimeUint(uint64_t secondsSinceEpoch);

// Returns the formatted date time string with millisecond precision
// Note that the maximum supported date is 9999-12-31T23:59:59+00:00, if
// the given |secondsSinceEpoch| is too large, we return the maximum supported
// date.
std::string getDateTimeUintMs(uint64_t milliSecondsSinceEpoch);

// Returns the formatted date time string with microsecond precision
std::string getDateTimeUintUs(uint64_t microSecondsSinceEpoch);

// Tz variants: format using the provided timezone
std::string getDateTimeUintTz(uint64_t secondsSinceEpoch,
                              const std::chrono::time_zone& outputTimezone);
std::string getDateTimeUintMsTz(uint64_t milliSecondsSinceEpoch,
                                const std::chrono::time_zone& outputTimezone);
std::string getDateTimeUintUsTz(uint64_t microSecondsSinceEpoch,
                                const std::chrono::time_zone& outputTimezone);
std::string getDateTimeStdtimeTz(std::time_t secondsSinceEpoch,
                                 const std::chrono::time_zone& outputTimezone);

// DateFormat overloads: caller specifies UTC or LocalTimezone
std::string getDateTimeUint(uint64_t secondsSinceEpoch, DateFormat dateFormat);
std::string getDateTimeUintMs(uint64_t milliSecondsSinceEpoch,
                              DateFormat dateFormat);
std::string getDateTimeUintUs(uint64_t microSecondsSinceEpoch,
                              DateFormat dateFormat);
std::string getDateTimeStdtime(std::time_t secondsSinceEpoch,
                               DateFormat dateFormat);

/**
 * Returns the current Date, Time & the local Time Offset
 * information in a pair
 *
 * @param[in] bool - true if the time is to be returned in local time, false if
 * in UTC time
 *
 * @return std::pair<std::string, std::string>, which consist
 * of current DateTime & the TimeOffset strings respectively.
 */
std::pair<std::string, std::string> getDateTimeOffsetNow(DateFormat dateFormat);

using usSinceEpoch = std::chrono::duration<int64_t, std::micro>;
std::optional<usSinceEpoch> dateStringToEpoch(std::string_view datetime);

/**
 * @brief Returns the datetime in ISO 8601 format
 *
 * @param[in] std::string_view the date of item manufacture in ISO 8601 format,
 *            either as YYYYMMDD or YYYYMMDDThhmmssZ
 * Ref: https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/
 *      xyz/openbmc_project/Inventory/Decorator/Asset.interface.yaml#L16
 *
 * @return std::string which consist the datetime
 */
std::optional<std::string> getDateTimeIso8601(std::string_view datetime);

/**
 * @brief ProductionDate report
 */
void productionDateReport(crow::Response& res, const std::string& buildDate);

} // namespace time_utils
} // namespace redfish
