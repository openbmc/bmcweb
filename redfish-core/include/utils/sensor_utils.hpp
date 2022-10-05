// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "bmcweb_config.h"

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/resource.hpp"
#include "generated/enums/sensor.hpp"
#include "generated/enums/thermal.hpp"
#include "logging.hpp"
#include "str_utility.hpp"
#include "utils/dbus_utils.hpp"

#include <asm-generic/errno.h>

#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

namespace redfish
{
namespace sensor_utils
{

enum class ChassisSubNode
{
    environmentMetricsNode,
    fansNode,
    powerNode,
    sensorsNode,
    thermalNode,
    thermalMetricsNode,
    unknownNode,
};

constexpr std::string_view chassisSubNodeToString(ChassisSubNode subNode)
{
    switch (subNode)
    {
        case ChassisSubNode::environmentMetricsNode:
            return "EnvironmentMetrics";
        case ChassisSubNode::fansNode:
            return "Fans";
        case ChassisSubNode::powerNode:
            return "Power";
        case ChassisSubNode::sensorsNode:
            return "Sensors";
        case ChassisSubNode::thermalNode:
            return "Thermal";
        case ChassisSubNode::thermalMetricsNode:
            return "ThermalMetrics";
        case ChassisSubNode::unknownNode:
        default:
            return "";
    }
}

inline ChassisSubNode chassisSubNodeFromString(const std::string& subNodeStr)
{
    // If none match unknownNode is returned
    ChassisSubNode subNode = ChassisSubNode::unknownNode;

    if (subNodeStr == "EnvironmentMetrics")
    {
        subNode = ChassisSubNode::environmentMetricsNode;
    }
    else if (subNodeStr == "Fans")
    {
        subNode = ChassisSubNode::fansNode;
    }
    else if (subNodeStr == "Power")
    {
        subNode = ChassisSubNode::powerNode;
    }
    else if (subNodeStr == "Sensors")
    {
        subNode = ChassisSubNode::sensorsNode;
    }
    else if (subNodeStr == "Thermal")
    {
        subNode = ChassisSubNode::thermalNode;
    }
    else if (subNodeStr == "ThermalMetrics")
    {
        subNode = ChassisSubNode::thermalMetricsNode;
    }

    return subNode;
}

inline bool isExcerptNode(const ChassisSubNode subNode)
{
    return ((subNode == ChassisSubNode::thermalMetricsNode) ||
            (subNode == ChassisSubNode::fansNode) ||
            (subNode == ChassisSubNode::environmentMetricsNode));
}

/**
 * Possible states for physical inventory leds
 */
enum class LedState
{
    OFF,
    ON,
    BLINK,
    UNKNOWN
};

/**
 * D-Bus inventory item associated with one or more sensors.
 */
class InventoryItem
{
  public:
    explicit InventoryItem(const std::string& objPath) : objectPath(objPath)
    {
        // Set inventory item name to last node of object path
        sdbusplus::message::object_path path(objectPath);
        name = path.filename();
        if (name.empty())
        {
            BMCWEB_LOG_ERROR("Failed to find '/' in {}", objectPath);
        }
    }

