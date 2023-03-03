#pragma once

#include "logging.hpp"

#include <boost/date_time.hpp>

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ratio>
#include <string>
#include <string_view>
#include <system_error>

// IWYU pragma: no_include <stddef.h>
// IWYU pragma: no_include <stdint.h>

namespace redfish
{

namespace time_utils
{

namespace details
{

constexpr intmax_t dayDuration = static_cast<intmax_t>(24 * 60 * 60);
using Days = std::chrono::duration<long long, std::ratio<dayDuration>>;

// Creates a string from an integer in the most efficient way possible without
// using std::locale.  Adds an exact zero pad based on the pad input parameter.
// Does not handle negative numbers.
inline std::string padZeros(int64_t value, size_t pad)
{
    std::string result(pad, '0');
    for (int64_t val = value; pad > 0; pad--)
    {
        result[pad - 1] = static_cast<char>('0' + val % 10);
        val /= 10;
    }
    return result;
}

template <typename FromTime>
bool fromDurationItem(std::string_view& fmt, const char postfix,
                      std::chrono::milliseconds& out)
{
    const size_t pos = fmt.find(postfix);
    if (pos == std::string::npos)
    {
        return true;
    }
    if ((pos + 1U) > fmt.size())
    {
        return false;
    }

    const char* end = nullptr;
    std::chrono::milliseconds::rep ticks = 0;
    if constexpr (std::is_same_v<FromTime, std::chrono::milliseconds>)
    {
        end = fmt.data() + std::min<size_t>(pos, 3U);
    }
    else
    {
        end = fmt.data() + pos;
    }

    auto [ptr, ec] = std::from_chars(fmt.data(), end, ticks);
    if (ptr != end || ec != std::errc())
    {
        BMCWEB_LOG_ERROR << "Failed to convert string to decimal with err: "
                         << static_cast<int>(ec) << "("
                         << std::make_error_code(ec).message() << "), ptr{"
                         << static_cast<const void*>(ptr) << "} != end{"
                         << static_cast<const void*>(end) << "})";
        return false;
    }

    if constexpr (std::is_same_v<FromTime, std::chrono::milliseconds>)
    {
        ticks *= static_cast<std::chrono::milliseconds::rep>(
            std::pow(10, 3 - std::min<size_t>(pos, 3U)));
    }
    if (ticks < 0)
    {
        return false;
    }

    out += FromTime(ticks);
    const auto maxConversionRange =
        std::chrono::duration_cast<FromTime>(std::chrono::milliseconds::max())
            .count();
    if (out < FromTime(ticks) || maxConversionRange < ticks)
    {
        return false;
    }

    fmt.remove_prefix(pos + 1U);
    return true;
}
} // namespace details

/**
 * @brief Convert string that represents value in Duration Format to its numeric
 *        equivalent.
 */
inline std::optional<std::chrono::milliseconds>
    fromDurationString(const std::string& str)
{
    std::chrono::milliseconds out = std::chrono::milliseconds::zero();
    std::string_view v = str;

    if (v.empty())
    {
        return out;
    }
    if (v.front() != 'P')
    {
        BMCWEB_LOG_ERROR << "Invalid duration format: " << str;
        return std::nullopt;
    }

    v.remove_prefix(1);
    if (!details::fromDurationItem<details::Days>(v, 'D', out))
    {
        BMCWEB_LOG_ERROR << "Invalid duration format: " << str;
        return std::nullopt;
    }

    if (v.empty())
    {
        return out;
    }
    if (v.front() != 'T')
    {
        BMCWEB_LOG_ERROR << "Invalid duration format: " << str;
        return std::nullopt;
    }

    v.remove_prefix(1);
    if (!details::fromDurationItem<std::chrono::hours>(v, 'H', out) ||
        !details::fromDurationItem<std::chrono::minutes>(v, 'M', out))
    {
        BMCWEB_LOG_ERROR << "Invalid duration format: " << str;
        return std::nullopt;
    }

    if (v.find('.') != std::string::npos && v.find('S') != std::string::npos)
    {
        if (!details::fromDurationItem<std::chrono::seconds>(v, '.', out) ||
            !details::fromDurationItem<std::chrono::milliseconds>(v, 'S', out))
        {
            BMCWEB_LOG_ERROR << "Invalid duration format: " << str;
            return std::nullopt;
        }
    }
    else if (!details::fromDurationItem<std::chrono::seconds>(v, 'S', out))
    {
        BMCWEB_LOG_ERROR << "Invalid duration format: " << str;
        return std::nullopt;
    }

    if (!v.empty())
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
inline std::string toDurationString(std::chrono::milliseconds ms)
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
        fmt += details::padZeros(ms.count(), 3);
        fmt += "S";
    }

