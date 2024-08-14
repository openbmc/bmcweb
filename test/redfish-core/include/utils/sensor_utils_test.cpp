#include "utils/sensor_utils.hpp"

#include <string>

#include <gtest/gtest.h>

namespace redfish::sensor_utils
{
namespace
{

TEST(SensorUtils, GetSensorIdSuccess)
{
    std::string sensorId;

    sensorId = getSensorId("fan0_0", "fan_tach");
    EXPECT_EQ(sensorId, "fantach_fan0_0");

    sensorId = getSensorId("0_1", "fan_pwm");
    EXPECT_EQ(sensorId, "fanpwm_0_1");

    sensorId = getSensorId("fan2", "fan_tach");
    EXPECT_EQ(sensorId, "fantach_fan2");

    sensorId = getSensorId("fan_3", "fan_tach");
    EXPECT_EQ(sensorId, "fantach_fan_3");

    sensorId = getSensorId("temp2", "temperature");
    EXPECT_EQ(sensorId, "temperature_temp2");
}

} // namespace
} // namespace redfish::sensor_utils
