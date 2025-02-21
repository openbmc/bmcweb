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
#include "registries.hpp"

#include <array>

// clang-format off

namespace redfish::registries::environmental
{
const Header header = {
    "Copyright 2024 DMTF. All rights reserved.",
    "#MessageRegistry.v1_6_2.MessageRegistry",
    1,
    1,
    0,
    "Environmental Message Registry",
    "en",
    "This registry defines messages related to environmental sensors, heating and cooling equipment, or other environmental conditions.",
    "Environmental",
    "DMTF",
};
constexpr const char* url =
    "https://redfish.dmtf.org/registries/Environmental.1.1.0.json";

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
        "FilterInserted",
        {
            "Indicates that a filter was inserted or installed.",
            "Filter '%1' was inserted.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "FilterRemoved",
        {
            "Indicates that a filter was removed.",
            "Filter '%1' was removed.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "FilterRequiresService",
        {
            "Indicates that a filter requires service.",
            "Filter '%1' requires service.",
            "Warning",
            1,
            {
                "string",
            },
            "Replace the filter or filter media.",
        }},
    MessageEntry{
        "FilterRestored",
        {
            "Indicates that a filter was repaired or restored to normal operation.",
            "Filter '%1' was restored.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "FlowRateAboveLowerCriticalThreshold",
        {
            "Indicates that a flow rate reading is no longer below the lower critical threshold but is still outside of normal operating range.",
            "Flow rate '%1' reading of %2 L/min is now above the %3 lower critical threshold but remains outside of normal range.",
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
        "FlowRateAboveLowerFatalThreshold",
        {
            "Indicates that a flow rate reading is no longer below the lower fatal threshold but is still outside of normal operating range.",
            "Flow rate '%1' reading of %2 L/min is now above the %3 lower fatal threshold but remains outside of normal range.",
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
        "FlowRateAboveUpperCautionThreshold",
        {
            "Indicates that a flow rate reading is above the upper caution threshold.",
            "Flow rate '%1' reading of %2 L/min is above the %3 upper caution threshold.",
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
        "FlowRateAboveUpperCriticalThreshold",
        {
            "Indicates that a flow rate reading is above the upper critical threshold.",
            "Flow rate '%1' reading of %2 L/min is above the %3 upper critical threshold.",
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
        "FlowRateAboveUpperFatalThreshold",
        {
            "Indicates that a flow rate reading is above the upper fatal threshold.",
            "Flow rate '%1' reading of %2 L/min is above the %3 upper fatal threshold.",
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
        "FlowRateBelowLowerCautionThreshold",
        {
            "Indicates that a flow rate reading is below the lower caution threshold.",
            "Flow rate '%1' reading of %2 L/min is below the %3 lower caution threshold.",
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
        "FlowRateBelowLowerCriticalThreshold",
        {
            "Indicates that a flow rate reading is below the lower critical threshold.",
            "Flow rate '%1' reading of %2 L/min is below the %3 lower critical threshold.",
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
        "FlowRateBelowLowerFatalThreshold",
        {
            "Indicates that a flow rate reading is below the lower fatal threshold.",
            "Flow rate '%1' reading of %2 L/min is below the %3 lower fatal threshold.",
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
        "FlowRateBelowUpperCriticalThreshold",
        {
            "Indicates that a flow rate reading is no longer above the upper critical threshold but is still outside of normal operating range.",
            "Flow rate '%1' reading of %2 L/min is now below the %3 upper critical threshold but remains outside of normal range.",
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
        "FlowRateBelowUpperFatalThreshold",
        {
            "Indicates that a flow rate reading is no longer above the upper fatal threshold but is still outside of normal operating range.",
            "Flow rate '%1' reading of %2 L/min is now below the %3 upper fatal threshold but remains outside of normal range.",
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
        "FlowRateCritical",
        {
            "Indicates that a flow rate reading exceeds an internal critical level.",
            "Flow rate '%1' reading of %2 L/min exceeds the critical level.",
            "Critical",
            2,
            {
                "string",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "FlowRateFatal",
        {
            "Indicates that a flow rate reading exceeds an internal fatal level or flow rate reading is zero (0).",
            "Flow rate '%1' reading of %2 L/min exceeds the fatal level.",
            "Critical",
            2,
            {
                "string",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "FlowRateNoLongerCritical",
        {
            "Indicates that a flow rate reading no longer exceeds an internal critical level but still exceeds an internal warning level.",
            "Flow rate '%1' reading of %2 L/min no longer exceeds the critical level.",
            "Warning",
            2,
            {
                "string",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "FlowRateNoLongerFatal",
        {
            "Indicates that a flow rate reading is no longer exceeds an internal fatal level but is still exceeds an internal critical level.",
            "Flow rate '%1' reading of %2 L/min no longer exceeds the fatal level.",
            "Critical",
            2,
            {
                "string",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "FlowRateNormal",
        {
            "Indicates that a flow rate reading is now within normal operating range.",
            "Flow rate '%1' reading of %2 L/min is within normal operating range.",
            "OK",
            2,
            {
                "string",
                "number",
            },
            "None.",
        }},
    MessageEntry{
        "FlowRateWarning",
        {
            "Indicates that a flow rate reading exceeds an internal warning level.",
            "Flow rate '%1' reading of %2 L/min exceeds the warning level.",
            "Warning",
            2,
            {
                "string",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "FluidLevelAboveLowerCriticalThreshold",
        {
            "Indicates that a fluid level reading is no longer below the lower critical threshold but is still outside of normal operating range.",
            "Fluid level '%1' reading of %2 percent is now above the %3 lower critical threshold but remains outside of normal range.",
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
        "FluidLevelAboveLowerFatalThreshold",
        {
            "Indicates that a fluid level reading is no longer below the lower fatal threshold but is still outside of normal operating range.",
            "Fluid level '%1' reading of %2 percent is now above the %3 lower fatal threshold but remains outside of normal range.",
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
        "FluidLevelAboveUpperCautionThreshold",
        {
            "Indicates that a fluid level reading is above the upper caution threshold.",
            "Fluid level '%1' reading of %2 percent is above the %3 upper caution threshold.",
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
        "FluidLevelAboveUpperCriticalThreshold",
        {
            "Indicates that a fluid level reading is above the upper critical threshold.",
            "Fluid level '%1' reading of %2 percent is above the %3 upper critical threshold.",
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
        "FluidLevelAboveUpperFatalThreshold",
        {
            "Indicates that a fluid level reading is above the upper fatal threshold.",
            "Fluid level '%1' reading of %2 percent is above the %3 upper fatal threshold.",
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
        "FluidLevelBelowLowerCautionThreshold",
        {
            "Indicates that a fluid level reading is below the lower caution threshold.",
            "Fluid level '%1' reading of %2 percent is below the %3 lower caution threshold.",
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
        "FluidLevelBelowLowerCriticalThreshold",
        {
            "Indicates that a fluid level reading is below the lower critical threshold.",
            "Fluid level '%1' reading of %2 percent is below the %3 lower critical threshold.",
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
        "FluidLevelBelowLowerFatalThreshold",
        {
            "Indicates that a fluid level reading is below the lower fatal threshold.",
            "Fluid level '%1' reading of %2 percent is below the %3 lower fatal threshold.",
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
        "FluidLevelBelowUpperCriticalThreshold",
        {
            "Indicates that a fluid level reading is no longer above the upper critical threshold but is still outside of normal operating range.",
            "Fluid level '%1' reading of %2 percent is now below the %3 upper critical threshold but remains outside of normal range.",
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
        "FluidLevelBelowUpperFatalThreshold",
        {
            "Indicates that a fluid level reading is no longer above the upper fatal threshold but is still outside of normal operating range.",
            "Fluid level '%1' reading of %2 percent is now below the %3 upper fatal threshold but remains outside of normal range.",
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
        "FluidLevelCritical",
        {
            "Indicates that a fluid level reading violates an internal critical level.",
            "Fluid level '%1' reading violates the critical level.",
            "Critical",
            1,
            {
                "string",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "FluidLevelNoLongerCritical",
        {
            "Indicates that a fluid level reading no longer violates an internal critical level but still violates an internal warning level.",
            "Fluid level '%1' reading no longer violates the critical level.",
            "Warning",
            1,
            {
                "string",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "FluidLevelNormal",
        {
            "Indicates that a fluid level reading is now within normal operating range.",
            "Fluid level '%1' reading is within normal operating range.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "FluidLevelWarning",
        {
            "Indicates that a fluid level reading violates an internal warning level.",
            "Fluid level '%1' reading violates the warning level.",
            "Warning",
            1,
            {
                "string",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "FluidQualityCritical",
        {
            "Indicates that a fluid quality reading exceeds an internal critical level.",
            "Fluid quality '%1' reading exceeds the critical level.",
            "Critical",
            1,
            {
                "string",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "FluidQualityNoLongerCritical",
        {
            "Indicates that a fluid quality reading no longer exceeds an internal critical level but still exceeds an internal warning level.",
            "Fluid quality '%1' reading no longer exceeds the critical level.",
            "Warning",
            1,
            {
                "string",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "FluidQualityNormal",
        {
            "Indicates that a fluid quality reading is now within normal operating range.",
            "Fluid quality '%1' reading is within normal operating range.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "FluidQualityWarning",
        {
            "Indicates that a fluid quality reading exceeds an internal warning level.",
            "Fluid quality '%1' reading exceeds the warning level.",
            "Warning",
            1,
            {
                "string",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
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
        "LeakDetectedCritical",
        {
            "Indicates that a leak detector is in a critical state.",
            "Leak detector '%1' reports a critical level leak.",
            "Critical",
            1,
            {
                "string",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "LeakDetectedNormal",
        {
            "Indicates that a leak detector is within normal operating range.",
            "Leak detector '%1' has returned to normal.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "LeakDetectedWarning",
        {
            "Indicates that a leak detector is in a warning state.",
            "Leak detector '%1' reports a warning level leak.",
            "Warning",
            1,
            {
                "string",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "PressureAboveLowerCriticalThreshold",
        {
            "Indicates that a pressure reading is no longer below the lower critical threshold but is still outside of normal operating range.",
            "Pressure '%1' reading of %2 kPa is now above the %3 lower critical threshold but remains outside of normal range.",
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
        "PressureAboveLowerFatalThreshold",
        {
            "Indicates that a pressure reading is no longer below the lower fatal threshold but is still outside of normal operating range.",
            "Pressure '%1' reading of %2 kPa is now above the %3 lower fatal threshold but remains outside of normal range.",
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
        "PressureAboveUpperCautionThreshold",
        {
            "Indicates that a pressure reading is above the upper caution threshold.",
            "Pressure '%1' reading of %2 kPa is above the %3 upper caution threshold.",
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
        "PressureAboveUpperCriticalThreshold",
        {
            "Indicates that a pressure reading is above the upper critical threshold.",
            "Pressure '%1' reading of %2 kPa is above the %3 upper critical threshold.",
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
        "PressureAboveUpperFatalThreshold",
        {
            "Indicates that a pressure reading is above the upper fatal threshold.",
            "Pressure '%1' reading of %2 kPa is above the %3 upper fatal threshold.",
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
        "PressureBelowLowerCautionThreshold",
        {
            "Indicates that a pressure reading is below the lower caution threshold.",
            "Pressure '%1' reading of %2 kPa is below the %3 lower caution threshold.",
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
        "PressureBelowLowerCriticalThreshold",
        {
            "Indicates that a pressure reading is below the lower critical threshold.",
            "Pressure '%1' reading of %2 kPa is below the %3 lower critical threshold.",
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
        "PressureBelowLowerFatalThreshold",
        {
            "Indicates that a pressure reading is below the lower fatal threshold.",
            "Pressure '%1' reading of %2 kPa is below the %3 lower fatal threshold.",
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
        "PressureBelowUpperCriticalThreshold",
        {
            "Indicates that a pressure reading is no longer above the upper critical threshold but is still outside of normal operating range.",
            "Pressure '%1' reading of %2 kPa is now below the %3 upper critical threshold but remains outside of normal range.",
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
        "PressureBelowUpperFatalThreshold",
        {
            "Indicates that a pressure reading is no longer above the upper fatal threshold but is still outside of normal operating range.",
            "Pressure '%1' reading of %2 kPa is now below the %3 upper fatal threshold but remains outside of normal range.",
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
        "PressureCritical",
        {
            "Indicates that a pressure reading exceeds an internal critical level.",
            "Pressure '%1' reading of %2 kPa exceeds the critical level.",
            "Critical",
            2,
            {
                "string",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "PressureNoLongerCritical",
        {
            "Indicates that a pressure reading no longer violates an internal critical level but still violates an internal warning level.",
            "Pressure '%1' reading of %2 kPa no longer violates the critical level.",
            "Warning",
            2,
            {
                "string",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "PressureNormal",
        {
            "Indicates that a pressure reading is now within normal operating range.",
            "Pressure '%1' reading of %2 kPa is within normal operating range.",
            "OK",
            2,
            {
                "string",
                "number",
            },
            "None.",
        }},
    MessageEntry{
        "PressureWarning",
        {
            "Indicates that a pressure reading exceeds an internal warning level.",
            "Pressure '%1' reading of %2 kPa exceeds the warning level.",
            "Warning",
            2,
            {
                "string",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "PumpFailed",
        {
            "Indicates that a pump has failed.",
            "Pump '%1' has failed.",
            "Warning",
            1,
            {
                "string",
            },
            "Check the pump hardware and replace any faulty component.",
        }},
    MessageEntry{
        "PumpInserted",
        {
            "Indicates that a pump was inserted or installed.",
            "Pump '%1' was inserted.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "PumpRemoved",
        {
            "Indicates that a pump was removed.",
            "Pump '%1' was removed.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "PumpRestored",
        {
            "Indicates that a pump was repaired or restored to normal operation.",
            "Pump '%1' was restored.",
            "OK",
            1,
            {
                "string",
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
    filterInserted = 7,
    filterRemoved = 8,
    filterRequiresService = 9,
    filterRestored = 10,
    flowRateAboveLowerCriticalThreshold = 11,
    flowRateAboveLowerFatalThreshold = 12,
    flowRateAboveUpperCautionThreshold = 13,
    flowRateAboveUpperCriticalThreshold = 14,
    flowRateAboveUpperFatalThreshold = 15,
    flowRateBelowLowerCautionThreshold = 16,
    flowRateBelowLowerCriticalThreshold = 17,
    flowRateBelowLowerFatalThreshold = 18,
    flowRateBelowUpperCriticalThreshold = 19,
    flowRateBelowUpperFatalThreshold = 20,
    flowRateCritical = 21,
    flowRateFatal = 22,
    flowRateNoLongerCritical = 23,
    flowRateNoLongerFatal = 24,
    flowRateNormal = 25,
    flowRateWarning = 26,
    fluidLevelAboveLowerCriticalThreshold = 27,
    fluidLevelAboveLowerFatalThreshold = 28,
    fluidLevelAboveUpperCautionThreshold = 29,
    fluidLevelAboveUpperCriticalThreshold = 30,
    fluidLevelAboveUpperFatalThreshold = 31,
    fluidLevelBelowLowerCautionThreshold = 32,
    fluidLevelBelowLowerCriticalThreshold = 33,
    fluidLevelBelowLowerFatalThreshold = 34,
    fluidLevelBelowUpperCriticalThreshold = 35,
    fluidLevelBelowUpperFatalThreshold = 36,
    fluidLevelCritical = 37,
    fluidLevelNoLongerCritical = 38,
    fluidLevelNormal = 39,
    fluidLevelWarning = 40,
    fluidQualityCritical = 41,
    fluidQualityNoLongerCritical = 42,
    fluidQualityNormal = 43,
    fluidQualityWarning = 44,
    humidityAboveLowerCriticalThreshold = 45,
    humidityAboveUpperCautionThreshold = 46,
    humidityAboveUpperCriticalThreshold = 47,
    humidityBelowLowerCautionThreshold = 48,
    humidityBelowLowerCriticalThreshold = 49,
    humidityBelowUpperCriticalThreshold = 50,
    humidityNormal = 51,
    leakDetectedCritical = 52,
    leakDetectedNormal = 53,
    leakDetectedWarning = 54,
    pressureAboveLowerCriticalThreshold = 55,
    pressureAboveLowerFatalThreshold = 56,
    pressureAboveUpperCautionThreshold = 57,
    pressureAboveUpperCriticalThreshold = 58,
    pressureAboveUpperFatalThreshold = 59,
    pressureBelowLowerCautionThreshold = 60,
    pressureBelowLowerCriticalThreshold = 61,
    pressureBelowLowerFatalThreshold = 62,
    pressureBelowUpperCriticalThreshold = 63,
    pressureBelowUpperFatalThreshold = 64,
    pressureCritical = 65,
    pressureNoLongerCritical = 66,
    pressureNormal = 67,
    pressureWarning = 68,
    pumpFailed = 69,
    pumpInserted = 70,
    pumpRemoved = 71,
    pumpRestored = 72,
    temperatureAboveLowerCriticalThreshold = 73,
    temperatureAboveLowerFatalThreshold = 74,
    temperatureAboveUpperCautionThreshold = 75,
    temperatureAboveUpperCriticalThreshold = 76,
    temperatureAboveUpperFatalThreshold = 77,
    temperatureBelowLowerCautionThreshold = 78,
    temperatureBelowLowerCriticalThreshold = 79,
    temperatureBelowLowerFatalThreshold = 80,
    temperatureBelowUpperCriticalThreshold = 81,
    temperatureBelowUpperFatalThreshold = 82,
    temperatureCritical = 83,
    temperatureNoLongerCritical = 84,
    temperatureNormal = 85,
    temperatureWarning = 86,
};
} // namespace redfish::registries::environmental
