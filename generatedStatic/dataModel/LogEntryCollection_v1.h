#ifndef LOGENTRYCOLLECTION_V1
#define LOGENTRYCOLLECTION_V1

#include "LogEntry_v1.h"
#include "Resource_v1.h"

struct LogEntryCollectionV1LogEntryCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    LogEntryV1LogEntry members;
};
#endif
