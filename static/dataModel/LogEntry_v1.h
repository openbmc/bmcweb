#ifndef LOGENTRY_V1
#define LOGENTRY_V1

#include "Event_v1.h"
#include "LogEntry_v1.h"
#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

#include <chrono>

enum class LogEntryV1EventSeverity
{
    OK,
    Warning,
    Critical,
};
enum class LogEntryV1LogDiagnosticDataTypes
{
    Manager,
    PreOS,
    OS,
    OEM,
};
enum class LogEntryV1LogEntryType
{
    Event,
    SEL,
    Oem,
};
struct LogEntryV1OemActions
{};
struct LogEntryV1Actions
{
    LogEntryV1OemActions oem;
};
struct LogEntryV1Links
{
    ResourceV1Resource oem;
    NavigationReferenceRedfish originOfCondition;
};
struct LogEntryV1LogEntry
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    LogEntryV1EventSeverity severity;
    std::chrono::time_point<std::chrono::system_clock> created;
    LogEntryV1LogEntryType entryType;
    std::string oemRecordFormat;
    std::string entryCode;
    std::string sensorType;
    int64_t sensorNumber;
    std::string message;
    std::string messageId;
    std::string messageArgs;
    LogEntryV1Links links;
    EventV1Event eventType;
    std::string eventId;
    std::chrono::time_point<std::chrono::system_clock> eventTimestamp;
    LogEntryV1Actions actions;
    std::string oemLogEntryCode;
    std::string oemSensorType;
    int64_t eventGroupId;
    std::string generatorId;
    std::chrono::time_point<std::chrono::system_clock> modified;
    int64_t additionalDataSizeBytes;
    std::string additionalDataURI;
    LogEntryV1LogDiagnosticDataTypes diagnosticDataType;
    std::string oEMDiagnosticDataType;
    bool resolved;
    bool serviceProviderNotified;
    std::string resolution;
};
#endif
