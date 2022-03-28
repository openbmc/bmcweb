#include "utils/asset_utils.hpp"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using redfish::asset_utils::parseProperty;
using ::testing::IsFalse;
using ::testing::Optional;
using ::testing::Pair;

TEST(AssetUtils, OptionalProperty)
{
    EXPECT_THAT(parseProperty("PartNumber", ""),
                Pair(IsFalse(), Optional(nlohmann::json(""))));
    EXPECT_THAT(parseProperty("SparePartNumber", ""),
                Pair(IsFalse(), std::nullopt));
}

TEST(AssetUtils, TypeError)
{
    EXPECT_TRUE(parseProperty("SparePartNumber", 1).first);
}
