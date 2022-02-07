#include "registries.hpp"

#include "gmock/gmock.h"

TEST(RedfishRegistries, fillMessageArgs)
{
    using redfish::message_registries::fillMessageArgs;
    std::string toFill("%1");
    fillMessageArgs(std::to_array<std::string_view>({"foo"}), toFill);
    EXPECT_EQ(toFill, "foo");

    toFill = "";
    fillMessageArgs({}, toFill);
    EXPECT_EQ(toFill, "");

    toFill = "%1, %2";
    fillMessageArgs( std::to_array<std::string_view>({"foo", "bar"}), toFill);
    EXPECT_EQ(toFill, "foo, bar");
}
