#include "openbmc_dbus_rest.hpp"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

TEST(OpenBmcDbusTest, TestArgSplit)
{
    // test the basic types
    EXPECT_THAT(crow::openbmc_mapper::dbusArgSplit("x"),
                ::testing::ElementsAre("x"));
    EXPECT_THAT(crow::openbmc_mapper::dbusArgSplit("y"),
                ::testing::ElementsAre("y"));
    EXPECT_THAT(crow::openbmc_mapper::dbusArgSplit("b"),
                ::testing::ElementsAre("b"));
    EXPECT_THAT(crow::openbmc_mapper::dbusArgSplit("n"),
                ::testing::ElementsAre("n"));
    EXPECT_THAT(crow::openbmc_mapper::dbusArgSplit("q"),
                ::testing::ElementsAre("q"));
    EXPECT_THAT(crow::openbmc_mapper::dbusArgSplit("i"),
                ::testing::ElementsAre("i"));
    EXPECT_THAT(crow::openbmc_mapper::dbusArgSplit("u"),
                ::testing::ElementsAre("u"));
    EXPECT_THAT(crow::openbmc_mapper::dbusArgSplit("x"),
                ::testing::ElementsAre("x"));
    EXPECT_THAT(crow::openbmc_mapper::dbusArgSplit("t"),
                ::testing::ElementsAre("t"));
    EXPECT_THAT(crow::openbmc_mapper::dbusArgSplit("d"),
                ::testing::ElementsAre("d"));
    EXPECT_THAT(crow::openbmc_mapper::dbusArgSplit("h"),
                ::testing::ElementsAre("h"));
    // test arrays
    EXPECT_THAT(crow::openbmc_mapper::dbusArgSplit("ai"),
                ::testing::ElementsAre("ai"));
    EXPECT_THAT(crow::openbmc_mapper::dbusArgSplit("ax"),
                ::testing::ElementsAre("ax"));
    // test tuples
    EXPECT_THAT(crow::openbmc_mapper::dbusArgSplit("(sss)"),
                ::testing::ElementsAre("(sss)"));
    EXPECT_THAT(crow::openbmc_mapper::dbusArgSplit("(sss)b"),
                ::testing::ElementsAre("(sss)", "b"));
    EXPECT_THAT(crow::openbmc_mapper::dbusArgSplit("b(sss)"),
                ::testing::ElementsAre("b", "(sss)"));

    // Test nested types
    EXPECT_THAT(crow::openbmc_mapper::dbusArgSplit("a{si}b"),
                ::testing::ElementsAre("a{si}", "b"));
    EXPECT_THAT(crow::openbmc_mapper::dbusArgSplit("a(sss)b"),
                ::testing::ElementsAre("a(sss)", "b"));
    EXPECT_THAT(crow::openbmc_mapper::dbusArgSplit("aa{si}b"),
                ::testing::ElementsAre("aa{si}", "b"));
    EXPECT_THAT(crow::openbmc_mapper::dbusArgSplit("i{si}b"),
                ::testing::ElementsAre("i", "{si}", "b"));
}
