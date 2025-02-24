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
#include <array>
#include <cstddef>

namespace redfish::dbus_registries_map
{

struct ArgInfo
{
    const char* name;
    const char* type;
};

struct EntryInfo
{
    const char* registryName;
    const char* redfishMessageId;
    const std::array<ArgInfo, 10> argsInfo;
    const size_t numberOfArgs;
};

using DbusMessageEntry = std::pair<const char*, const EntryInfo>;

constexpr std::array dbusToRedfishMessageId = {
    DbusMessageEntry{
        "xyz.openbmc_project.Logging.Cleared",
        {
            "OpenBMC_Logging",
            "Cleared",
            {},
            0,
        },
    },
    DbusMessageEntry{
        "xyz.openbmc_project.Sensor.Threshold.InvalidSensorReading",
        {
            "SensorEvent",
            "InvalidSensorReading",
            {
                ArgInfo{"SENSOR_NAME", "sdbusplus::message::object_path"},
            },
            1,
        },
    },
    DbusMessageEntry{
        "xyz.openbmc_project.Sensor.Threshold.ReadingAboveLowerCriticalThreshold",
        {
            "SensorEvent",
            "ReadingAboveLowerCriticalThreshold",
            {
                ArgInfo{"SENSOR_NAME", "sdbusplus::message::object_path"},
                ArgInfo{"READING_VALUE", "double"},
                ArgInfo{
                    "UNITS",
                    "sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit"},
                ArgInfo{"THRESHOLD_VALUE", "double"},
            },
            4,
        },
    },
    DbusMessageEntry{
        "xyz.openbmc_project.Sensor.Threshold.ReadingAboveLowerHardShutdownThreshold",
        {
            "SensorEvent",
            "ReadingAboveLowerFatalThreshold",
            {
                ArgInfo{"SENSOR_NAME", "sdbusplus::message::object_path"},
                ArgInfo{"READING_VALUE", "double"},
                ArgInfo{
                    "UNITS",
                    "sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit"},
                ArgInfo{"THRESHOLD_VALUE", "double"},
            },
            4,
        },
    },
    DbusMessageEntry{
        "xyz.openbmc_project.Sensor.Threshold.ReadingAboveLowerSoftShutdownThreshold",
        {
            "SensorEvent",
            "ReadingAboveLowerFatalThreshold",
            {
                ArgInfo{"SENSOR_NAME", "sdbusplus::message::object_path"},
                ArgInfo{"READING_VALUE", "double"},
                ArgInfo{
                    "UNITS",
                    "sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit"},
                ArgInfo{"THRESHOLD_VALUE", "double"},
            },
            4,
        },
    },
    DbusMessageEntry{
        "xyz.openbmc_project.Sensor.Threshold.ReadingAboveUpperCriticalThreshold",
        {
            "SensorEvent",
            "ReadingAboveUpperCriticalThreshold",
            {
                ArgInfo{"SENSOR_NAME", "sdbusplus::message::object_path"},
                ArgInfo{"READING_VALUE", "double"},
                ArgInfo{
                    "UNITS",
                    "sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit"},
                ArgInfo{"THRESHOLD_VALUE", "double"},
            },
            4,
        },
    },
    DbusMessageEntry{
        "xyz.openbmc_project.Sensor.Threshold.ReadingAboveUpperHardShutdownThreshold",
        {
            "SensorEvent",
            "ReadingAboveUpperFatalThreshold",
            {
                ArgInfo{"SENSOR_NAME", "sdbusplus::message::object_path"},
                ArgInfo{"READING_VALUE", "double"},
                ArgInfo{
                    "UNITS",
                    "sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit"},
                ArgInfo{"THRESHOLD_VALUE", "double"},
            },
            4,
        },
    },
    DbusMessageEntry{
        "xyz.openbmc_project.Sensor.Threshold.ReadingAboveUpperPerformanceLossThreshold",
        {
            "SensorEvent",
            "ReadingAboveUpperCautionThreshold",
            {
                ArgInfo{"SENSOR_NAME", "sdbusplus::message::object_path"},
                ArgInfo{"READING_VALUE", "double"},
                ArgInfo{
                    "UNITS",
                    "sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit"},
                ArgInfo{"THRESHOLD_VALUE", "double"},
            },
            4,
        },
    },
    DbusMessageEntry{
        "xyz.openbmc_project.Sensor.Threshold.ReadingAboveUpperSoftShutdownThreshold",
        {
            "SensorEvent",
            "ReadingAboveUpperFatalThreshold",
            {
                ArgInfo{"SENSOR_NAME", "sdbusplus::message::object_path"},
                ArgInfo{"READING_VALUE", "double"},
                ArgInfo{
                    "UNITS",
                    "sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit"},
                ArgInfo{"THRESHOLD_VALUE", "double"},
            },
            4,
        },
    },
    DbusMessageEntry{
        "xyz.openbmc_project.Sensor.Threshold.ReadingAboveUpperWarningThreshold",
        {
            "SensorEvent",
            "ReadingAboveUpperCautionThreshold",
            {
                ArgInfo{"SENSOR_NAME", "sdbusplus::message::object_path"},
                ArgInfo{"READING_VALUE", "double"},
                ArgInfo{
                    "UNITS",
                    "sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit"},
                ArgInfo{"THRESHOLD_VALUE", "double"},
            },
            4,
        },
    },
    DbusMessageEntry{
        "xyz.openbmc_project.Sensor.Threshold.ReadingBelowLowerCriticalThreshold",
        {
            "SensorEvent",
            "ReadingBelowLowerCriticalThreshold",
            {
                ArgInfo{"SENSOR_NAME", "sdbusplus::message::object_path"},
                ArgInfo{"READING_VALUE", "double"},
                ArgInfo{
                    "UNITS",
                    "sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit"},
                ArgInfo{"THRESHOLD_VALUE", "double"},
            },
            4,
        },
    },
    DbusMessageEntry{
        "xyz.openbmc_project.Sensor.Threshold.ReadingBelowLowerHardShutdownThreshold",
        {
            "SensorEvent",
            "ReadingBelowLowerFatalThreshold",
            {
                ArgInfo{"SENSOR_NAME", "sdbusplus::message::object_path"},
                ArgInfo{"READING_VALUE", "double"},
                ArgInfo{
                    "UNITS",
                    "sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit"},
                ArgInfo{"THRESHOLD_VALUE", "double"},
            },
            4,
        },
    },
    DbusMessageEntry{
        "xyz.openbmc_project.Sensor.Threshold.ReadingBelowLowerPerformanceLossThreshold",
        {
            "SensorEvent",
            "ReadingBelowLowerCautionThreshold",
            {
                ArgInfo{"SENSOR_NAME", "sdbusplus::message::object_path"},
                ArgInfo{"READING_VALUE", "double"},
                ArgInfo{
                    "UNITS",
                    "sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit"},
                ArgInfo{"THRESHOLD_VALUE", "double"},
            },
            4,
        },
    },
    DbusMessageEntry{
        "xyz.openbmc_project.Sensor.Threshold.ReadingBelowLowerSoftShutdownThreshold",
        {
            "SensorEvent",
            "ReadingBelowLowerFatalThreshold",
            {
                ArgInfo{"SENSOR_NAME", "sdbusplus::message::object_path"},
                ArgInfo{"READING_VALUE", "double"},
                ArgInfo{
                    "UNITS",
                    "sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit"},
                ArgInfo{"THRESHOLD_VALUE", "double"},
            },
            4,
        },
    },
    DbusMessageEntry{
        "xyz.openbmc_project.Sensor.Threshold.ReadingBelowLowerWarningThreshold",
        {
            "SensorEvent",
            "ReadingBelowLowerCautionThreshold",
            {
                ArgInfo{"SENSOR_NAME", "sdbusplus::message::object_path"},
                ArgInfo{"READING_VALUE", "double"},
                ArgInfo{
                    "UNITS",
                    "sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit"},
                ArgInfo{"THRESHOLD_VALUE", "double"},
            },
            4,
        },
    },
    DbusMessageEntry{
        "xyz.openbmc_project.Sensor.Threshold.ReadingBelowUpperCriticalThreshold",
        {
            "SensorEvent",
            "ReadingBelowUpperCriticalThreshold",
            {
                ArgInfo{"SENSOR_NAME", "sdbusplus::message::object_path"},
                ArgInfo{"READING_VALUE", "double"},
                ArgInfo{
                    "UNITS",
                    "sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit"},
                ArgInfo{"THRESHOLD_VALUE", "double"},
            },
            4,
        },
    },
    DbusMessageEntry{
        "xyz.openbmc_project.Sensor.Threshold.ReadingBelowUpperHardShutdownThreshold",
        {
            "SensorEvent",
            "ReadingBelowFatalThreshold",
            {
                ArgInfo{"SENSOR_NAME", "sdbusplus::message::object_path"},
                ArgInfo{"READING_VALUE", "double"},
                ArgInfo{
                    "UNITS",
                    "sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit"},
                ArgInfo{"THRESHOLD_VALUE", "double"},
            },
            4,
        },
    },
    DbusMessageEntry{
        "xyz.openbmc_project.Sensor.Threshold.ReadingBelowUpperSoftShutdownThreshold",
        {
            "SensorEvent",
            "ReadingBelowFatalThreshold",
            {
                ArgInfo{"SENSOR_NAME", "sdbusplus::message::object_path"},
                ArgInfo{"READING_VALUE", "double"},
                ArgInfo{
                    "UNITS",
                    "sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit"},
                ArgInfo{"THRESHOLD_VALUE", "double"},
            },
            4,
        },
    },
    DbusMessageEntry{
        "xyz.openbmc_project.Sensor.Threshold.SensorFailure",
        {
            "SensorEvent",
            "SensorFailure",
            {
                ArgInfo{"SENSOR_NAME", "sdbusplus::message::object_path"},
            },
            1,
        },
    },
    DbusMessageEntry{
        "xyz.openbmc_project.Sensor.Threshold.SensorReadingNormalRange",
        {
            "SensorEvent",
            "SensorReadingNormalRange",
            {
                ArgInfo{"SENSOR_NAME", "sdbusplus::message::object_path"},
                ArgInfo{"READING_VALUE", "double"},
                ArgInfo{
                    "UNITS",
                    "sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit"},
            },
            3,
        },
    },
    DbusMessageEntry{
        "xyz.openbmc_project.Sensor.Threshold.SensorRestored",
        {
            "SensorEvent",
            "SensorRestored",
            {
                ArgInfo{"SENSOR_NAME", "sdbusplus::message::object_path"},
            },
            1,
        },
    },
    DbusMessageEntry{
        "xyz.openbmc_project.State.Cable.CableConnected",
        {
            "OpenBMC_StateCable",
            "CableConnected",
            {
                ArgInfo{"PORT_ID", "std::string"},
            },
            1,
        },
    },
    DbusMessageEntry{
        "xyz.openbmc_project.State.Cable.CableDisconnected",
        {
            "OpenBMC_StateCable",
            "CableDisconnected",
            {
                ArgInfo{"PORT_ID", "std::string"},
            },
            1,
        },
    },
    DbusMessageEntry{
        "xyz.openbmc_project.State.Leak.Detector.LeakDetectedCritical",
        {
            "Environmental",
            "LeakDetectedCritical",
            {
                ArgInfo{"DETECTOR_NAME", "sdbusplus::message::object_path"},
            },
            1,
        },
    },
    DbusMessageEntry{
        "xyz.openbmc_project.State.Leak.Detector.LeakDetectedNormal",
        {
            "Environmental",
            "LeakDetectedNormal",
            {
                ArgInfo{"DETECTOR_NAME", "sdbusplus::message::object_path"},
            },
            1,
        },
    },
    DbusMessageEntry{
        "xyz.openbmc_project.State.Leak.Detector.LeakDetectedWarning",
        {
            "Environmental",
            "LeakDetectedWarning",
            {
                ArgInfo{"DETECTOR_NAME", "sdbusplus::message::object_path"},
            },
            1,
        },
    },
    DbusMessageEntry{
        "xyz.openbmc_project.State.Leak.DetectorGroup.DetectorGroupCritical",
        {
            "OpenBMC_StateLeakDetectorGroup",
            "DetectorGroupCritical",
            {
                ArgInfo{"DETECTOR_GROUP_NAME",
                        "sdbusplus::message::object_path"},
            },
            1,
        },
    },
    DbusMessageEntry{
        "xyz.openbmc_project.State.Leak.DetectorGroup.DetectorGroupNormal",
        {
            "OpenBMC_StateLeakDetectorGroup",
            "DetectorGroupNormal",
            {
                ArgInfo{"DETECTOR_GROUP_NAME",
                        "sdbusplus::message::object_path"},
            },
            1,
        },
    },
    DbusMessageEntry{
        "xyz.openbmc_project.State.Leak.DetectorGroup.DetectorGroupWarning",
        {
            "OpenBMC_StateLeakDetectorGroup",
            "DetectorGroupWarning",
            {
                ArgInfo{"DETECTOR_GROUP_NAME",
                        "sdbusplus::message::object_path"},
            },
            1,
        },
    },
};
} // namespace redfish::dbus_registries_map