    std::string objectPath;
    std::string name;
    bool isPresent = true;
    bool isFunctional = true;
    bool isPowerSupply = false;
    int powerSupplyEfficiencyPercent = -1;
    std::string manufacturer;
    std::string model;
    std::string partNumber;
    std::string serialNumber;
    std::set<std::string> sensors;
    std::string ledObjectPath;
    LedState ledState = LedState::UNKNOWN;
};

inline std::string getSensorId(std::string_view sensorName,
                               std::string_view sensorType)
{
    std::string normalizedType(sensorType);
    auto remove = std::ranges::remove(normalizedType, '_');
    normalizedType.erase(std::ranges::begin(remove), normalizedType.end());

    return std::format("{}_{}", normalizedType, sensorName);
}

inline std::pair<std::string, std::string> splitSensorNameAndType(
    std::string_view sensorId)
{
    size_t index = sensorId.find('_');
    if (index == std::string::npos)
    {
        return std::make_pair<std::string, std::string>("", "");
    }
    std::string sensorType{sensorId.substr(0, index)};
    std::string sensorName{sensorId.substr(index + 1)};
    // fan_pwm and fan_tach need special handling
    if (sensorType == "fantach" || sensorType == "fanpwm")
    {
        sensorType.insert(3, 1, '_');
    }
    return std::make_pair(sensorType, sensorName);
}

namespace sensors
{
inline std::string_view toReadingUnits(std::string_view sensorType)
{
    if (sensorType == "voltage")
    {
        return "V";
    }
    if (sensorType == "power")
    {
        return "W";
    }
    if (sensorType == "current")
    {
        return "A";
    }
    if (sensorType == "fan_tach")
    {
        if constexpr (BMCWEB_REDFISH_ALLOW_ROTATIONAL_FANS)
        {
            return "RPM";
        }
        else
        {
            return "%";
        }
    }
    if (sensorType == "temperature")
    {
        return "Cel";
    }
    if (sensorType == "fan_pwm" || sensorType == "utilization" ||
        sensorType == "humidity")
    {
        return "%";
    }
    if (sensorType == "altitude")
    {
        return "m";
    }
    if (sensorType == "airflow")
    {
        return "cft_i/min";
    }
    if (sensorType == "energy")
    {
        return "J";
    }
    if (sensorType == "liquidflow")
    {
        return "L/min";
    }
    if (sensorType == "pressure")
    {
        return "Pa";
    }
    return "";
}

inline sensor::ReadingType toReadingType(std::string_view sensorType)
{
    if (sensorType == "voltage")
    {
        return sensor::ReadingType::Voltage;
    }
    if (sensorType == "power")
    {
        return sensor::ReadingType::Power;
    }
    if (sensorType == "current")
    {
        return sensor::ReadingType::Current;
    }
    if (sensorType == "fan_tach")
    {
        if constexpr (BMCWEB_REDFISH_ALLOW_ROTATIONAL_FANS)
        {
            return sensor::ReadingType::Rotational;
        }
        else
        {
            return sensor::ReadingType::Percent;
        }
    }
    if (sensorType == "temperature")
    {
        return sensor::ReadingType::Temperature;
    }
    if (sensorType == "fan_pwm" || sensorType == "utilization")
    {
        return sensor::ReadingType::Percent;
    }
    if (sensorType == "humidity")
    {
        return sensor::ReadingType::Humidity;
    }
    if (sensorType == "altitude")
    {
        return sensor::ReadingType::Altitude;
    }
    if (sensorType == "airflow")
    {
        return sensor::ReadingType::AirFlow;
    }
    if (sensorType == "energy")
    {
        return sensor::ReadingType::EnergyJoules;
    }
    if (sensorType == "liquidflow")
    {
        return sensor::ReadingType::LiquidFlowLPM;
    }
    if (sensorType == "pressure")
    {
        return sensor::ReadingType::PressurePa;
    }
    return sensor::ReadingType::Invalid;
}

} // namespace sensors

// represents metric Id, metadata, reading value and timestamp of single
// reading update in milliseconds
using Reading = std::tuple<std::string, std::string, double, uint64_t>;
// represents multiple independent readings
using Readings = std::vector<Reading>;
// represents a timestamp and multiple independent readings
using Statistics = std::tuple<uint64_t, Readings>;
// represents sensor's path, its metadata
using SensorPaths =
    std::vector<std::tuple<sdbusplus::message::object_path, std::string>>;
// represents reading parameters for statistics readings
using ReadingParameters =
    std::vector<std::tuple<SensorPaths, std::string, std::string, uint64_t>>;

inline void updateSensorStatistics(
    nlohmann::json& sensorJson, const std::optional<Statistics>& statistics,
    const std::optional<ReadingParameters>& readingParameters)
{
    if (statistics.has_value() && readingParameters.has_value())
    {
        Readings metrics = std::get<1>(*statistics);
        for (const auto& [sensorPaths, operationType, metricId, duration] :
             *readingParameters)
        {
            if (operationType ==
                "xyz.openbmc_project.Telemetry.Report.OperationType.Maximum")
            {
                if (metrics.size() == 1)
                {
                    const auto& [id, metadata, value, timestamp] = metrics[0];
                    sensorJson["PeakReading"] = value;
                    if (timestamp != 0)
                    {
                        sensorJson["PeakReadingTime"] = timestamp;
                    }
                }
            }
        }
    }
}

/**
 * @brief Returns the Redfish State value for the specified inventory item.
 * @param inventoryItem D-Bus inventory item associated with a sensor.
 * @param sensorAvailable Boolean representing if D-Bus sensor is marked as
 * available.
 * @return State value for inventory item.
 */
inline resource::State getState(const InventoryItem* inventoryItem,
                                const bool sensorAvailable)
{
    if ((inventoryItem != nullptr) && !(inventoryItem->isPresent))
    {
        return resource::State::Absent;
    }

    if (!sensorAvailable)
    {
        return resource::State::UnavailableOffline;
    }

    return resource::State::Enabled;
}

/**
 * @brief Returns the Redfish Health value for the specified sensor.
 * @param sensorJson Sensor JSON object.
 * @param valuesDict Map of all sensor DBus values.
 * @param inventoryItem D-Bus inventory item associated with the sensor.  Will
 * be nullptr if no associated inventory item was found.
 * @return Health value for sensor.
 */
inline std::string getHealth(nlohmann::json& sensorJson,
                             const dbus::utility::DBusPropertiesMap& valuesDict,
                             const InventoryItem* inventoryItem)
{
    // Get current health value (if any) in the sensor JSON object.  Some JSON
    // objects contain multiple sensors (such as PowerSupplies).  We want to set
    // the overall health to be the most severe of any of the sensors.
    std::string currentHealth;
    auto statusIt = sensorJson.find("Status");
    if (statusIt != sensorJson.end())
    {
        auto healthIt = statusIt->find("Health");
        if (healthIt != statusIt->end())
        {
            std::string* health = healthIt->get_ptr<std::string*>();
            if (health != nullptr)
            {
                currentHealth = *health;
            }
        }
    }

    // If current health in JSON object is already Critical, return that.  This
    // should override the sensor health, which might be less severe.
    if (currentHealth == "Critical")
    {
        return "Critical";
    }

    const bool* criticalAlarmHigh = nullptr;
    const bool* criticalAlarmLow = nullptr;
    const bool* warningAlarmHigh = nullptr;
    const bool* warningAlarmLow = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), valuesDict, "CriticalAlarmHigh",
        criticalAlarmHigh, "CriticalAlarmLow", criticalAlarmLow,
        "WarningAlarmHigh", warningAlarmHigh, "WarningAlarmLow",
        warningAlarmLow);

