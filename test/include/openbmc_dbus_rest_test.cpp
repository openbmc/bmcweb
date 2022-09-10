#include "openbmc_dbus_rest.hpp"

#include <gmock/gmock.h> // IWYU pragma: keep
#include <gtest/gtest.h> // IWYU pragma: keep

// IWYU pragma: no_include <gtest/gtest-message.h>
// IWYU pragma: no_include <gtest/gtest-test-part.h>
// IWYU pragma: no_include "gtest/gtest_pred_impl.h"
// IWYU pragma: no_include <gmock/gmock-matchers.h>

namespace crow::openbmc_mapper
{
namespace
{

using ::testing::ElementsAre;
// Also see redfish-core/ut/configfile_test.cpp
TEST(OpenbmcDbusRestTest, ValidFilenameGood)
{
    EXPECT_TRUE(validateFilename("GoodConfigFile"));
    EXPECT_TRUE(validateFilename("_Underlines_"));
    EXPECT_TRUE(validateFilename("8675309"));
    EXPECT_TRUE(validateFilename("-Dashes-"));
    EXPECT_TRUE(validateFilename("With Spaces"));
    EXPECT_TRUE(validateFilename("One.Dot"));
    EXPECT_TRUE(validateFilename("trailingdot."));
    EXPECT_TRUE(validateFilename("-_ o _-"));
    EXPECT_TRUE(validateFilename(" "));
    EXPECT_TRUE(validateFilename(" ."));
}

// There is no length test yet because validateFilename() does not care yet
TEST(OpenbmcDbusRestTest, ValidFilenameBad)
{
    EXPECT_FALSE(validateFilename(""));
    EXPECT_FALSE(validateFilename("Bad@file"));
    EXPECT_FALSE(validateFilename("/../../../../../etc/badpath"));
    EXPECT_FALSE(validateFilename("/../../etc/badpath"));
    EXPECT_FALSE(validateFilename("/mydir/configFile"));
    EXPECT_FALSE(validateFilename("/"));
    EXPECT_FALSE(validateFilename(".leadingdot"));
    EXPECT_FALSE(validateFilename("Two..Dots"));
    EXPECT_FALSE(validateFilename("../../../../../../etc/shadow"));
    EXPECT_FALSE(validateFilename("."));
}

TEST(OpenBmcDbusTest, TestArgSplit)
{
    // test the basic types
    EXPECT_THAT(dbusArgSplit("x"), ElementsAre("x"));
    EXPECT_THAT(dbusArgSplit("y"), ElementsAre("y"));
    EXPECT_THAT(dbusArgSplit("b"), ElementsAre("b"));
    EXPECT_THAT(dbusArgSplit("n"), ElementsAre("n"));
    EXPECT_THAT(dbusArgSplit("q"), ElementsAre("q"));
    EXPECT_THAT(dbusArgSplit("i"), ElementsAre("i"));
    EXPECT_THAT(dbusArgSplit("u"), ElementsAre("u"));
    EXPECT_THAT(dbusArgSplit("x"), ElementsAre("x"));
    EXPECT_THAT(dbusArgSplit("t"), ElementsAre("t"));
    EXPECT_THAT(dbusArgSplit("d"), ElementsAre("d"));
    EXPECT_THAT(dbusArgSplit("h"), ElementsAre("h"));
    // test arrays
    EXPECT_THAT(dbusArgSplit("ai"), ElementsAre("ai"));
    EXPECT_THAT(dbusArgSplit("ax"), ElementsAre("ax"));
    // test tuples
    EXPECT_THAT(dbusArgSplit("(sss)"), ElementsAre("(sss)"));
    EXPECT_THAT(dbusArgSplit("(sss)b"), ElementsAre("(sss)", "b"));
    EXPECT_THAT(dbusArgSplit("b(sss)"), ElementsAre("b", "(sss)"));

    // Test nested types
    EXPECT_THAT(dbusArgSplit("a{si}b"), ElementsAre("a{si}", "b"));
    EXPECT_THAT(dbusArgSplit("a(sss)b"), ElementsAre("a(sss)", "b"));
    EXPECT_THAT(dbusArgSplit("aa{si}b"), ElementsAre("aa{si}", "b"));
    EXPECT_THAT(dbusArgSplit("i{si}b"), ElementsAre("i", "{si}", "b"));
}
} // namespace
} // namespace crow::openbmc_mapper
