#ifndef EVENT_V1
#define EVENT_V1

#include "Event_v1.h"
#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

#include <chrono>

enum class EventV1EventType
{
    StatusChange,
    ResourceUpdated,
    ResourceAdded,
    ResourceRemoved,
    Alert,
    MetricReport,
    Other,
};
struct EventV1OemActions
{};
struct EventV1Actions
{
    EventV1OemActions oem;
};
struct EventV1EventRecordOemActions
{};
struct EventV1EventRecordActions
{
    EventV1EventRecordOemActions oem;
};
struct EventV1EventRecord
{
    ResourceV1Resource oem;
    std::string memberId;
    EventV1EventType eventType;
    std::string eventId;
    std::chrono::time_point<std::chrono::system_clock> eventTimestamp;
    std::string severity;
    std::string message;
    std::string messageId;
    std::string messageArgs;
    std::string context;
    NavigationReferenceRedfish originOfCondition;
    EventV1EventRecordActions actions;
    int64_t eventGroupId;
    ResourceV1Resource messageSeverity;
    bool specificEventExistsInGroup;
};
struct EventV1Event
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    EventV1EventRecord events;
    std::string context;
    EventV1Actions actions;
};
#endif
