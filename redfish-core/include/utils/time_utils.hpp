#pragma once

#include <chrono>
#include <string>

namespace redfish
{

namespace time_utils
{

namespace details
{

inline void leftZeroPadding(std::string& str, const std::size_t padding)
{
    if (str.size() < padding)
    {
        str.insert(0, padding - str.size(), '0');
    }
}
} // namespace details

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

    using Days = std::chrono::duration<long, std::ratio<24 * 60 * 60>>;
    Days days = std::chrono::floor<Days>(ms);
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
