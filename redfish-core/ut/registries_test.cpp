#include "registries.hpp"

#include <gtest/gtest.h> // IWYU pragma: keep

// IWYU pragma: no_include <gtest/gtest-message.h>
// IWYU pragma: no_include <gtest/gtest-test-part.h>
// IWYU pragma: no_include "gtest/gtest_pred_impl.h"

namespace redfish::registries
{
namespace
{

TEST(FillMessageArgs, ArgsAreFilledCorrectly)
{
    std::string toFill("%1");
    fillMessageArgs({{"foo"}}, toFill);
    EXPECT_EQ(toFill, "foo");

    toFill = "";
    fillMessageArgs({}, toFill);
    EXPECT_EQ(toFill, "");

    toFill = "%1, %2";
    fillMessageArgs({{"foo", "bar"}}, toFill);
    EXPECT_EQ(toFill, "foo, bar");
}
} // namespace
} // namespace redfish::registries