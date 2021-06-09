#ifndef DATASECURITYLINEOFSERVICE_V1
#define DATASECURITYLINEOFSERVICE_V1

#include "DataSecurityLineOfService_v1.h"
#include "DataSecurityLoSCapabilities_v1.h"
#include "Resource_v1.h"

struct DataSecurityLineOfService_v1_Actions
{
    DataSecurityLineOfService_v1_OemActions oem;
};
struct DataSecurityLineOfService_v1_DataSecurityLineOfService
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    DataSecurityLoSCapabilities_v1_DataSecurityLoSCapabilities
        mediaEncryptionStrength;
    DataSecurityLoSCapabilities_v1_DataSecurityLoSCapabilities
        channelEncryptionStrength;
    DataSecurityLoSCapabilities_v1_DataSecurityLoSCapabilities
        hostAuthenticationType;
    DataSecurityLoSCapabilities_v1_DataSecurityLoSCapabilities
        userAuthenticationType;
    DataSecurityLoSCapabilities_v1_DataSecurityLoSCapabilities
        secureChannelProtocol;
    DataSecurityLoSCapabilities_v1_DataSecurityLoSCapabilities
        antivirusScanPolicies;
    std::string antivirusEngineProvider;
    DataSecurityLoSCapabilities_v1_DataSecurityLoSCapabilities
        dataSanitizationPolicy;
    DataSecurityLineOfService_v1_Actions actions;
};
struct DataSecurityLineOfService_v1_OemActions
{};
#endif
