#include "account_service.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <optional>

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
    EXPECT_EQ(value, R"("2024-10-17T18:12:04+00:00")"_json);
}

} // namespace
} // namespace redfish
