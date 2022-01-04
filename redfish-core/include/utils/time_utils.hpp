#pragma once

#include "logging.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <compare>
#include <cstddef>
#include <optional>
#include <ratio>
#include <string>
#include <string_view>
#include <system_error>

namespace redfish
{

namespace time_utils
{

namespace details
{

using Days = std::chrono::duration<long long, std::ratio<24 * 60 * 60>>;

inline void leftZeroPadding(std::string& str, const std::size_t padding)
{
    if (str.size() < padding)
    {
        str.insert(0, padding - str.size(), '0');
    }
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

    const char* end;
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
        std::string msStr = std::to_string(ms.count());
        details::leftZeroPadding(msStr, 3);
        fmt += msStr + "S";
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

} // namespace time_utils
} // namespace redfish
