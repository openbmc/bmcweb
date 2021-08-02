#include "utils/time_utils.hpp"

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
    EXPECT_EQ(fromDurationString("P9999999999999999999999999DT"), std::nullopt);
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

TEST(ToDurationStringFromUintTest, PositiveTests)
{
    using redfish::time_utils::toDurationStringFromUint;
    uint64_t maxAcceptedTimeMs =
        static_cast<uint64_t>(std::chrono::milliseconds::max().count());

    EXPECT_THAT(toDurationStringFromUint(maxAcceptedTimeMs),
                ::testing::Ne(std::nullopt));
    EXPECT_EQ(toDurationStringFromUint(0), "PT");
    EXPECT_EQ(toDurationStringFromUint(250), "PT0.250S");
    EXPECT_EQ(toDurationStringFromUint(5000), "PT5.000S");
}

TEST(ToDurationStringFromUintTest, NegativeTests)
{
    using redfish::time_utils::toDurationStringFromUint;
    uint64_t minNotAcceptedTimeMs =
        static_cast<uint64_t>(std::chrono::milliseconds::max().count()) + 1;

    EXPECT_EQ(toDurationStringFromUint(minNotAcceptedTimeMs), std::nullopt);
    EXPECT_EQ(toDurationStringFromUint(static_cast<uint64_t>(-1)),
              std::nullopt);
}
