#ifndef DATASECURITYLINEOFSERVICE_V1
#define DATASECURITYLINEOFSERVICE_V1

#include "DataSecurityLineOfService_v1.h"
#include "DataSecurityLoSCapabilities_v1.h"
#include "Resource_v1.h"

struct DataSecurityLineOfServiceV1OemActions
{};
struct DataSecurityLineOfServiceV1Actions
{
    DataSecurityLineOfServiceV1OemActions oem;
};
struct DataSecurityLineOfServiceV1DataSecurityLineOfService
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    DataSecurityLoSCapabilitiesV1DataSecurityLoSCapabilities
        mediaEncryptionStrength;
    DataSecurityLoSCapabilitiesV1DataSecurityLoSCapabilities
        channelEncryptionStrength;
    DataSecurityLoSCapabilitiesV1DataSecurityLoSCapabilities
        hostAuthenticationType;
    DataSecurityLoSCapabilitiesV1DataSecurityLoSCapabilities
        userAuthenticationType;
    DataSecurityLoSCapabilitiesV1DataSecurityLoSCapabilities
        secureChannelProtocol;
    DataSecurityLoSCapabilitiesV1DataSecurityLoSCapabilities
        antivirusScanPolicies;
    std::string antivirusEngineProvider;
    DataSecurityLoSCapabilitiesV1DataSecurityLoSCapabilities
        dataSanitizationPolicy;
    DataSecurityLineOfServiceV1Actions actions;
};
#endif
