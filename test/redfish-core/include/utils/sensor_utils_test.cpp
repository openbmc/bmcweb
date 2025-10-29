// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "bmcweb_config.h"

#include "dbus_utility.hpp"
#include "generated/enums/resource.hpp"
#include "generated/enums/sensor.hpp"
#include "generated/enums/thermal.hpp"
#include "utils/sensor_utils.hpp"

#include <cmath>
#include <functional>
#include <optional>
#include <string>
#include <tuple>

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

TEST(FillSensorStatus, Success)
{
    nlohmann::json sensorJson;
    auto properties = dbus::utility::DBusPropertiesMap();

    // Cases where no properties specified

    // Default status
    fillSensorStatus(properties, sensorJson, nullptr);
    EXPECT_EQ(sensorJson["Status"]["State"], resource::State::Enabled);
    EXPECT_EQ(sensorJson["Status"]["Health"], resource::Health::OK);

    // Existing health retained
    sensorJson.clear();
    sensorJson["Status"]["Health"] = resource::Health::Warning;
    fillSensorStatus(properties, sensorJson, nullptr);
    EXPECT_EQ(sensorJson["Status"]["State"], resource::State::Enabled);
    EXPECT_EQ(sensorJson["Status"]["Health"], resource::Health::Warning);

    // Existing health retained
    sensorJson.clear();
    sensorJson["Status"]["Health"] = resource::Health::Critical;
    fillSensorStatus(properties, sensorJson, nullptr);
    EXPECT_EQ(sensorJson["Status"]["State"], resource::State::Enabled);
    EXPECT_EQ(sensorJson["Status"]["Health"], resource::Health::Critical);

    // Cases with different properties

    properties = {
        {"Available", true},
    };
    sensorJson.clear();
    fillSensorStatus(properties, sensorJson, nullptr);
    EXPECT_EQ(sensorJson["Status"]["State"], resource::State::Enabled);
    EXPECT_EQ(sensorJson["Status"]["Health"], resource::Health::OK);

    properties = {
        {"Available", false},
    };
    sensorJson.clear();
    fillSensorStatus(properties, sensorJson, nullptr);
    EXPECT_EQ(sensorJson["Status"]["State"],
              resource::State::UnavailableOffline);
    EXPECT_EQ(sensorJson["Status"]["Health"], resource::Health::OK);

    properties = {
        {"CriticalAlarmHigh", true},
        {"CriticalAlarmLow", false},
    };
    sensorJson.clear();
    fillSensorStatus(properties, sensorJson, nullptr);
    EXPECT_EQ(sensorJson["Status"]["State"], resource::State::Enabled);
    EXPECT_EQ(sensorJson["Status"]["Health"], resource::Health::Critical);

    properties = {
        {"CriticalAlarmHigh", false},
        {"CriticalAlarmLow", true},
    };
    sensorJson.clear();
    fillSensorStatus(properties, sensorJson, nullptr);
    EXPECT_EQ(sensorJson["Status"]["State"], resource::State::Enabled);
    EXPECT_EQ(sensorJson["Status"]["Health"], resource::Health::Critical);

    properties = {
        {"WarningAlarmHigh", true},
        {"WarningAlarmLow", false},
    };
    sensorJson.clear();
    fillSensorStatus(properties, sensorJson, nullptr);
    EXPECT_EQ(sensorJson["Status"]["State"], resource::State::Enabled);
    EXPECT_EQ(sensorJson["Status"]["Health"], resource::Health::Warning);

    properties = {
        {"WarningAlarmHigh", false},
        {"WarningAlarmLow", true},
    };
    sensorJson.clear();
    fillSensorStatus(properties, sensorJson, nullptr);
    EXPECT_EQ(sensorJson["Status"]["State"], resource::State::Enabled);
    EXPECT_EQ(sensorJson["Status"]["Health"], resource::Health::Warning);

    properties = {
        {"Available", true},        {"CriticalAlarmHigh", true},
        {"CriticalAlarmLow", true}, {"WarningAlarmHigh", true},
        {"WarningAlarmLow", true},
    };
    sensorJson.clear();
    fillSensorStatus(properties, sensorJson, nullptr);
    EXPECT_EQ(sensorJson["Status"]["State"], resource::State::Enabled);
    EXPECT_EQ(sensorJson["Status"]["Health"], resource::Health::Critical);

    properties = {
        {"Available", false},        {"CriticalAlarmHigh", false},
        {"CriticalAlarmLow", false}, {"WarningAlarmHigh", true},
        {"WarningAlarmLow", true},
    };
    sensorJson.clear();
    fillSensorStatus(properties, sensorJson, nullptr);
    EXPECT_EQ(sensorJson["Status"]["State"],
              resource::State::UnavailableOffline);
    EXPECT_EQ(sensorJson["Status"]["Health"], resource::Health::Warning);
}

