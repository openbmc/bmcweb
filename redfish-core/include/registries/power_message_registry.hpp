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

namespace redfish::registries::power
{
const Header header = {
    "Copyright 2023 DMTF. All rights reserved.",
    "#MessageRegistry.v1_6_2.MessageRegistry",
    "Power.1.0.1",
    "Power Message Registry",
    "en",
    "This registry defines messages related to electrical measurements and power distribution equipment.",
    "Power",
    "1.0.1",
    "DMTF",
};
constexpr const char* url =
    "https://redfish.dmtf.org/registries/Power.1.0.1.json";

constexpr std::array registry =
{
    MessageEntry{
        "BreakerFault",
        {
            "Indicates that a circuit breaker has an internal fault.",
            "Fault detected in breaker '%1'.",
            "Critical",
            1,
            {
                "string",
            },
            "Check the breaker hardware and replace any faulty components.",
        }},
    MessageEntry{
        "BreakerReset",
        {
            "Indicates that a circuit breaker reset.",
            "Breaker '%1' reset.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "BreakerTripped",
        {
            "Indicates that a circuit breaker tripped.",
            "Breaker '%1' has tripped.",
            "Critical",
            1,
            {
                "string",
            },
            "Check the circuit and connected devices, and disconnect or replace any faulty devices.",
        }},
    MessageEntry{
        "CircuitPoweredOff",
        {
            "Indicates that a circuit was powered off.",
            "Circuit '%1' powered off.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "CircuitPoweredOn",
        {
            "Indicates that a circuit was powered on.",
            "Circuit '%1' powered on.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "CurrentAboveLowerCriticalThreshold",
        {
            "Indicates that a current reading is no longer below the lower critical threshold but is still outside of normal operating range.",
            "Current '%1' reading of %2 amperes is now above the %3 lower critical threshold but remains outside of normal range.",
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
        "CurrentAboveLowerFatalThreshold",
        {
            "Indicates that a current reading is no longer below the lower fatal threshold but is still outside of normal operating range.",
            "Current '%1' reading of %2 amperes is now above the %3 lower fatal threshold but remains outside of normal range.",
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
        "CurrentAboveUpperCautionThreshold",
        {
            "Indicates that a current reading is above the upper caution threshold.",
            "Current '%1' reading of %2 amperes is above the %3 upper caution threshold.",
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
        "CurrentAboveUpperCriticalThreshold",
        {
            "Indicates that a current reading is above the upper critical threshold.",
            "Current '%1' reading of %2 amperes is above the %3 upper critical threshold.",
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
        "CurrentAboveUpperFatalThreshold",
        {
            "Indicates that a current reading is above the upper fatal threshold.",
            "Current '%1' reading of %2 amperes is above the %3 upper fatal threshold.",
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
        "CurrentBelowLowerCautionThreshold",
        {
            "Indicates that a current reading is below the lower caution threshold.",
            "Current '%1' reading of %2 amperes is below the %3 lower caution threshold.",
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
        "CurrentBelowLowerCriticalThreshold",
        {
            "Indicates that a current reading is below the lower critical threshold.",
            "Current '%1' reading of %2 amperes is below the %3 lower critical threshold.",
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
        "CurrentBelowLowerFatalThreshold",
        {
            "Indicates that a current reading is below the lower fatal threshold.",
            "Current '%1' reading of %2 amperes is below the %3 lower fatal threshold.",
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
        "CurrentBelowUpperCriticalThreshold",
        {
            "Indicates that a current reading is no longer above the upper critical threshold but is still outside of normal operating range.",
            "Current '%1' reading of %2 amperes is now below the %3 upper critical threshold but remains outside of normal range.",
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
        "CurrentBelowUpperFatalThreshold",
        {
            "Indicates that a current reading is no longer above the upper fatal threshold but is still outside of normal operating range.",
            "Current '%1' reading of %2 amperes is now below the %3 upper fatal threshold but remains outside of normal range.",
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
        "CurrentCritical",
        {
            "Indicates that a current reading exceeds an internal critical level.",
            "Current '%1' reading of %2 amperes exceeds the critical level.",
            "Critical",
            2,
            {
                "string",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "CurrentNoLongerCritical",
        {
            "Indicates that a current reading no longer exceeds an internal critical level but still exceeds an internal warning level.",
            "Current '%1' reading of %2 amperes no longer exceeds the critical level.",
            "Warning",
            2,
            {
                "string",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "CurrentNormal",
        {
            "Indicates that a current reading is now within normal operating range.",
            "Current '%1' reading of %2 amperes is within normal operating range.",
            "OK",
            2,
            {
                "string",
                "number",
            },
            "None.",
        }},
    MessageEntry{
        "CurrentWarning",
        {
            "Indicates that a current reading exceeds an internal warning level.",
            "Current '%1' reading of %2 amperes exceeds the warning level.",
            "Warning",
            2,
            {
                "string",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "FrequencyAboveLowerCriticalThreshold",
        {
            "Indicates that a frequency reading is no longer below the lower critical threshold but is still outside of normal operating range.",
            "Frequency '%1' reading of %2 hertz is now above the %3 lower critical threshold but remains outside of normal range.",
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
        "FrequencyAboveUpperCautionThreshold",
        {
            "Indicates that a frequency reading is above the upper caution threshold.",
            "Frequency '%1' reading of %2 hertz is above the %3 upper caution threshold.",
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
        "FrequencyAboveUpperCriticalThreshold",
        {
            "Indicates that a frequency reading is above the upper critical threshold.",
            "Frequency '%1' reading of %2 hertz is above the %3 upper critical threshold.",
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
        "FrequencyBelowLowerCautionThreshold",
        {
            "Indicates that a frequency reading is below the lower caution threshold.",
            "Frequency '%1' reading of %2 hertz is below the %3 lower caution threshold.",
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
        "FrequencyBelowLowerCriticalThreshold",
        {
            "Indicates that a frequency reading is below the lower critical threshold.",
            "Frequency '%1' reading of %2 hertz is below the %3 lower critical threshold.",
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
        "FrequencyBelowUpperCriticalThreshold",
        {
            "Indicates that a frequency reading is no longer above the upper critical threshold but is still outside of normal operating range.",
            "Frequency '%1' reading of %2 hertz is now below the %3 upper critical threshold but remains outside of normal range.",
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
        "FrequencyCritical",
        {
            "Indicates that a frequency reading exceeds an internal critical level.",
            "Frequency '%1' reading of %2 hertz exceeds the critical level.",
            "Critical",
            2,
            {
                "string",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "FrequencyNoLongerCritical",
        {
            "Indicates that a frequency reading no longer exceeds an internal critical level but still exceeds an internal warning level.",
            "Frequency '%1' reading of %2 hertz no longer exceeds the critical level.",
            "Warning",
            2,
            {
                "string",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "FrequencyNormal",
        {
            "Indicates that a frequency reading is now within normal operating range.",
            "Frequency '%1' reading of %2 hertz is within normal operating range.",
            "OK",
            2,
            {
                "string",
                "number",
            },
            "None.",
        }},
    MessageEntry{
        "FrequencyWarning",
        {
            "Indicates that a frequency reading exceeds an internal warning level.",
            "Frequency '%1' reading of %2 hertz exceeds the warning level.",
            "Warning",
            2,
            {
                "string",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "LineInputPowerFault",
        {
            "Indicates a fault on an electrical power input.",
            "Line input power fault at '%1'.",
            "Warning",
            1,
            {
                "string",
            },
            "Check the electrical power input and connections.",
        }},
    MessageEntry{
        "LineInputPowerRestored",
        {
            "Indicates that an electrical power input was restored to normal operation.",
            "Line input power at '%1' was restored.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "LossOfInputPower",
        {
            "Indicates a loss of power at an electrical input.",
            "Loss of input power at '%1'.",
            "Warning",
            1,
            {
                "string",
            },
            "Check the electrical power input and connections.",
        }},
    MessageEntry{
        "OutletPoweredOff",
        {
            "Indicates that an outlet was powered off.",
            "Outlet '%1' powered off.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "OutletPoweredOn",
        {
            "Indicates that an outlet was powered on.",
            "Outlet '%1' powered on.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "PowerAboveLowerCriticalThreshold",
        {
            "Indicates that a power reading is no longer below the lower critical threshold but is still outside of normal operating range.",
            "Power '%1' reading of %2 watts is now above the %3 lower critical threshold but remains outside of normal range.",
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
        "PowerAboveLowerFatalThreshold",
        {
            "Indicates that a power reading is no longer below the lower fatal threshold but is still outside of normal operating range.",
            "Power '%1' reading of %2 watts is now above the %3 lower fatal threshold but remains outside of normal range.",
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
        "PowerAboveUpperCautionThreshold",
        {
            "Indicates that a power reading is above the upper caution threshold.",
            "Power '%1' reading of %2 watts is above the %3 upper caution threshold.",
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
        "PowerAboveUpperCriticalThreshold",
        {
            "Indicates that a power reading is above the upper critical threshold.",
            "Power '%1' reading of %2 watts is above the %3 upper critical threshold.",
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
        "PowerAboveUpperFatalThreshold",
        {
            "Indicates that a power reading is above the upper fatal threshold.",
            "Power '%1' reading of %2 watts is above the %3 upper fatal threshold.",
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
        "PowerBelowLowerCautionThreshold",
        {
            "Indicates that a power reading is below the lower caution threshold.",
            "Power '%1' reading of %2 watts is below the %3 lower caution threshold.",
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
        "PowerBelowLowerCriticalThreshold",
        {
            "Indicates that a power reading is below the lower critical threshold.",
            "Power '%1' reading of %2 watts is below the %3 lower critical threshold.",
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
        "PowerBelowLowerFatalThreshold",
        {
            "Indicates that a power reading is below the lower fatal threshold.",
            "Power '%1' reading of %2 watts is below the %3 lower fatal threshold.",
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
        "PowerBelowUpperCriticalThreshold",
        {
            "Indicates that a power reading is no longer above the upper critical threshold but is still outside of normal operating range.",
            "Power '%1' reading of %2 watts is now below the %3 upper critical threshold but remains outside of normal range.",
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
        "PowerBelowUpperFatalThreshold",
        {
            "Indicates that a power reading is no longer above the upper fatal threshold but is still outside of normal operating range.",
            "Power '%1' reading of %2 watts is now below the %3 upper fatal threshold but remains outside of normal range.",
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
        "PowerCritical",
        {
            "Indicates that a power reading exceeds an internal critical level.",
            "Power '%1' reading of %2 watts exceeds the critical level.",
            "Critical",
            2,
            {
                "string",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "PowerNoLongerCritical",
        {
            "Indicates that a power reading no longer exceeds an internal critical level but still exceeds an internal warning level.",
            "Power '%1' reading of %2 watts no longer exceeds the critical level.",
            "Warning",
            2,
            {
                "string",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "PowerNormal",
        {
            "Indicates that a power reading is now within normal operating range.",
            "Power '%1' reading of %2 watts is within normal operating range.",
            "OK",
            2,
            {
                "string",
                "number",
            },
            "None.",
        }},
    MessageEntry{
        "PowerSupplyFailed",
        {
            "Indicates that a power supply has failed.",
            "Power supply '%1' has failed.",
            "Warning",
            1,
            {
                "string",
            },
            "Check the power supply hardware and replace any faulty component.",
        }},
    MessageEntry{
        "PowerSupplyGroupCritical",
        {
            "Indicates that a power supply group has a critical status.",
            "Power supply group '%1' is in a critical state.",
            "Critical",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "PowerSupplyGroupNormal",
        {
            "Indicates that a power supply group has returned to normal operations.",
            "Power supply group '%1' is operating normally.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "PowerSupplyGroupWarning",
        {
            "Indicates that a power supply group has a warning status.",
            "Power supply group '%1' is in a warning state.",
            "Warning",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "PowerSupplyInserted",
        {
            "Indicates that a power supply was inserted or installed.",
            "Power supply '%1' was inserted.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "PowerSupplyPredictiveFailure",
        {
            "Indicates that the power supply predicted a future failure condition.",
            "Power supply '%1' has a predicted failure condition.",
            "Warning",
            1,
            {
                "string",
            },
            "Check the power supply hardware and replace any faulty component.",
        }},
    MessageEntry{
        "PowerSupplyRemoved",
        {
            "Indicates that a power supply was removed.",
            "Power supply '%1' was removed.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "PowerSupplyRestored",
        {
            "Indicates that a power supply was repaired or restored to normal operation.",
            "Power supply '%1' was restored.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "PowerSupplyWarning",
        {
            "Indicates that a power supply has a warning condition.",
            "Power supply '%1' has a warning condition.",
            "Warning",
            1,
            {
                "string",
            },
            "Check the power supply hardware and replace any faulty component.",
        }},
    MessageEntry{
        "PowerWarning",
        {
            "Indicates that a power reading exceeds an internal warning level.",
            "Power '%1' reading of %2 watts exceeds the warning level.",
            "Warning",
            2,
            {
                "string",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "VoltageAboveLowerCriticalThreshold",
        {
            "Indicates that a voltage reading is no longer below the lower critical threshold but is still outside of normal operating range.",
            "Voltage '%1' reading of %2 volts is now above the %3 lower critical threshold but remains outside of normal range.",
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
        "VoltageAboveLowerFatalThreshold",
        {
            "Indicates that a voltage reading is no longer below the lower fatal threshold but is still outside of normal operating range.",
            "Voltage '%1' reading of %2 volts is now above the %3 lower fatal threshold but remains outside of normal range.",
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
        "VoltageAboveUpperCautionThreshold",
        {
            "Indicates that a voltage reading is above the upper caution threshold.",
            "Voltage '%1' reading of %2 volts is above the %3 upper caution threshold.",
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
        "VoltageAboveUpperCriticalThreshold",
        {
            "Indicates that a voltage reading is above the upper critical threshold.",
            "Voltage '%1' reading of %2 volts is above the %3 upper critical threshold.",
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
        "VoltageAboveUpperFatalThreshold",
        {
            "Indicates that a voltage reading is above the upper fatal threshold.",
            "Voltage '%1' reading of %2 volts is above the %3 upper fatal threshold.",
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
        "VoltageBelowLowerCautionThreshold",
        {
            "Indicates that a voltage reading is below the lower caution threshold.",
            "Voltage '%1' reading of %2 volts is below the %3 lower caution threshold.",
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
        "VoltageBelowLowerCriticalThreshold",
        {
            "Indicates that a voltage reading is below the lower critical threshold.",
            "Voltage '%1' reading of %2 volts is below the %3 lower critical threshold.",
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
        "VoltageBelowLowerFatalThreshold",
        {
            "Indicates that a voltage reading is below the lower fatal threshold.",
            "Voltage '%1' reading of %2 volts is below the %3 lower fatal threshold.",
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
        "VoltageBelowUpperCriticalThreshold",
        {
            "Indicates that a voltage reading is no longer above the upper critical threshold but is still outside of normal operating range.",
            "Voltage '%1' reading of %2 volts is now below the %3 upper critical threshold but remains outside of normal range.",
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
        "VoltageBelowUpperFatalThreshold",
        {
            "Indicates that a voltage reading is no longer above the upper fatal threshold but is still outside of normal operating range.",
            "Voltage '%1' reading of %2 volts is now below the %3 upper fatal threshold but remains outside of normal range.",
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
        "VoltageCritical",
        {
            "Indicates that a voltage reading exceeds an internal critical level.",
            "Voltage '%1' reading of %2 volts exceeds the critical level.",
            "Critical",
            2,
            {
                "string",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "VoltageNoLongerCritical",
        {
            "Indicates that a voltage reading no longer exceeds an internal critical level but still exceeds an internal warning level.",
            "Voltage '%1' reading of %2 volts no longer exceeds the critical level.",
            "Warning",
            2,
            {
                "string",
                "number",
            },
            "Check the condition of the resource listed in OriginOfCondition.",
        }},
    MessageEntry{
        "VoltageNormal",
        {
            "Indicates that a voltage reading is now within normal operating range.",
            "Voltage '%1' reading of %2 volts is within normal operating range.",
            "OK",
            2,
            {
                "string",
                "number",
            },
            "None.",
        }},
    MessageEntry{
        "VoltageWarning",
        {
            "Indicates that a voltage reading exceeds an internal warning level.",
            "Voltage '%1' reading of %2 volts exceeds the warning level.",
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
    breakerFault = 0,
    breakerReset = 1,
    breakerTripped = 2,
    circuitPoweredOff = 3,
    circuitPoweredOn = 4,
    currentAboveLowerCriticalThreshold = 5,
    currentAboveLowerFatalThreshold = 6,
    currentAboveUpperCautionThreshold = 7,
    currentAboveUpperCriticalThreshold = 8,
    currentAboveUpperFatalThreshold = 9,
    currentBelowLowerCautionThreshold = 10,
    currentBelowLowerCriticalThreshold = 11,
    currentBelowLowerFatalThreshold = 12,
    currentBelowUpperCriticalThreshold = 13,
    currentBelowUpperFatalThreshold = 14,
    currentCritical = 15,
    currentNoLongerCritical = 16,
    currentNormal = 17,
    currentWarning = 18,
    frequencyAboveLowerCriticalThreshold = 19,
    frequencyAboveUpperCautionThreshold = 20,
    frequencyAboveUpperCriticalThreshold = 21,
    frequencyBelowLowerCautionThreshold = 22,
    frequencyBelowLowerCriticalThreshold = 23,
    frequencyBelowUpperCriticalThreshold = 24,
    frequencyCritical = 25,
    frequencyNoLongerCritical = 26,
    frequencyNormal = 27,
    frequencyWarning = 28,
    lineInputPowerFault = 29,
    lineInputPowerRestored = 30,
    lossOfInputPower = 31,
    outletPoweredOff = 32,
    outletPoweredOn = 33,
    powerAboveLowerCriticalThreshold = 34,
    powerAboveLowerFatalThreshold = 35,
    powerAboveUpperCautionThreshold = 36,
    powerAboveUpperCriticalThreshold = 37,
    powerAboveUpperFatalThreshold = 38,
    powerBelowLowerCautionThreshold = 39,
    powerBelowLowerCriticalThreshold = 40,
    powerBelowLowerFatalThreshold = 41,
    powerBelowUpperCriticalThreshold = 42,
    powerBelowUpperFatalThreshold = 43,
    powerCritical = 44,
    powerNoLongerCritical = 45,
    powerNormal = 46,
    powerSupplyFailed = 47,
    powerSupplyGroupCritical = 48,
    powerSupplyGroupNormal = 49,
    powerSupplyGroupWarning = 50,
    powerSupplyInserted = 51,
    powerSupplyPredictiveFailure = 52,
    powerSupplyRemoved = 53,
    powerSupplyRestored = 54,
    powerSupplyWarning = 55,
    powerWarning = 56,
    voltageAboveLowerCriticalThreshold = 57,
    voltageAboveLowerFatalThreshold = 58,
    voltageAboveUpperCautionThreshold = 59,
    voltageAboveUpperCriticalThreshold = 60,
    voltageAboveUpperFatalThreshold = 61,
    voltageBelowLowerCautionThreshold = 62,
    voltageBelowLowerCriticalThreshold = 63,
    voltageBelowLowerFatalThreshold = 64,
    voltageBelowUpperCriticalThreshold = 65,
    voltageBelowUpperFatalThreshold = 66,
    voltageCritical = 67,
    voltageNoLongerCritical = 68,
    voltageNormal = 69,
    voltageWarning = 70,
};
} // namespace redfish::registries::power
