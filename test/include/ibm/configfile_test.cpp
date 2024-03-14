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
    EXPECT_TRUE(
        isValidConfigFileName("GoodConfigFile", asyncResp->res.jsonValue));
}
TEST(IsValidConfigFileName, FileNameInvalidCharReturnsFalse)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    EXPECT_FALSE(isValidConfigFileName("Bad@file", asyncResp->res.jsonValue));
}
TEST(IsValidConfigFileName, FileNameInvalidPathReturnsFalse)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    EXPECT_FALSE(isValidConfigFileName("/../../../../../etc/badpath",
                                       asyncResp->res.jsonValue));
    EXPECT_FALSE(
        isValidConfigFileName("/../../etc/badpath", asyncResp->res.jsonValue));
    EXPECT_FALSE(
        isValidConfigFileName("/mydir/configFile", asyncResp->res.jsonValue));
}

TEST(IsValidConfigFileName, EmptyFileNameReturnsFalse)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    EXPECT_FALSE(isValidConfigFileName("", asyncResp->res.jsonValue));
}

TEST(IsValidConfigFileName, SlashFileNameReturnsFalse)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    EXPECT_FALSE(isValidConfigFileName("/", asyncResp->res.jsonValue));
}
TEST(IsValidConfigFileName, FileNameMoreThan20CharReturnsFalse)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    EXPECT_FALSE(isValidConfigFileName("BadfileBadfileBadfile",
                                       asyncResp->res.jsonValue));
}

} // namespace ibm_mc
} // namespace crow
