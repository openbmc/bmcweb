#pragma once
#include <nlohmann/json.hpp>

namespace event_destination
{
// clang-format off

enum class EventFormatType{
    Invalid,
    Event,
    MetricReport,
};

enum class EventDestinationProtocol{
    Invalid,
    Redfish,
    Kafka,
    SNMPv1,
    SNMPv2c,
    SNMPv3,
    SMTP,
    SyslogTLS,
    SyslogTCP,
    SyslogUDP,
    SyslogRELP,
    OEM,
};

enum class SubscriptionType{
    Invalid,
    RedfishEvent,
    SSE,
    SNMPTrap,
    SNMPInform,
    Syslog,
    OEM,
};

enum class DeliveryRetryPolicy{
    Invalid,
    TerminateAfterRetries,
    SuspendRetries,
    RetryForever,
    RetryForeverWithBackoff,
};

enum class SNMPAuthenticationProtocols{
    Invalid,
    None,
    CommunityString,
    HMAC_MD5,
    HMAC_SHA96,
    HMAC128_SHA224,
    HMAC192_SHA256,
    HMAC256_SHA384,
    HMAC384_SHA512,
};

enum class SNMPEncryptionProtocols{
    Invalid,
    None,
    CBC_DES,
    CFB128_AES128,
};

enum class SyslogSeverity{
    Invalid,
    Emergency,
    Alert,
    Critical,
    Error,
    Warning,
    Notice,
    Informational,
    Debug,
    All,
};

enum class SyslogFacility{
    Invalid,
    Kern,
    User,
    Mail,
    Daemon,
    Auth,
    Syslog,
    LPR,
    News,
    UUCP,
    Cron,
    Authpriv,
    FTP,
    NTP,
    Security,
    Console,
    SolarisCron,
    Local0,
    Local1,
    Local2,
    Local3,
    Local4,
    Local5,
    Local6,
    Local7,
};

NLOHMANN_JSON_SERIALIZE_ENUM(EventFormatType, {
    {EventFormatType::Invalid, "Invalid"},
    {EventFormatType::Event, "Event"},
    {EventFormatType::MetricReport, "MetricReport"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(EventDestinationProtocol, {
    {EventDestinationProtocol::Invalid, "Invalid"},
    {EventDestinationProtocol::Redfish, "Redfish"},
    {EventDestinationProtocol::Kafka, "Kafka"},
    {EventDestinationProtocol::SNMPv1, "SNMPv1"},
    {EventDestinationProtocol::SNMPv2c, "SNMPv2c"},
    {EventDestinationProtocol::SNMPv3, "SNMPv3"},
    {EventDestinationProtocol::SMTP, "SMTP"},
    {EventDestinationProtocol::SyslogTLS, "SyslogTLS"},
    {EventDestinationProtocol::SyslogTCP, "SyslogTCP"},
    {EventDestinationProtocol::SyslogUDP, "SyslogUDP"},
    {EventDestinationProtocol::SyslogRELP, "SyslogRELP"},
    {EventDestinationProtocol::OEM, "OEM"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(SubscriptionType, {
    {SubscriptionType::Invalid, "Invalid"},
    {SubscriptionType::RedfishEvent, "RedfishEvent"},
    {SubscriptionType::SSE, "SSE"},
    {SubscriptionType::SNMPTrap, "SNMPTrap"},
    {SubscriptionType::SNMPInform, "SNMPInform"},
    {SubscriptionType::Syslog, "Syslog"},
    {SubscriptionType::OEM, "OEM"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(DeliveryRetryPolicy, {
    {DeliveryRetryPolicy::Invalid, "Invalid"},
    {DeliveryRetryPolicy::TerminateAfterRetries, "TerminateAfterRetries"},
    {DeliveryRetryPolicy::SuspendRetries, "SuspendRetries"},
    {DeliveryRetryPolicy::RetryForever, "RetryForever"},
    {DeliveryRetryPolicy::RetryForeverWithBackoff, "RetryForeverWithBackoff"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(SNMPAuthenticationProtocols, {
    {SNMPAuthenticationProtocols::Invalid, "Invalid"},
    {SNMPAuthenticationProtocols::None, "None"},
    {SNMPAuthenticationProtocols::CommunityString, "CommunityString"},
    {SNMPAuthenticationProtocols::HMAC_MD5, "HMAC_MD5"},
    {SNMPAuthenticationProtocols::HMAC_SHA96, "HMAC_SHA96"},
    {SNMPAuthenticationProtocols::HMAC128_SHA224, "HMAC128_SHA224"},
    {SNMPAuthenticationProtocols::HMAC192_SHA256, "HMAC192_SHA256"},
    {SNMPAuthenticationProtocols::HMAC256_SHA384, "HMAC256_SHA384"},
    {SNMPAuthenticationProtocols::HMAC384_SHA512, "HMAC384_SHA512"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(SNMPEncryptionProtocols, {
    {SNMPEncryptionProtocols::Invalid, "Invalid"},
    {SNMPEncryptionProtocols::None, "None"},
    {SNMPEncryptionProtocols::CBC_DES, "CBC_DES"},
    {SNMPEncryptionProtocols::CFB128_AES128, "CFB128_AES128"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(SyslogSeverity, {
    {SyslogSeverity::Invalid, "Invalid"},
    {SyslogSeverity::Emergency, "Emergency"},
    {SyslogSeverity::Alert, "Alert"},
    {SyslogSeverity::Critical, "Critical"},
    {SyslogSeverity::Error, "Error"},
    {SyslogSeverity::Warning, "Warning"},
    {SyslogSeverity::Notice, "Notice"},
    {SyslogSeverity::Informational, "Informational"},
    {SyslogSeverity::Debug, "Debug"},
    {SyslogSeverity::All, "All"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(SyslogFacility, {
    {SyslogFacility::Invalid, "Invalid"},
    {SyslogFacility::Kern, "Kern"},
    {SyslogFacility::User, "User"},
    {SyslogFacility::Mail, "Mail"},
    {SyslogFacility::Daemon, "Daemon"},
    {SyslogFacility::Auth, "Auth"},
    {SyslogFacility::Syslog, "Syslog"},
    {SyslogFacility::LPR, "LPR"},
    {SyslogFacility::News, "News"},
    {SyslogFacility::UUCP, "UUCP"},
    {SyslogFacility::Cron, "Cron"},
    {SyslogFacility::Authpriv, "Authpriv"},
    {SyslogFacility::FTP, "FTP"},
    {SyslogFacility::NTP, "NTP"},
    {SyslogFacility::Security, "Security"},
    {SyslogFacility::Console, "Console"},
    {SyslogFacility::SolarisCron, "SolarisCron"},
    {SyslogFacility::Local0, "Local0"},
    {SyslogFacility::Local1, "Local1"},
    {SyslogFacility::Local2, "Local2"},
    {SyslogFacility::Local3, "Local3"},
    {SyslogFacility::Local4, "Local4"},
    {SyslogFacility::Local5, "Local5"},
    {SyslogFacility::Local6, "Local6"},
    {SyslogFacility::Local7, "Local7"},
});

}
// clang-format on
