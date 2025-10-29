// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "utils/sensor_utils.hpp"

#include <cmath>
#include <optional>
#include <string>
#include <tuple>

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

    subNodeStr = chassisSubNodeToString(ChassisSubNode::environmentMetricsNode);
    EXPECT_EQ(subNodeStr, "EnvironmentMetrics");

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

    subNode = chassisSubNodeFromString("EnvironmentMetrics");
    EXPECT_EQ(subNode, ChassisSubNode::environmentMetricsNode);

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
    EXPECT_TRUE(isExcerptNode(ChassisSubNode::environmentMetricsNode));
    EXPECT_TRUE(isExcerptNode(ChassisSubNode::thermalMetricsNode));
}

TEST(IsExcerptNode, False)
{
    EXPECT_FALSE(isExcerptNode(ChassisSubNode::sensorsNode));
    EXPECT_FALSE(isExcerptNode(ChassisSubNode::powerNode));
    EXPECT_FALSE(isExcerptNode(ChassisSubNode::thermalNode));
    EXPECT_FALSE(isExcerptNode(ChassisSubNode::unknownNode));
}

TEST(UpdateSensorStatistics, ParametersValid)
{
    nlohmann::json sensorJson;
    Reading reading =
        std::make_tuple("metricId1", "metadata1", 42.5, 1234567890);
    Readings metrics = {reading};
    Statistics statistics = std::make_tuple(1234567890, metrics);
    SensorPaths sensorPaths;
    ReadingParameters readingParams = {std::make_tuple(
        sensorPaths,
        "xyz.openbmc_project.Telemetry.Report.OperationType.Maximum",
        "metricId1", 60)};

    updateSensorStatistics(sensorJson, statistics, readingParams);

    EXPECT_EQ(sensorJson["PeakReading"], 42.5);
    EXPECT_EQ(sensorJson["PeakReadingTime"], 1234567890);
}

TEST(UpdateSensorStatistics, EmptyMetrics)
{
    nlohmann::json sensorJson;
    Readings metrics;
    Statistics statistics = std::make_tuple(1234567890, metrics);
    SensorPaths sensorPaths;
    ReadingParameters readingParams = {std::make_tuple(
        sensorPaths,
        "xyz.openbmc_project.Telemetry.Report.OperationType.Maximum",
        "metricId1", 60)};

    updateSensorStatistics(sensorJson, statistics, readingParams);

    EXPECT_FALSE(sensorJson.contains("PeakReading"));
    EXPECT_FALSE(sensorJson.contains("PeakReadingTime"));
}

TEST(UpdateSensorStatistics, NonMaximumOperationType)
{
    nlohmann::json sensorJson;
    Reading reading =
        std::make_tuple("metricId1", "metadata1", 42.5, 1234567890);
    Readings metrics = {reading};
    Statistics statistics = std::make_tuple(1234567890, metrics);
    SensorPaths sensorPaths;
    ReadingParameters readingParams = {std::make_tuple(
        sensorPaths,
        "xyz.openbmc_project.Telemetry.Report.OperationType.Minimum",
        "metricId1", 60)};

    updateSensorStatistics(sensorJson, statistics, readingParams);

    EXPECT_FALSE(sensorJson.contains("PeakReading"));
    EXPECT_FALSE(sensorJson.contains("PeakReadingTime"));
}

TEST(UpdateSensorStatistics, ParamsNullopt)
{
    nlohmann::json sensorJson;
    std::optional<Statistics> statistics = std::nullopt;
    std::optional<ReadingParameters> readingParams = std::nullopt;

    updateSensorStatistics(sensorJson, statistics, readingParams);

    EXPECT_FALSE(sensorJson.contains("PeakReading"));
    EXPECT_FALSE(sensorJson.contains("PeakReadingTime"));
}

TEST(GetFanPercent, Success)
{
    std::optional<long> percentValue;

    std::optional<double> value;
    std::optional<double> maxValue;
    std::optional<double> minValue;

    value = 4096;
    maxValue = 4096;
    minValue = 0;
    percentValue = getFanPercent("atMax", maxValue, minValue, value);
    EXPECT_EQ(percentValue.value_or(-1), 100);
    percentValue.reset();

    value = 1024;
    maxValue = 4096;
    minValue = 0;
    percentValue = getFanPercent("atQuarter", maxValue, minValue, value);
    EXPECT_EQ(percentValue.value_or(-1), 25);
    percentValue.reset();

    value = 80;
    maxValue = 90;
    minValue = 70;
    percentValue = getFanPercent("nonZeroMin", maxValue, minValue, value);
    EXPECT_EQ(percentValue.value_or(-1), 50);
    percentValue.reset();

    // Check for expected rounding
    value = 200;
    maxValue = 300;
    minValue = 0;
    percentValue = getFanPercent("roundedPercent", maxValue, minValue, value);
    EXPECT_EQ(percentValue.value_or(-1), 67);
    percentValue.reset();
}

TEST(GetFanPercent, Fail)
{
    std::optional<long> percentValue;

    std::optional<double> value;
    std::optional<double> maxValue;
    std::optional<double> minValue;
    std::optional<double> noProperty;

    value = 1024;
    percentValue = getFanPercent("valOnly", noProperty, noProperty, value);
    EXPECT_FALSE(percentValue.has_value());
    percentValue.reset();

    maxValue = 4096;
    percentValue = getFanPercent("noMinValue", maxValue, noProperty, value);
    EXPECT_FALSE(percentValue.has_value());
    percentValue.reset();

    value = 2048;
    minValue = 1024;
    percentValue = getFanPercent("noMaxValue", noProperty, minValue, value);
    EXPECT_FALSE(percentValue.has_value());
    percentValue.reset();

    value = 1024;
    maxValue = 0;
    minValue = 4096;
    percentValue = getFanPercent("badMaxValue", maxValue, minValue, value);
    EXPECT_FALSE(percentValue.has_value());
    percentValue.reset();

    value = 1024;
    maxValue = NAN;
    minValue = -NAN;
    percentValue = getFanPercent("defaultMinMax", maxValue, minValue, value);
    EXPECT_FALSE(percentValue.has_value());
    percentValue.reset();
}

} // namespace
} // namespace redfish::sensor_utils
