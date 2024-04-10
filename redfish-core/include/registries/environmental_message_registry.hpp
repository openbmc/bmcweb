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

namespace redfish::registries::environmental
{
const Header header = {
    "Copyright 2023 DMTF. All rights reserved.",
    "#MessageRegistry.v1_6_2.MessageRegistry",
    "Environmental.1.0.1",
    "Environmental Message Registry",
    "en",
    "This registry defines messages related to environmental sensors, heating and cooling equipment, or other environmental conditions.",
    "Environmental",
    "1.0.1",
    "DMTF",
};
constexpr const char* url =
    "https://redfish.dmtf.org/registries/Environmental.1.0.1.json";

constexpr std::array registry =
{
    MessageEntry{
        "FanFailed",
        {
            "Indicates that a fan has failed.",
            "Fan '%1' has failed.",
            "Warning",
            1,
            {
                "string",
            },
            "Check the fan hardware and replace any faulty component.",
        }},
    MessageEntry{
        "FanGroupCritical",
        {
            "Indicates that a fan group has a critical status.",
            "Fan group '%1' is in a critical state.",
            "Critical",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "FanGroupNormal",
        {
            "Indicates that a fan group has returned to normal operations.",
            "Fan group '%1' is operating normally.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "FanGroupWarning",
        {
            "Indicates that a fan group has a warning status.",
            "Fan group '%1' is in a warning state.",
            "Warning",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "FanInserted",
        {
            "Indicates that a fan was inserted or installed.",
            "Fan '%1' was inserted.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "FanRemoved",
        {
            "Indicates that a fan was removed.",
            "Fan '%1' was removed.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "FanRestored",
        {
            "Indicates that a fan was repaired or restored to normal operation.",
            "Fan '%1' was restored.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "HumidityAboveLowerCriticalThreshold",
        {
            "Indicates that a humidity reading is no longer below the lower critical threshold but is still outside of normal operating range.",
            "Humidity '%1' reading of %2 percent is now above the %3 lower critical threshold but remains outside of normal range.",
            "Warning",
            3,
            {
                "string",
                "number",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "HumidityAboveUpperCautionThreshold",
        {
            "Indicates that a humidity reading is above the upper caution threshold.",
            "Humidity '%1' reading of %2 percent is above the %3 upper caution threshold.",
            "Warning",
            3,
            {
                "string",
                "number",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "HumidityAboveUpperCriticalThreshold",
        {
            "Indicates that a humidity reading is above the upper critical threshold.",
            "Humidity '%1' reading of %2 percent is above the %3 upper critical threshold.",
            "Critical",
            3,
            {
                "string",
                "number",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "HumidityBelowLowerCautionThreshold",
        {
            "Indicates that a humidity reading is below the lower caution threshold.",
            "Humidity '%1' reading of %2 percent is below the %3 lower caution threshold.",
            "Warning",
            3,
            {
                "string",
                "number",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "HumidityBelowLowerCriticalThreshold",
        {
            "Indicates that a humidity reading is below the lower critical threshold.",
            "Humidity '%1' reading of %2 percent is below the %3 lower critical threshold.",
            "Critical",
            3,
            {
                "string",
                "number",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "HumidityBelowUpperCriticalThreshold",
        {
            "Indicates that a humidity reading is no longer above the upper critical threshold but is still outside of normal operating range.",
            "Humidity '%1' reading of %2 percent is now below the %3 upper critical threshold but remains outside of normal range.",
            "Warning",
            3,
            {
                "string",
                "number",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "HumidityNormal",
        {
            "Indicates that a humidity reading is now within normal operating range.",
            "Humidity '%1' reading of %2 percent is within normal operating range.",
            "OK",
            2,
            {
                "string",
                "number",
            },
            "None.",
        }},
    MessageEntry{
        "TemperatureAboveLowerCriticalThreshold",
        {
            "Indicates that a temperature reading is no longer below the lower critical threshold but is still outside of normal operating range.",
            "Temperature '%1' reading of %2 degrees (C) is now above the %3 lower critical threshold but remains outside of normal range.",
            "Warning",
            3,
            {
                "string",
                "number",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "TemperatureAboveLowerFatalThreshold",
        {
            "Indicates that a temperature reading is no longer below the lower fatal threshold but is still outside of normal operating range.",
            "Temperature '%1' reading of %2 degrees (C) is now above the %3 lower fatal threshold but remains outside of normal range.",
            "Critical",
            3,
            {
                "string",
                "number",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "TemperatureAboveUpperCautionThreshold",
        {
            "Indicates that a temperature reading is above the upper caution threshold.",
            "Temperature '%1' reading of %2 degrees (C) is above the %3 upper caution threshold.",
            "Warning",
            3,
            {
                "string",
                "number",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "TemperatureAboveUpperCriticalThreshold",
        {
            "Indicates that a temperature reading is above the upper critical threshold.",
            "Temperature '%1' reading of %2 degrees (C) is above the %3 upper critical threshold.",
            "Critical",
            3,
            {
                "string",
                "number",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "TemperatureAboveUpperFatalThreshold",
        {
            "Indicates that a temperature reading is above the upper fatal threshold.",
            "Temperature '%1' reading of %2 degrees (C) is above the %3 upper fatal threshold.",
            "Critical",
            3,
            {
                "string",
                "number",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "TemperatureBelowLowerCautionThreshold",
        {
            "Indicates that a temperature reading is below the lower caution threshold.",
            "Temperature '%1' reading of %2 degrees (C) is below the %3 lower caution threshold.",
            "Warning",
            3,
            {
                "string",
                "number",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "TemperatureBelowLowerCriticalThreshold",
        {
            "Indicates that a temperature reading is below the lower critical threshold.",
            "Temperature '%1' reading of %2 degrees (C) is below the %3 lower critical threshold.",
            "Critical",
            3,
            {
                "string",
                "number",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "TemperatureBelowLowerFatalThreshold",
        {
            "Indicates that a temperature reading is below the lower fatal threshold.",
            "Temperature '%1' reading of %2 degrees (C) is below the %3 lower fatal threshold.",
            "Critical",
            3,
            {
                "string",
                "number",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "TemperatureBelowUpperCriticalThreshold",
        {
            "Indicates that a temperature reading is no longer above the upper critical threshold but is still outside of normal operating range.",
            "Temperature '%1' reading of %2 degrees (C) is now below the %3 upper critical threshold but remains outside of normal range.",
            "Warning",
            3,
            {
                "string",
                "number",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "TemperatureBelowUpperFatalThreshold",
        {
            "Indicates that a temperature reading is no longer above the upper fatal threshold but is still outside of normal operating range.",
            "Temperature '%1' reading of %2 degrees (C) is now below the %3 upper fatal threshold but remains outside of normal range.",
            "Critical",
            3,
            {
                "string",
                "number",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "TemperatureCritical",
        {
            "Indicates that a temperature reading exceeds an internal critical level.",
            "Temperature '%1' reading of %2 degrees (C) exceeds the critical level.",
            "Critical",
            2,
            {
                "string",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "TemperatureNoLongerCritical",
        {
            "Indicates that a temperature reading no longer exceeds an internal critical level but still exceeds an internal warning level.",
            "Temperature '%1' reading of %2 degrees (C) no longer exceeds the critical level.",
            "Warning",
            2,
            {
                "string",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "TemperatureNormal",
        {
            "Indicates that a temperature reading is now within normal operating range.",
            "Temperature '%1' reading of %2 degrees (C) is within normal operating range.",
            "OK",
            2,
            {
                "string",
                "number",
            },
            "None.",
        }},
    MessageEntry{
        "TemperatureWarning",
        {
            "Indicates that a temperature reading exceeds an internal warning level.",
            "Temperature '%1' reading of %2 degrees (C) exceeds the warning level.",
            "Warning",
            2,
            {
                "string",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},

};

enum class Index
{
    fanFailed = 0,
    fanGroupCritical = 1,
    fanGroupNormal = 2,
    fanGroupWarning = 3,
    fanInserted = 4,
    fanRemoved = 5,
    fanRestored = 6,
    humidityAboveLowerCriticalThreshold = 7,
    humidityAboveUpperCautionThreshold = 8,
    humidityAboveUpperCriticalThreshold = 9,
    humidityBelowLowerCautionThreshold = 10,
    humidityBelowLowerCriticalThreshold = 11,
    humidityBelowUpperCriticalThreshold = 12,
    humidityNormal = 13,
    temperatureAboveLowerCriticalThreshold = 14,
    temperatureAboveLowerFatalThreshold = 15,
    temperatureAboveUpperCautionThreshold = 16,
    temperatureAboveUpperCriticalThreshold = 17,
    temperatureAboveUpperFatalThreshold = 18,
    temperatureBelowLowerCautionThreshold = 19,
    temperatureBelowLowerCriticalThreshold = 20,
    temperatureBelowLowerFatalThreshold = 21,
    temperatureBelowUpperCriticalThreshold = 22,
    temperatureBelowUpperFatalThreshold = 23,
    temperatureCritical = 24,
    temperatureNoLongerCritical = 25,
    temperatureNormal = 26,
    temperatureWarning = 27,
};
} // namespace redfish::registries::environmental
