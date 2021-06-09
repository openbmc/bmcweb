#ifndef EVENT_V1
#define EVENT_V1

#include <chrono>
#include "Event_v1.h"
#include "NavigationReference__.h"
#include "Resource_v1.h"

enum class Event_v1_EventType {
    StatusChange,
    ResourceUpdated,
    ResourceAdded,
    ResourceRemoved,
    Alert,
    MetricReport,
    Other,
};
struct Event_v1_Actions
{
    Event_v1_OemActions oem;
};
struct Event_v1_Event
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Event_v1_EventRecord events;
    std::string context;
    Event_v1_Actions actions;
};
struct Event_v1_EventRecord
{
    Resource_v1_Resource oem;
    std::string memberId;
    Event_v1_EventType eventType;
    std::string eventId;
    std::chrono::time_point eventTimestamp;
    std::string severity;
    std::string message;
    std::string messageId;
    std::string messageArgs;
    std::string context;
    NavigationReference__ originOfCondition;
    Event_v1_EventRecordActions actions;
    int64_t eventGroupId;
    Resource_v1_Resource messageSeverity;
    bool specificEventExistsInGroup;
};
struct Event_v1_EventRecordActions
{
    Event_v1_EventRecordOemActions oem;
};
struct Event_v1_EventRecordOemActions
{
};
struct Event_v1_OemActions
{
};
#endif
