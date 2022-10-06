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

TEST(FanUtils, GetFanSensorIdAndNameSuccess)
{
    std::string sensorId;
    std::string sensorName;

    getFanSensorIdAndName("/xyz/openbmc_project/sensors/fan_tach/fan0_0",
                          std::ref(sensorId), std::ref(sensorName));
    EXPECT_EQ(sensorId, "fantach_fan0_0");
    EXPECT_EQ(sensorName, "fan0 0");

    getFanSensorIdAndName("/xyz/openbmc_project/sensors/fan_pwm/fan0_1",
                          std::ref(sensorId), std::ref(sensorName));
    EXPECT_EQ(sensorId, "fanpwm_fan0_1");
    EXPECT_EQ(sensorName, "fan0 1");

    getFanSensorIdAndName("/xyz/openbmc_project/sensors/fan_tach/fan2",
                          std::ref(sensorId), std::ref(sensorName));
    EXPECT_EQ(sensorId, "fantach_fan2");
    EXPECT_EQ(sensorName, "fan2");

    getFanSensorIdAndName("/xyz/openbmc_project/sensors/fan_tach/fan_3",
                          std::ref(sensorId), std::ref(sensorName));
    EXPECT_EQ(sensorId, "fantach_fan_3");
    EXPECT_EQ(sensorName, "fan 3");
}

TEST(FanUtils, GetFanSensorIdAndNameInvalid)
{
    std::string sensorId;
    std::string sensorName;

    sensorId = "unexpected";
    sensorName = "unexpected";
    getFanSensorIdAndName("/xyz/openbmc_project/sensors/power/fan0_0",
                          std::ref(sensorId), std::ref(sensorName));
    EXPECT_EQ(sensorId, "");
    EXPECT_EQ(sensorName, "");

    sensorId = "unexpected";
    sensorName = "unexpected";
    getFanSensorIdAndName("/xyz/openbmc_project/sensors/fantach/fan0_0",
                          std::ref(sensorId), std::ref(sensorName));
    EXPECT_EQ(sensorId, "");
    EXPECT_EQ(sensorName, "");

    sensorId = "unexpected";
    sensorName = "unexpected";
    getFanSensorIdAndName("/xyz/openbmc_project/sensors/fanpwm/fan0_0",
                          std::ref(sensorId), std::ref(sensorName));
    EXPECT_EQ(sensorId, "");
    EXPECT_EQ(sensorName, "");
}

} // namespace
} // namespace redfish::fan_utils
