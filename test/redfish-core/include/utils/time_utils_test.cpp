// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "utils/time_utils.hpp"
#include "utils/time_utils_internal.hpp"

#include <chrono>
#include <cstdint>
#include <ctime>
#include <limits>
#include <optional>
#include <ratio>
#include <string>
#include <version>

#include <gtest/gtest.h>

namespace redfish::time_utils
{
namespace
{

TEST(FromDurationTest, PositiveTests)
{
    EXPECT_EQ(fromDurationString("PT12S"), std::chrono::milliseconds(12000));
    EXPECT_EQ(fromDurationString("PT0.204S"), std::chrono::milliseconds(204));
    EXPECT_EQ(fromDurationString("PT0.2S"), std::chrono::milliseconds(200));
    EXPECT_EQ(fromDurationString("PT50M"), std::chrono::milliseconds(3000000));
    EXPECT_EQ(fromDurationString("PT23H"), std::chrono::milliseconds(82800000));
    EXPECT_EQ(fromDurationString("P51D"),
              std::chrono::milliseconds(4406400000));
    EXPECT_EQ(fromDurationString("PT2H40M10.1S"),
              std::chrono::milliseconds(9610100));
    EXPECT_EQ(fromDurationString("P20DT2H40M10.1S"),
              std::chrono::milliseconds(1737610100));
    EXPECT_EQ(fromDurationString(""), std::chrono::milliseconds(0));
}

TEST(FromDurationTest, NegativeTests)
{
    EXPECT_EQ(fromDurationString("PTS"), std::nullopt);
    EXPECT_EQ(fromDurationString("P1T"), std::nullopt);
    EXPECT_EQ(fromDurationString("PT100M1000S100"), std::nullopt);
    EXPECT_EQ(fromDurationString("PDTHMS"), std::nullopt);
    EXPECT_EQ(fromDurationString("P9999999999999999999999999DT"), std::nullopt);
    EXPECT_EQ(fromDurationString("PD222T222H222M222.222S"), std::nullopt);
    EXPECT_EQ(fromDurationString("PT99999H9999999999999999999999M99999999999S"),
              std::nullopt);
    EXPECT_EQ(fromDurationString("PT-9H"), std::nullopt);
}
TEST(ToDurationTest, PositiveTests)
{
    EXPECT_EQ(toDurationString(std::chrono::milliseconds(12000)), "PT12.000S");
    EXPECT_EQ(toDurationString(std::chrono::milliseconds(204)), "PT0.204S");
    EXPECT_EQ(toDurationString(std::chrono::milliseconds(200)), "PT0.200S");
    EXPECT_EQ(toDurationString(std::chrono::milliseconds(3000000)), "PT50M");
    EXPECT_EQ(toDurationString(std::chrono::milliseconds(82800000)), "PT23H");
    EXPECT_EQ(toDurationString(std::chrono::milliseconds(4406400000)), "P51DT");
    EXPECT_EQ(toDurationString(std::chrono::milliseconds(9610100)),
              "PT2H40M10.100S");
    EXPECT_EQ(toDurationString(std::chrono::milliseconds(1737610100)),
              "P20DT2H40M10.100S");
}

TEST(ToDurationTest, NegativeTests)
{
    EXPECT_EQ(toDurationString(std::chrono::milliseconds(-250)), "");
}

TEST(ToDurationStringFromUintTest, PositiveTests)
{
    uint64_t maxAcceptedTimeMs =
        static_cast<uint64_t>(std::chrono::milliseconds::max().count());

    EXPECT_NE(toDurationStringFromUint(maxAcceptedTimeMs), std::nullopt);
    EXPECT_EQ(toDurationStringFromUint(0), "PT");
    EXPECT_EQ(toDurationStringFromUint(250), "PT0.250S");
    EXPECT_EQ(toDurationStringFromUint(5000), "PT5.000S");
}

TEST(ToDurationStringFromUintTest, NegativeTests)
{
    uint64_t minNotAcceptedTimeMs =
        static_cast<uint64_t>(std::chrono::milliseconds::max().count()) + 1;

    EXPECT_EQ(toDurationStringFromUint(minNotAcceptedTimeMs), std::nullopt);
    EXPECT_EQ(toDurationStringFromUint(static_cast<uint64_t>(-1)),
              std::nullopt);
}

TEST(ToISO8061ExtendedStr, OffsetTests)
{
    using std::chrono::minutes;

    // UTC offset (0)
    EXPECT_EQ(details::toISO8061ExtendedStrStdtime(1638312095, minutes(0)),
              "2021-11-30T22:41:35+00:00");

    // Berlin winter (+1h = 60min)
    EXPECT_EQ(details::toISO8061ExtendedStrStdtime(1638312095, minutes(60)),
              "2021-11-30T23:41:35+01:00");

    // New York winter (-5h = -300min)
    EXPECT_EQ(details::toISO8061ExtendedStrStdtime(1638312095, minutes(-300)),
              "2021-11-30T17:41:35-05:00");

    // Kolkata (+5:30 = 330min)
    EXPECT_EQ(details::toISO8061ExtendedStrStdtime(1638312095, minutes(330)),
              "2021-12-01T04:11:35+05:30");

    // Kathmandu (+5:45 = 345min)
    EXPECT_EQ(details::toISO8061ExtendedStrStdtime(1638312095, minutes(345)),
              "2021-12-01T04:26:35+05:45");

    // Extreme west (-12h = -720min)
    EXPECT_EQ(details::toISO8061ExtendedStrStdtime(1638312095, minutes(-720)),
              "2021-11-30T10:41:35-12:00");

    // Extreme east (+14h = 840min)
    EXPECT_EQ(details::toISO8061ExtendedStrStdtime(1638312095, minutes(840)),
              "2021-12-01T12:41:35+14:00");

    // Limits with offset (+02:00 = 120min)
    EXPECT_EQ(details::toISO8061ExtendedStrStdtime(
                  std::numeric_limits<std::time_t>::max(), minutes(120)),
              "9999-12-31T23:59:59+02:00");

    EXPECT_EQ(details::toISO8061ExtendedStrStdtime(
                  std::numeric_limits<std::time_t>::min(), minutes(120)),
              "1970-01-01T00:00:00+02:00");

    // Limits with UTC offset
    EXPECT_EQ(details::toISO8061ExtendedStrStdtime(
                  std::numeric_limits<std::time_t>::max(), minutes(0)),
              "9999-12-31T23:59:59+00:00");

    EXPECT_EQ(details::toISO8061ExtendedStrStdtime(
                  std::numeric_limits<std::time_t>::min(), minutes(0)),
              "1970-01-01T00:00:00+00:00");
}

TEST(ToISO8061ExtendedStr, InvalidOffsets)
{
    using std::chrono::minutes;

    // Offset beyond +14h (+15h = 900min) — falls back to UTC
    EXPECT_EQ(details::toISO8061ExtendedStrStdtime(1638312095, minutes(900)),
              "2021-11-30T22:41:35+00:00");

    // Offset beyond -12h (-13h = -780min) — falls back to UTC
    EXPECT_EQ(details::toISO8061ExtendedStrStdtime(1638312095, minutes(-780)),
              "2021-11-30T22:41:35+00:00");

    // Very large positive offset (+24h = 1440min) — falls back to UTC
    EXPECT_EQ(details::toISO8061ExtendedStrStdtime(1638312095, minutes(1440)),
              "2021-11-30T22:41:35+00:00");

    // Very large negative offset (-24h = -1440min) — falls back to UTC
    EXPECT_EQ(details::toISO8061ExtendedStrStdtime(1638312095, minutes(-1440)),
              "2021-11-30T22:41:35+00:00");

    // Boundary: exactly at +14h (840min) — valid, not clamped
    EXPECT_EQ(details::toISO8061ExtendedStrStdtime(1638312095, minutes(840)),
              "2021-12-01T12:41:35+14:00");

    // Boundary: exactly at -12h (-720min) — valid, not clamped
    EXPECT_EQ(details::toISO8061ExtendedStrStdtime(1638312095, minutes(-720)),
              "2021-11-30T10:41:35-12:00");

    // Just beyond +14h (+841min) — falls back to UTC
    EXPECT_EQ(details::toISO8061ExtendedStrStdtime(1638312095, minutes(841)),
              "2021-11-30T22:41:35+00:00");

    // Just beyond -12h (-721min) — falls back to UTC
    EXPECT_EQ(details::toISO8061ExtendedStrStdtime(1638312095, minutes(-721)),
              "2021-11-30T22:41:35+00:00");
}

TEST(ToISO8061ExtendedStr, SecondsUint)
{
    using std::chrono::minutes;

    // UTC
    EXPECT_EQ(
        details::toISO8061ExtendedStrSeconds(uint64_t{1638312095}, minutes(0)),
        "2021-11-30T22:41:35+00:00");

    // New York winter (-05:00 = -300min)
    EXPECT_EQ(details::toISO8061ExtendedStrSeconds(uint64_t{1638312095},
                                                   minutes(-300)),
              "2021-11-30T17:41:35-05:00");

    // Limits
    EXPECT_EQ(details::toISO8061ExtendedStrSeconds(
                  std::numeric_limits<uint64_t>::max(), minutes(0)),
              "9999-12-31T23:59:59+00:00");

    EXPECT_EQ(details::toISO8061ExtendedStrSeconds(
                  std::numeric_limits<uint64_t>::min(), minutes(0)),
              "1970-01-01T00:00:00+00:00");
}

TEST(ToISO8061ExtendedStr, MillisecondPrecision)
{
    using std::chrono::minutes;

    EXPECT_EQ(
        details::toISO8061ExtendedStrMs(uint64_t{1638312095123}, minutes(-300)),
        "2021-11-30T17:41:35.123-05:00");

    EXPECT_EQ(
        details::toISO8061ExtendedStrMs(uint64_t{1638312095123}, minutes(0)),
        "2021-11-30T22:41:35.123+00:00");

    // Limits
    EXPECT_EQ(details::toISO8061ExtendedStrMs(
                  std::numeric_limits<uint64_t>::max(), minutes(0)),
              "9999-12-31T23:59:59.999+00:00");

    EXPECT_EQ(details::toISO8061ExtendedStrMs(
                  std::numeric_limits<uint64_t>::min(), minutes(0)),
              "1970-01-01T00:00:00.000+00:00");
}

TEST(ToISO8061ExtendedStr, MicrosecondPrecision)
{
    using std::chrono::minutes;

    EXPECT_EQ(details::toISO8061ExtendedStrUs(uint64_t{1638312095123456},
                                              minutes(-300)),
              "2021-11-30T17:41:35.123456-05:00");

    EXPECT_EQ(
        details::toISO8061ExtendedStrUs(uint64_t{1638312095123456}, minutes(0)),
        "2021-11-30T22:41:35.123456+00:00");

    // Limits
    EXPECT_EQ(details::toISO8061ExtendedStrUs(
                  std::numeric_limits<uint64_t>::max(), minutes(0)),
              "9999-12-31T23:59:59.999999+00:00");

    EXPECT_EQ(details::toISO8061ExtendedStrUs(
                  std::numeric_limits<uint64_t>::min(), minutes(0)),
              "1970-01-01T00:00:00.000000+00:00");
}

TEST(GetDateTimeStdtime, UTCTests)
{
    EXPECT_EQ(getDateTimeStdtime(std::time_t{1638312095}, DateFormat::UTC),
              "2021-11-30T22:41:35+00:00");

    EXPECT_EQ(getDateTimeStdtime(std::numeric_limits<std::time_t>::max(),
                                 DateFormat::UTC),
              "9999-12-31T23:59:59+00:00");

    EXPECT_EQ(getDateTimeStdtime(std::numeric_limits<std::time_t>::min(),
                                 DateFormat::UTC),
              "1970-01-01T00:00:00+00:00");

    EXPECT_EQ(getDateTimeStdtime(std::time_t{0}, DateFormat::UTC),
              "1970-01-01T00:00:00+00:00");
}

TEST(ResolveOffset, UTCAlwaysReturnsZero)
{
    using minutes = std::chrono::minutes;
    using DurationType = std::chrono::duration<uint64_t>;
    std::chrono::sys_time<DurationType> sysTime{DurationType(1638312095)};

    EXPECT_EQ(resolveOffset(DateFormat::UTC, sysTime), minutes(0));
}

TEST(ResolveOffset, LocalTimezoneMatchesCurrentZone)
{
    using minutes = std::chrono::minutes;
    using DurationType = std::chrono::duration<uint64_t>;
    std::chrono::sys_time<DurationType> sysTime{DurationType(1638312095)};

    minutes expected = std::chrono::duration_cast<minutes>(
        std::chrono::current_zone()->get_info(sysTime).offset);
    EXPECT_EQ(resolveOffset(DateFormat::LocalTimezone, sysTime), expected);
}

TEST(GetDateTimeUint, ConversionTests)
{
    EXPECT_EQ(getDateTimeUint(uint64_t{1638312095}, DateFormat::UTC),
              "2021-11-30T22:41:35+00:00");
    // some time in the future, beyond 2038
    EXPECT_EQ(getDateTimeUint(uint64_t{41638312095}, DateFormat::UTC),
              "3289-06-18T21:48:15+00:00");
    // the maximum time we support
    EXPECT_EQ(getDateTimeUint(uint64_t{253402300799}, DateFormat::UTC),
              "9999-12-31T23:59:59+00:00");

    // returns the maximum Redfish date
    EXPECT_EQ(getDateTimeUint(std::numeric_limits<uint64_t>::max(),
                              DateFormat::UTC),
              "9999-12-31T23:59:59+00:00");

    EXPECT_EQ(getDateTimeUint(std::numeric_limits<uint64_t>::min(),
                              DateFormat::UTC),
              "1970-01-01T00:00:00+00:00");
}

TEST(GetDateTimeUintMs, ConverstionTests)
{
    EXPECT_EQ(getDateTimeUintMs(uint64_t{1638312095123}, DateFormat::UTC),
              "2021-11-30T22:41:35.123+00:00");
    // returns the maximum Redfish date
    EXPECT_EQ(getDateTimeUintMs(std::numeric_limits<uint64_t>::max(),
                                DateFormat::UTC),
              "9999-12-31T23:59:59.999+00:00");
    EXPECT_EQ(getDateTimeUintMs(std::numeric_limits<uint64_t>::min(),
                                DateFormat::UTC),
              "1970-01-01T00:00:00.000+00:00");
}

TEST(Utility, GetDateTimeUintUs)
{
    EXPECT_EQ(getDateTimeUintUs(uint64_t{1638312095123456}, DateFormat::UTC),
              "2021-11-30T22:41:35.123456+00:00");
    // returns the maximum Redfish date
    EXPECT_EQ(getDateTimeUintUs(std::numeric_limits<uint64_t>::max(),
                                DateFormat::UTC),
              "9999-12-31T23:59:59.999999+00:00");
    EXPECT_EQ(getDateTimeUintUs(std::numeric_limits<uint64_t>::min(),
                                DateFormat::UTC),
              "1970-01-01T00:00:00.000000+00:00");
}

TEST(GetDateTimeUint, LocalTimezoneTests)
{
    using DurationType = std::chrono::duration<uint64_t>;
    std::chrono::sys_time<DurationType> sysTime{DurationType(1638312095)};
    std::chrono::minutes offset =
        std::chrono::duration_cast<std::chrono::minutes>(
            std::chrono::current_zone()->get_info(sysTime).offset);

    // LocalTimezone result should match manual offset calculation
    EXPECT_EQ(
        getDateTimeUint(uint64_t{1638312095}, DateFormat::LocalTimezone),
        details::toISO8061ExtendedStrSeconds(uint64_t{1638312095}, offset));
}

TEST(GetDateTimeUintMs, LocalTimezoneTests)
{
    using DurationType = std::chrono::duration<uint64_t, std::milli>;
    std::chrono::sys_time<DurationType> sysTime{DurationType(1638312095123)};
    std::chrono::minutes offset =
        std::chrono::duration_cast<std::chrono::minutes>(
            std::chrono::current_zone()->get_info(sysTime).offset);

    EXPECT_EQ(
        getDateTimeUintMs(uint64_t{1638312095123}, DateFormat::LocalTimezone),
        details::toISO8061ExtendedStrMs(uint64_t{1638312095123}, offset));
}

TEST(GetDateTimeUintUs, LocalTimezoneTests)
{
    using DurationType = std::chrono::duration<uint64_t, std::micro>;
    std::chrono::sys_time<DurationType> sysTime{DurationType(1638312095123456)};
    std::chrono::minutes offset =
        std::chrono::duration_cast<std::chrono::minutes>(
            std::chrono::current_zone()->get_info(sysTime).offset);

    EXPECT_EQ(
        getDateTimeUintUs(uint64_t{1638312095123456},
                          DateFormat::LocalTimezone),
        details::toISO8061ExtendedStrUs(uint64_t{1638312095123456}, offset));
}

TEST(GetDateTimeStdtime, LocalTimezoneTests)
{
    using DurationType = std::chrono::duration<std::time_t>;
    std::chrono::sys_time<DurationType> sysTime{DurationType(1638312095)};
    std::chrono::minutes offset =
        std::chrono::duration_cast<std::chrono::minutes>(
            std::chrono::current_zone()->get_info(sysTime).offset);

    EXPECT_EQ(
        getDateTimeStdtime(std::time_t{1638312095}, DateFormat::LocalTimezone),
        details::toISO8061ExtendedStrStdtime(std::time_t{1638312095}, offset));
}

TEST(ApplyOffsetClamped, SecondsUnsigned)
{
    using Dur = std::chrono::duration<uint64_t>;

    // Zero offset returns duration unchanged
    EXPECT_EQ(applyOffsetClamped(Dur(1000), std::chrono::seconds(0)),
              Dur(1000));

    // Positive offset adds normally
    EXPECT_EQ(applyOffsetClamped(Dur(1000), std::chrono::seconds(500)),
              Dur(1500));

    // Negative offset subtracts normally
    EXPECT_EQ(applyOffsetClamped(Dur(1000), std::chrono::seconds(-500)),
              Dur(500));

    // Positive offset overflow clamps to max
    EXPECT_EQ(applyOffsetClamped(Dur(Dur::max().count() - 10),
                                 std::chrono::seconds(20)),
              Dur::max());

    // Negative offset underflow clamps to min (0 for unsigned)
    EXPECT_EQ(applyOffsetClamped(Dur(10), std::chrono::seconds(-20)),
              Dur::min());

    // Positive offset exactly reaches max (no clamp)
    EXPECT_EQ(applyOffsetClamped(Dur(Dur::max().count() - 10),
                                 std::chrono::seconds(10)),
              Dur::max());

    // Negative offset exactly reaches min (no clamp)
    EXPECT_EQ(applyOffsetClamped(Dur(10), std::chrono::seconds(-10)),
              Dur::min());
}

TEST(ApplyOffsetClamped, MicrosecondsUnsigned)
{
    using Dur = std::chrono::duration<uint64_t, std::micro>;

    // Normal positive offset (5 seconds = 5000000 us)
    EXPECT_EQ(applyOffsetClamped(Dur(1000000), std::chrono::seconds(5)),
              Dur(6000000));

    // Normal negative offset
    EXPECT_EQ(applyOffsetClamped(Dur(6000000), std::chrono::seconds(-5)),
              Dur(1000000));

    // Overflow clamps to max
    EXPECT_EQ(applyOffsetClamped(Dur(Dur::max().count() - 1000000),
                                 std::chrono::seconds(2)),
              Dur::max());

    // Underflow clamps to min
    EXPECT_EQ(applyOffsetClamped(Dur(1000000), std::chrono::seconds(-2)),
              Dur::min());
}

TEST(ApplyOffsetClamped, SignedSeconds)
{
    using Dur = std::chrono::duration<int64_t>;

    // Normal positive offset
    EXPECT_EQ(applyOffsetClamped(Dur(1000), std::chrono::seconds(500)),
              Dur(1500));

    // Normal negative offset
    EXPECT_EQ(applyOffsetClamped(Dur(1000), std::chrono::seconds(-500)),
              Dur(500));

    // Negative offset on large positive value (no overflow)
    EXPECT_EQ(applyOffsetClamped(Dur(1638312095), std::chrono::seconds(-18000)),
              Dur(1638294095));

    // Positive offset overflow clamps to max
    EXPECT_EQ(applyOffsetClamped(Dur(Dur::max().count() - 10),
                                 std::chrono::seconds(20)),
              Dur::max());

    // Negative offset underflow clamps to min
    EXPECT_EQ(applyOffsetClamped(Dur(Dur::min().count() + 10),
                                 std::chrono::seconds(-20)),
              Dur::min());

    // Negative value with negative offset
    EXPECT_EQ(applyOffsetClamped(Dur(-1000), std::chrono::seconds(-500)),
              Dur(-1500));
}

TEST(Utility, DateStringToEpoch)
{
    EXPECT_EQ(dateStringToEpoch("2021-11-30T22:41:35.123456+00:00"),
              usSinceEpoch{1638312095123456});
    // no timezone
    EXPECT_EQ(dateStringToEpoch("2021-11-30T22:41:35.123456"),
              usSinceEpoch{1638312095123456});
    // Milliseconds precision
    EXPECT_EQ(dateStringToEpoch("2021-11-30T22:41:35.123"),
              usSinceEpoch{1638312095123000});
    // Seconds precision
    EXPECT_EQ(dateStringToEpoch("2021-11-30T22:41:35"),
              usSinceEpoch{1638312095000000});

    // valid datetime format
    EXPECT_EQ(dateStringToEpoch("20230531T000000Z"),
              usSinceEpoch{1685491200000000});

    // valid datetime format
    EXPECT_EQ(dateStringToEpoch("20230531"), usSinceEpoch{1685491200000000});

    // Non zero timezone
    EXPECT_EQ(dateStringToEpoch("2021-11-30T22:41:35.123456+04:00"),
              usSinceEpoch{1638297695123456});

    // Epoch
    EXPECT_EQ(dateStringToEpoch("1970-01-01T00:00:00.000000+00:00"),
              usSinceEpoch{0});

    // Max time
    EXPECT_EQ(dateStringToEpoch("9999-12-31T23:59:59.999999+00:00"),
              usSinceEpoch{253402300799999999});

    // Underflow
    EXPECT_EQ(dateStringToEpoch("1969-12-30T23:59:59.999999+00:00"),
              std::nullopt);
}

TEST(Utility, DateStringToEpochWithInvalidDateTimeFormats)
{
    // invalid month (13)
    EXPECT_EQ(dateStringToEpoch("2024-13-01T12:00:00Z"), std::nullopt);

    // invalid character for month
    EXPECT_EQ(dateStringToEpoch("2024-X-01T12:00:00Z"), std::nullopt);

    // invalid day (32)
    EXPECT_EQ(dateStringToEpoch("2024-07-32T12:00:00Z"), std::nullopt);

    // invalid character for day
    EXPECT_EQ(dateStringToEpoch("2024-07-XT12:00:00Z"), std::nullopt);

    // invalid hour (25)
    EXPECT_EQ(dateStringToEpoch("2024-07-01T25:00:00Z"), std::nullopt);

    // invalid character for hour
    EXPECT_EQ(dateStringToEpoch("2024-07-01TX:00:00Z"), std::nullopt);

    // invalid minute (60)
    // Date.h and std::chrono seem to disagree about whether there is a 60th
    // minute in an hour.  Not clear if this is intended or not, but really
    // isn't that important.  Let std::chrono pass with 61
#if __cpp_lib_chrono >= 201907L
    EXPECT_EQ(dateStringToEpoch("2024-07-01T12:61:00Z"), std::nullopt);
#else
    EXPECT_EQ(dateStringToEpoch("2024-07-01T12:60:00Z"), std::nullopt);
#endif

    // invalid character for minute
    EXPECT_EQ(dateStringToEpoch("2024-13-01T12:X:00Z"), std::nullopt);

    // invalid second (60)
    EXPECT_EQ(dateStringToEpoch("2024-07-01T12:00:XZ"), std::nullopt);

    // invalid character for second
    EXPECT_EQ(dateStringToEpoch("2024-13-01T12:00:00Z"), std::nullopt);

    // invalid timezone
    EXPECT_EQ(dateStringToEpoch("2024-07-01T12:00:00X"), std::nullopt);

    // invalid datetime format
    EXPECT_EQ(dateStringToEpoch("202305"), std::nullopt);

    // invalid month (13), day (99)
    EXPECT_EQ(dateStringToEpoch("19991399"), std::nullopt);
}

TEST(Utility, GetDateTimeIso8601)
{
    // 2023-05-31T00:00:00 UTC = 1685491200 seconds since epoch
    constexpr uint64_t epoch20230531 = 1685491200;
    using DurationType = std::chrono::duration<uint64_t>;
    std::chrono::sys_time<DurationType> sysTime{DurationType(epoch20230531)};
    std::chrono::minutes offset =
        std::chrono::duration_cast<std::chrono::minutes>(
            std::chrono::current_zone()->get_info(sysTime).offset);
    std::string expected =
        details::toISO8061ExtendedStrSeconds(epoch20230531, offset);

    EXPECT_EQ(getDateTimeIso8601("20230531"), expected);

    EXPECT_EQ(getDateTimeIso8601("20230531T000000Z"), expected);

    // invalid datetime
    EXPECT_EQ(getDateTimeIso8601("202305"), std::nullopt);
}

} // namespace
} // namespace redfish::time_utils
