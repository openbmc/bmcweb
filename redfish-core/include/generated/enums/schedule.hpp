#pragma once
#include <nlohmann/json.hpp>

namespace schedule
{
// clang-format off

enum class DayOfWeek{
    Invalid,
    Monday,
    Tuesday,
    Wednesday,
    Thursday,
    Friday,
    Saturday,
    Sunday,
    Every,
};

enum class MonthOfYear{
    Invalid,
    January,
    February,
    March,
    April,
    May,
    June,
    July,
    August,
    September,
    October,
    November,
    December,
    Every,
};

NLOHMANN_JSON_SERIALIZE_ENUM(DayOfWeek, {
    {DayOfWeek::Invalid, "Invalid"},
    {DayOfWeek::Monday, "Monday"},
    {DayOfWeek::Tuesday, "Tuesday"},
    {DayOfWeek::Wednesday, "Wednesday"},
    {DayOfWeek::Thursday, "Thursday"},
    {DayOfWeek::Friday, "Friday"},
    {DayOfWeek::Saturday, "Saturday"},
    {DayOfWeek::Sunday, "Sunday"},
    {DayOfWeek::Every, "Every"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(MonthOfYear, {
    {MonthOfYear::Invalid, "Invalid"},
    {MonthOfYear::January, "January"},
    {MonthOfYear::February, "February"},
    {MonthOfYear::March, "March"},
    {MonthOfYear::April, "April"},
    {MonthOfYear::May, "May"},
    {MonthOfYear::June, "June"},
    {MonthOfYear::July, "July"},
    {MonthOfYear::August, "August"},
    {MonthOfYear::September, "September"},
    {MonthOfYear::October, "October"},
    {MonthOfYear::November, "November"},
    {MonthOfYear::December, "December"},
    {MonthOfYear::Every, "Every"},
});

}
// clang-format on
