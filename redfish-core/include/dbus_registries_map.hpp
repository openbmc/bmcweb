// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
/****************************************************************
 *                 READ THIS WARNING FIRST
 * This is an auto-generated header which contains definitions
 * for Redfish DMTF defined messages.
 * DO NOT modify this registry outside of running the
 * parse_registries.py script.  The definitions contained within
 * this file are owned by DMTF.  Any modifications to these files
 * should be first pushed to the relevant registry in the DMTF
 * github organization.
 ***************************************************************/
#include <string_view>
#include <unordered_map>
#include <vector>

namespace redfish::dbus_registries_map
{

constexpr const std::string_view dbusNamespacePrefix{"xyz.openbmc_project."};

struct EntryInfo
{
    const char* RegistryName;
    const char* RedfishMessageId;
    const std::vector<std::pair<const std::string_view, const std::string_view>>
        ArgsInfo;
};

// The key is the dbus event name without DBUS_NAMESPACE_PREFIX prefix
const std::unordered_map<std::string_view, const EntryInfo> dbusToRedfishMessageId = {
    {
        "Logging.Cleared",
        EntryInfo{
            "OpenBMC_Logging",
            "Cleared",
            {},
        },
    },
    {
        "Sensor.Threshold.InvalidSensorReading",
        EntryInfo{"SensorEvent",
                  "InvalidSensorReading",
                  {
                      {"SENSOR_NAME", "sdbusplus::message::object_path"},
                  }},
    },
    {
        "Sensor.Threshold.ReadingAboveLowerCriticalThreshold",
        EntryInfo{
            "SensorEvent",
            "ReadingAboveLowerCriticalThreshold",
            {
                {"SENSOR_NAME", "sdbusplus::message::object_path"},
                {"READING_VALUE", "double"},
                {"UNITS",
                 "sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit"},
                {"THRESHOLD_VALUE", "double"},
            }},
    },
    {
        "Sensor.Threshold.ReadingAboveLowerHardShutdownThreshold",
        EntryInfo{
            "SensorEvent",
            "ReadingAboveLowerFatalThreshold",
            {
                {"SENSOR_NAME", "sdbusplus::message::object_path"},
                {"READING_VALUE", "double"},
                {"UNITS",
                 "sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit"},
                {"THRESHOLD_VALUE", "double"},
            }},
    },
    {
        "Sensor.Threshold.ReadingAboveLowerSoftShutdownThreshold",
        EntryInfo{
            "SensorEvent",
            "ReadingAboveLowerFatalThreshold",
            {
                {"SENSOR_NAME", "sdbusplus::message::object_path"},
                {"READING_VALUE", "double"},
                {"UNITS",
                 "sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit"},
                {"THRESHOLD_VALUE", "double"},
            }},
    },
    {
        "Sensor.Threshold.ReadingAboveUpperCriticalThreshold",
        EntryInfo{
            "SensorEvent",
            "ReadingAboveUpperCriticalThreshold",
            {
                {"SENSOR_NAME", "sdbusplus::message::object_path"},
                {"READING_VALUE", "double"},
                {"UNITS",
                 "sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit"},
                {"THRESHOLD_VALUE", "double"},
            }},
    },
    {
        "Sensor.Threshold.ReadingAboveUpperHardShutdownThreshold",
        EntryInfo{
            "SensorEvent",
            "ReadingAboveUpperFatalThreshold",
            {
                {"SENSOR_NAME", "sdbusplus::message::object_path"},
                {"READING_VALUE", "double"},
                {"UNITS",
                 "sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit"},
                {"THRESHOLD_VALUE", "double"},
            }},
    },
    {
        "Sensor.Threshold.ReadingAboveUpperPerformanceLossThreshold",
        EntryInfo{
            "SensorEvent",
            "ReadingAboveUpperCautionThreshold",
            {
                {"SENSOR_NAME", "sdbusplus::message::object_path"},
                {"READING_VALUE", "double"},
                {"UNITS",
                 "sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit"},
                {"THRESHOLD_VALUE", "double"},
            }},
    },
    {
        "Sensor.Threshold.ReadingAboveUpperSoftShutdownThreshold",
        EntryInfo{
            "SensorEvent",
            "ReadingAboveUpperFatalThreshold",
            {
                {"SENSOR_NAME", "sdbusplus::message::object_path"},
                {"READING_VALUE", "double"},
                {"UNITS",
                 "sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit"},
                {"THRESHOLD_VALUE", "double"},
            }},
    },
    {
        "Sensor.Threshold.ReadingAboveUpperWarningThreshold",
        EntryInfo{
            "SensorEvent",
            "ReadingAboveUpperCautionThreshold",
            {
                {"SENSOR_NAME", "sdbusplus::message::object_path"},
                {"READING_VALUE", "double"},
                {"UNITS",
                 "sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit"},
                {"THRESHOLD_VALUE", "double"},
            }},
    },
    {
        "Sensor.Threshold.ReadingBelowLowerCriticalThreshold",
        EntryInfo{
            "SensorEvent",
            "ReadingBelowLowerCriticalThreshold",
            {
                {"SENSOR_NAME", "sdbusplus::message::object_path"},
                {"READING_VALUE", "double"},
                {"UNITS",
                 "sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit"},
                {"THRESHOLD_VALUE", "double"},
            }},
    },
    {
        "Sensor.Threshold.ReadingBelowLowerHardShutdownThreshold",
        EntryInfo{
            "SensorEvent",
            "ReadingBelowLowerFatalThreshold",
            {
                {"SENSOR_NAME", "sdbusplus::message::object_path"},
                {"READING_VALUE", "double"},
                {"UNITS",
                 "sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit"},
                {"THRESHOLD_VALUE", "double"},
            }},
    },
    {
        "Sensor.Threshold.ReadingBelowLowerPerformanceLossThreshold",
        EntryInfo{
            "SensorEvent",
            "ReadingBelowLowerCautionThreshold",
            {
                {"SENSOR_NAME", "sdbusplus::message::object_path"},
                {"READING_VALUE", "double"},
                {"UNITS",
                 "sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit"},
                {"THRESHOLD_VALUE", "double"},
            }},
    },
    {
        "Sensor.Threshold.ReadingBelowLowerSoftShutdownThreshold",
        EntryInfo{
            "SensorEvent",
            "ReadingBelowLowerFatalThreshold",
            {
                {"SENSOR_NAME", "sdbusplus::message::object_path"},
                {"READING_VALUE", "double"},
                {"UNITS",
                 "sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit"},
                {"THRESHOLD_VALUE", "double"},
            }},
    },
    {
        "Sensor.Threshold.ReadingBelowLowerWarningThreshold",
        EntryInfo{
            "SensorEvent",
            "ReadingBelowLowerCautionThreshold",
            {
                {"SENSOR_NAME", "sdbusplus::message::object_path"},
                {"READING_VALUE", "double"},
                {"UNITS",
                 "sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit"},
                {"THRESHOLD_VALUE", "double"},
            }},
    },
    {
        "Sensor.Threshold.ReadingBelowUpperCriticalThreshold",
        EntryInfo{
            "SensorEvent",
            "ReadingBelowUpperCriticalThreshold",
            {
                {"SENSOR_NAME", "sdbusplus::message::object_path"},
                {"READING_VALUE", "double"},
                {"UNITS",
                 "sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit"},
                {"THRESHOLD_VALUE", "double"},
            }},
    },
    {
        "Sensor.Threshold.ReadingBelowUpperHardShutdownThreshold",
        EntryInfo{
            "SensorEvent",
            "ReadingBelowFatalThreshold",
            {
                {"SENSOR_NAME", "sdbusplus::message::object_path"},
                {"READING_VALUE", "double"},
                {"UNITS",
                 "sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit"},
                {"THRESHOLD_VALUE", "double"},
            }},
    },
    {
        "Sensor.Threshold.ReadingBelowUpperSoftShutdownThreshold",
        EntryInfo{
            "SensorEvent",
            "ReadingBelowFatalThreshold",
            {
                {"SENSOR_NAME", "sdbusplus::message::object_path"},
                {"READING_VALUE", "double"},
                {"UNITS",
                 "sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit"},
                {"THRESHOLD_VALUE", "double"},
            }},
    },
    {
        "Sensor.Threshold.SensorFailure",
        EntryInfo{"SensorEvent",
                  "SensorFailure",
                  {
                      {"SENSOR_NAME", "sdbusplus::message::object_path"},
                  }},
    },
    {
        "Sensor.Threshold.SensorReadingNormalRange",
        EntryInfo{
            "SensorEvent",
            "SensorReadingNormalRange",
            {
                {"SENSOR_NAME", "sdbusplus::message::object_path"},
                {"READING_VALUE", "double"},
                {"UNITS",
                 "sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit"},
            }},
    },
    {
        "Sensor.Threshold.SensorRestored",
        EntryInfo{"SensorEvent",
                  "SensorRestored",
                  {
                      {"SENSOR_NAME", "sdbusplus::message::object_path"},
                  }},
    },
    {
        "State.Cable.CableConnected",
        EntryInfo{"OpenBMC_StateCable",
                  "CableConnected",
                  {
                      {"PORT_ID", "std::string"},
                  }},
    },
    {
        "State.Cable.CableDisconnected",
        EntryInfo{"OpenBMC_StateCable",
                  "CableDisconnected",
                  {
                      {"PORT_ID", "std::string"},
                  }},
    },
    {
        "State.Leak.Detector.LeakDetectedCritical",
        EntryInfo{"Environmental",
                  "LeakDetectedCritical",
                  {
                      {"DETECTOR_NAME", "sdbusplus::message::object_path"},
                  }},
    },
    {
        "State.Leak.Detector.LeakDetectedNormal",
        EntryInfo{"Environmental",
                  "LeakDetectedNormal",
                  {
                      {"DETECTOR_NAME", "sdbusplus::message::object_path"},
                  }},
    },
    {
        "State.Leak.Detector.LeakDetectedWarning",
        EntryInfo{"Environmental",
                  "LeakDetectedWarning",
                  {
                      {"DETECTOR_NAME", "sdbusplus::message::object_path"},
                  }},
    },
    {
        "State.Leak.DetectorGroup.DetectorGroupCritical",
        EntryInfo{
            "OpenBMC_StateLeakDetectorGroup",
            "DetectorGroupCritical",
            {
                {"DETECTOR_GROUP_NAME", "sdbusplus::message::object_path"},
            }},
    },
    {
        "State.Leak.DetectorGroup.DetectorGroupNormal",
        EntryInfo{
            "OpenBMC_StateLeakDetectorGroup",
            "DetectorGroupNormal",
            {
                {"DETECTOR_GROUP_NAME", "sdbusplus::message::object_path"},
            }},
    },
    {
        "State.Leak.DetectorGroup.DetectorGroupWarning",
        EntryInfo{
            "OpenBMC_StateLeakDetectorGroup",
            "DetectorGroupWarning",
            {
                {"DETECTOR_GROUP_NAME", "sdbusplus::message::object_path"},
            }},
    },
};
} // namespace redfish::dbus_registries_map
