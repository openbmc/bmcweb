#ifndef IOCONNECTIVITYLOSCAPABILITIES_V1
#define IOCONNECTIVITYLOSCAPABILITIES_V1

#include "IOConnectivityLineOfService_v1.h"
#include "IOConnectivityLoSCapabilities_v1.h"
#include "Protocol_v1.h"
#include "Resource_v1.h"

struct IOConnectivityLoSCapabilitiesV1OemActions
{};
struct IOConnectivityLoSCapabilitiesV1Actions
{
    IOConnectivityLoSCapabilitiesV1OemActions oem;
};
struct IOConnectivityLoSCapabilitiesV1IOConnectivityLoSCapabilities
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource identifier;
    ProtocolV1Protocol supportedAccessProtocols;
    int64_t maxSupportedBytesPerSecond;
    IOConnectivityLineOfServiceV1IOConnectivityLineOfService
        supportedLinesOfService;
    IOConnectivityLoSCapabilitiesV1Actions actions;
    int64_t maxSupportedIOPS;
};
#endif
