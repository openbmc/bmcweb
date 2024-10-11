#pragma once

#include <string>
#include <vector>

struct EventLogObjectsType
{
    std::string id;
    std::string timestamp;
    std::string messageId;
    std::vector<std::string> messageArgs;
};
