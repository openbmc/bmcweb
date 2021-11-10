#include "utils/time_utils.hpp"

#include <limits>

#include <gmock/gmock.h>

TEST(FromDurationTest, PositiveTests)
{
    using redfish::time_utils::fromDurationString;
    using std::chrono::milliseconds;
    EXPECT_EQ(fromDurationString("PT12S"), milliseconds(12000));
    EXPECT_EQ(fromDurationString("PT0.204S"), milliseconds(204));
    EXPECT_EQ(fromDurationString("PT0.2S"), milliseconds(200));
    EXPECT_EQ(fromDurationString("PT50M"), milliseconds(3000000));
    EXPECT_EQ(fromDurationString("PT23H"), milliseconds(82800000));
    EXPECT_EQ(fromDurationString("P51D"), milliseconds(4406400000));
    EXPECT_EQ(fromDurationString("PT2H40M10.1S"), milliseconds(9610100));
    EXPECT_EQ(fromDurationString("P20DT2H40M10.1S"), milliseconds(1737610100));
    EXPECT_EQ(fromDurationString(""), milliseconds(0));
}

TEST(FromDurationTest, NegativeTests)
{
    using redfish::time_utils::fromDurationString;
    EXPECT_EQ(fromDurationString("PTS"), std::nullopt);
    EXPECT_EQ(fromDurationString("P1T"), std::nullopt);
    EXPECT_EQ(fromDurationString("PT100M1000S100"), std::nullopt);
    EXPECT_EQ(fromDurationString("PDTHMS"), std::nullopt);
    EXPECT_EQ(fromDurationString("P99999999999999999DT"), std::nullopt);
    EXPECT_EQ(fromDurationString("PD222T222H222M222.222S"), std::nullopt);
    EXPECT_EQ(fromDurationString("PT99999H9999999999999999999999M99999999999S"),
              std::nullopt);
    EXPECT_EQ(fromDurationString("PT-9H"), std::nullopt);
}

TEST(ToDurationTest, PositiveTests)
{
    using redfish::time_utils::toDurationString;
    using std::chrono::milliseconds;
    EXPECT_EQ(toDurationString(milliseconds(12000)), "PT12.000S");
    EXPECT_EQ(toDurationString(milliseconds(204)), "PT0.204S");
    EXPECT_EQ(toDurationString(milliseconds(200)), "PT0.200S");
    EXPECT_EQ(toDurationString(milliseconds(3000000)), "PT50M");
    EXPECT_EQ(toDurationString(milliseconds(82800000)), "PT23H");
    EXPECT_EQ(toDurationString(milliseconds(4406400000)), "P51DT");
    EXPECT_EQ(toDurationString(milliseconds(9610100)), "PT2H40M10.100S");
    EXPECT_EQ(toDurationString(milliseconds(1737610100)), "P20DT2H40M10.100S");
}

TEST(ToDurationTest, NegativeTests)
{
    using redfish::time_utils::toDurationString;
    using std::chrono::milliseconds;
    EXPECT_EQ(toDurationString(milliseconds(-250)), "");
}

TEST(SafeDurationCountTest, PositiveTests)
{
    using redfish::time_utils::safeDurationCount;
    using std::chrono::milliseconds;
    using Duration32u = std::chrono::duration<uint32_t, std::giga>;
    using Duration16 = std::chrono::duration<int16_t, std::kilo>;

    EXPECT_EQ(safeDurationCount<uint64_t>(milliseconds(0)), 0ull);
    EXPECT_EQ(safeDurationCount<uint64_t>(milliseconds(10)), 10ull);
    EXPECT_EQ(safeDurationCount<uint64_t>(
                  milliseconds(std::numeric_limits<int64_t>::max())),
              std::numeric_limits<int64_t>::max());

    EXPECT_EQ(safeDurationCount<uint32_t>(Duration32u(0)), 0ull);
    EXPECT_EQ(safeDurationCount<uint32_t>(Duration32u(10)), 10ull);
    EXPECT_EQ(safeDurationCount<uint32_t>(
                  Duration32u(std::numeric_limits<int32_t>::max())),
              std::numeric_limits<int32_t>::max());
    EXPECT_EQ(safeDurationCount<uint32_t>(
                  Duration32u(std::numeric_limits<uint32_t>::max())),
              std::numeric_limits<uint32_t>::max());

    EXPECT_EQ(safeDurationCount<int32_t>(milliseconds(-123)), -123);
    EXPECT_EQ(safeDurationCount<int32_t>(milliseconds(10)), 10ull);
    EXPECT_EQ(safeDurationCount<int32_t>(Duration16(-123)), -123);
    EXPECT_EQ(safeDurationCount<int32_t>(Duration16(10)), 10ull);
    EXPECT_EQ(safeDurationCount<int32_t>(
                  milliseconds(std::numeric_limits<int32_t>::min())),
              std::numeric_limits<int32_t>::min());
    EXPECT_EQ(safeDurationCount<int32_t>(
                  milliseconds(std::numeric_limits<int32_t>::max())),
              std::numeric_limits<int32_t>::max());
}

TEST(SafeDurationCountTest, NegativeTests)
{
    using redfish::time_utils::safeDurationCount;
    using std::chrono::milliseconds;
    using Duration64u = std::chrono::duration<uint64_t, std::milli>;

    EXPECT_EQ(safeDurationCount<uint64_t>(milliseconds(-1)), std::nullopt);
    EXPECT_EQ(safeDurationCount<int64_t>(
                  Duration64u(std::numeric_limits<uint64_t>::max())),
              std::nullopt);

    EXPECT_EQ(safeDurationCount<uint32_t>(milliseconds(-10000)), std::nullopt);
    EXPECT_EQ(safeDurationCount<uint32_t>(
                  Duration64u(std::numeric_limits<uint64_t>::max())),
              std::nullopt);

    EXPECT_EQ(safeDurationCount<int32_t>(
                  milliseconds(std::numeric_limits<int64_t>::max())),
              std::nullopt);
    EXPECT_EQ(safeDurationCount<int32_t>(
                  milliseconds(std::numeric_limits<int64_t>::min())),
              std::nullopt);
}
