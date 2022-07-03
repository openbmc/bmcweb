#include "registries.hpp"

#include <gtest/gtest-message.h>
#include <gtest/gtest-test-part.h>
#include <gtest/gtest.h> // IWYU pragma: keep

// IWYU pragma: no_include "gtest/gtest_pred_impl.h"

namespace redfish::registries
{
namespace
{

TEST(RedfishRegistries, fillMessageArgs)
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