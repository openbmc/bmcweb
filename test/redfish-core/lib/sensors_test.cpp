#include "sensors.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(SplitSensorNameAndType, Type)
{
    EXPECT_EQ(splitSensorNameAndType("fantach_foo_1").first, "fan_tach");
    EXPECT_EQ(splitSensorNameAndType("temperature_foo2").first, "temperature");
}

TEST(SplitSensorNameAndType, Name)
{
    EXPECT_EQ(splitSensorNameAndType("fantach_foo_1").second, "foo_1");
    EXPECT_EQ(splitSensorNameAndType("temperature_foo2").second, "foo2");
}

TEST(SplitSensorNameAndType, Error)
{
    EXPECT_TRUE(splitSensorNameAndType("fantach").first.empty());
    EXPECT_TRUE(splitSensorNameAndType("temperature").second.empty());
}

} // namespace
} // namespace redfish
