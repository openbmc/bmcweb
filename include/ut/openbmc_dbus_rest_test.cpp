#include "include/openbmc_dbus_rest.hpp"

#include "gtest/gtest.h"

// Also see redfish-core/ut/configfile_test.cpp
TEST(OpenbmcDbusRestTest, ValidFilenameGood)
{
    EXPECT_TRUE(crow::openbmc_mapper::validateFilename("GoodConfigFile"));
    EXPECT_TRUE(crow::openbmc_mapper::validateFilename("_Underlines_"));
    EXPECT_TRUE(crow::openbmc_mapper::validateFilename("8675309"));
    EXPECT_TRUE(crow::openbmc_mapper::validateFilename("-Dashes-"));
    EXPECT_TRUE(crow::openbmc_mapper::validateFilename("With Spaces"));
    EXPECT_TRUE(crow::openbmc_mapper::validateFilename("One.Dot"));
    EXPECT_TRUE(crow::openbmc_mapper::validateFilename("trailingdot."));
    EXPECT_TRUE(crow::openbmc_mapper::validateFilename("-_ o _-"));
    EXPECT_TRUE(crow::openbmc_mapper::validateFilename(" "));
    EXPECT_TRUE(crow::openbmc_mapper::validateFilename(" ."));
}

// There is no length test yet because validateFilename() does not care yet
TEST(OpenbmcDbusRestTest, ValidFilenameBad)
{
    EXPECT_FALSE(crow::openbmc_mapper::validateFilename(""));
    EXPECT_FALSE(crow::openbmc_mapper::validateFilename("Bad@file"));
    EXPECT_FALSE(
        crow::openbmc_mapper::validateFilename("/../../../../../etc/badpath"));
    EXPECT_FALSE(crow::openbmc_mapper::validateFilename("/../../etc/badpath"));
    EXPECT_FALSE(crow::openbmc_mapper::validateFilename("/mydir/configFile"));
    EXPECT_FALSE(crow::openbmc_mapper::validateFilename("/"));
    EXPECT_FALSE(crow::openbmc_mapper::validateFilename(".leadingdot"));
    EXPECT_FALSE(crow::openbmc_mapper::validateFilename("Two..Dots"));
    EXPECT_FALSE(
        crow::openbmc_mapper::validateFilename("../../../../../../etc/shadow"));
    EXPECT_FALSE(crow::openbmc_mapper::validateFilename("."));
}
