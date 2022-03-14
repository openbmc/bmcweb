#pragma once
#include <nlohmann/json.hpp>

namespace event_service
{
// clang-format off

enum class SMTPConnectionProtocol{
    Invalid,
    None,
    AutoDetect,
    StartTLS,
    TLS_SSL,
};

enum class SMTPAuthenticationMethods{
    Invalid,
    None,
    AutoDetect,
    Plain,
    Login,
    CRAM_MD5,
};

NLOHMANN_JSON_SERIALIZE_ENUM(SMTPConnectionProtocol, {
    {SMTPConnectionProtocol::Invalid, "Invalid"},
    {SMTPConnectionProtocol::None, "None"},
    {SMTPConnectionProtocol::AutoDetect, "AutoDetect"},
    {SMTPConnectionProtocol::StartTLS, "StartTLS"},
    {SMTPConnectionProtocol::TLS_SSL, "TLS_SSL"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(SMTPAuthenticationMethods, {
    {SMTPAuthenticationMethods::Invalid, "Invalid"},
    {SMTPAuthenticationMethods::None, "None"},
    {SMTPAuthenticationMethods::AutoDetect, "AutoDetect"},
    {SMTPAuthenticationMethods::Plain, "Plain"},
    {SMTPAuthenticationMethods::Login, "Login"},
    {SMTPAuthenticationMethods::CRAM_MD5, "CRAM_MD5"},
});

}
// clang-format on
