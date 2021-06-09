#ifndef EVENTSERVICE_V1
#define EVENTSERVICE_V1

#include "EventDestinationCollection_v1.h"
#include "EventDestination_v1.h"
#include "EventService_v1.h"
#include "Event_v1.h"
#include "Resource_v1.h"

enum class EventServiceV1SMTPAuthenticationMethods
{
    None,
    AutoDetect,
    Plain,
    Login,
    CRAM_MD5,
};
enum class EventServiceV1SMTPConnectionProtocol
{
    None,
    AutoDetect,
    StartTLS,
    TLS_SSL,
};
struct EventServiceV1OemActions
{};
struct EventServiceV1Actions
{
    EventServiceV1OemActions oem;
};
struct EventServiceV1SSEFilterPropertiesSupported
{
    bool eventType;
    bool registryPrefix;
    bool resourceType;
    bool eventFormatType;
    bool messageId;
    bool originResource;
    bool subordinateResources;
};
struct EventServiceV1SMTP
{
    bool serviceEnabled;
    int64_t port;
    std::string serverAddress;
    std::string fromAddress;
    EventServiceV1SMTPConnectionProtocol connectionProtocol;
    EventServiceV1SMTPAuthenticationMethods authentication;
    std::string username;
    std::string password;
};
struct EventServiceV1EventService
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    bool serviceEnabled;
    int64_t deliveryRetryAttempts;
    int64_t deliveryRetryIntervalSeconds;
    EventV1Event eventTypesForSubscription;
    EventServiceV1Actions actions;
    EventDestinationCollectionV1EventDestinationCollection subscriptions;
    ResourceV1Resource status;
    std::string serverSentEventUri;
    std::string registryPrefixes;
    std::string resourceTypes;
    bool subordinateResourcesSupported;
    EventDestinationV1EventDestination eventFormatTypes;
    EventServiceV1SSEFilterPropertiesSupported sSEFilterPropertiesSupported;
    EventServiceV1SMTP SMTP;
    bool includeOriginOfConditionSupported;
};
#endif
