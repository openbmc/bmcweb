#include "sensors.hpp"

#include <gmock/gmock.h> // IWYU pragma: keep
#include <gtest/gtest.h> // IWYU pragma: keep

// IWYU pragma: no_include <gtest/gtest-message.h>
// IWYU pragma: no_include <gtest/gtest-test-part.h>
// IWYU pragma: no_include "gtest/gtest_pred_impl.h"
// IWYU pragma: no_include <gmock/gmock-matchers.h>
// IWYU pragma: no_include <gtest/gtest-matchers.h>

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
