#pragma once
#include <nlohmann/json.hpp>

namespace account_service
{
// clang-format off

enum class LocalAccountAuth{
    Invalid,
    Enabled,
    Disabled,
    Fallback,
    LocalFirst,
};

enum class AccountProviderTypes{
    Invalid,
    RedfishService,
    ActiveDirectoryService,
    LDAPService,
    OEM,
    TACACSplus,
    OAuth2,
};

enum class AuthenticationTypes{
    Invalid,
    Token,
    KerberosKeytab,
    UsernameAndPassword,
    OEM,
};

enum class TACACSplusPasswordExchangeProtocol{
    Invalid,
    ASCII,
    PAP,
    CHAP,
    MSCHAPv1,
    MSCHAPv2,
};

enum class OAuth2Mode{
    Invalid,
    Discovery,
    Offline,
};

NLOHMANN_JSON_SERIALIZE_ENUM(LocalAccountAuth, { //NOLINT
    {LocalAccountAuth::Invalid, "Invalid"},
    {LocalAccountAuth::Enabled, "Enabled"},
    {LocalAccountAuth::Disabled, "Disabled"},
    {LocalAccountAuth::Fallback, "Fallback"},
    {LocalAccountAuth::LocalFirst, "LocalFirst"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(AccountProviderTypes, { //NOLINT
    {AccountProviderTypes::Invalid, "Invalid"},
    {AccountProviderTypes::RedfishService, "RedfishService"},
    {AccountProviderTypes::ActiveDirectoryService, "ActiveDirectoryService"},
    {AccountProviderTypes::LDAPService, "LDAPService"},
    {AccountProviderTypes::OEM, "OEM"},
    {AccountProviderTypes::TACACSplus, "TACACSplus"},
    {AccountProviderTypes::OAuth2, "OAuth2"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(AuthenticationTypes, { //NOLINT
    {AuthenticationTypes::Invalid, "Invalid"},
    {AuthenticationTypes::Token, "Token"},
    {AuthenticationTypes::KerberosKeytab, "KerberosKeytab"},
    {AuthenticationTypes::UsernameAndPassword, "UsernameAndPassword"},
    {AuthenticationTypes::OEM, "OEM"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(TACACSplusPasswordExchangeProtocol, { //NOLINT
    {TACACSplusPasswordExchangeProtocol::Invalid, "Invalid"},
    {TACACSplusPasswordExchangeProtocol::ASCII, "ASCII"},
    {TACACSplusPasswordExchangeProtocol::PAP, "PAP"},
    {TACACSplusPasswordExchangeProtocol::CHAP, "CHAP"},
    {TACACSplusPasswordExchangeProtocol::MSCHAPv1, "MSCHAPv1"},
    {TACACSplusPasswordExchangeProtocol::MSCHAPv2, "MSCHAPv2"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(OAuth2Mode, { //NOLINT
    {OAuth2Mode::Invalid, "Invalid"},
    {OAuth2Mode::Discovery, "Discovery"},
    {OAuth2Mode::Offline, "Offline"},
});

}
// clang-format on
