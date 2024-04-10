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
#include "registries.hpp"

#include <array>

// clang-format off

namespace redfish::registries::sensor_event
{
const Header header = {
    "Copyright 2022-2023 DMTF. All rights reserved.",
    "#MessageRegistry.v1_6_2.MessageRegistry",
    "SensorEvent.1.0.1",
    "Sensor Event Message Registry",
    "en",
    "This registry defines messages used for general events related to Sensor resources.",
    "SensorEvent",
    "1.0.1",
    "DMTF",
};
constexpr const char* url =
    "https://redfish.dmtf.org/registries/SensorEvent.1.0.1.json";

constexpr std::array registry =
{
    MessageEntry{
        "InvalidSensorReading",
        {
            "Indicates that the service received an invalid reading from a sensor.",
            "Invalid reading received from sensor '%1'.",
            "Warning",
            1,
            {
                "string",
            },
            "Check the sensor hardware or connection.",
        }},
    MessageEntry{
        "ReadingAboveLowerCriticalThreshold",
        {
            "Indicates that a sensor reading is no longer below the lower critical threshold but is still outside of normal operating range.",
            "Sensor '%1' reading of %2 (%3) is now above the %4 lower critical threshold but remains outside of normal range.",
            "Warning",
            4,
            {
                "string",
                "number",
                "string",
                "number",
            },
            "Check the condition of the resources listed in RelatedItem.",
        }},
    MessageEntry{
        "ReadingAboveLowerFatalThreshold",
        {
            "Indicates that a sensor reading is no longer below the lower fatal threshold but is still outside of normal operating range.",
            "Sensor '%1' reading of %2 (%3) is now above the %4 lower fatal threshold but remains outside of normal range.",
            "Critical",
            4,
            {
                "string",
                "number",
                "string",
                "number",
            },
            "Check the condition of the resources listed in RelatedItem.",
        }},
    MessageEntry{
        "ReadingAboveUpperCautionThreshold",
        {
            "Indicates that a sensor reading is above the upper caution threshold.",
            "Sensor '%1' reading of %2 (%3) is above the %4 upper caution threshold.",
            "Warning",
            4,
            {
                "string",
                "number",
                "string",
                "number",
            },
            "Check the condition of the resources listed in RelatedItem.",
        }},
    MessageEntry{
        "ReadingAboveUpperCriticalThreshold",
        {
            "Indicates that a sensor reading is above the upper critical threshold.",
            "Sensor '%1' reading of %2 (%3) is above the %4 upper critical threshold.",
            "Critical",
            4,
            {
                "string",
                "number",
                "string",
                "number",
            },
            "Check the condition of the resources listed in RelatedItem.",
        }},
    MessageEntry{
        "ReadingAboveUpperFatalThreshold",
        {
            "Indicates that a sensor reading is above the upper fatal threshold.",
            "Sensor '%1' reading of %2 (%3) is above the %4 upper fatal threshold.",
            "Critical",
            4,
            {
                "string",
                "number",
                "string",
                "number",
            },
            "Check the condition of the resources listed in RelatedItem.",
        }},
    MessageEntry{
        "ReadingBelowLowerCautionThreshold",
        {
            "Indicates that a sensor reading is below the lower caution threshold.",
            "Sensor '%1' reading of %2 (%3) is below the %4 lower caution threshold.",
            "Warning",
            4,
            {
                "string",
                "number",
                "string",
                "number",
            },
            "Check the condition of the resources listed in RelatedItem.",
        }},
    MessageEntry{
        "ReadingBelowLowerCriticalThreshold",
        {
            "Indicates that a sensor reading is below the lower critical threshold.",
            "Sensor '%1' reading of %2 (%3) is below the %4 lower critical threshold.",
            "Critical",
            4,
            {
                "string",
                "number",
                "string",
                "number",
            },
            "Check the condition of the resources listed in RelatedItem.",
        }},
    MessageEntry{
        "ReadingBelowLowerFatalThreshold",
        {
            "Indicates that a sensor reading is below the lower fatal threshold.",
            "Sensor '%1' reading of %2 (%3) is below the %4 lower fatal threshold.",
            "Critical",
            4,
            {
                "string",
                "number",
                "string",
                "number",
            },
            "Check the condition of the resources listed in RelatedItem.",
        }},
    MessageEntry{
        "ReadingBelowUpperCriticalThreshold",
        {
            "Indicates that a sensor reading is no longer above the upper critical threshold but is still outside of normal operating range.",
            "Sensor '%1' reading of %2 (%3) is now below the %4 upper critical threshold but remains outside of normal range.",
            "Warning",
            4,
            {
                "string",
                "number",
                "string",
                "number",
            },
            "Check the condition of the resources listed in RelatedItem.",
        }},
    MessageEntry{
        "ReadingBelowUpperFatalThreshold",
        {
            "Indicates that a sensor reading is no longer above the upper fatal threshold but is still outside of normal operating range.",
            "Sensor '%1' reading of %2 (%3) is now below the %4 upper fatal threshold but remains outside of normal range.",
            "Critical",
            4,
            {
                "string",
                "number",
                "string",
                "number",
            },
            "Check the condition of the resources listed in RelatedItem.",
        }},
    MessageEntry{
        "ReadingCritical",
        {
            "Indicates that a sensor reading exceeds an internal critical level.",
            "Sensor '%1' reading of %2 (%3) exceeds the critical level.",
            "Critical",
            3,
            {
                "string",
                "number",
                "string",
            },
            "Check the condition of the resources listed in RelatedItem.",
        }},
    MessageEntry{
        "ReadingNoLongerCritical",
        {
            "Indicates that a sensor reading no longer exceeds an internal critical level but still exceeds an internal warning level.",
            "Sensor '%1' reading of %2 (%3) no longer exceeds the critical level.",
            "Warning",
            3,
            {
                "string",
                "number",
                "string",
            },
            "Check the condition of the resources listed in RelatedItem.",
        }},
    MessageEntry{
        "ReadingWarning",
        {
            "Indicates that a sensor reading exceeds an internal warning level.",
            "Sensor '%1' reading of %2 (%3) exceeds the warning level.",
            "Warning",
            3,
            {
                "string",
                "number",
                "string",
            },
            "Check the condition of the resources listed in RelatedItem.",
        }},
    MessageEntry{
        "SensorFailure",
        {
            "Indicates that the service cannot communicate with a sensor or has detected a failure.",
            "Sensor '%1' has failed.",
            "Warning",
            1,
            {
                "string",
            },
            "Check the sensor hardware or connection.",
        }},
    MessageEntry{
        "SensorReadingNormalRange",
        {
            "Indicates that a sensor reading is now within normal operating range.",
            "Sensor '%1' reading of %2 (%3) is within normal operating range.",
            "OK",
            3,
            {
                "string",
                "number",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "SensorRestored",
        {
            "Indicates that a sensor was repaired or communications were restored.  It may also indicate that the service is receiving valid data from a sensor.",
            "Sensor '%1' was restored.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},

};

enum class Index
{
    invalidSensorReading = 0,
    readingAboveLowerCriticalThreshold = 1,
    readingAboveLowerFatalThreshold = 2,
    readingAboveUpperCautionThreshold = 3,
    readingAboveUpperCriticalThreshold = 4,
    readingAboveUpperFatalThreshold = 5,
    readingBelowLowerCautionThreshold = 6,
    readingBelowLowerCriticalThreshold = 7,
    readingBelowLowerFatalThreshold = 8,
    readingBelowUpperCriticalThreshold = 9,
    readingBelowUpperFatalThreshold = 10,
    readingCritical = 11,
    readingNoLongerCritical = 12,
    readingWarning = 13,
    sensorFailure = 14,
    sensorReadingNormalRange = 15,
    sensorRestored = 16,
};
} // namespace redfish::registries::sensor_event
