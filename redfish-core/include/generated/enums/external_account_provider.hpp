#pragma once
#include <nlohmann/json.hpp>

namespace external_account_provider
{
// clang-format off

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

NLOHMANN_JSON_SERIALIZE_ENUM(AccountProviderTypes, {
    {AccountProviderTypes::Invalid, "Invalid"},
    {AccountProviderTypes::RedfishService, "RedfishService"},
    {AccountProviderTypes::ActiveDirectoryService, "ActiveDirectoryService"},
    {AccountProviderTypes::LDAPService, "LDAPService"},
    {AccountProviderTypes::OEM, "OEM"},
    {AccountProviderTypes::TACACSplus, "TACACSplus"},
    {AccountProviderTypes::OAuth2, "OAuth2"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(AuthenticationTypes, {
    {AuthenticationTypes::Invalid, "Invalid"},
    {AuthenticationTypes::Token, "Token"},
    {AuthenticationTypes::KerberosKeytab, "KerberosKeytab"},
    {AuthenticationTypes::UsernameAndPassword, "UsernameAndPassword"},
    {AuthenticationTypes::OEM, "OEM"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(TACACSplusPasswordExchangeProtocol, {
    {TACACSplusPasswordExchangeProtocol::Invalid, "Invalid"},
    {TACACSplusPasswordExchangeProtocol::ASCII, "ASCII"},
    {TACACSplusPasswordExchangeProtocol::PAP, "PAP"},
    {TACACSplusPasswordExchangeProtocol::CHAP, "CHAP"},
    {TACACSplusPasswordExchangeProtocol::MSCHAPv1, "MSCHAPv1"},
    {TACACSplusPasswordExchangeProtocol::MSCHAPv2, "MSCHAPv2"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(OAuth2Mode, {
    {OAuth2Mode::Invalid, "Invalid"},
    {OAuth2Mode::Discovery, "Discovery"},
    {OAuth2Mode::Offline, "Offline"},
});

}
// clang-format on
