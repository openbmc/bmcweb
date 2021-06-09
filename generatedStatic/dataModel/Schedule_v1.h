#ifndef SCHEDULE_V1
#define SCHEDULE_V1

#include "Schedule_v1.h"

#include <chrono>

enum class ScheduleV1DayOfWeek
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
enum class ScheduleV1MonthOfYear
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
struct ScheduleV1Schedule
{
    std::string name;
    std::chrono::milliseconds lifetime;
    int64_t maxOccurrences;
    std::chrono::time_point<std::chrono::system_clock> initialStartTime;
    std::chrono::milliseconds recurrenceInterval;
    ScheduleV1DayOfWeek enabledDaysOfWeek;
    int64_t enabledDaysOfMonth;
    ScheduleV1MonthOfYear enabledMonthsOfYear;
    std::string enabledIntervals;
};
#endif
