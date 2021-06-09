#ifndef IOCONNECTIVITYLOSCAPABILITIES_V1
#define IOCONNECTIVITYLOSCAPABILITIES_V1

#include "IOConnectivityLineOfService_v1.h"
#include "IOConnectivityLoSCapabilities_v1.h"
#include "Protocol_v1.h"
#include "Resource_v1.h"

struct IOConnectivityLoSCapabilities_v1_Actions
{
    IOConnectivityLoSCapabilities_v1_OemActions oem;
};
struct IOConnectivityLoSCapabilities_v1_IOConnectivityLoSCapabilities
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource identifier;
    Protocol_v1_Protocol supportedAccessProtocols;
    int64_t maxSupportedBytesPerSecond;
    IOConnectivityLineOfService_v1_IOConnectivityLineOfService supportedLinesOfService;
    IOConnectivityLoSCapabilities_v1_Actions actions;
    int64_t maxSupportedIOPS;
};
struct IOConnectivityLoSCapabilities_v1_OemActions
{
};
#endif
