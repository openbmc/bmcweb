// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

namespace resolution_step
{
// clang-format off

enum class ResolutionType{
    Invalid,
    ContactVendor,
    ReplaceComponent,
    FirmwareUpdate,
    Reset,
    PowerCycle,
    ResetToDefaults,
    CollectDiagnosticData,
    OEM,
    RetryOperation,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ResolutionType, {
    {ResolutionType::Invalid, "Invalid"},
    {ResolutionType::ContactVendor, "ContactVendor"},
    {ResolutionType::ReplaceComponent, "ReplaceComponent"},
    {ResolutionType::FirmwareUpdate, "FirmwareUpdate"},
    {ResolutionType::Reset, "Reset"},
    {ResolutionType::PowerCycle, "PowerCycle"},
    {ResolutionType::ResetToDefaults, "ResetToDefaults"},
    {ResolutionType::CollectDiagnosticData, "CollectDiagnosticData"},
    {ResolutionType::OEM, "OEM"},
    {ResolutionType::RetryOperation, "RetryOperation"},
});

// clang-format on
} // namespace resolution_step
