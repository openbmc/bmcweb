#ifndef EVENTDESTINATION_V1
#define EVENTDESTINATION_V1

#include "CertificateCollection_v1.h"
#include "EventDestination_v1.h"
#include "Event_v1.h"
#include "NavigationReference_.h"
#include "Resource_v1.h"

enum class EventDestinationV1DeliveryRetryPolicy
{
    TerminateAfterRetries,
    SuspendRetries,
    RetryForever,
    RetryForeverWithBackoff,
};
enum class EventDestinationV1EventDestinationProtocol
{
    Redfish,
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
enum class EventDestinationV1EventFormatType
{
    Event,
    MetricReport,
};
enum class EventDestinationV1SNMPAuthenticationProtocols
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
enum class EventDestinationV1SNMPEncryptionProtocols
{
    None,
    CBC_DES,
    CFB128_AES128,
};
enum class EventDestinationV1SubscriptionType
{
    RedfishEvent,
    SSE,
    SNMPTrap,
    SNMPInform,
    Syslog,
    OEM,
};
enum class EventDestinationV1SyslogFacility
{
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
enum class EventDestinationV1SyslogSeverity
{
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
struct EventDestinationV1OemActions
{};
struct EventDestinationV1Actions
{
    EventDestinationV1OemActions oem;
};
struct EventDestinationV1HttpHeaderProperty
{};
struct EventDestinationV1SNMPSettings
{
    std::string trapCommunity;
    std::string authenticationKey;
    EventDestinationV1SNMPAuthenticationProtocols authenticationProtocol;
    std::string encryptionKey;
    EventDestinationV1SNMPEncryptionProtocols encryptionProtocol;
    bool authenticationKeySet;
    bool encryptionKeySet;
};
struct EventDestinationV1SyslogFilter
{
    EventDestinationV1SyslogSeverity lowestSeverity;
    EventDestinationV1SyslogFacility logFacilities;
};
struct EventDestinationV1EventDestination
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string destination;
    EventV1Event eventTypes;
    std::string context;
    EventDestinationV1EventDestinationProtocol protocol;
    EventDestinationV1HttpHeaderProperty httpHeaders;
    NavigationReference_ originResources;
    std::string messageIds;
    EventDestinationV1Actions actions;
    EventDestinationV1SubscriptionType subscriptionType;
    std::string registryPrefixes;
    std::string resourceTypes;
    bool subordinateResources;
    EventDestinationV1EventFormatType eventFormatType;
    EventDestinationV1DeliveryRetryPolicy deliveryRetryPolicy;
    ResourceV1Resource status;
    NavigationReference_ metricReportDefinitions;
    EventDestinationV1SNMPSettings SNMP;
    bool includeOriginOfCondition;
    CertificateCollectionV1CertificateCollection certificates;
    bool verifyCertificate;
    EventDestinationV1SyslogFilter syslogFilters;
    std::string oEMProtocol;
    std::string oEMSubscriptionType;
};
#endif