    if (success)
    {
        // Check if sensor has critical threshold alarm
        if ((criticalAlarmHigh != nullptr && *criticalAlarmHigh) ||
            (criticalAlarmLow != nullptr && *criticalAlarmLow))
        {
            return "Critical";
        }
    }

    // Check if associated inventory item is not functional
    if ((inventoryItem != nullptr) && !(inventoryItem->isFunctional))
    {
        return "Critical";
    }

    // If current health in JSON object is already Warning, return that. This
    // should override the sensor status, which might be less severe.
    if (currentHealth == "Warning")
    {
        return "Warning";
    }

    if (success)
    {
        // Check if sensor has warning threshold alarm
        if ((warningAlarmHigh != nullptr && *warningAlarmHigh) ||
            (warningAlarmLow != nullptr && *warningAlarmLow))
        {
            return "Warning";
        }
    }

    return "OK";
}

inline void setLedState(nlohmann::json& sensorJson,
                        const InventoryItem* inventoryItem)
{
    if (inventoryItem != nullptr && !inventoryItem->ledObjectPath.empty())
    {
        switch (inventoryItem->ledState)
        {
            case LedState::OFF:
                sensorJson["IndicatorLED"] = resource::IndicatorLED::Off;
                break;
            case LedState::ON:
                sensorJson["IndicatorLED"] = resource::IndicatorLED::Lit;
                break;
            case LedState::BLINK:
                sensorJson["IndicatorLED"] = resource::IndicatorLED::Blinking;
                break;
            default:
                break;
        }
    }
}

inline sensor::ReadingBasisType dBusSensorReadingBasisToRedfish(
    const std::string& readingBasis)
{
    if (readingBasis ==
        "xyz.openbmc_project.Sensor.Type.ReadingBasisType.Headroom")
    {
        return sensor::ReadingBasisType::Headroom;
    }
    if (readingBasis ==
        "xyz.openbmc_project.Sensor.Type.ReadingBasisType.Delta")
    {
        return sensor::ReadingBasisType::Delta;
    }
    if (readingBasis == "xyz.openbmc_project.Sensor.Type.ReadingBasisType.Zero")
    {
        return sensor::ReadingBasisType::Zero;
    }

    return sensor::ReadingBasisType::Invalid;
}

/**
 * @brief Computes percent value for Rotational Fan (fan_tach)
 * @param[in] sensorName  The name of the fan_tach sensor
 * @param[in] maxValue The MaxValue D-Bus value
 * @param[in] minValue The MinValue D-Bus value
 * @param[in] value The Value D-Bus value
 * @return Computed percent returned here if sanity checks pass
 */
inline std::optional<long> getFanPercent(
    std::string_view sensorName, std::optional<double> maxValue,
    std::optional<double> minValue, std::optional<double> value)
{
    /* Sanity check values to be used to compute the percent and guard against
     * divide by zero.
     */
    if (!value.has_value() || !std::isfinite(*value))
    {
        BMCWEB_LOG_DEBUG("No Value for {}", sensorName);
        return std::nullopt;
    }

    if (!maxValue.has_value() || !std::isfinite(*maxValue))
    {
        BMCWEB_LOG_DEBUG("maxValue not available for {}", sensorName);
        return std::nullopt;
    }

    if (!minValue.has_value() || !std::isfinite(*minValue))
    {
        BMCWEB_LOG_DEBUG("minValue not available for {}", sensorName);
        return std::nullopt;
    }

    double range = *maxValue - *minValue;
    if (range <= 0)
    {
        BMCWEB_LOG_ERROR("Bad min/max for {}", sensorName);
        return std::nullopt;
    }

    // Compute fan's RPM Percent value
    return std::lround(((*value - *minValue) * 100) / range);
}

