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

namespace redfish::registries::telemetry
{
const Header header = {
    "Copyright 2023 DMTF. All rights reserved.",
    "#MessageRegistry.v1_6_2.MessageRegistry",
    "Telemetry.1.0.0",
    "Telemetry Message Registry",
    "en",
    "This registry defines the messages for telemetry related events.",
    "Telemetry",
    "1.0.0",
    "DMTF",
};
constexpr const char* url =
    "https://redfish.dmtf.org/registries/Telemetry.1.0.0.json";

constexpr std::array registry =
{
    MessageEntry{
        "TriggerDiscreteConditionMet",
        {
            "Indicates that a discrete trigger condition is met.",
            "Metric '%1' has the value '%2', which meets the discrete condition of trigger '%3'",
            "OK",
            3,
            {
                "string",
                "string",
                "string",
            },
            "Check the condition of the metric that reported the trigger.",
        }},
    MessageEntry{
        "TriggerNumericAboveLowerCritical",
        {
            "Indicates that a numeric metric reading is no longer below the lower critical trigger threshold, but is still outside of normal operating range.",
            "Metric '%1' value of %2 is now above the %3 lower critical threshold of trigger '%4' but remains outside of normal range",
            "Warning",
            4,
            {
                "string",
                "number",
                "number",
                "string",
            },
            "Check the condition of the metric that reported the trigger.",
        }},
    MessageEntry{
        "TriggerNumericAboveUpperCritical",
        {
            "Indicates that a numeric metric reading is above the upper critical trigger threshold.",
            "Metric '%1' value of %2 is above the %3 upper critical threshold of trigger '%4'",
            "Critical",
            4,
            {
                "string",
                "number",
                "number",
                "string",
            },
            "Check the condition of the metric that reported the trigger.",
        }},
    MessageEntry{
        "TriggerNumericAboveUpperWarning",
        {
            "Indicates that a numeric metric reading is above the upper warning trigger threshold.",
            "Metric '%1' value of %2 is above the %3 upper warning threshold of trigger '%4'",
            "Warning",
            4,
            {
                "string",
                "number",
                "number",
                "string",
            },
            "Check the condition of the metric that reported the trigger.",
        }},
    MessageEntry{
        "TriggerNumericBelowLowerCritical",
        {
            "Indicates that a numeric metric reading is below the lower critical trigger threshold.",
            "Metric '%1' value of %2 is below the %3 lower critical threshold of trigger '%4'",
            "Critical",
            4,
            {
                "string",
                "number",
                "number",
                "string",
            },
            "Check the condition of the metric that reported the trigger.",
        }},
    MessageEntry{
        "TriggerNumericBelowLowerWarning",
        {
            "Indicates that a numeric metric reading is below the lower warning trigger threshold.",
            "Metric '%1' value of %2 is below the %3 lower warning threshold of trigger '%4'",
            "Warning",
            4,
            {
                "string",
                "number",
                "number",
                "string",
            },
            "Check the condition of the metric that reported the trigger.",
        }},
    MessageEntry{
        "TriggerNumericBelowUpperCritical",
        {
            "Indicates that a numeric metric reading is no longer above the upper critical trigger threshold, but is still outside of normal operating range.",
            "Metric '%1' value of %2 is now below the %3 upper critical threshold of trigger '%4' but remains outside of normal range",
            "Warning",
            4,
            {
                "string",
                "number",
                "number",
                "string",
            },
            "Check the condition of the metric that reported the trigger.",
        }},
    MessageEntry{
        "TriggerNumericReadingNormal",
        {
            "Indicates that a numeric metric reading is now within normal operating range.",
            "Metric '%1' value of %2 is within normal operating range of trigger '%3'",
            "OK",
            3,
            {
                "string",
                "number",
                "string",
            },
            "None.",
        }},

};

enum class Index
{
    triggerDiscreteConditionMet = 0,
    triggerNumericAboveLowerCritical = 1,
    triggerNumericAboveUpperCritical = 2,
    triggerNumericAboveUpperWarning = 3,
    triggerNumericBelowLowerCritical = 4,
    triggerNumericBelowLowerWarning = 5,
    triggerNumericBelowUpperCritical = 6,
    triggerNumericReadingNormal = 7,
};
} // namespace redfish::registries::telemetry
