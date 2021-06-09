#ifndef IOCONNECTIVITYLINEOFSERVICE_V1
#define IOCONNECTIVITYLINEOFSERVICE_V1

#include "IOConnectivityLineOfService_v1.h"
#include "Protocol_v1.h"
#include "Resource_v1.h"

struct IOConnectivityLineOfServiceV1OemActions
{};
struct IOConnectivityLineOfServiceV1Actions
{
    IOConnectivityLineOfServiceV1OemActions oem;
};
struct IOConnectivityLineOfServiceV1IOConnectivityLineOfService
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ProtocolV1Protocol accessProtocols;
    int64_t maxBytesPerSecond;
    int64_t maxIOPS;
    IOConnectivityLineOfServiceV1Actions actions;
};
#endif
