#pragma once
#include <nlohmann/json.hpp>

namespace update_service
{
// clang-format off

enum class TransferProtocolType{
    Invalid,
    CIFS,
    FTP,
    SFTP,
    HTTP,
    HTTPS,
    NSF,
    SCP,
    TFTP,
    OEM,
    NFS,
};

enum class ApplyTime{
    Invalid,
    Immediate,
    OnReset,
    AtMaintenanceWindowStart,
    InMaintenanceWindowOnReset,
    OnStartUpdateRequest,
};

NLOHMANN_JSON_SERIALIZE_ENUM(TransferProtocolType, {
    {TransferProtocolType::Invalid, "Invalid"},
    {TransferProtocolType::CIFS, "CIFS"},
    {TransferProtocolType::FTP, "FTP"},
    {TransferProtocolType::SFTP, "SFTP"},
    {TransferProtocolType::HTTP, "HTTP"},
    {TransferProtocolType::HTTPS, "HTTPS"},
    {TransferProtocolType::NSF, "NSF"},
    {TransferProtocolType::SCP, "SCP"},
    {TransferProtocolType::TFTP, "TFTP"},
    {TransferProtocolType::OEM, "OEM"},
    {TransferProtocolType::NFS, "NFS"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ApplyTime, {
    {ApplyTime::Invalid, "Invalid"},
    {ApplyTime::Immediate, "Immediate"},
    {ApplyTime::OnReset, "OnReset"},
    {ApplyTime::AtMaintenanceWindowStart, "AtMaintenanceWindowStart"},
    {ApplyTime::InMaintenanceWindowOnReset, "InMaintenanceWindowOnReset"},
    {ApplyTime::OnStartUpdateRequest, "OnStartUpdateRequest"},
});

}
// clang-format on
