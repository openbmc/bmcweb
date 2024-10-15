#include "account_service.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <optional>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

using json = nlohmann::json;

constexpr uint64_t unexpiringPasswordExpiration = 0;

TEST(Conversion, PositiveToUint64)
{
    auto passwordExpiration = passwordExpirationToUint64(nullptr);
    EXPECT_TRUE(passwordExpiration.has_value());
    EXPECT_EQ(passwordExpiration.value_or(0), unexpiringPasswordExpiration);

    passwordExpiration = passwordExpirationToUint64("1970-01-01T00:00:00");
    EXPECT_TRUE(passwordExpiration.has_value());
    EXPECT_EQ(passwordExpiration.value_or(1), 0);

    passwordExpiration = passwordExpirationToUint64("2024-10-17T18:12:04");
    EXPECT_TRUE(passwordExpiration.has_value());
    EXPECT_EQ(passwordExpiration.value_or(0), 1729188724);

    passwordExpiration = passwordExpirationToUint64("2024-10-17T18:12:04Z");
    EXPECT_TRUE(passwordExpiration.has_value());
    EXPECT_EQ(passwordExpiration.value_or(0), 1729188724);

    passwordExpiration =
        passwordExpirationToUint64("2024-10-17T18:12:04+03:00");
    EXPECT_TRUE(passwordExpiration.has_value());
    EXPECT_EQ(passwordExpiration.value_or(0), 1729177924);
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
    json value = passwordExpirationToJson(unexpiringPasswordExpiration);
    EXPECT_TRUE(value.is_null());

    value = passwordExpirationToJson(1729188724);
    EXPECT_TRUE(value.is_string());
    EXPECT_EQ(value, R"("2024-10-17T18:12:04+00:00")"_json);
}

} // namespace
} // namespace redfish
