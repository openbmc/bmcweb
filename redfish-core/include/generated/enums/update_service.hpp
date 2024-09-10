SPDX-License-Identifier: Apache-2.0
SPDX-FileCopyrightText: Copyright OpenBMC Authors
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
    OnTargetReset,
};

enum class SupportedUpdateImageFormatType{
    Invalid,
    PLDMv1_0,
    PLDMv1_1,
    PLDMv1_2,
    PLDMv1_3,
    UEFICapsule,
    VendorDefined,
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
    {ApplyTime::OnTargetReset, "OnTargetReset"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(SupportedUpdateImageFormatType, {
    {SupportedUpdateImageFormatType::Invalid, "Invalid"},
    {SupportedUpdateImageFormatType::PLDMv1_0, "PLDMv1_0"},
    {SupportedUpdateImageFormatType::PLDMv1_1, "PLDMv1_1"},
    {SupportedUpdateImageFormatType::PLDMv1_2, "PLDMv1_2"},
    {SupportedUpdateImageFormatType::PLDMv1_3, "PLDMv1_3"},
    {SupportedUpdateImageFormatType::UEFICapsule, "UEFICapsule"},
    {SupportedUpdateImageFormatType::VendorDefined, "VendorDefined"},
});

}
// clang-format on
