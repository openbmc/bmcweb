#ifndef LOGSERVICE_V1
#define LOGSERVICE_V1

#include "LogEntryCollection_v1.h"
#include "LogService_v1.h"
#include "Resource_v1.h"

#include <chrono>

enum class LogServiceV1LogDiagnosticDataTypes
{
    Manager,
    PreOS,
    OS,
    OEM,
};
enum class LogServiceV1LogEntryTypes
{
    Event,
    SEL,
    Multiple,
    OEM,
};
enum class LogServiceV1OverWritePolicy
{
    Unknown,
    WrapsWhenFull,
    NeverOverWrites,
};
enum class LogServiceV1SyslogFacility
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
enum class LogServiceV1SyslogSeverity
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
struct LogServiceV1OemActions
{};
struct LogServiceV1Actions
{
    LogServiceV1OemActions oem;
};
struct LogServiceV1SyslogFilter
{
    LogServiceV1SyslogSeverity lowestSeverity;
    LogServiceV1SyslogFacility logFacilities;
};
struct LogServiceV1LogService
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    bool serviceEnabled;
    int64_t maxNumberOfRecords;
    LogServiceV1OverWritePolicy overWritePolicy;
    std::chrono::time_point<std::chrono::system_clock> dateTime;
    std::string dateTimeLocalOffset;
    LogEntryCollectionV1LogEntryCollection entries;
    LogServiceV1Actions actions;
    ResourceV1Resource status;
    LogServiceV1LogEntryTypes logEntryType;
    LogServiceV1SyslogFilter syslogFilters;
};
#endif
