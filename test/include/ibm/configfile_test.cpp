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
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    EXPECT_TRUE(isValidConfigFileName("GoodConfigFile", asyncResp));
}
TEST(IsValidConfigFileName, FileNameInvalidCharReturnsFalse)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    EXPECT_FALSE(isValidConfigFileName("Bad@file", asyncResp));
}
TEST(IsValidConfigFileName, FileNameInvalidPathReturnsFalse)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    EXPECT_FALSE(
        isValidConfigFileName("/../../../../../etc/badpath", asyncResp));
    EXPECT_FALSE(isValidConfigFileName("/../../etc/badpath", asyncResp));
    EXPECT_FALSE(isValidConfigFileName("/mydir/configFile", asyncResp));
}

TEST(IsValidConfigFileName, EmptyFileNameReturnsFalse)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    EXPECT_FALSE(isValidConfigFileName("", asyncResp));
}

TEST(IsValidConfigFileName, SlashFileNameReturnsFalse)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    EXPECT_FALSE(isValidConfigFileName("/", asyncResp));
}
TEST(IsValidConfigFileName, FileNameMoreThan20CharReturnsFalse)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    EXPECT_FALSE(isValidConfigFileName("BadfileBadfileBadfile", asyncResp));
}

} // namespace ibm_mc
} // namespace crow
