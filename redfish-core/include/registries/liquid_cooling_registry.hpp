/*
// Copyright 2024-2025 Intel Corporation.

// This software and the related documents are Intel copyrighted materials,
// and your use of them is governed by the express license under which they
// were provided to you ("License"). Unless the License provides otherwise,
// you may not use, modify, copy, publish, distribute, disclose or transmit
// this software or the related documents without Intel's prior written
// permission.

// This software and the related documents are provided as is, with
// no express or implied warranties, other than those that are expressly
// stated in the License.
*/

#pragma once
#include "registries.hpp"

#include <array>

namespace redfish::registries
{
struct LiquidCooling
{
    static constexpr Header header = {
        "Copyright 2024 Intel. All rights reserved.",
        "#MessageRegistry.v1_6_3.MessageRegistry",
        0,
        1,
        0,
        "LiquidCooling Message Registry",
        "en",
        "This registry defines the LiquidCooling messages for OpenBMC.",
        "LiquidCooling",
        "Intel",
    };

    static constexpr const char* url = "";

    static constexpr std::array registry = {
        MessageEntry{"CoolingModeDetect",
                     {
                         "Indicates that cooling mode has been discovered.",
                         "Cooling mode detected: %1",
                         "OK",
                         1,
                         {"string"},
                         "None.",
                     }},
        MessageEntry{"CoolingModeUnknown",
                     {
                         "Indicates that cooling mode could not be detected.",
                         "Cooling mode detected: Unknown",
                         "Critical",
                         0,
                         {},
                         "None.",
                     }},
        MessageEntry{
            "CoolingModeChange",
            {
                "Indicates that cooling mode has been changed by user.",
                "Switching to cooling mode: %1",
                "OK",
                1,
                {"string"},
                "None.",
            }},
        MessageEntry{
            "LeakSensorFailure",
            {
                "Indicates that leak rope cable sensor is not working properly.",
                "Leak rope sensor status: Failure",
                "Critical",
                0,
                {},
                "None.",
            }},
        MessageEntry{
            "LeakSensorOk",
            {
                "Indicates that leak rope cable sensor is working properly.",
                "Leak rope sensor status: Operational",
                "OK",
                0,
                {},
                "None.",
            }},
        MessageEntry{
            "LeakDetected",
            {
                "Indicates that leak rope cable sensor detected a leak.",
                "Leak detected",
                "Critical",
                0,
                {},
                "None.",
            }},
        MessageEntry{
            "NoLeakDetected",
            {
                "Indicates that leak rope cable sensor did not detect a leak.",
                "Leak not detected",
                "OK",
                0,
                {},
                "None.",
            }},
        MessageEntry{
            "ImmersionTsod",
            {
                "Indicates that there is likely an incorrect configuration of Memory temperature sensor with Immersion cooling.",
                "Incorrect Configuration: TSOD readings with Immersion cooling detected",
                "Critical",
                0,
                {},
                "None.",
            }},
        MessageEntry{
            "FanboardInvalid",
            {
                "Indicates that fanboard was not detected and proper cooling method cannot be applied.",
                "Incorrect Configuration: Fan board not detected",
                "Critical",
                0,
                {},
                "None.",
            }},
    };
}; // struct LiquidCooling

[[gnu::constructor]] inline void registerLiquidCooling()
{
    registerRegistry<LiquidCooling>();
}

} // namespace redfish::registries
