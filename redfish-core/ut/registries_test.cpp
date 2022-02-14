#include "registries.hpp"

#include "gmock/gmock.h"

namespace redfish::registries
{
namespace
{

TEST(FillMessageArgs, ArgsAreFilledCorrectly)
{
    EXPECT_EQ(fillMessageArgs({{"foo"}}, "%1"), "foo");
    EXPECT_EQ(fillMessageArgs({}, ""), "");
    EXPECT_EQ(fillMessageArgs({{"foo", "bar"}}, "%1, %2"), "foo, bar");
    EXPECT_EQ(fillMessageArgs({{"foo"}}, "%1 bar"), "foo bar");
    EXPECT_EQ(fillMessageArgs({}, "%1"), "");
    EXPECT_EQ(fillMessageArgs({}, "%"), "");
    EXPECT_EQ(fillMessageArgs({}, "%foo"), "");
}
} // namespace
} // namespace redfish::registries
