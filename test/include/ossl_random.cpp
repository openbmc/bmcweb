#include "ossl_random.hpp"

#include <gmock/gmock.h> // IWYU pragma: keep
#include <gtest/gtest.h> // IWYU pragma: keep

namespace
{

using testing::MatchesRegex;

TEST(Bmcweb, GetRandomUUID)
{
    using bmcweb::getRandomUUID;
    // 78e96a4b-62fe-48d8-ac09-7f75a94671e0
    EXPECT_THAT(
        getRandomUUID(),
        MatchesRegex(
            "^[a-f0-9]{8}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{12}$"));
}

} // namespace
