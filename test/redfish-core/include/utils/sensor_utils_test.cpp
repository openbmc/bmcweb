#include "utils/sensor_utils.hpp"

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace redfish::sensor_utils
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

TEST(GetSensorId, Success)
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

TEST(ChassisSubNodeToString, Success)
{
    std::string subNodeStr;

    subNodeStr = chassisSubNodeToString(ChassisSubNode::powerNode);
    EXPECT_EQ(subNodeStr, "Power");

    subNodeStr = chassisSubNodeToString(ChassisSubNode::sensorsNode);
    EXPECT_EQ(subNodeStr, "Sensors");

    subNodeStr = chassisSubNodeToString(ChassisSubNode::thermalNode);
    EXPECT_EQ(subNodeStr, "Thermal");

    subNodeStr = chassisSubNodeToString(ChassisSubNode::thermalMetricsNode);
    EXPECT_EQ(subNodeStr, "ThermalMetrics");

    subNodeStr = chassisSubNodeToString(ChassisSubNode::unknownNode);
    EXPECT_EQ(subNodeStr, "");
}

TEST(ChassisSubNodeFromString, Success)
{
    ChassisSubNode subNode = ChassisSubNode::unknownNode;

    subNode = chassisSubNodeFromString("Power");
    EXPECT_EQ(subNode, ChassisSubNode::powerNode);

    subNode = chassisSubNodeFromString("Sensors");
    EXPECT_EQ(subNode, ChassisSubNode::sensorsNode);

    subNode = chassisSubNodeFromString("Thermal");
    EXPECT_EQ(subNode, ChassisSubNode::thermalNode);

    subNode = chassisSubNodeFromString("ThermalMetrics");
    EXPECT_EQ(subNode, ChassisSubNode::thermalMetricsNode);

    subNode = chassisSubNodeFromString("BadNode");
    EXPECT_EQ(subNode, ChassisSubNode::unknownNode);

    subNode = chassisSubNodeFromString("");
    EXPECT_EQ(subNode, ChassisSubNode::unknownNode);
}

TEST(IsExcerptNode, True)
{
    EXPECT_TRUE(isExcerptNode(ChassisSubNode::thermalMetricsNode));
}

TEST(IsExcerptNode, False)
{
    EXPECT_FALSE(isExcerptNode(ChassisSubNode::sensorsNode));
    EXPECT_FALSE(isExcerptNode(ChassisSubNode::powerNode));
    EXPECT_FALSE(isExcerptNode(ChassisSubNode::thermalNode));
    EXPECT_FALSE(isExcerptNode(ChassisSubNode::unknownNode));
}

} // namespace
} // namespace redfish::sensor_utils
