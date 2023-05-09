#pragma once
#include <boost/describe/enum.hpp>
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

BOOST_DESCRIBE_ENUM(FirmwareProvisioningStatus,

    Invalid,
    NotProvisioned,
    ProvisionedButNotLocked,
    ProvisionedAndLocked,
);

}
// clang-format on