inline void convertToFanPercent(
    nlohmann::json& sensorJson, std::string_view sensorName,
    nlohmann::json::json_pointer& unit, std::optional<double> maxValue,
    std::optional<double> minValue, std::optional<double> value, bool isExcerpt)
{
    // RPM value reported under SpeedRPM only
    unit = "/SpeedRPM"_json_pointer;

    if (!isExcerpt)
    {
        // Convert max/min to percent range to match Reading
        sensorJson["ReadingRangeMax"] = 100;
        sensorJson["ReadingRangeMin"] = 0;
    }

    std::optional<long> percentValue =
        getFanPercent(sensorName, maxValue, minValue, value);
    if (percentValue.has_value())
    {
        sensorJson["Reading"] = percentValue;
    }
    else
    {
        // Indication that percent couldn't be computed
        sensorJson["Reading"] = nullptr;
    }
}

inline sensor::ImplementationType dBusSensorImplementationToRedfish(
    const std::string& implementation)
{
    if (implementation ==
        "xyz.openbmc_project.Sensor.Type.ImplementationType.Physical")
    {
        return sensor::ImplementationType::PhysicalSensor;
    }
    if (implementation ==
        "xyz.openbmc_project.Sensor.Type.ImplementationType.Synthesized")
    {
        return sensor::ImplementationType::Synthesized;
    }
    if (implementation ==
        "xyz.openbmc_project.Sensor.Type.ImplementationType.Reported")
    {
        return sensor::ImplementationType::Reported;
    }

    return sensor::ImplementationType::Invalid;
}

inline void fillSensorStatus(
    const dbus::utility::DBusPropertiesMap& propertiesDict,
    nlohmann::json& sensorJson, InventoryItem* inventoryItem)
{
    const bool* checkAvailable = nullptr;
    bool available = true;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), propertiesDict, "Available",
        checkAvailable);
    if (!success)
    {
        messages::internalError();
    }
    if (checkAvailable != nullptr)
    {
        available = *checkAvailable;
    }

    sensorJson["Status"]["State"] = getState(inventoryItem, available);
    sensorJson["Status"]["Health"] =
        getHealth(sensorJson, propertiesDict, inventoryItem);
}

inline bool fillSensorIdentity(
    std::string_view sensorName, std::string_view sensorType,
    const dbus::utility::DBusPropertiesMap& propertiesDict,
    nlohmann::json& sensorJson, bool isExcerpt,
    nlohmann::json::json_pointer& unit)
{
    std::optional<std::string> readingBasis;
    std::optional<std::string> implementation;
    std::optional<Statistics> statistics;
    std::optional<ReadingParameters> readingParameters;
    std::optional<double> maxValue;
    std::optional<double> minValue;
    std::optional<double> value;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), propertiesDict, "ReadingBasis",
        readingBasis, "Implementation", implementation, "Readings", statistics,
        "ReadingParameters", readingParameters, "MaxValue", maxValue,
        "MinValue", minValue, "Value", value);

    if (!success)
    {
        BMCWEB_LOG_ERROR("Failed to unpack properties");
        messages::internalError();
        return false;
    }

    /* Sensor excerpts use different keys to reference the sensor. These are
     * built by the caller.
     * Additionally they don't include these additional properties.
     */
    if (!isExcerpt)
    {
        std::string subNodeEscaped = getSensorId(sensorName, sensorType);
        // For sensors in SensorCollection we set Id instead of MemberId,
        // including power sensors.
        sensorJson["Id"] = std::move(subNodeEscaped);

        std::string sensorNameEs(sensorName);
        std::ranges::replace(sensorNameEs, '_', ' ');
        sensorJson["Name"] = std::move(sensorNameEs);
        sensorJson["@odata.type"] = "#Sensor.v1_11_1.Sensor";

        sensor::ReadingType readingType = sensors::toReadingType(sensorType);
        if (readingType == sensor::ReadingType::Invalid)
        {
            BMCWEB_LOG_ERROR("Redfish cannot map reading type for {}",
                             sensorType);
        }
        else
        {
            sensorJson["ReadingType"] = readingType;
        }

        std::string_view readingUnits = sensors::toReadingUnits(sensorType);
        if (readingUnits.empty())
        {
            BMCWEB_LOG_ERROR("Redfish cannot map reading unit for {}",
                             sensorType);
        }
        else
        {
            sensorJson["ReadingUnits"] = readingUnits;
        }

        if (readingBasis.has_value())
        {
            sensor::ReadingBasisType readingBasisOpt =
                dBusSensorReadingBasisToRedfish(*readingBasis);
            if (readingBasisOpt != sensor::ReadingBasisType::Invalid)
            {
                sensorJson["ReadingBasis"] = readingBasisOpt;
            }
        }

        if (implementation.has_value())
        {
            sensor::ImplementationType implementationOpt =
                dBusSensorImplementationToRedfish(*implementation);
            if (implementationOpt != sensor::ImplementationType::Invalid)
            {
                sensorJson["Implementation"] = implementationOpt;
            }
        }

        updateSensorStatistics(sensorJson, statistics, readingParameters);
    }

    if (sensorType == "fan_tach")
    {
        if constexpr (BMCWEB_REDFISH_ALLOW_ROTATIONAL_FANS)
        {
            if (value.has_value() && std::isfinite(*value))
            {
                sensorJson["SpeedRPM"] = *value;
            }
        }
        else
        {
            // Convert fan's RPM value to Percent value for Reading
            convertToFanPercent(sensorJson, sensorName, unit, maxValue,
                                minValue, value, isExcerpt);
        }
    }

    return true;
}

