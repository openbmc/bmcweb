// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "ossl_random.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace
{

using testing::IsEmpty;
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

TEST(Bmcweb, GetRandomIdOfLength)
{
    using bmcweb::getRandomIdOfLength;
    EXPECT_THAT(getRandomIdOfLength(1), MatchesRegex("^[a-zA-Z0-9]$"));
    EXPECT_THAT(getRandomIdOfLength(10), MatchesRegex("^[a-zA-Z0-9]{10}$"));
    EXPECT_THAT(getRandomIdOfLength(0), IsEmpty());
}

} // namespace
