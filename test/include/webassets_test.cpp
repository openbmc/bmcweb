// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "webassets.hpp"

#include <string_view>

#include <gtest/gtest.h>

namespace crow::webassets
{
namespace
{

struct StaticEtagTestCase
{
    // filename is string_view; it converts implicitly to
    // std::filesystem::path when passed to getStaticEtag().
    std::string_view filename;
    std::string_view expectedEtag;
};

class GetStaticEtagTest : public ::testing::TestWithParam<StaticEtagTestCase>
{};

TEST_P(GetStaticEtagTest, FilenameCases)
{
    const StaticEtagTestCase& testCase = GetParam();
    EXPECT_EQ(getStaticEtag(testCase.filename), testCase.expectedEtag);
}

INSTANTIATE_TEST_SUITE_P(
    BuildArtifactFilenames, GetStaticEtagTest,
    ::testing::Values(
        // Webpack-style hash in filename.
        StaticEtagTestCase{"app.63e2c453.css", "\"63e2c453\""},
        // Vite-style hash in filename.
        StaticEtagTestCase{"app.DhhjLIym.js", "\"DhhjLIym\""},
        // Hash extraction still works when file has multiple dots.
        StaticEtagTestCase{"vendor.app.DhhjLIym.js", "\"DhhjLIym\""},
        // Path prefixes do not affect hash extraction.
        StaticEtagTestCase{"/usr/share/www/app.63e2c453.css", "\"63e2c453\""},
        StaticEtagTestCase{"./public/assets/app.DhhjLIym.js", "\"DhhjLIym\""},
        StaticEtagTestCase{"nested/dir/vendor.app.DhhjLIym.js",
                           "\"DhhjLIym\""},
        // Too few segments to contain name/hash/extension.
        StaticEtagTestCase{"app.js", ""},
        StaticEtagTestCase{"/usr/share/www/app.js", ""},
        // Hash must be exactly 8 characters.
        StaticEtagTestCase{"app.63e2c45.css", ""},
        StaticEtagTestCase{"app.63e2c4537.css", ""},
        // Empty string and no-dot filenames yield no hash.
        StaticEtagTestCase{"", ""},
        StaticEtagTestCase{"appjs", ""},
        // Mixed-case hex hash still matches under alphanumeric rule.
        StaticEtagTestCase{"app.63E2C453.css", "\"63E2C453\""},
        // 8-char alphabetic segment matches by design (accepted false positive).
        StaticEtagTestCase{"vendor.settings.js", "\"settings\""},
        // Hash must be alphanumeric only.
        StaticEtagTestCase{"app.63e2c45-.css", ""},
        StaticEtagTestCase{"app.63e2c45_.css", ""}));

} // namespace
} // namespace crow::webassets