using dbus::utility::DbusVariantType;
using testing::StartsWith;

TEST(FillSensorIdentity, Success)
{
    nlohmann::json sensorJson;
    auto properties = dbus::utility::DBusPropertiesMap();
    bool result = false;
    bool isExcerpt = false;
    nlohmann::json::json_pointer unit;

    unit = "/Reading"_json_pointer;
    result = fillSensorIdentity("fan0_0", "fan_tach", properties, sensorJson,
                                isExcerpt, unit);
    EXPECT_TRUE(result);
    EXPECT_THAT(sensorJson["@odata.type"], StartsWith("#Sensor.v1_"));
    EXPECT_EQ(sensorJson["Id"].get<std::string>(), "fantach_fan0_0");
    EXPECT_EQ(sensorJson["Name"].get<std::string>(), "fan0 0");
    if constexpr (BMCWEB_REDFISH_ALLOW_ROTATIONAL_FANS)
    {
        EXPECT_EQ(sensorJson["ReadingType"], sensor::ReadingType::Rotational);
        EXPECT_EQ(sensorJson["ReadingUnits"].get<std::string>(), "RPM");
        EXPECT_EQ(unit.to_string(), "/Reading");
    }
    else
    {
        EXPECT_EQ(sensorJson["ReadingType"], sensor::ReadingType::Percent);
        EXPECT_EQ(sensorJson["ReadingUnits"].get<std::string>(), "%");
        EXPECT_EQ(unit.to_string(), "/SpeedRPM");
    }

    // Add properties, all are correct
    isExcerpt = true;
    unit = "/Reading"_json_pointer;
    properties = {
        {"MaxValue", DbusVariantType(static_cast<double>(200))},
        {"MinValue", DbusVariantType(static_cast<double>(0))},
        {"Value", DbusVariantType(static_cast<double>(100))},
    };
    sensorJson.clear();
    result = fillSensorIdentity("fan0_0", "fan_tach", properties, sensorJson,
                                isExcerpt, unit);
    EXPECT_TRUE(result);
    EXPECT_FALSE(sensorJson.contains("@odata.type"));
    EXPECT_FALSE(sensorJson.contains("Id"));
    EXPECT_FALSE(sensorJson.contains("Name"));
    EXPECT_FALSE(sensorJson.contains("ReadingType"));
    EXPECT_FALSE(sensorJson.contains("ReadingUnits"));
    if constexpr (BMCWEB_REDFISH_ALLOW_ROTATIONAL_FANS)
    {
        EXPECT_EQ(unit.to_string(), "/Reading");
        EXPECT_EQ(sensorJson["SpeedRPM"], 100);
    }
    else
    {
        EXPECT_EQ(unit.to_string(), "/SpeedRPM");
        EXPECT_EQ(sensorJson["Reading"], 50);
    }

    isExcerpt = false;
    unit = "/Reading"_json_pointer;
    properties = {
        {"Implementation",
         "xyz.openbmc_project.Sensor.Type.ImplementationType.Reported"},
        {"ReadingBasis",
         "xyz.openbmc_project.Sensor.Type.ReadingBasisType.Delta"},
    };
    sensorJson.clear();
    result = fillSensorIdentity("power1_0", "power", properties, sensorJson,
                                isExcerpt, unit);
    EXPECT_TRUE(result);
    EXPECT_THAT(sensorJson["@odata.type"], StartsWith("#Sensor.v1_"));
    EXPECT_EQ(sensorJson["Id"].get<std::string>(), "power_power1_0");
    EXPECT_EQ(sensorJson["Name"].get<std::string>(), "power1 0");
    EXPECT_EQ(sensorJson["ReadingType"], sensor::ReadingType::Power);
    EXPECT_EQ(sensorJson["ReadingUnits"].get<std::string>(), "W");
    EXPECT_EQ(sensorJson["Implementation"],
              sensor::ImplementationType::Reported);
    EXPECT_EQ(sensorJson["ReadingBasis"], sensor::ReadingBasisType::Delta);
    EXPECT_EQ(unit.to_string(), "/Reading");

    // Add invalid properties
    properties = {
        {"Implementation",
         "xyz.openbmc_project.Sensor.Type.ImplementationType.BADTYPE"},
        {"ReadingBasis",
         "xyz.openbmc_project.Sensor.Type.ReadingBasisType.BADTYPE"},
    };
    sensorJson.clear();
    result = fillSensorIdentity("temp2", "temperature", properties, sensorJson,
                                isExcerpt, unit);
    EXPECT_TRUE(result);
    EXPECT_THAT(sensorJson["@odata.type"], StartsWith("#Sensor.v1_"));
    EXPECT_EQ(sensorJson["Id"].get<std::string>(), "temperature_temp2");
    EXPECT_EQ(sensorJson["Name"].get<std::string>(), "temp2");
    EXPECT_EQ(sensorJson["ReadingType"], sensor::ReadingType::Temperature);
    EXPECT_EQ(sensorJson["ReadingUnits"].get<std::string>(), "Cel");
    EXPECT_FALSE(sensorJson.contains("Implementation"));
    EXPECT_FALSE(sensorJson.contains("ReadingBasis"));
    EXPECT_EQ(unit.to_string(), "/Reading");
}

