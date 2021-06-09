#ifndef LOGSERVICE_V1
#define LOGSERVICE_V1

#include "LogEntryCollection_v1.h"
#include "LogService_v1.h"
#include "Resource_v1.h"

#include <chrono>

enum class LogService_v1_LogDiagnosticDataTypes
{
    Manager,
    PreOS,
    OS,
    OEM,
};
enum class LogService_v1_LogEntryTypes
{
    Event,
    SEL,
    Multiple,
    OEM,
};
enum class LogService_v1_OverWritePolicy
{
    Unknown,
    WrapsWhenFull,
    NeverOverWrites,
};
enum class LogService_v1_SyslogFacility
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
enum class LogService_v1_SyslogSeverity
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
struct LogService_v1_Actions
{
    LogService_v1_OemActions oem;
};
struct LogService_v1_LogService
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    bool serviceEnabled;
    int64_t maxNumberOfRecords;
    LogService_v1_OverWritePolicy overWritePolicy;
    std::chrono::time_point dateTime;
    std::string dateTimeLocalOffset;
    LogEntryCollection_v1_LogEntryCollection entries;
    LogService_v1_Actions actions;
    Resource_v1_Resource status;
    LogService_v1_LogEntryTypes logEntryType;
    LogService_v1_SyslogFilter syslogFilters;
};
struct LogService_v1_OemActions
{};
struct LogService_v1_SyslogFilter
{
    LogService_v1_SyslogSeverity lowestSeverity;
    LogService_v1_SyslogFacility logFacilities;
};
#endif
