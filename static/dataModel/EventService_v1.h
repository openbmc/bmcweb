#ifndef EVENTSERVICE_V1
#define EVENTSERVICE_V1

#include "Event_v1.h"
#include "EventDestination_v1.h"
#include "EventDestinationCollection_v1.h"
#include "EventService_v1.h"
#include "Resource_v1.h"

enum class EventService_v1_EventFormatType {
    Event,
    MetricReport,
};
enum class EventService_v1_SMTPAuthenticationMethods {
    None,
    AutoDetect,
    Plain,
    Login,
    CRAM_MD5,
};
enum class EventService_v1_SMTPConnectionProtocol {
    None,
    AutoDetect,
    StartTLS,
    TLS_SSL,
};
struct EventService_v1_Actions
{
    EventService_v1_OemActions oem;
};
struct EventService_v1_EventService
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    bool serviceEnabled;
    int64_t deliveryRetryAttempts;
    int64_t deliveryRetryIntervalSeconds;
    Event_v1_Event eventTypesForSubscription;
    EventService_v1_Actions actions;
    EventDestinationCollection_v1_EventDestinationCollection subscriptions;
    Resource_v1_Resource status;
    std::string serverSentEventUri;
    std::string registryPrefixes;
    std::string resourceTypes;
    bool subordinateResourcesSupported;
    EventDestination_v1_EventDestination eventFormatTypes;
    EventService_v1_SSEFilterPropertiesSupported sSEFilterPropertiesSupported;
    EventService_v1_SMTP SMTP;
    bool includeOriginOfConditionSupported;
};
struct EventService_v1_OemActions
{
};
struct EventService_v1_SMTP
{
    bool serviceEnabled;
    int64_t port;
    std::string serverAddress;
    std::string fromAddress;
    EventService_v1_SMTPConnectionProtocol connectionProtocol;
    EventService_v1_SMTPAuthenticationMethods authentication;
    std::string username;
    std::string password;
};
struct EventService_v1_SSEFilterPropertiesSupported
{
    bool eventType;
    bool registryPrefix;
    bool resourceType;
    bool eventFormatType;
    bool messageId;
    bool originResource;
    bool subordinateResources;
};
#endif
