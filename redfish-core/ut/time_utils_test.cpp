#include "utils/time_utils.hpp"

#include <gmock/gmock.h>

using namespace testing;

class FromDurationTest :
    public Test,
    public WithParamInterface<
        std::pair<std::string, std::optional<std::chrono::milliseconds>>>
{};

INSTANTIATE_TEST_SUITE_P(
    _, FromDurationTest,
    Values(std::make_pair("PT12S", std::chrono::milliseconds(12000)),
           std::make_pair("PT0.204S", std::chrono::milliseconds(204)),
           std::make_pair("PT0.2S", std::chrono::milliseconds(200)),
           std::make_pair("PT50M", std::chrono::milliseconds(3000000)),
           std::make_pair("PT23H", std::chrono::milliseconds(82800000)),
           std::make_pair("P51D", std::chrono::milliseconds(4406400000)),
           std::make_pair("PT2H40M10.1S", std::chrono::milliseconds(9610100)),
           std::make_pair("P20DT2H40M10.1S",
                          std::chrono::milliseconds(1737610100)),
           std::make_pair("", std::chrono::milliseconds(0)),
           std::make_pair("PTS", std::nullopt),
           std::make_pair("P1T", std::nullopt),
           std::make_pair("PT100M1000S100", std::nullopt),
           std::make_pair("PDTHMS", std::nullopt),
           std::make_pair("P99999999999999999DT", std::nullopt),
           std::make_pair("PD222T222H222M222.222S", std::nullopt),
           std::make_pair("PT99999H9999999999999999999999M99999999999S",
                          std::nullopt),
           std::make_pair("PT-9H", std::nullopt)));

TEST_P(FromDurationTest, convertToMilliseconds)
{
    const auto& [str, expected] = GetParam();
    EXPECT_THAT(redfish::time_utils::fromDurationString(str), Eq(expected));
}

class ToDurationTest :
    public Test,
    public WithParamInterface<std::pair<std::chrono::milliseconds, std::string>>
{};

INSTANTIATE_TEST_SUITE_P(
    _, ToDurationTest,
    Values(std::make_pair(std::chrono::milliseconds(12000), "PT12.000S"),
           std::make_pair(std::chrono::milliseconds(204), "PT0.204S"),
           std::make_pair(std::chrono::milliseconds(200), "PT0.200S"),
           std::make_pair(std::chrono::milliseconds(3000000), "PT50M"),
           std::make_pair(std::chrono::milliseconds(82800000), "PT23H"),
           std::make_pair(std::chrono::milliseconds(4406400000), "P51DT"),
           std::make_pair(std::chrono::milliseconds(9610100), "PT2H40M10.100S"),
           std::make_pair(std::chrono::milliseconds(1737610100),
                          "P20DT2H40M10.100S"),
           std::make_pair(std::chrono::milliseconds(-250), "")));

TEST_P(ToDurationTest, convertToDuration)
{
    const auto& [ms, expected] = GetParam();
    EXPECT_THAT(redfish::time_utils::toDurationString(ms), Eq(expected));
}
