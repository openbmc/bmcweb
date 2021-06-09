#ifndef DATASECURITYLOSCAPABILITIES_V1
#define DATASECURITYLOSCAPABILITIES_V1

#include "DataSecurityLineOfService_v1.h"
#include "DataSecurityLoSCapabilities_v1.h"
#include "Resource_v1.h"

enum class DataSecurityLoSCapabilities_v1_AntiVirusScanTrigger
{
    None,
    OnFirstRead,
    OnPatternUpdate,
    OnUpdate,
    OnRename,
};
enum class DataSecurityLoSCapabilities_v1_AuthenticationType
{
    None,
    PKI,
    Ticket,
    Password,
};
enum class DataSecurityLoSCapabilities_v1_DataSanitizationPolicy
{
    None,
    Clear,
    CryptographicErase,
};
enum class DataSecurityLoSCapabilities_v1_KeySize
{
    Bits_0,
    Bits_112,
    Bits_128,
    Bits_192,
    Bits_256,
};
enum class DataSecurityLoSCapabilities_v1_SecureChannelProtocol
{
    None,
    TLS,
    IPsec,
    RPCSEC_GSS,
};
struct DataSecurityLoSCapabilities_v1_Actions
{
    DataSecurityLoSCapabilities_v1_OemActions oem;
};
struct DataSecurityLoSCapabilities_v1_DataSecurityLoSCapabilities
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource identifier;
    DataSecurityLoSCapabilities_v1_KeySize supportedMediaEncryptionStrengths;
    DataSecurityLoSCapabilities_v1_KeySize supportedChannelEncryptionStrengths;
    DataSecurityLoSCapabilities_v1_AuthenticationType
        supportedHostAuthenticationTypes;
    DataSecurityLoSCapabilities_v1_AuthenticationType
        supportedUserAuthenticationTypes;
    DataSecurityLoSCapabilities_v1_SecureChannelProtocol
        supportedSecureChannelProtocols;
    DataSecurityLoSCapabilities_v1_AntiVirusScanTrigger
        supportedAntivirusScanPolicies;
    std::string supportedAntivirusEngineProviders;
    DataSecurityLoSCapabilities_v1_DataSanitizationPolicy
        supportedDataSanitizationPolicies;
    DataSecurityLineOfService_v1_DataSecurityLineOfService
        supportedLinesOfService;
    DataSecurityLoSCapabilities_v1_Actions actions;
};
struct DataSecurityLoSCapabilities_v1_OemActions
{};
#endif
