#ifndef REDFISHERROR_V1
#define REDFISHERROR_V1

#include "RedfishError_v1.h"

struct RedfishErrorV1RedfishErrorContents
{
    std::string code;
    std::string message;
};
struct RedfishErrorV1RedfishError
{
    RedfishErrorV1RedfishErrorContents error;
};
#endif
