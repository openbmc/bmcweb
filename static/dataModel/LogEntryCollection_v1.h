#ifndef LOGENTRYCOLLECTION_V1
#define LOGENTRYCOLLECTION_V1

#include "LogEntry_v1.h"
#include "Resource_v1.h"

struct LogEntryCollection_v1_LogEntryCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    LogEntry_v1_LogEntry members;
};
#endif
