#include "account_service.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <optional>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::AnyOf;
using ::testing::Eq;
using ::testing::MatchesRegex;
using ::testing::Optional;
using ::testing::StartsWith;

namespace redfish
{
namespace
{

using json = nlohmann::json;

TEST(Conversion, PositiveToUint64)
{
    std::optional<uint64_t> passwordExpiration;

    passwordExpiration = passwordExpirationToUint64(nullptr);
    EXPECT_THAT(passwordExpiration, Optional(Eq(0)));

    passwordExpiration = passwordExpirationToUint64("1970-01-01T00:00:00");
    EXPECT_THAT(passwordExpiration, Eq(std::nullopt));

    passwordExpiration = passwordExpirationToUint64("2024-10-17T18:12:04");
    EXPECT_THAT(passwordExpiration, Optional(Eq(1729188724)));

    passwordExpiration = passwordExpirationToUint64("2024-10-17T18:12:04Z");
    EXPECT_THAT(passwordExpiration, Optional(Eq(1729188724)));

    passwordExpiration =
        passwordExpirationToUint64("2024-10-17T18:12:04+03:00");
    EXPECT_THAT(passwordExpiration, Optional(Eq(1729177924)));
}

TEST(Conversion, NegativeToUint64)
{
    EXPECT_EQ(passwordExpirationToUint64("01-02-03"), std::nullopt);

    EXPECT_EQ(passwordExpirationToUint64("1912-02-03"), std::nullopt);

    EXPECT_EQ(passwordExpirationToUint64("2024-10-17T00:00"), std::nullopt);

    EXPECT_EQ(passwordExpirationToUint64("ABC"), std::nullopt);
}

TEST(Conversion, PositiveToJson)
{
    constexpr uint64_t unexpiringPasswordExpiration = 0;

    json value = passwordExpirationToJson(unexpiringPasswordExpiration);
    EXPECT_TRUE(value.is_null());

    value = passwordExpirationToJson(1729188724);
    EXPECT_TRUE(value.is_string());

    // passwordExpirationToJson returns local time, so the exact wall-clock
    // string depends on the machine timezone. The local date must be within
    // a day of the UTC date (max real-world offset is UTC+/-14:00), the
    // string must have Redfish/ISO 8601 date-time shape, and it must
    // round-trip back to the original epoch.
    EXPECT_THAT(value.get<std::string>(),
                AnyOf(StartsWith("2024-10-16"), StartsWith("2024-10-17"),
                      StartsWith("2024-10-18")));
    EXPECT_THAT(
        value.get<std::string>(),
        MatchesRegex(
            "[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}[-+][0-9]{2}:[0-9]{2}"));
    EXPECT_THAT(
        passwordExpirationToUint64(value.get<std::string>()),
        Optional(Eq(1729188724)));
}

} // namespace
} // namespace redfish
