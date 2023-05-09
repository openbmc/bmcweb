#pragma once
#include <boost/describe/enum.hpp>
#include <nlohmann/json.hpp>

namespace settings
{
// clang-format off

enum class OperationApplyTime{
    Invalid,
    Immediate,
    OnReset,
    AtMaintenanceWindowStart,
    InMaintenanceWindowOnReset,
    OnStartUpdateRequest,
    OnTargetReset,
};

enum class ApplyTime{
    Invalid,
    Immediate,
    OnReset,
    AtMaintenanceWindowStart,
    InMaintenanceWindowOnReset,
};

NLOHMANN_JSON_SERIALIZE_ENUM(OperationApplyTime, {
    {OperationApplyTime::Invalid, "Invalid"},
    {OperationApplyTime::Immediate, "Immediate"},
    {OperationApplyTime::OnReset, "OnReset"},
    {OperationApplyTime::AtMaintenanceWindowStart, "AtMaintenanceWindowStart"},
    {OperationApplyTime::InMaintenanceWindowOnReset, "InMaintenanceWindowOnReset"},
    {OperationApplyTime::OnStartUpdateRequest, "OnStartUpdateRequest"},
    {OperationApplyTime::OnTargetReset, "OnTargetReset"},
});

BOOST_DESCRIBE_ENUM(OperationApplyTime,

    Invalid,
    Immediate,
    OnReset,
    AtMaintenanceWindowStart,
    InMaintenanceWindowOnReset,
    OnStartUpdateRequest,
    OnTargetReset,
);

NLOHMANN_JSON_SERIALIZE_ENUM(ApplyTime, {
    {ApplyTime::Invalid, "Invalid"},
    {ApplyTime::Immediate, "Immediate"},
    {ApplyTime::OnReset, "OnReset"},
    {ApplyTime::AtMaintenanceWindowStart, "AtMaintenanceWindowStart"},
    {ApplyTime::InMaintenanceWindowOnReset, "InMaintenanceWindowOnReset"},
});

BOOST_DESCRIBE_ENUM(ApplyTime,

    Invalid,
    Immediate,
    OnReset,
    AtMaintenanceWindowStart,
    InMaintenanceWindowOnReset,
);

}
// clang-format on
