#include "http_response.hpp"
#include "ibm/management_console_rest.hpp"

#include <string>

#include <gtest/gtest.h> // IWYU pragma: keep

// IWYU pragma: no_include <gtest/gtest-message.h>
// IWYU pragma: no_include <gtest/gtest-test-part.h>
// IWYU pragma: no_include "gtest/gtest_pred_impl.h"

namespace crow
{
namespace ibm_mc
{

TEST(IsValidConfigFileName, FileNameValidCharReturnsTrue)
{
    crow::Response res;

    EXPECT_TRUE(isValidConfigFileName("GoodConfigFile", res));
}
TEST(IsValidConfigFileName, FileNameInvalidCharReturnsFalse)
{
    crow::Response res;

    EXPECT_FALSE(isValidConfigFileName("Bad@file", res));
}
TEST(IsValidConfigFileName, FileNameInvalidPathReturnsFalse)
{
    crow::Response res;

    EXPECT_FALSE(isValidConfigFileName("/../../../../../etc/badpath", res));
    EXPECT_FALSE(isValidConfigFileName("/../../etc/badpath", res));
    EXPECT_FALSE(isValidConfigFileName("/mydir/configFile", res));
}

TEST(IsValidConfigFileName, EmptyFileNameReturnsFalse)
{
    crow::Response res;
    EXPECT_FALSE(isValidConfigFileName("", res));
}

TEST(IsValidConfigFileName, SlashFileNameReturnsFalse)
{
    crow::Response res;
    EXPECT_FALSE(isValidConfigFileName("/", res));
}
TEST(IsValidConfigFileName, FileNameMoreThan20CharReturnsFalse)
{
    crow::Response res;
    EXPECT_FALSE(isValidConfigFileName("BadfileBadfileBadfile", res));
}

} // namespace ibm_mc
} // namespace crow
