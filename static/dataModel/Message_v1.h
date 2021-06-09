#ifndef MESSAGE_V1
#define MESSAGE_V1

#include "Resource_v1.h"

struct Message_v1_Message
{
    std::string messageId;
    std::string message;
    std::string relatedProperties;
    std::string messageArgs;
    std::string severity;
    std::string resolution;
    Resource_v1_Resource oem;
    Resource_v1_Resource messageSeverity;
};
#endif
