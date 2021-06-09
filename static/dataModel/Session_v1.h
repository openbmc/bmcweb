#ifndef SESSION_V1
#define SESSION_V1

#include "Resource_v1.h"
#include "Session_v1.h"

enum class Session_v1_SessionTypes
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
struct Session_v1_Actions
{
    Session_v1_OemActions oem;
};
struct Session_v1_OemActions
{};
struct Session_v1_Session
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string userName;
    std::string password;
    Session_v1_Actions actions;
    Session_v1_SessionTypes sessionType;
    std::string oemSessionType;
    std::string clientOriginIPAddress;
};
#endif
