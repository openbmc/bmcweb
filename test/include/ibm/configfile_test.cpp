#include "http_response.hpp"
#include "ibm/management_console_rest.hpp"

#include <string>

#include <gtest/gtest.h>

namespace bmcweb
{
namespace ibm_mc
{

TEST(IsValidConfigFileName, FileNameValidCharReturnsTrue)
{
    bmcweb::Response res;

    EXPECT_TRUE(isValidConfigFileName("GoodConfigFile", res));
}
TEST(IsValidConfigFileName, FileNameInvalidCharReturnsFalse)
{
    bmcweb::Response res;

    EXPECT_FALSE(isValidConfigFileName("Bad@file", res));
}
TEST(IsValidConfigFileName, FileNameInvalidPathReturnsFalse)
{
    bmcweb::Response res;

    EXPECT_FALSE(isValidConfigFileName("/../../../../../etc/badpath", res));
    EXPECT_FALSE(isValidConfigFileName("/../../etc/badpath", res));
    EXPECT_FALSE(isValidConfigFileName("/mydir/configFile", res));
}

TEST(IsValidConfigFileName, EmptyFileNameReturnsFalse)
{
    bmcweb::Response res;
    EXPECT_FALSE(isValidConfigFileName("", res));
}

TEST(IsValidConfigFileName, SlashFileNameReturnsFalse)
{
    bmcweb::Response res;
    EXPECT_FALSE(isValidConfigFileName("/", res));
}
TEST(IsValidConfigFileName, FileNameMoreThan20CharReturnsFalse)
{
    bmcweb::Response res;
    EXPECT_FALSE(isValidConfigFileName("BadfileBadfileBadfile", res));
}

} // namespace ibm_mc
} // namespace bmcweb
