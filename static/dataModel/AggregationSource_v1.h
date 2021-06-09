#ifndef AGGREGATIONSOURCE_V1
#define AGGREGATIONSOURCE_V1

#include "AggregationSource_v1.h"
#include "NavigationReference_.h"
#include "Resource_v1.h"

enum class AggregationSourceV1SNMPAuthenticationProtocols
{
    None,
    CommunityString,
    HMAC_MD5,
    HMAC_SHA96,
    HMAC128_SHA224,
    HMAC192_SHA256,
    HMAC256_SHA384,
    HMAC384_SHA512,
};
enum class AggregationSourceV1SNMPEncryptionProtocols
{
    None,
    CBC_DES,
    CFB128_AES128,
};
struct AggregationSourceV1OemActions
{};
struct AggregationSourceV1Actions
{
    AggregationSourceV1OemActions oem;
};
struct AggregationSourceV1Links
{
    ResourceV1Resource oem;
    NavigationReference_ connectionMethod;
    NavigationReference_ resourcesAccessed;
};
struct AggregationSourceV1SNMPSettings
{
    std::string authenticationKey;
    AggregationSourceV1SNMPAuthenticationProtocols authenticationProtocol;
    std::string encryptionKey;
    AggregationSourceV1SNMPEncryptionProtocols encryptionProtocol;
    bool authenticationKeySet;
    bool encryptionKeySet;
};
struct AggregationSourceV1AggregationSource
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string hostName;
    std::string userName;
    std::string password;
    AggregationSourceV1Links links;
    AggregationSourceV1Actions actions;
    AggregationSourceV1SNMPSettings SNMP;
};
#endif
