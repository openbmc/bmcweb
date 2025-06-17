// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

namespace virtual_pci2_pci_bridge
{
// clang-format off

enum class vPPBStatusTypes{
    Invalid,
    Unbound,
    Busy,
    BoundPhysicalPort,
    BoundLD,
    BoundPID,
};

NLOHMANN_JSON_SERIALIZE_ENUM(vPPBStatusTypes, {
    {vPPBStatusTypes::Invalid, "Invalid"},
    {vPPBStatusTypes::Unbound, "Unbound"},
    {vPPBStatusTypes::Busy, "Busy"},
    {vPPBStatusTypes::BoundPhysicalPort, "BoundPhysicalPort"},
    {vPPBStatusTypes::BoundLD, "BoundLD"},
    {vPPBStatusTypes::BoundPID, "BoundPID"},
});

}
// clang-format on
