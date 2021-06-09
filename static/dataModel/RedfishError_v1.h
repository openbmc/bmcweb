#ifndef REDFISHERROR_V1
#define REDFISHERROR_V1

#include "RedfishError_v1.h"

struct RedfishError_v1_RedfishError
{
    RedfishError_v1_RedfishErrorContents error;
};
struct RedfishError_v1_RedfishErrorContents
{
    std::string code;
    std::string message;
};
#endif
