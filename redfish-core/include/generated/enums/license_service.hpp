#pragma once
#include <boost/describe/enum.hpp>
#include <nlohmann/json.hpp>

namespace license_service
{
// clang-format off

enum class TransferProtocolType{
    Invalid,
    CIFS,
    FTP,
    SFTP,
    HTTP,
    HTTPS,
    SCP,
    TFTP,
    OEM,
    NFS,
};

NLOHMANN_JSON_SERIALIZE_ENUM(TransferProtocolType, {
    {TransferProtocolType::Invalid, "Invalid"},
    {TransferProtocolType::CIFS, "CIFS"},
    {TransferProtocolType::FTP, "FTP"},
    {TransferProtocolType::SFTP, "SFTP"},
    {TransferProtocolType::HTTP, "HTTP"},
    {TransferProtocolType::HTTPS, "HTTPS"},
    {TransferProtocolType::SCP, "SCP"},
    {TransferProtocolType::TFTP, "TFTP"},
    {TransferProtocolType::OEM, "OEM"},
    {TransferProtocolType::NFS, "NFS"},
});

BOOST_DESCRIBE_ENUM(TransferProtocolType,

    Invalid,
    CIFS,
    FTP,
    SFTP,
    HTTP,
    HTTPS,
    SCP,
    TFTP,
    OEM,
    NFS,
);

}
// clang-format on
