#ifndef SESSION_V1
#define SESSION_V1

#include "Resource_v1.h"
#include "Session_v1.h"

enum class SessionV1SessionTypes
{
    HostConsole,
    ManagerConsole,
    IPMI,
    KVMIP,
    OEM,
    Redfish,
    VirtualMedia,
    WebUI,
};
struct SessionV1OemActions
{};
struct SessionV1Actions
{
    SessionV1OemActions oem;
};
struct SessionV1Session
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string userName;
    std::string password;
    SessionV1Actions actions;
    SessionV1SessionTypes sessionType;
    std::string oemSessionType;
    std::string clientOriginIPAddress;
};
#endif