inline bool fillPowerThermalIdentity(
    std::string_view sensorName, std::string_view sensorType,
    nlohmann::json& sensorJson, InventoryItem* inventoryItem,
    nlohmann::json::json_pointer& unit, bool& forceToInt)
{
    if (sensorType != "power")
    {
        // Set MemberId and Name for non-power sensors.  For PowerSupplies
        // and PowerControl, those properties have more general values
        // because multiple sensors can be stored in the same JSON object.
        std::string sensorNameEs(sensorName);
        std::ranges::replace(sensorNameEs, '_', ' ');
        sensorJson["Name"] = std::move(sensorNameEs);
    }

    if (sensorType == "temperature")
    {
        unit = "/ReadingCelsius"_json_pointer;
        sensorJson["@odata.type"] = "#Thermal.v1_3_0.Temperature";
        // TODO(ed) Documentation says that path should be type fan_tach,
        // implementation seems to implement fan
        return true;
    }

    if (sensorType == "fan" || sensorType == "fan_tach")
    {
        unit = "/Reading"_json_pointer;
        sensorJson["ReadingUnits"] = thermal::ReadingUnits::RPM;
        sensorJson["@odata.type"] = "#Thermal.v1_3_0.Fan";
        if constexpr (BMCWEB_REDFISH_ALLOW_DEPRECATED_INDICATORLED)
        {
            setLedState(sensorJson, inventoryItem);
        }
        forceToInt = true;
        return true;
    }

    if (sensorType == "fan_pwm")
    {
        unit = "/Reading"_json_pointer;
        sensorJson["ReadingUnits"] = thermal::ReadingUnits::Percent;
        sensorJson["@odata.type"] = "#Thermal.v1_3_0.Fan";
        if constexpr (BMCWEB_REDFISH_ALLOW_DEPRECATED_INDICATORLED)
        {
            setLedState(sensorJson, inventoryItem);
        }
        forceToInt = true;
        return true;
    }

    if (sensorType == "voltage")
    {
        unit = "/ReadingVolts"_json_pointer;
        sensorJson["@odata.type"] = "#Power.v1_0_0.Voltage";
        return true;
    }

    if (sensorType == "power")
    {
        std::string lower;
        std::ranges::transform(sensorName, std::back_inserter(lower),
                               bmcweb::asciiToLower);
        if (lower == "total_power")
        {
            sensorJson["@odata.type"] = "#Power.v1_0_0.PowerControl";
            // Put multiple "sensors" into a single PowerControl, so have
            // generic names for MemberId and Name. Follows Redfish mockup.
            sensorJson["MemberId"] = "0";
            sensorJson["Name"] = "Chassis Power Control";
            unit = "/PowerConsumedWatts"_json_pointer;
        }
        else if (lower.find("input") != std::string::npos)
        {
            unit = "/PowerInputWatts"_json_pointer;
        }
        else
        {
            unit = "/PowerOutputWatts"_json_pointer;
        }
        return true;
    }

    BMCWEB_LOG_ERROR("Redfish cannot map object type for {}", sensorName);
    return false;
}

// Map of dbus interface name, dbus property name and redfish property_name
using SensorPropertyMap = std::tuple<std::string_view, std::string_view,
                                     nlohmann::json::json_pointer>;
using SensorPropertyList = std::vector<SensorPropertyMap>;

