#pragma once
#include <boost/describe/enum.hpp>
#include <nlohmann/json.hpp>

namespace account_service
{
// clang-format off

enum class MFABypassType{
    Invalid,
    All,
    SecurID,
    GoogleAuthenticator,
    MicrosoftAuthenticator,
    ClientCertificate,
    OneTimePasscode,
    TimeBasedOneTimePassword,
    OEM,
};

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

enum class CertificateMappingAttribute{
    Invalid,
    Whole,
    CommonName,
    UserPrincipalName,
};

enum class BasicAuthState{
    Invalid,
    Enabled,
    Unadvertised,
    Disabled,
};

NLOHMANN_JSON_SERIALIZE_ENUM(MFABypassType, {
    {MFABypassType::Invalid, "Invalid"},
    {MFABypassType::All, "All"},
    {MFABypassType::SecurID, "SecurID"},
    {MFABypassType::GoogleAuthenticator, "GoogleAuthenticator"},
    {MFABypassType::MicrosoftAuthenticator, "MicrosoftAuthenticator"},
    {MFABypassType::ClientCertificate, "ClientCertificate"},
    {MFABypassType::OneTimePasscode, "OneTimePasscode"},
    {MFABypassType::TimeBasedOneTimePassword, "TimeBasedOneTimePassword"},
    {MFABypassType::OEM, "OEM"},
});

BOOST_DESCRIBE_ENUM(MFABypassType,

    Invalid,
    All,
    SecurID,
    GoogleAuthenticator,
    MicrosoftAuthenticator,
    ClientCertificate,
    OneTimePasscode,
    TimeBasedOneTimePassword,
    OEM,
);

NLOHMANN_JSON_SERIALIZE_ENUM(LocalAccountAuth, {
    {LocalAccountAuth::Invalid, "Invalid"},
    {LocalAccountAuth::Enabled, "Enabled"},
    {LocalAccountAuth::Disabled, "Disabled"},
    {LocalAccountAuth::Fallback, "Fallback"},
    {LocalAccountAuth::LocalFirst, "LocalFirst"},
});

BOOST_DESCRIBE_ENUM(LocalAccountAuth,

    Invalid,
    Enabled,
    Disabled,
    Fallback,
    LocalFirst,
);

NLOHMANN_JSON_SERIALIZE_ENUM(AccountProviderTypes, {
    {AccountProviderTypes::Invalid, "Invalid"},
    {AccountProviderTypes::RedfishService, "RedfishService"},
    {AccountProviderTypes::ActiveDirectoryService, "ActiveDirectoryService"},
    {AccountProviderTypes::LDAPService, "LDAPService"},
    {AccountProviderTypes::OEM, "OEM"},
    {AccountProviderTypes::TACACSplus, "TACACSplus"},
    {AccountProviderTypes::OAuth2, "OAuth2"},
});

BOOST_DESCRIBE_ENUM(AccountProviderTypes,

    Invalid,
    RedfishService,
    ActiveDirectoryService,
    LDAPService,
    OEM,
    TACACSplus,
    OAuth2,
);

NLOHMANN_JSON_SERIALIZE_ENUM(AuthenticationTypes, {
    {AuthenticationTypes::Invalid, "Invalid"},
    {AuthenticationTypes::Token, "Token"},
    {AuthenticationTypes::KerberosKeytab, "KerberosKeytab"},
    {AuthenticationTypes::UsernameAndPassword, "UsernameAndPassword"},
    {AuthenticationTypes::OEM, "OEM"},
});

BOOST_DESCRIBE_ENUM(AuthenticationTypes,

    Invalid,
    Token,
    KerberosKeytab,
    UsernameAndPassword,
    OEM,
);

NLOHMANN_JSON_SERIALIZE_ENUM(TACACSplusPasswordExchangeProtocol, {
    {TACACSplusPasswordExchangeProtocol::Invalid, "Invalid"},
    {TACACSplusPasswordExchangeProtocol::ASCII, "ASCII"},
    {TACACSplusPasswordExchangeProtocol::PAP, "PAP"},
    {TACACSplusPasswordExchangeProtocol::CHAP, "CHAP"},
    {TACACSplusPasswordExchangeProtocol::MSCHAPv1, "MSCHAPv1"},
    {TACACSplusPasswordExchangeProtocol::MSCHAPv2, "MSCHAPv2"},
});

BOOST_DESCRIBE_ENUM(TACACSplusPasswordExchangeProtocol,

    Invalid,
    ASCII,
    PAP,
    CHAP,
    MSCHAPv1,
    MSCHAPv2,
);

NLOHMANN_JSON_SERIALIZE_ENUM(OAuth2Mode, {
    {OAuth2Mode::Invalid, "Invalid"},
    {OAuth2Mode::Discovery, "Discovery"},
    {OAuth2Mode::Offline, "Offline"},
});

BOOST_DESCRIBE_ENUM(OAuth2Mode,

    Invalid,
    Discovery,
    Offline,
);

NLOHMANN_JSON_SERIALIZE_ENUM(CertificateMappingAttribute, {
    {CertificateMappingAttribute::Invalid, "Invalid"},
    {CertificateMappingAttribute::Whole, "Whole"},
    {CertificateMappingAttribute::CommonName, "CommonName"},
    {CertificateMappingAttribute::UserPrincipalName, "UserPrincipalName"},
});

BOOST_DESCRIBE_ENUM(CertificateMappingAttribute,

    Invalid,
    Whole,
    CommonName,
    UserPrincipalName,
);

NLOHMANN_JSON_SERIALIZE_ENUM(BasicAuthState, {
    {BasicAuthState::Invalid, "Invalid"},
    {BasicAuthState::Enabled, "Enabled"},
    {BasicAuthState::Unadvertised, "Unadvertised"},
    {BasicAuthState::Disabled, "Disabled"},
});

BOOST_DESCRIBE_ENUM(BasicAuthState,

    Invalid,
    Enabled,
    Unadvertised,
    Disabled,
);

}
// clang-format on
