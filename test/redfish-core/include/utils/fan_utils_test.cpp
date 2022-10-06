#include "utils/fan_utils.hpp"

#include <cctype>
#include <cstdint>
#include <limits>
#include <vector>

#include <gtest/gtest.h>

namespace redfish::fan_utils
{
namespace
{

TEST(FanUtils, GetFanSensorIdSuccess)
{
    std::string sensorId;

    getFanSensorId("/xyz/openbmc_project/sensors/fan_tach/fan0_0",
                   std::ref(sensorId));
    EXPECT_EQ(sensorId, "fantach_fan0_0");

    getFanSensorId("/xyz/openbmc_project/sensors/fan_pwm/fan0_1",
                   std::ref(sensorId));
    EXPECT_EQ(sensorId, "fanpwm_fan0_1");

    getFanSensorId("/xyz/openbmc_project/sensors/fan_tach/fan2",
                   std::ref(sensorId));
    EXPECT_EQ(sensorId, "fantach_fan2");

    getFanSensorId("/xyz/openbmc_project/sensors/fan_tach/fan_3",
                   std::ref(sensorId));
    EXPECT_EQ(sensorId, "fantach_fan_3");
}

TEST(FanUtils, GetFanSensorIdInvalid)
{
    std::string sensorId;

    sensorId = "unexpected";
    getFanSensorId("/xyz/openbmc_project/sensors/power/fan0_0",
                   std::ref(sensorId));
    EXPECT_EQ(sensorId, "");

    sensorId = "unexpected";
    getFanSensorId("/xyz/openbmc_project/sensors/fantach/fan0_0",
                   std::ref(sensorId));
    EXPECT_EQ(sensorId, "");

    sensorId = "unexpected";
    getFanSensorId("/xyz/openbmc_project/sensors/fanpwm/fan0_0",
                   std::ref(sensorId));
    EXPECT_EQ(sensorId, "");
}

} // namespace
} // namespace redfish::fan_utils
