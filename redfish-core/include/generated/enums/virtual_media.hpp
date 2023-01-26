#pragma once
#include <nlohmann/json.hpp>

namespace virtual_media
{
// clang-format off

enum class MediaType{
    Invalid,
    CD,
    Floppy,
    USBStick,
    DVD,
};

enum class ConnectedVia{
    Invalid,
    NotConnected,
    URI,
    Applet,
    Oem,
};

enum class TransferProtocolType{
    Invalid,
    CIFS,
    FTP,
    SFTP,
    HTTP,
    HTTPS,
    NFS,
    SCP,
    TFTP,
    OEM,
};

enum class TransferMethod{
    Invalid,
    Stream,
    Upload,
};

enum class EjectPolicy{
    Invalid,
    OnPowerOff,
    Session,
    Timed,
    AfterUse,
    Persistent,
};

NLOHMANN_JSON_SERIALIZE_ENUM(MediaType, {
    {MediaType::Invalid, "Invalid"},
    {MediaType::CD, "CD"},
    {MediaType::Floppy, "Floppy"},
    {MediaType::USBStick, "USBStick"},
    {MediaType::DVD, "DVD"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ConnectedVia, {
    {ConnectedVia::Invalid, "Invalid"},
    {ConnectedVia::NotConnected, "NotConnected"},
    {ConnectedVia::URI, "URI"},
    {ConnectedVia::Applet, "Applet"},
    {ConnectedVia::Oem, "Oem"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(TransferProtocolType, {
    {TransferProtocolType::Invalid, "Invalid"},
    {TransferProtocolType::CIFS, "CIFS"},
    {TransferProtocolType::FTP, "FTP"},
    {TransferProtocolType::SFTP, "SFTP"},
    {TransferProtocolType::HTTP, "HTTP"},
    {TransferProtocolType::HTTPS, "HTTPS"},
    {TransferProtocolType::NFS, "NFS"},
    {TransferProtocolType::SCP, "SCP"},
    {TransferProtocolType::TFTP, "TFTP"},
    {TransferProtocolType::OEM, "OEM"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(TransferMethod, {
    {TransferMethod::Invalid, "Invalid"},
    {TransferMethod::Stream, "Stream"},
    {TransferMethod::Upload, "Upload"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(EjectPolicy, {
    {EjectPolicy::Invalid, "Invalid"},
    {EjectPolicy::OnPowerOff, "OnPowerOff"},
    {EjectPolicy::Session, "Session"},
    {EjectPolicy::Timed, "Timed"},
    {EjectPolicy::AfterUse, "AfterUse"},
    {EjectPolicy::Persistent, "Persistent"},
});

}
// clang-format on