inline void mapPropertiesBySubnode(
    std::string_view sensorType, ChassisSubNode chassisSubNode,
    SensorPropertyList& properties, nlohmann::json::json_pointer& unit,
    bool isExcerpt)
{
    // unit contains the redfish property_name based on the sensor type/node
    properties.emplace_back("xyz.openbmc_project.Sensor.Value", "Value", unit);

    if (isExcerpt)
    {
        // Excerpts don't have any of these extended properties
        return;
    }

    if (chassisSubNode == ChassisSubNode::sensorsNode)
    {
        properties.emplace_back(
            "xyz.openbmc_project.Sensor.Threshold.Warning", "WarningHigh",
            "/Thresholds/UpperCaution/Reading"_json_pointer);
        properties.emplace_back(
            "xyz.openbmc_project.Sensor.Threshold.Warning", "WarningLow",
            "/Thresholds/LowerCaution/Reading"_json_pointer);
        properties.emplace_back(
            "xyz.openbmc_project.Sensor.Threshold.Critical", "CriticalHigh",
            "/Thresholds/UpperCritical/Reading"_json_pointer);
        properties.emplace_back(
            "xyz.openbmc_project.Sensor.Threshold.Critical", "CriticalLow",
            "/Thresholds/LowerCritical/Reading"_json_pointer);
        properties.emplace_back(
            "xyz.openbmc_project.Sensor.Threshold.HardShutdown",
            "HardShutdownHigh", "/Thresholds/UpperFatal/Reading"_json_pointer);
        properties.emplace_back(
            "xyz.openbmc_project.Sensor.Threshold.HardShutdown",
            "HardShutdownLow", "/Thresholds/LowerFatal/Reading"_json_pointer);
    }
    else if (sensorType != "power")
    {
        properties.emplace_back("xyz.openbmc_project.Sensor.Threshold.Warning",
                                "WarningHigh",
                                "/UpperThresholdNonCritical"_json_pointer);
        properties.emplace_back("xyz.openbmc_project.Sensor.Threshold.Warning",
                                "WarningLow",
                                "/LowerThresholdNonCritical"_json_pointer);
        properties.emplace_back("xyz.openbmc_project.Sensor.Threshold.Critical",
                                "CriticalHigh",
                                "/UpperThresholdCritical"_json_pointer);
        properties.emplace_back("xyz.openbmc_project.Sensor.Threshold.Critical",
                                "CriticalLow",
                                "/LowerThresholdCritical"_json_pointer);
    }

    // TODO Need to get UpperThresholdFatal and LowerThresholdFatal

    if (chassisSubNode == ChassisSubNode::sensorsNode)
    {
        if constexpr (BMCWEB_REDFISH_ALLOW_ROTATIONAL_FANS)
        {
            properties.emplace_back("xyz.openbmc_project.Sensor.Value",
                                    "MinValue",
                                    "/ReadingRangeMin"_json_pointer);
            properties.emplace_back("xyz.openbmc_project.Sensor.Value",
                                    "MaxValue",
                                    "/ReadingRangeMax"_json_pointer);
        }
        else
        {
            /* fan_tach sensors are converted to percent by
             * convertToFanPercent() called from fillSensorIdentity().
             * ReadingRangeMin and ReadingRangeMax converted to percent range in
             * that function as well.
             */
            if (sensorType != "fan_tach")
            {
                properties.emplace_back("xyz.openbmc_project.Sensor.Value",
                                        "MinValue",
                                        "/ReadingRangeMin"_json_pointer);
                properties.emplace_back("xyz.openbmc_project.Sensor.Value",
                                        "MaxValue",
                                        "/ReadingRangeMax"_json_pointer);
            }
        }
        properties.emplace_back("xyz.openbmc_project.Sensor.Accuracy",
                                "Accuracy", "/Accuracy"_json_pointer);
    }
    else if (sensorType == "temperature")
    {
        properties.emplace_back("xyz.openbmc_project.Sensor.Value", "MinValue",
                                "/MinReadingRangeTemp"_json_pointer);
        properties.emplace_back("xyz.openbmc_project.Sensor.Value", "MaxValue",
                                "/MaxReadingRangeTemp"_json_pointer);
    }
    else if (sensorType != "power")
    {
        properties.emplace_back("xyz.openbmc_project.Sensor.Value", "MinValue",
                                "/MinReadingRange"_json_pointer);
        properties.emplace_back("xyz.openbmc_project.Sensor.Value", "MaxValue",
                                "/MaxReadingRange"_json_pointer);
    }
}

/**
 * @brief Builds a json sensor representation of a sensor.
 * @param sensorName  The name of the sensor to be built
 * @param sensorType  The type (temperature, fan_tach, etc) of the sensor to
 * build
 * @param chassisSubNode The subnode (thermal, sensor, etc) of the sensor
 * @param propertiesDict A dictionary of the properties to build the sensor
 * from.
 * @param sensorJson  The json object to fill
 * @param inventoryItem D-Bus inventory item associated with the sensor.  Will
 * be nullptr if no associated inventory item was found.
 */