TEST(FillSensorIdentity, Failure)
{
    nlohmann::json sensorJson;
    dbus::utility::DBusPropertiesMap properties = {
        {"MaxValue", "BadType"},
    };
    nlohmann::json::json_pointer unit;
    bool result = false;
    bool isExcerpt = false;

    result = fillSensorIdentity("temp3", "temperature", properties, sensorJson,
                                isExcerpt, unit);
    EXPECT_FALSE(result);
}

TEST(FillPowerThermalIdentity, Success)
{
    nlohmann::json sensorJson;
    nlohmann::json::json_pointer unit;
    bool forceToInt = false;
    bool result = false;

    result = fillPowerThermalIdentity("temp2", "temperature", sensorJson,
                                      nullptr, unit, std::ref(forceToInt));
    EXPECT_TRUE(result);
    EXPECT_THAT(sensorJson["@odata.type"], StartsWith("#Thermal.v1_"));
    EXPECT_EQ(sensorJson["Name"].get<std::string>(), "temp2");
    EXPECT_FALSE(sensorJson.contains("Id"));
    EXPECT_FALSE(sensorJson.contains("ReadingType"));
    EXPECT_FALSE(sensorJson.contains("ReadingUnits"));
    EXPECT_FALSE(forceToInt);
    EXPECT_EQ(unit.to_string(), "/ReadingCelsius");

    sensorJson.clear();
    forceToInt = false;
    result = fillPowerThermalIdentity("fan0_0", "fan_tach", sensorJson, nullptr,
                                      unit, std::ref(forceToInt));
    EXPECT_TRUE(result);
    EXPECT_THAT(sensorJson["@odata.type"], StartsWith("#Thermal.v1_"));
    EXPECT_EQ(sensorJson["Name"].get<std::string>(), "fan0 0");
    EXPECT_EQ(sensorJson["ReadingUnits"], thermal::ReadingUnits::RPM);
    EXPECT_FALSE(sensorJson.contains("Id"));
    EXPECT_FALSE(sensorJson.contains("ReadingType"));
    EXPECT_TRUE(forceToInt);
    EXPECT_EQ(unit.to_string(), "/Reading");

    sensorJson.clear();
    forceToInt = false;
    result = fillPowerThermalIdentity("fan2_0", "fan_pwm", sensorJson, nullptr,
                                      unit, std::ref(forceToInt));
    EXPECT_TRUE(result);
    EXPECT_THAT(sensorJson["@odata.type"], StartsWith("#Thermal.v1_"));
    EXPECT_EQ(sensorJson["Name"].get<std::string>(), "fan2 0");
    EXPECT_EQ(sensorJson["ReadingUnits"], thermal::ReadingUnits::Percent);
    EXPECT_FALSE(sensorJson.contains("Id"));
    EXPECT_FALSE(sensorJson.contains("ReadingType"));
    EXPECT_TRUE(forceToInt);
    EXPECT_EQ(unit.to_string(), "/Reading");

    sensorJson.clear();
    forceToInt = false;
    result = fillPowerThermalIdentity("input0", "voltage", sensorJson, nullptr,
                                      unit, std::ref(forceToInt));
    EXPECT_TRUE(result);
    EXPECT_THAT(sensorJson["@odata.type"], StartsWith("#Power.v1_"));
    EXPECT_EQ(sensorJson["Name"].get<std::string>(), "input0");
    EXPECT_FALSE(sensorJson.contains("ReadingUnits"));
    EXPECT_FALSE(sensorJson.contains("Id"));
    EXPECT_FALSE(sensorJson.contains("ReadingType"));
    EXPECT_FALSE(forceToInt);
    EXPECT_EQ(unit.to_string(), "/ReadingVolts");

    sensorJson.clear();
    forceToInt = false;
    result =
        fillPowerThermalIdentity("power1_0_input_power", "power", sensorJson,
                                 nullptr, unit, std::ref(forceToInt));
    EXPECT_TRUE(result);
    EXPECT_FALSE(sensorJson.contains("@odata.type"));
    EXPECT_FALSE(sensorJson.contains("MemberId"));
    EXPECT_FALSE(sensorJson.contains("Name"));
    EXPECT_FALSE(sensorJson.contains("ReadingType"));
    EXPECT_FALSE(sensorJson.contains("ReadingUnits"));
    EXPECT_FALSE(forceToInt);
    EXPECT_EQ(unit.to_string(), "/PowerInputWatts");

    sensorJson.clear();
    forceToInt = false;
    result =
        fillPowerThermalIdentity("power1_0_output_power", "power", sensorJson,
                                 nullptr, unit, std::ref(forceToInt));
    EXPECT_TRUE(result);
    EXPECT_FALSE(sensorJson.contains("@odata.type"));
    EXPECT_FALSE(sensorJson.contains("MemberId"));
    EXPECT_FALSE(sensorJson.contains("Name"));
    EXPECT_FALSE(sensorJson.contains("ReadingType"));
    EXPECT_FALSE(sensorJson.contains("ReadingUnits"));
    EXPECT_FALSE(forceToInt);
    EXPECT_EQ(unit.to_string(), "/PowerOutputWatts");

    sensorJson.clear();
    forceToInt = false;
    result = fillPowerThermalIdentity("total_power", "power", sensorJson,
                                      nullptr, unit, std::ref(forceToInt));
    EXPECT_TRUE(result);
    EXPECT_THAT(sensorJson["@odata.type"], StartsWith("#Power.v1_"));
    EXPECT_EQ(sensorJson["MemberId"].get<std::string>(), "0");
    EXPECT_EQ(sensorJson["Name"].get<std::string>(), "Chassis Power Control");
    EXPECT_FALSE(sensorJson.contains("ReadingType"));
    EXPECT_FALSE(sensorJson.contains("ReadingUnits"));
    EXPECT_FALSE(forceToInt);
    EXPECT_EQ(unit.to_string(), "/PowerConsumedWatts");
}

