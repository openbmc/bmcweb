#ifndef LOGENTRY_V1
#define LOGENTRY_V1

#include "Event_v1.h"
#include "LogEntry_v1.h"
#include "NavigationReference__.h"
#include "Resource_v1.h"

#include <chrono>

enum class LogEntry_v1_EventSeverity
{
    OK,
    Warning,
    Critical,
};
enum class LogEntry_v1_LogDiagnosticDataTypes
{
    Manager,
    PreOS,
    OS,
    OEM,
};
enum class LogEntry_v1_LogEntryType
{
    Event,
    SEL,
    Oem,
};
struct LogEntry_v1_Actions
{
    LogEntry_v1_OemActions oem;
};
struct LogEntry_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ originOfCondition;
};
struct LogEntry_v1_LogEntry
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    LogEntry_v1_EventSeverity severity;
    std::chrono::time_point created;
    LogEntry_v1_LogEntryType entryType;
    std::string oemRecordFormat;
    std::string entryCode;
    std::string sensorType;
    int64_t sensorNumber;
    std::string message;
    std::string messageId;
    std::string messageArgs;
    LogEntry_v1_Links links;
    Event_v1_Event eventType;
    std::string eventId;
    std::chrono::time_point eventTimestamp;
    LogEntry_v1_Actions actions;
    std::string oemLogEntryCode;
    std::string oemSensorType;
    int64_t eventGroupId;
    std::string generatorId;
    std::chrono::time_point modified;
    int64_t additionalDataSizeBytes;
    std::string additionalDataURI;
    LogEntry_v1_LogDiagnosticDataTypes diagnosticDataType;
    std::string oEMDiagnosticDataType;
};
struct LogEntry_v1_OemActions
{};
#endif