inline void objectPropertiesToJson(
    std::string_view sensorName, std::string_view sensorType,
    ChassisSubNode chassisSubNode,
    const dbus::utility::DBusPropertiesMap& propertiesDict,
    nlohmann::json& sensorJson, InventoryItem* inventoryItem)
{
    // Parameter to set to override the type we get from dbus, and force it to
    // int, regardless of what is available.  This is used for schemas like fan,
    // that require integers, not floats.
    bool forceToInt = false;

    nlohmann::json::json_pointer unit("/Reading");

    // This ChassisSubNode builds sensor excerpts
    bool isExcerpt = isExcerptNode(chassisSubNode);
    bool filledOk = false;

    if (chassisSubNode == ChassisSubNode::sensorsNode || isExcerpt)
    {
        filledOk = fillSensorIdentity(sensorName, sensorType, propertiesDict,
                                      sensorJson, isExcerpt, unit);
    }
    else
    {
        filledOk =
            fillPowerThermalIdentity(sensorName, sensorType, sensorJson,
                                     inventoryItem, unit, std::ref(forceToInt));
    }
    if (!filledOk)
    {
        return;
    }

    if (!isExcerpt)
    {
        fillSensorStatus(propertiesDict, sensorJson, inventoryItem);
    }

    // Map of dbus interface name, dbus property name and redfish property_name
    SensorPropertyList properties;

    // Add additional property mappings based on the sensor type/node
    mapPropertiesBySubnode(sensorType, chassisSubNode, properties, unit,
                           isExcerpt);

    for (const SensorPropertyMap& p : properties)
    {
        for (const auto& [valueName, valueVariant] : propertiesDict)
        {
            if (valueName != std::get<1>(p))
            {
                continue;
            }

            // The property we want to set may be nested json, so use
            // a json_pointer for easy indexing into the json structure.
            const nlohmann::json::json_pointer& key = std::get<2>(p);

            const double* doubleValue = std::get_if<double>(&valueVariant);
            if (doubleValue == nullptr)
            {
                BMCWEB_LOG_ERROR("Got value interface that wasn't double");
                continue;
            }
            if (!std::isfinite(*doubleValue))
            {
                if (valueName == "Value")
                {
                    // Readings are allowed to be NAN for unavailable;  coerce
                    // them to null in the json response.
                    sensorJson[key] = nullptr;
                    continue;
                }
                BMCWEB_LOG_WARNING("Sensor value for {} was unexpectedly {}",
                                   valueName, *doubleValue);
                continue;
            }
            if (forceToInt)
            {
                sensorJson[key] = static_cast<int64_t>(*doubleValue);
            }
            else
            {
                sensorJson[key] = *doubleValue;
            }
        }
    }
}

/**
 * @brief Builds a json sensor excerpt representation of a sensor.
 *
 * @details This is a wrapper function to provide consistent setting of
 * "DataSourceUri" for sensor excerpts and filling of properties. Since sensor
 * excerpts usually have just the D-Bus path for the sensor that is accepted
 * and used to build "DataSourceUri".

 * @param path The D-Bus path to the sensor to be built
 * @param chassisId The Chassis Id for the sensor
 * @param chassisSubNode The subnode (e.g. ThermalMetrics) of the sensor
 * @param sensorTypeExpected The expected ReadingType of the sensor
 * @param propertiesDict A dictionary of the properties to build the sensor
 * from.
 * @param sensorJson  The json object to fill
 * @returns True if sensorJson object filled. False on any error.
 * Caller is responsible for handling error.
 */
inline bool objectExcerptToJson(
    const std::string& path, const std::string_view chassisId,
    ChassisSubNode chassisSubNode,
    const std::optional<sensor::ReadingType>& sensorTypeExpected,
    const dbus::utility::DBusPropertiesMap& propertiesDict,
    nlohmann::json& sensorJson)
{
    if (!isExcerptNode(chassisSubNode))
    {
        BMCWEB_LOG_DEBUG("{} is not a sensor excerpt",
                         chassisSubNodeToString(chassisSubNode));
        return false;
    }

    sdbusplus::message::object_path sensorPath(path);
    std::string sensorName = sensorPath.filename();
    std::string sensorType = sensorPath.parent_path().filename();
    if (sensorName.empty() || sensorType.empty())
    {
        BMCWEB_LOG_DEBUG("Invalid sensor path {}", path);
        return false;
    }

    if (sensorTypeExpected &&
        (sensors::toReadingType(sensorType) != *sensorTypeExpected))
    {
        BMCWEB_LOG_DEBUG("{} is not expected ReadingType", path);
        return false;
    }

    // Sensor excerpts use DataSourceUri to reference full sensor Redfish path
    sensorJson["DataSourceUri"] =
        boost::urls::format("/redfish/v1/Chassis/{}/Sensors/{}", chassisId,
                            getSensorId(sensorName, sensorType));

    // Fill in sensor excerpt properties
    objectPropertiesToJson(sensorName, sensorType, chassisSubNode,
                           propertiesDict, sensorJson, nullptr);

    return true;
}

// Maps D-Bus: Service, SensorPath
using SensorServicePathMap = std::pair<std::string, std::string>;
using SensorServicePathList = std::vector<SensorServicePathMap>;

inline void getAllSensorObjects(
    const std::string& associatedPath, const std::string& path,
    std::span<const std::string_view> interfaces, const int32_t depth,
    std::function<void(const boost::system::error_code& ec,
                       SensorServicePathList&)>&& callback)
{
    sdbusplus::message::object_path endpointPath{associatedPath};
    endpointPath /= "all_sensors";

    dbus::utility::getAssociatedSubTree(
        endpointPath, sdbusplus::message::object_path(path), depth, interfaces,
        [callback = std::move(callback)](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreeResponse& subtree) {
            SensorServicePathList sensorsServiceAndPath;

            if (ec)
            {
                callback(ec, sensorsServiceAndPath);
                return;
            }

            for (const auto& [sensorPath, serviceMaps] : subtree)
            {
                for (const auto& [service, mapInterfaces] : serviceMaps)
                {
                    sensorsServiceAndPath.emplace_back(service, sensorPath);
                }
            }

            callback(ec, sensorsServiceAndPath);
        });
}

