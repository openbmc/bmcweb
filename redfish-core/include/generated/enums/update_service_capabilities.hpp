// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

namespace update_service_capabilities
{
// clang-format off

enum class AllowableTargetReferenceType{
    Invalid,
    SoftwareInventory,
    DeviceResource,
    ResourceCollection,
    Aggregate,
    ComputerSystem,
};

NLOHMANN_JSON_SERIALIZE_ENUM(AllowableTargetReferenceType, {
    {AllowableTargetReferenceType::Invalid, "Invalid"},
    {AllowableTargetReferenceType::SoftwareInventory, "SoftwareInventory"},
    {AllowableTargetReferenceType::DeviceResource, "DeviceResource"},
    {AllowableTargetReferenceType::ResourceCollection, "ResourceCollection"},
    {AllowableTargetReferenceType::Aggregate, "Aggregate"},
    {AllowableTargetReferenceType::ComputerSystem, "ComputerSystem"},
});

// clang-format on
} // namespace update_service_capabilities
