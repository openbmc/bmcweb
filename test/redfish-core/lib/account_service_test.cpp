#include "account_service.hpp"
#include "utils/time_utils.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <optional>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::Eq;
using ::testing::Optional;

namespace redfish
{
namespace
{

using json = nlohmann::json;

TEST(Conversion, PositiveToUint64)
{
    std::optional<uint64_t> passwordExpiration;

    passwordExpiration = expirationToUint64(nullptr);
    EXPECT_THAT(passwordExpiration, Optional(Eq(0)));

    passwordExpiration = expirationToUint64("1970-01-01T00:00:00");
    EXPECT_THAT(passwordExpiration, Eq(std::nullopt));

    passwordExpiration = expirationToUint64("2024-10-17T18:12:04");
    EXPECT_THAT(passwordExpiration, Optional(Eq(1729188724)));

    passwordExpiration = expirationToUint64("2024-10-17T18:12:04Z");
    EXPECT_THAT(passwordExpiration, Optional(Eq(1729188724)));

    passwordExpiration = expirationToUint64("2024-10-17T18:12:04+03:00");
    EXPECT_THAT(passwordExpiration, Optional(Eq(1729177924)));
}

TEST(Conversion, NegativeToUint64)
{
    EXPECT_EQ(expirationToUint64("01-02-03"), std::nullopt);

    EXPECT_EQ(expirationToUint64("1912-02-03"), std::nullopt);

    EXPECT_EQ(expirationToUint64("2024-10-17T00:00"), std::nullopt);

    EXPECT_EQ(expirationToUint64("ABC"), std::nullopt);
}

TEST(Conversion, PositiveToJson)
{
    constexpr uint64_t unexpiringExpiration = 0;

    json value = expirationToJson(unexpiringExpiration);
    EXPECT_TRUE(value.is_null());

    value = expirationToJson(1729188724, "UTC");
    EXPECT_TRUE(value.is_string());
    EXPECT_EQ(value, R"("2024-10-17T18:12:04+00:00")"_json);

    value = expirationToJson(1729188724, "America/New_York");
    EXPECT_TRUE(value.is_string());
    EXPECT_EQ(value, R"("2024-10-17T14:12:04-04:00")"_json);

    // Default (empty timezone) uses the local zone, matching the previous
    // behavior of getDateTimeUint(value).
    value = expirationToJson(1729188724);
    EXPECT_TRUE(value.is_string());
    EXPECT_EQ(value.get<std::string>(),
              time_utils::getDateTimeUint(1729188724));
}

} // namespace
} // namespace redfish
