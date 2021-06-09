#ifndef DATASECURITYLOSCAPABILITIES_V1
#define DATASECURITYLOSCAPABILITIES_V1

#include "DataSecurityLineOfService_v1.h"
#include "DataSecurityLoSCapabilities_v1.h"
#include "Resource_v1.h"

enum class DataSecurityLoSCapabilitiesV1AntiVirusScanTrigger
{
    None,
    OnFirstRead,
    OnPatternUpdate,
    OnUpdate,
    OnRename,
};
enum class DataSecurityLoSCapabilitiesV1AuthenticationType
{
    None,
    PKI,
    Ticket,
    Password,
};
enum class DataSecurityLoSCapabilitiesV1DataSanitizationPolicy
{
    None,
    Clear,
    CryptographicErase,
};
enum class DataSecurityLoSCapabilitiesV1KeySize
{
    Bits_0,
    Bits_112,
    Bits_128,
    Bits_192,
    Bits_256,
};
enum class DataSecurityLoSCapabilitiesV1SecureChannelProtocol
{
    None,
    TLS,
    IPsec,
    RPCSEC_GSS,
};
struct DataSecurityLoSCapabilitiesV1OemActions
{};
struct DataSecurityLoSCapabilitiesV1Actions
{
    DataSecurityLoSCapabilitiesV1OemActions oem;
};
struct DataSecurityLoSCapabilitiesV1DataSecurityLoSCapabilities
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource identifier;
    DataSecurityLoSCapabilitiesV1KeySize supportedMediaEncryptionStrengths;
    DataSecurityLoSCapabilitiesV1KeySize supportedChannelEncryptionStrengths;
    DataSecurityLoSCapabilitiesV1AuthenticationType
        supportedHostAuthenticationTypes;
    DataSecurityLoSCapabilitiesV1AuthenticationType
        supportedUserAuthenticationTypes;
    DataSecurityLoSCapabilitiesV1SecureChannelProtocol
        supportedSecureChannelProtocols;
    DataSecurityLoSCapabilitiesV1AntiVirusScanTrigger
        supportedAntivirusScanPolicies;
    std::string supportedAntivirusEngineProviders;
    DataSecurityLoSCapabilitiesV1DataSanitizationPolicy
        supportedDataSanitizationPolicies;
    DataSecurityLineOfServiceV1DataSecurityLineOfService
        supportedLinesOfService;
    DataSecurityLoSCapabilitiesV1Actions actions;
};
#endif
