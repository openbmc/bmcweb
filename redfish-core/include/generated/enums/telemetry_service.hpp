// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

// clang-format off

namespace telemetry_service
{
enum class CollectionFunction{
    Invalid,
    Average,
    Maximum,
    Minimum,
    Summation,
};

NLOHMANN_JSON_SERIALIZE_ENUM(CollectionFunction, {
    {CollectionFunction::Invalid, "Invalid"},
    {CollectionFunction::Average, "Average"},
    {CollectionFunction::Maximum, "Maximum"},
    {CollectionFunction::Minimum, "Minimum"},
    {CollectionFunction::Summation, "Summation"},
});

// clang-format on
}