    return fmt;
}

inline std::optional<std::string>
    toDurationStringFromUint(const uint64_t timeMs)
{
    static const uint64_t maxTimeMs =
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
// Returns year/month/day triple in civil calendar
// Preconditions:  z is number of days since 1970-01-01 and is in the range:
//                   [numeric_limits<Int>::min(),
//                   numeric_limits<Int>::max()-719468].
// Algorithm sourced from
// https://howardhinnant.github.io/date_algorithms.html#civil_from_days
// All constants are explained in the above
template <class IntType>
constexpr std::tuple<IntType, unsigned, unsigned>
    civilFromDays(IntType z) noexcept
{
    z += 719468;
    IntType era = (z >= 0 ? z : z - 146096) / 146097;
    unsigned doe = static_cast<unsigned>(z - era * 146097); // [0, 146096]
    unsigned yoe =
        (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; // [0, 399]
    IntType y = static_cast<IntType>(yoe) + era * 400;
    unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100); // [0, 365]
    unsigned mp = (5 * doy + 2) / 153;                      // [0, 11]
    unsigned d = doy - (153 * mp + 2) / 5 + 1;              // [1, 31]
    unsigned m = mp < 10 ? mp + 3 : mp - 9;                 // [1, 12]

    return std::tuple<IntType, unsigned, unsigned>(y + (m <= 2), m, d);
}

template <typename IntType, typename Period>
std::string toISO8061ExtendedStr(std::chrono::duration<IntType, Period> t)
{
    using seconds = std::chrono::duration<int>;
    using minutes = std::chrono::duration<int, std::ratio<60>>;
    using hours = std::chrono::duration<int, std::ratio<3600>>;
    using days = std::chrono::duration<
        IntType, std::ratio_multiply<hours::period, std::ratio<24>>>;

    // d is days since 1970-01-01
    days d = std::chrono::duration_cast<days>(t);

    // t is now time duration since midnight of day d
    t -= d;

    // break d down into year/month/day
    int year = 0;
    int month = 0;
    int day = 0;
    std::tie(year, month, day) = details::civilFromDays(d.count());
    // Check against limits.  Can't go above year 9999, and can't go below epoch
    // (1970)
    if (year >= 10000)
    {
        year = 9999;
        month = 12;
        day = 31;
        t = days(1) - std::chrono::duration<IntType, Period>(1);
    }
    else if (year < 1970)
    {
        year = 1970;
        month = 1;
        day = 1;
        t = std::chrono::duration<IntType, Period>::zero();
    }

    std::string out;
    out += details::padZeros(year, 4);
    out += '-';
    out += details::padZeros(month, 2);
    out += '-';
    out += details::padZeros(day, 2);
    out += 'T';
    hours hr = duration_cast<hours>(t);
    out += details::padZeros(hr.count(), 2);
    t -= hr;
    out += ':';

    minutes mt = duration_cast<minutes>(t);
    out += details::padZeros(mt.count(), 2);
    t -= mt;
    out += ':';

    seconds se = duration_cast<seconds>(t);
    out += details::padZeros(se.count(), 2);
    t -= se;

    if constexpr (std::is_same_v<typename decltype(t)::period, std::milli>)
    {
        out += '.';
        using MilliDuration = std::chrono::duration<int, std::milli>;
        MilliDuration subsec = duration_cast<MilliDuration>(t);
        out += details::padZeros(subsec.count(), 3);
    }
    else if constexpr (std::is_same_v<typename decltype(t)::period, std::micro>)
    {
        out += '.';

        using MicroDuration = std::chrono::duration<int, std::micro>;
        MicroDuration subsec = duration_cast<MicroDuration>(t);
        out += details::padZeros(subsec.count(), 6);
    }

    out += "+00:00";
    return out;
}
} // namespace details

// Returns the formatted date time string.
// Note that the maximum supported date is 9999-12-31T23:59:59+00:00, if
// the given |secondsSinceEpoch| is too large, we return the maximum supported
// date.
inline std::string getDateTimeUint(uint64_t secondsSinceEpoch)
{
    using DurationType = std::chrono::duration<uint64_t>;
    DurationType sinceEpoch(secondsSinceEpoch);
    return details::toISO8061ExtendedStr(sinceEpoch);
}

// Returns the formatted date time string with millisecond precision
// Note that the maximum supported date is 9999-12-31T23:59:59+00:00, if
// the given |secondsSinceEpoch| is too large, we return the maximum supported
// date.
inline std::string getDateTimeUintMs(uint64_t milliSecondsSinceEpoch)
{
    using DurationType = std::chrono::duration<uint64_t, std::milli>;
    DurationType sinceEpoch(milliSecondsSinceEpoch);
    return details::toISO8061ExtendedStr(sinceEpoch);
}

// Returns the formatted date time string with microsecond precision
inline std::string getDateTimeUintUs(uint64_t microSecondsSinceEpoch)
{
    using DurationType = std::chrono::duration<uint64_t, std::micro>;
    DurationType sinceEpoch(microSecondsSinceEpoch);
    return details::toISO8061ExtendedStr(sinceEpoch);
}

inline std::string getDateTimeStdtime(std::time_t secondsSinceEpoch)
{
    using DurationType = std::chrono::duration<std::time_t>;
    DurationType sinceEpoch(secondsSinceEpoch);
    return details::toISO8061ExtendedStr(sinceEpoch);
}

/**
 * Returns the current Date, Time & the local Time Offset
 * infromation in a pair
 *
 * @param[in] None
 *
 * @return std::pair<std::string, std::string>, which consist
 * of current DateTime & the TimeOffset strings respectively.
 */
inline std::pair<std::string, std::string> getDateTimeOffsetNow()
{
    std::time_t time = std::time(nullptr);
    std::string dateTime = getDateTimeStdtime(time);

    /* extract the local Time Offset value from the
     * recevied dateTime string.
     */
    std::string timeOffset("Z00:00");
    size_t lastPos = dateTime.size();
    size_t len = timeOffset.size();
    if (lastPos > len)
    {
        timeOffset = dateTime.substr(lastPos - len);
    }

    return std::make_pair(dateTime, timeOffset);
}

using usSinceEpoch = std::chrono::duration<uint64_t, std::micro>;
inline std::optional<usSinceEpoch> dateStringToEpoch(std::string_view datetime)
{
    std::string date(datetime);
    std::stringstream stream(date);
    // Convert from ISO 8601 to boost local_time
    // (BMC only has time in UTC)
    boost::posix_time::ptime posixTime;
    boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));
    // Facet gets deleted with the stringsteam
    auto ifc = std::make_unique<boost::local_time::local_time_input_facet>(
        "%Y-%m-%d %H:%M:%S%F %ZP");
    stream.imbue(std::locale(stream.getloc(), ifc.release()));

    boost::local_time::local_date_time ldt(boost::local_time::not_a_date_time);

    if (!(stream >> ldt))
    {
        return std::nullopt;
    }
    posixTime = ldt.utc_time();
    boost::posix_time::time_duration dur = posixTime - epoch;
    uint64_t durMicroSecs = static_cast<uint64_t>(dur.total_microseconds());
    return std::chrono::duration<uint64_t, std::micro>{durMicroSecs};
}

} // namespace time_utils
} // namespace redfish
