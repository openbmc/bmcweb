#include "account_service.hpp"

#include <chrono>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace redfish
{
namespace
{

using namespace std::chrono;
using json = nlohmann::json;

TEST(Conversion, PositiveToUint64)
{
    EXPECT_EQ(*passwordExpirationToUint64(nullptr),
              unexpiringPasswordExpiration);

    EXPECT_EQ(*passwordExpirationToUint64("2024-10-17T00:00:00"), 1729123200);

    EXPECT_EQ(*passwordExpirationToUint64("2024-10-17T18:12:04"), 1729188724);

    EXPECT_EQ(*passwordExpirationToUint64("2024-10-17T18:12:04Z"), 1729188724);

    EXPECT_EQ(*passwordExpirationToUint64("2024-10-17T18:12:04+03:00"),
              1729177924);
}

TEST(Conversion, NegativeToUint64)
{
    EXPECT_EQ(passwordExpirationToUint64({}), std::nullopt);

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
    EXPECT_EQ(value, R"("2024-10-17T18:12:04Z")"_json);
}

} // namespace
} // namespace redfish