TEST(FillPowerThermalIdentity, Failure)
{
    nlohmann::json sensorJson;
    nlohmann::json::json_pointer unit;
    bool forceToInt = false;
    bool result = false;

    result = fillPowerThermalIdentity("BadSensor", "BadType", sensorJson,
                                      nullptr, unit, std::ref(forceToInt));
    EXPECT_FALSE(result);
}

using testing::UnorderedElementsAreArray;

TEST(MapPropertiesBySubnode, Success)
{
    SensorPropertyList properties;
    SensorPropertyList expectedProps;
    SensorPropertyMap valueElement;
    nlohmann::json::json_pointer unit;
    bool isExcerpt = true;

    unit = "/Reading"_json_pointer;
    valueElement = {"xyz.openbmc_project.Sensor.Value", "Value", unit};

    expectedProps = {valueElement};
    mapPropertiesBySubnode("fan_tach", ChassisSubNode::environmentMetricsNode,
                           properties, unit, isExcerpt);
    EXPECT_THAT(properties, testing::ContainerEq(expectedProps));

    isExcerpt = false;
    properties.clear();
    expectedProps = {valueElement};
    mapPropertiesBySubnode("power", ChassisSubNode::powerNode, properties, unit,
                           isExcerpt);
    EXPECT_THAT(properties, testing::ContainerEq(expectedProps));

    isExcerpt = false;
    properties.clear();
    expectedProps = {
        valueElement,
        {"xyz.openbmc_project.Sensor.Threshold.Warning", "WarningHigh",
         "/Thresholds/UpperCaution/Reading"_json_pointer},
        {"xyz.openbmc_project.Sensor.Threshold.Warning", "WarningLow",
         "/Thresholds/LowerCaution/Reading"_json_pointer},
        {"xyz.openbmc_project.Sensor.Threshold.Critical", "CriticalHigh",
         "/Thresholds/UpperCritical/Reading"_json_pointer},
        {"xyz.openbmc_project.Sensor.Threshold.Critical", "CriticalLow",
         "/Thresholds/LowerCritical/Reading"_json_pointer},
        {"xyz.openbmc_project.Sensor.Threshold.HardShutdown",
         "HardShutdownHigh", "/Thresholds/UpperFatal/Reading"_json_pointer},
        {"xyz.openbmc_project.Sensor.Threshold.HardShutdown", "HardShutdownLow",
         "/Thresholds/LowerFatal/Reading"_json_pointer},
        {"xyz.openbmc_project.Sensor.Value", "MinValue",
         "/ReadingRangeMin"_json_pointer},
        {"xyz.openbmc_project.Sensor.Value", "MaxValue",
         "/ReadingRangeMax"_json_pointer},
        {"xyz.openbmc_project.Sensor.Accuracy", "Accuracy",
         "/Accuracy"_json_pointer},
    };
    mapPropertiesBySubnode("power", ChassisSubNode::sensorsNode, properties,
                           unit, isExcerpt);
    EXPECT_THAT(properties, UnorderedElementsAreArray(expectedProps));

    if constexpr (!BMCWEB_REDFISH_ALLOW_ROTATIONAL_FANS)
    {
        /* fan_tach sets ReadingRangeMax and ReadingRangeMin elsewhere
         * Remove these properties and keep the rest for validation of
         * the results.
         */
        expectedProps = {
            valueElement,
            {"xyz.openbmc_project.Sensor.Threshold.Warning", "WarningHigh",
             "/Thresholds/UpperCaution/Reading"_json_pointer},
            {"xyz.openbmc_project.Sensor.Threshold.Warning", "WarningLow",
             "/Thresholds/LowerCaution/Reading"_json_pointer},
            {"xyz.openbmc_project.Sensor.Threshold.Critical", "CriticalHigh",
             "/Thresholds/UpperCritical/Reading"_json_pointer},
            {"xyz.openbmc_project.Sensor.Threshold.Critical", "CriticalLow",
             "/Thresholds/LowerCritical/Reading"_json_pointer},
            {"xyz.openbmc_project.Sensor.Threshold.HardShutdown",
             "HardShutdownHigh", "/Thresholds/UpperFatal/Reading"_json_pointer},
            {"xyz.openbmc_project.Sensor.Threshold.HardShutdown",
             "HardShutdownLow", "/Thresholds/LowerFatal/Reading"_json_pointer},
            {"xyz.openbmc_project.Sensor.Accuracy", "Accuracy",
             "/Accuracy"_json_pointer},
        };
    }

    isExcerpt = false;
    properties.clear();
    mapPropertiesBySubnode("fan_tach", ChassisSubNode::sensorsNode, properties,
                           unit, isExcerpt);
    EXPECT_THAT(properties, UnorderedElementsAreArray(expectedProps));

    isExcerpt = false;
    properties.clear();
    expectedProps = {
        valueElement,
        {"xyz.openbmc_project.Sensor.Threshold.Warning", "WarningHigh",
         "/UpperThresholdNonCritical"_json_pointer},
        {"xyz.openbmc_project.Sensor.Threshold.Warning", "WarningLow",
         "/LowerThresholdNonCritical"_json_pointer},
        {"xyz.openbmc_project.Sensor.Threshold.Critical", "CriticalHigh",
         "/UpperThresholdCritical"_json_pointer},
        {"xyz.openbmc_project.Sensor.Threshold.Critical", "CriticalLow",
         "/LowerThresholdCritical"_json_pointer},
        {"xyz.openbmc_project.Sensor.Value", "MinValue",
         "/MinReadingRangeTemp"_json_pointer},
        {"xyz.openbmc_project.Sensor.Value", "MaxValue",
         "/MaxReadingRangeTemp"_json_pointer},
    };
    mapPropertiesBySubnode("temperature", ChassisSubNode::thermalNode,
                           properties, unit, isExcerpt);
    EXPECT_THAT(properties, UnorderedElementsAreArray(expectedProps));

    isExcerpt = false;
    properties.clear();
    expectedProps = {
        valueElement,
        {"xyz.openbmc_project.Sensor.Threshold.Warning", "WarningHigh",
         "/UpperThresholdNonCritical"_json_pointer},
        {"xyz.openbmc_project.Sensor.Threshold.Warning", "WarningLow",
         "/LowerThresholdNonCritical"_json_pointer},
        {"xyz.openbmc_project.Sensor.Threshold.Critical", "CriticalHigh",
         "/UpperThresholdCritical"_json_pointer},
        {"xyz.openbmc_project.Sensor.Threshold.Critical", "CriticalLow",
         "/LowerThresholdCritical"_json_pointer},
        {"xyz.openbmc_project.Sensor.Value", "MinValue",
         "/MinReadingRange"_json_pointer},
        {"xyz.openbmc_project.Sensor.Value", "MaxValue",
         "/MaxReadingRange"_json_pointer},
    };
    mapPropertiesBySubnode("voltage", ChassisSubNode::powerNode, properties,
                           unit, isExcerpt);
    EXPECT_THAT(properties, UnorderedElementsAreArray(expectedProps));
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
