#ifndef SCHEDULE_V1
#define SCHEDULE_V1

#include "Schedule_v1.h"

#include <chrono>

enum class Schedule_v1_DayOfWeek
{
    Monday,
    Tuesday,
    Wednesday,
    Thursday,
    Friday,
    Saturday,
    Sunday,
    Every,
};
enum class Schedule_v1_MonthOfYear
{
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
struct Schedule_v1_Schedule
{
    std::string name;
    std::chrono::milliseconds lifetime;
    int64_t maxOccurrences;
    std::chrono::time_point initialStartTime;
    std::chrono::milliseconds recurrenceInterval;
    Schedule_v1_DayOfWeek enabledDaysOfWeek;
    int64_t enabledDaysOfMonth;
    Schedule_v1_MonthOfYear enabledMonthsOfYear;
    std::string enabledIntervals;
};
#endif
