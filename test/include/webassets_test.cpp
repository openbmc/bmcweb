// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "webassets.hpp"

#include <gtest/gtest.h>

namespace crow::webassets
{
namespace
{

TEST(GetStaticEtagTest, CorrectValues)
{
    // Webpack-style hash in filename.
    EXPECT_EQ(getStaticEtag("app.63e2c453.css"), "\"63e2c453\"");

    // Vite-style hash in filename.
    EXPECT_EQ(getStaticEtag("app.DhhjLIym.js"), "\"DhhjLIym\"");

    // Hash extraction still works when file has multiple dots.
    EXPECT_EQ(getStaticEtag("vendor.app.DhhjLIym.js"), "\"DhhjLIym\"");

    // Path prefixes do not affect hash extraction.
    EXPECT_EQ(getStaticEtag("/usr/share/www/app.63e2c453.css"), "\"63e2c453\"");
    EXPECT_EQ(getStaticEtag("./public/assets/app.DhhjLIym.js"), "\"DhhjLIym\"");

    // Too few segments to contain name/hash/extension.
    EXPECT_EQ(getStaticEtag("nested/dir/vendor.app.DhhjLIym.js"),
              "\"DhhjLIym\"");
    EXPECT_EQ(getStaticEtag("app.js"), "");
    EXPECT_EQ(getStaticEtag("/usr/share/www/app.js"), "");

    // Hash must be exactly 8 characters.
    EXPECT_EQ(getStaticEtag("app.63e2c45.css"), "");
    EXPECT_EQ(getStaticEtag("app.63e2c4537.css"), "");

    // Empty string and no-dot filenames yield no hash.
    EXPECT_EQ(getStaticEtag(""), "");
    EXPECT_EQ(getStaticEtag("appjs"), "");

    // Mixed-case hex hash still matches under alphanumeric rule.
    EXPECT_EQ(getStaticEtag("app.63E2C453.css"), "\"63E2C453\"");

    // Hash must be alphanumeric only.
    EXPECT_EQ(getStaticEtag("app.63e2c45-.css"), "");
    EXPECT_EQ(getStaticEtag("app.63e2c45_.css"), "");
}

} // namespace
} // namespace crow::webassets
