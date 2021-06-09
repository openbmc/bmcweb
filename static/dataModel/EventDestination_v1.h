#ifndef EVENTDESTINATION_V1
#define EVENTDESTINATION_V1

#include "CertificateCollection_v1.h"
#include "Event_v1.h"
#include "EventDestination_v1.h"
#include "NavigationReference__.h"
#include "Resource_v1.h"

enum class EventDestination_v1_DeliveryRetryPolicy {
    TerminateAfterRetries,
    SuspendRetries,
    RetryForever,
};
enum class EventDestination_v1_EventDestinationProtocol {
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
enum class EventDestination_v1_EventFormatType {
    Event,
    MetricReport,
};
enum class EventDestination_v1_SNMPAuthenticationProtocols {
    None,
    CommunityString,
    HMAC_MD5,
    HMAC_SHA96,
};
enum class EventDestination_v1_SNMPEncryptionProtocols {
    None,
    CBC_DES,
    CFB128_AES128,
};
enum class EventDestination_v1_SubscriptionType {
    RedfishEvent,
    SSE,
    SNMPTrap,
    SNMPInform,
    Syslog,
    OEM,
};
enum class EventDestination_v1_SyslogFacility {
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
enum class EventDestination_v1_SyslogSeverity {
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
struct EventDestination_v1_Actions
{
    EventDestination_v1_OemActions oem;
};
struct EventDestination_v1_EventDestination
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string destination;
    Event_v1_Event eventTypes;
    std::string context;
    EventDestination_v1_EventDestinationProtocol protocol;
    EventDestination_v1_HttpHeaderProperty httpHeaders;
    NavigationReference__ originResources;
    std::string messageIds;
    EventDestination_v1_Actions actions;
    EventDestination_v1_SubscriptionType subscriptionType;
    std::string registryPrefixes;
    std::string resourceTypes;
    bool subordinateResources;
    EventDestination_v1_EventFormatType eventFormatType;
    EventDestination_v1_DeliveryRetryPolicy deliveryRetryPolicy;
    Resource_v1_Resource status;
    NavigationReference__ metricReportDefinitions;
    EventDestination_v1_SNMPSettings SNMP;
    bool includeOriginOfCondition;
    CertificateCollection_v1_CertificateCollection certificates;
    bool verifyCertificate;
    EventDestination_v1_SyslogFilter syslogFilters;
    std::string oEMProtocol;
    std::string oEMSubscriptionType;
};
struct EventDestination_v1_HttpHeaderProperty
{
};
struct EventDestination_v1_OemActions
{
};
struct EventDestination_v1_SNMPSettings
{
    std::string trapCommunity;
    std::string authenticationKey;
    EventDestination_v1_SNMPAuthenticationProtocols authenticationProtocol;
    std::string encryptionKey;
    EventDestination_v1_SNMPEncryptionProtocols encryptionProtocol;
};
struct EventDestination_v1_SyslogFilter
{
    EventDestination_v1_SyslogSeverity lowestSeverity;
    EventDestination_v1_SyslogFacility logFacilities;
};
#endif
