#include "registries.hpp"

#include "gmock/gmock.h"

TEST(RedfishRegistries, fillMessageArgs)
{
    using redfish::message_registries::fillMessageArgs;

    EXPECT_EQ(fillMessageArgs("%1", {"foo"}), "foo");
    EXPECT_EQ(fillMessageArgs("", {}), "");
    EXPECT_EQ(fillMessageArgs("%1, %2", {"foo", "bar"}), "foo,bar");
}