enum class SensorPurpose
{
    totalPower,
    unknownPurpose,
};

constexpr std::string_view sensorPurposeToString(SensorPurpose subNode)
{
    if (subNode == SensorPurpose::totalPower)
    {
        return "xyz.openbmc_project.Sensor.Purpose.SensorPurpose.TotalPower";
    }

    return "Unknown Purpose";
}

inline SensorPurpose sensorPurposeFromString(const std::string& subNodeStr)
{
    // If none match unknownNode is returned
    SensorPurpose subNode = SensorPurpose::unknownPurpose;

    if (subNodeStr ==
        "xyz.openbmc_project.Sensor.Purpose.SensorPurpose.TotalPower")
    {
        subNode = SensorPurpose::totalPower;
    }

    return subNode;
}

inline void checkSensorPurpose(
    const std::string& serviceName, const std::string& sensorPath,
    const SensorPurpose sensorPurpose,
    const std::shared_ptr<SensorServicePathList>& sensorMatches,
    const std::shared_ptr<boost::system::error_code>& asyncErrors,
    const boost::system::error_code& ec,
    const std::vector<std::string>& purposeList)
{
    // If not found this sensor will be skipped but allow to
    // continue processing remaining sensors
    if (ec && (ec != boost::system::errc::io_error) && (ec.value() != EBADR))
    {
        BMCWEB_LOG_DEBUG("D-Bus response error : {}", ec);
        *asyncErrors = ec;
    }

    if (!ec)
    {
        const std::string_view checkPurposeStr =
            sensorPurposeToString(sensorPurpose);

        BMCWEB_LOG_DEBUG("checkSensorPurpose: {}", checkPurposeStr);

        for (const std::string& purposeStr : purposeList)
        {
            if (purposeStr == checkPurposeStr)
            {
                // Add to list
                BMCWEB_LOG_DEBUG("checkSensorPurpose adding {} {}", serviceName,
                                 sensorPath);
                sensorMatches->emplace_back(serviceName, sensorPath);
                return;
            }
        }
    }
}

/**
 * @brief Gets sensors from list with specified purpose
 *
 * Checks the <sensorListIn> sensors for any which implement the specified
 * <sensorPurpose> in interface xyz.openbmc_project.Sensor.Purpose. Adds any
 * matches to the <sensorMatches> list. After checking all sensors in
 * <sensorListIn> the <callback> is called with just the list of matching
 * sensors, which could be an empty list. Additionally the <callback> is called
 * on error and error handling is left to the callback.
 *
 * @param asyncResp Response data
 * @param[in] sensorListIn List of sensors to check
 * @param[in] sensorPurpose Looks for sensors matching this purpose
 * @param[out] sensorMatches Sensors from sensorListIn with sensorPurpose
 * @param[in] callback Callback to handle list of matching sensors
 */
inline void getSensorsByPurpose(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const SensorServicePathList& sensorListIn,
    const SensorPurpose sensorPurpose,
    const std::shared_ptr<SensorServicePathList>& sensorMatches,
    const std::function<void(
        const boost::system::error_code& ec,
        const std::shared_ptr<SensorServicePathList>& sensorMatches)>& callback)
{
    /* Keeps track of number of asynchronous calls made. Once all handlers have
     * been called the callback function is called with the results.
     */
    std::shared_ptr<int> remainingSensorsToVist =
        std::make_shared<int>(sensorListIn.size());

    /* Holds last unrecoverable error returned by any of the asynchronous
     * handlers. Once all handlers have been called the callback will be sent
     * the error to handle.
     */
    std::shared_ptr<boost::system::error_code> asyncErrors =
        std::make_shared<boost::system::error_code>();

    BMCWEB_LOG_DEBUG("getSensorsByPurpose enter {}", *remainingSensorsToVist);

    for (const auto& [serviceName, sensorPath] : sensorListIn)
    {
        dbus::utility::getProperty<std::vector<std::string>>(
            serviceName, sensorPath, "xyz.openbmc_project.Sensor.Purpose",
            "Purpose",
            [asyncResp, serviceName, sensorPath, sensorPurpose, sensorMatches,
             callback, remainingSensorsToVist,
             asyncErrors](const boost::system::error_code& ec,
                          const std::vector<std::string>& purposeList) {
                // Keep track of sensor visited
                (*remainingSensorsToVist)--;
                BMCWEB_LOG_DEBUG("Visited {}. Remaining sensors {}", sensorPath,
                                 *remainingSensorsToVist);

                checkSensorPurpose(serviceName, sensorPath, sensorPurpose,
                                   sensorMatches, asyncErrors, ec, purposeList);

                // All sensors have been visited
                if (*remainingSensorsToVist == 0)
                {
                    BMCWEB_LOG_DEBUG(
                        "getSensorsByPurpose, exit found {} matches",
                        sensorMatches->size());
                    callback(*asyncErrors, sensorMatches);
                    return;
                }
            });
    }
}

} // namespace sensor_utils
} // namespace redfish
