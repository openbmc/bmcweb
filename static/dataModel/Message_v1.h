#ifndef MESSAGE_V1
#define MESSAGE_V1

#include "Resource_v1.h"

struct MessageV1Message
{
    std::string messageId;
    std::string message;
    std::string relatedProperties;
    std::string messageArgs;
    std::string severity;
    std::string resolution;
    ResourceV1Resource oem;
    ResourceV1Resource messageSeverity;
};
#endif
