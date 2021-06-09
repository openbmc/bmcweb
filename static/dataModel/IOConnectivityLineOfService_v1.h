#ifndef IOCONNECTIVITYLINEOFSERVICE_V1
#define IOCONNECTIVITYLINEOFSERVICE_V1

#include "IOConnectivityLineOfService_v1.h"
#include "Protocol_v1.h"
#include "Resource_v1.h"

struct IOConnectivityLineOfService_v1_Actions
{
    IOConnectivityLineOfService_v1_OemActions oem;
};
struct IOConnectivityLineOfService_v1_IOConnectivityLineOfService
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Protocol_v1_Protocol accessProtocols;
    int64_t maxBytesPerSecond;
    int64_t maxIOPS;
    IOConnectivityLineOfService_v1_Actions actions;
};
struct IOConnectivityLineOfService_v1_OemActions
{
};
#endif
