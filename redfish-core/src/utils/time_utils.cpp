// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "utils/time_utils.hpp"

#include "error_messages.hpp"
#include "http_response.hpp"
#include "logging.hpp"

#include <version>

#if __cpp_lib_chrono < 201907L
#include "utils/extern/date.h"
#endif
#include <array>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <ratio>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace redfish::time_utils
{

/**
 * @brief Convert string that represents value in Duration Format to its numeric
 *        equivalent.
 */
std::optional<std::chrono::milliseconds> fromDurationString(std::string_view v)
{
    std::chrono::milliseconds out = std::chrono::milliseconds::zero();
    enum class ProcessingStage
    {
        // P1DT1H1M1.100S
        P,
        Days,
        Hours,
        Minutes,
        Seconds,
        Milliseconds,
        Done,
    };
    ProcessingStage stage = ProcessingStage::P;

    while (!v.empty())
    {
        if (stage == ProcessingStage::P)
        {
            if (v.front() != 'P')
            {
                return std::nullopt;
            }
            v.remove_prefix(1);
            stage = ProcessingStage::Days;
            continue;
        }
        if (stage == ProcessingStage::Days)
        {
            if (v.front() == 'T')
            {
                v.remove_prefix(1);
                stage = ProcessingStage::Hours;
                continue;
            }
        }
        uint64_t ticks = 0;
        auto [ptr, ec] = std::from_chars(v.begin(), v.end(), ticks);
        if (ec != std::errc())
        {
            BMCWEB_LOG_ERROR("Failed to convert string \"{}\" to decimal", v);
            return std::nullopt;
        }
        size_t charactersRead = static_cast<size_t>(ptr - v.data());
        if (ptr >= v.end())
        {
            BMCWEB_LOG_ERROR("Missing postfix");
            return std::nullopt;
        }
        if (*ptr == 'D')
        {
            if (stage > ProcessingStage::Days)
            {
                return std::nullopt;
            }
            out += std::chrono::days(ticks);
        }
        else if (*ptr == 'H')
        {
            if (stage > ProcessingStage::Hours)
            {
                return std::nullopt;
            }
            out += std::chrono::hours(ticks);
        }
        else if (*ptr == 'M')
        {
            if (stage > ProcessingStage::Minutes)
            {
                return std::nullopt;
            }
            out += std::chrono::minutes(ticks);
        }
        else if (*ptr == '.')
        {
            if (stage > ProcessingStage::Seconds)
            {
                return std::nullopt;
            }
            out += std::chrono::seconds(ticks);
            stage = ProcessingStage::Milliseconds;
        }
        else if (*ptr == 'S')
        {
            // We could be seeing seconds for the first time, (as is the case in
            // 1S) or for the second time (in the case of 1.1S).
            if (stage <= ProcessingStage::Seconds)
            {
                out += std::chrono::seconds(ticks);
                stage = ProcessingStage::Milliseconds;
            }
            else if (stage > ProcessingStage::Milliseconds)
            {
                BMCWEB_LOG_ERROR("Got unexpected information at end of parse");
                return std::nullopt;
            }
            else
            {
                // Seconds could be any form of (1S, 1.1S, 1.11S, 1.111S);
                // Handle them all milliseconds are after the decimal point,
                // so they need right padded.
                if (charactersRead == 1)
                {
                    ticks *= 100;
                }
                else if (charactersRead == 2)
                {
                    ticks *= 10;
                }
                out += std::chrono::milliseconds(ticks);
                stage = ProcessingStage::Milliseconds;
            }
        }
        else
        {
            BMCWEB_LOG_ERROR("Unknown postfix {}", *ptr);
            return std::nullopt;
        }

        v.remove_prefix(charactersRead + 1U);
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

    std::chrono::days days = std::chrono::floor<std::chrono::days>(ms);
    ms -= days;

    std::chrono::hours hours = std::chrono::floor<std::chrono::hours>(ms);
    ms -= hours;

    std::chrono::minutes minutes = std::chrono::floor<std::chrono::minutes>(ms);
    ms -= minutes;

    std::chrono::seconds seconds = std::chrono::floor<std::chrono::seconds>(ms);
    ms -= seconds;
    std::string daysStr;
    if (days.count() > 0)
    {
        daysStr = std::format("{}D", days.count());
    }
    std::string hoursStr;
    if (hours.count() > 0)
    {
        hoursStr = std::format("{}H", hours.count());
    }
    std::string minStr;
    if (minutes.count() > 0)
    {
        minStr = std::format("{}M", minutes.count());
    }
    std::string secStr;
    if (seconds.count() != 0 || ms.count() != 0)
    {
        secStr = std::format("{}.{:03}S", seconds.count(), ms.count());
    }

    return std::format("P{}T{}{}{}", daysStr, hoursStr, minStr, secStr);
}

std::optional<std::string> toDurationStringFromUint(uint64_t timeMs)
{
    constexpr uint64_t maxTimeMs =
        static_cast<uint64_t>(std::chrono::milliseconds::max().count());

    if (maxTimeMs < timeMs)
    {
        return std::nullopt;
    }

    std::string duration = toDurationString(std::chrono::milliseconds(timeMs));
    if (duration.empty())
    {
        return std::nullopt;
    }

    return std::make_optional(duration);
}

namespace details
{

static const std::chrono::time_zone* getTimeZone(std::string_view tzName = "")
{
    const std::chrono::time_zone* tz = nullptr;
    try
    {
        if (tzName.empty())
        {
            tz = std::chrono::current_zone();
        }
        else
        {
            tz = std::chrono::locate_zone(tzName);
        }
    }
    catch (const std::runtime_error& e)
    {
        BMCWEB_LOG_ERROR("Error getting time zone: {}", e.what());
    }
    return tz;
}

template <typename IntType, typename Period>
std::string toISO8061ExtendedStr(
    const std::chrono::duration<IntType, Period> dur, std::string_view tzName)
{
    using namespace std::literals::chrono_literals;
    static_assert(sizeof(IntType) <= sizeof(uint64_t),
                  "IntType must be less than or equal to uint64_t");
    using SubType = std::chrono::duration<uint64_t, Period>;

    const std::chrono::time_zone* tz = getTimeZone(tzName);
    if (tz == nullptr)
    {
        BMCWEB_LOG_ERROR("Cannot find time zone: {}", tzName);
        return "";
    }

    std::chrono::sys_time<SubType> sysTimeOriginal(dur);

    std::chrono::zoned_time<SubType> zonedTime(tz, sysTimeOriginal);

    // Protect against integer overflow/underflow when adding offset
    std::chrono::local_time<SubType> duration(zonedTime.get_local_time());

    // Enforce 3 constraints
    // the result can't under or overflow the calculation
    // the resulting string needs to be representable as 4 digits

    // Midnight of the year 1970 in that timezone is the lowest representable
    // date
    using std::chrono::January;
    std::chrono::year_month_day tzEpoch(1970y, January, 1d);
    std::chrono::local_days tzEpochDays(tzEpoch);
    std::chrono::zoned_time<SubType> tzMin(tz, tzEpochDays);
    // We might underflow because the duration is negative (at which point our
    // unsigned timezone will underflow) or because the duration is not
    // representable in that timezone. Handle both by setting to 1970 in that
    // timezone
    if (dur.count() <= 0 || zonedTime.get_local_time() < tzMin.get_local_time())
    {
        BMCWEB_LOG_WARNING("Underflow from value {}",
                           duration.time_since_epoch().count());
        zonedTime = std::chrono::zoned_time<SubType>(tz, tzEpochDays);
    }
    else
    {
        // Midnight of the year 10000 is the first non representable
        // date as a string, so go back one tick to get the last representable
        // date
        std::chrono::year_month_day overflowDate(10000y, std::chrono::January,
                                                 1d);
        std::chrono::local_days maxDays(overflowDate);
        std::chrono::local_time<SubType> maxDuration(maxDays);
        maxDuration = maxDuration - SubType(1U);
        if (duration > maxDuration)
        {
            BMCWEB_LOG_WARNING("Overflow from value {}",
                               duration.time_since_epoch().count());
            zonedTime = std::chrono::zoned_time<SubType>(tz, maxDuration);
        }
    }

    return std::format("{:%Y-%m-%dT%H:%M:%S%Ez}", zonedTime);
}

} // namespace details

// Returns the formatted date time string.
// Note that the maximum supported date is 9999-12-31T23:59:59+00:00, if
// the given |secondsSinceEpoch| is too large, we return the maximum supported
// date.
std::string getDateTimeUint(uint64_t secondsSinceEpoch,
                            std::string_view timezoneName)
{
    using DurationType = std::chrono::duration<uint64_t>;
    DurationType sinceEpoch(secondsSinceEpoch);
    return details::toISO8061ExtendedStr(sinceEpoch, timezoneName);
}

// Returns the formatted date time string with millisecond precision
// Note that the maximum supported date is 9999-12-31T23:59:59+00:00, if
// the given |secondsSinceEpoch| is too large, we return the maximum supported
// date.
std::string getDateTimeUintMs(uint64_t milliSecondsSinceEpoch,
                              std::string_view timezoneName)
{
    using DurationType = std::chrono::duration<uint64_t, std::milli>;
    DurationType sinceEpoch(milliSecondsSinceEpoch);
    return details::toISO8061ExtendedStr(sinceEpoch, timezoneName);
}

// Returns the formatted date time string with microsecond precision
std::string getDateTimeUintUs(uint64_t microSecondsSinceEpoch,
                              std::string_view timezoneName)
{
    using DurationType = std::chrono::duration<uint64_t, std::micro>;
    DurationType sinceEpoch(microSecondsSinceEpoch);
    return details::toISO8061ExtendedStr(sinceEpoch, timezoneName);
}

std::string getDateTimeStdtime(std::chrono::system_clock::time_point timePoint,
                               std::string_view timezoneName)
{
    using std::chrono::floor;
    using std::chrono::seconds;
    // Floor to seconds
    auto secondsDuration = floor<seconds>(timePoint.time_since_epoch());
    return details::toISO8061ExtendedStr(secondsDuration, timezoneName);
}

/**
 * Returns the current Date, Time & the local Time Offset
 * information in a pair
 *
 * @param[in] None
 *
 * @return std::pair<std::string, std::string>, which consist
 * of current DateTime & the TimeOffset strings respectively.
 */
std::pair<std::string, std::string> getDateTimeOffsetNow()
{
    const std::chrono::time_zone* tz = nullptr;
    try
    {
        tz = std::chrono::current_zone();
    }
    catch (const std::runtime_error& e)
    {
        BMCWEB_LOG_ERROR("Error getting time zone: {}", e.what());
        return std::make_pair("", "");
    }
    using std::chrono::floor;
    using std::chrono::seconds;
    using std::chrono::system_clock;
    using std::chrono::time_point;
    system_clock::time_point now = system_clock::now();

    // Round down to the nearest second
    time_point<system_clock, seconds> nowSeconds = floor<seconds>(now);

    std::chrono::zoned_time zt{tz, nowSeconds};
    std::string datetime = std::format("{:%Y-%m-%dT%H:%M:%S%Ez}", zt);
    std::string timeOffset = std::format("{:%Ez}", zt);

    return std::make_pair(std::move(datetime), std::move(timeOffset));
}

using usSinceEpoch = std::chrono::duration<int64_t, std::micro>;

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
std::optional<std::string> getDateTimeIso8601(std::string_view datetime)
{
    std::optional<usSinceEpoch> us = dateStringToEpoch(datetime);
    if (!us)
    {
        return std::nullopt;
    }
    auto secondsDuration =
        std::chrono::duration_cast<std::chrono::seconds>(*us);

    return std::make_optional(
        getDateTimeUint(static_cast<uint64_t>(secondsDuration.count()), "UTC"));
}

/**
 * @brief ProductionDate report
 */
void productionDateReport(crow::Response& res, const std::string& buildDate)
{
    std::optional<std::string> valueStr = getDateTimeIso8601(buildDate);
    if (!valueStr)
    {
        messages::internalError();
        return;
    }
    res.jsonValue["ProductionDate"] = *valueStr;
}

std::optional<usSinceEpoch> dateStringToEpoch(std::string_view datetime)
{
    for (const char* format : std::to_array(
             {"%FT%T%Ez", "%FT%TZ", "%FT%T", "%Y%m%d", "%Y%m%dT%H%M%SZ"}))
    {
        // Parse using signed so we can detect negative dates
        std::chrono::sys_time<usSinceEpoch> date;
        std::istringstream iss(std::string{datetime});
#if __cpp_lib_chrono >= 201907L
        namespace chrono_from_stream = std::chrono;
#else
        namespace chrono_from_stream = date;
#endif
        if (chrono_from_stream::from_stream(iss, format, date))
        {
            if (date.time_since_epoch().count() < 0)
            {
                return std::nullopt;
            }
            if (iss.rdbuf()->in_avail() != 0)
            {
                // More information left at end of string.
                continue;
            }
            return date.time_since_epoch();
        }
    }
    return std::nullopt;
}
} // namespace redfish::time_utils
