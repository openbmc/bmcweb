// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

namespace open_bmc_computer_system
{
// clang-format off

enum class FirmwareProvisioningStatus{
    Invalid,
    NotProvisioned,
    ProvisionedButNotLocked,
    ProvisionedAndLocked,
};

NLOHMANN_JSON_SERIALIZE_ENUM(FirmwareProvisioningStatus, {
    {FirmwareProvisioningStatus::Invalid, "Invalid"},
    {FirmwareProvisioningStatus::NotProvisioned, "NotProvisioned"},
    {FirmwareProvisioningStatus::ProvisionedButNotLocked, "ProvisionedButNotLocked"},
    {FirmwareProvisioningStatus::ProvisionedAndLocked, "ProvisionedAndLocked"},
});

}
// clang-format on
