// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

// clang-format off

namespace facility
{
enum class FacilityType{
    Invalid,
    Room,
    Floor,
    Building,
    Site,
};

NLOHMANN_JSON_SERIALIZE_ENUM(FacilityType, {
    {FacilityType::Invalid, "Invalid"},
    {FacilityType::Room, "Room"},
    {FacilityType::Floor, "Floor"},
    {FacilityType::Building, "Building"},
    {FacilityType::Site, "Site"},
});

}
// clang-format on
