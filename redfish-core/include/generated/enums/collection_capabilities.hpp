SPDX-License-Identifier: Apache-2.0
SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

namespace collection_capabilities
{
// clang-format off

enum class UseCase{
    Invalid,
    ComputerSystemComposition,
    ComputerSystemConstrainedComposition,
    VolumeCreation,
    ResourceBlockComposition,
    ResourceBlockConstrainedComposition,
    RegisterResourceBlock,
};

NLOHMANN_JSON_SERIALIZE_ENUM(UseCase, {
    {UseCase::Invalid, "Invalid"},
    {UseCase::ComputerSystemComposition, "ComputerSystemComposition"},
    {UseCase::ComputerSystemConstrainedComposition, "ComputerSystemConstrainedComposition"},
    {UseCase::VolumeCreation, "VolumeCreation"},
    {UseCase::ResourceBlockComposition, "ResourceBlockComposition"},
    {UseCase::ResourceBlockConstrainedComposition, "ResourceBlockConstrainedComposition"},
    {UseCase::RegisterResourceBlock, "RegisterResourceBlock"},
});

}
// clang-format on
