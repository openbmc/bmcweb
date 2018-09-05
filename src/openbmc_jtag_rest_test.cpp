#include "openbmc_dbus_rest.hpp"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

TEST(OpenBmcDbusTest, TestArgSplit)
{
    // test the basic types
    EXPECT_THAT(crow::openbmc_mapper::dbus_arg_split("x"),
                ::testing::ElementsAre("x"));
    EXPECT_THAT(crow::openbmc_mapper::dbus_arg_split("y"),
                ::testing::ElementsAre("y"));
    EXPECT_THAT(crow::openbmc_mapper::dbus_arg_split("b"),
                ::testing::ElementsAre("b"));
    EXPECT_THAT(crow::openbmc_mapper::dbus_arg_split("n"),
                ::testing::ElementsAre("n"));
    EXPECT_THAT(crow::openbmc_mapper::dbus_arg_split("q"),
                ::testing::ElementsAre("q"));
    EXPECT_THAT(crow::openbmc_mapper::dbus_arg_split("i"),
                ::testing::ElementsAre("i"));
    EXPECT_THAT(crow::openbmc_mapper::dbus_arg_split("u"),
                ::testing::ElementsAre("u"));
    EXPECT_THAT(crow::openbmc_mapper::dbus_arg_split("x"),
                ::testing::ElementsAre("x"));
    EXPECT_THAT(crow::openbmc_mapper::dbus_arg_split("t"),
                ::testing::ElementsAre("t"));
    EXPECT_THAT(crow::openbmc_mapper::dbus_arg_split("d"),
                ::testing::ElementsAre("d"));
    EXPECT_THAT(crow::openbmc_mapper::dbus_arg_split("h"),
                ::testing::ElementsAre("h"));
    // test arrays
    EXPECT_THAT(crow::openbmc_mapper::dbus_arg_split("ai"),
                ::testing::ElementsAre("ai"));
    EXPECT_THAT(crow::openbmc_mapper::dbus_arg_split("ax"),
                ::testing::ElementsAre("ax"));
    // test tuples
    EXPECT_THAT(crow::openbmc_mapper::dbus_arg_split("(sss)"),
                ::testing::ElementsAre("(sss)"));
    EXPECT_THAT(crow::openbmc_mapper::dbus_arg_split("(sss)b"),
                ::testing::ElementsAre("(sss)", "b"));
    EXPECT_THAT(crow::openbmc_mapper::dbus_arg_split("b(sss)"),
                ::testing::ElementsAre("b", "(sss)"));

    // Test nested types
    EXPECT_THAT(crow::openbmc_mapper::dbus_arg_split("a{si}b"),
                ::testing::ElementsAre("a{si}", "b"));
    EXPECT_THAT(crow::openbmc_mapper::dbus_arg_split("a(sss)b"),
                ::testing::ElementsAre("a(sss)", "b"));
    EXPECT_THAT(crow::openbmc_mapper::dbus_arg_split("aa{si}b"),
                ::testing::ElementsAre("aa{si}", "b"));
    EXPECT_THAT(crow::openbmc_mapper::dbus_arg_split("i{si}b"),
                ::testing::ElementsAre("b", "aa{si}"));
}
