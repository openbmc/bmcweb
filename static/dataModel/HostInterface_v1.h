#ifndef HOSTINTERFACE_V1
#define HOSTINTERFACE_V1

#include "EthernetInterfaceCollection_v1.h"
#include "HostInterface_v1.h"
#include "NavigationReference__.h"
#include "Resource_v1.h"

enum class HostInterface_v1_AuthenticationMode
{
    AuthNone,
    BasicAuth,
    RedfishSessionAuth,
    OemAuth,
};
enum class HostInterface_v1_HostInterfaceType
{
    NetworkHostInterface,
};
struct HostInterface_v1_Actions
{
    HostInterface_v1_OemActions oem;
};
struct HostInterface_v1_CredentialBootstrapping
{
    bool enabled;
    bool enableAfterReset;
    std::string roleId;
};
struct HostInterface_v1_HostInterface
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    HostInterface_v1_HostInterfaceType hostInterfaceType;
    Resource_v1_Resource status;
    bool interfaceEnabled;
    bool externallyAccessible;
    HostInterface_v1_AuthenticationMode authenticationModes;
    std::string kernelAuthRoleId;
    bool kernelAuthEnabled;
    std::string firmwareAuthRoleId;
    bool firmwareAuthEnabled;
    EthernetInterfaceCollection_v1_EthernetInterfaceCollection
        hostEthernetInterfaces;
    NavigationReference__ managerEthernetInterface;
    NavigationReference__ networkProtocol;
    HostInterface_v1_Links links;
    HostInterface_v1_Actions actions;
    std::string authNoneRoleId;
    HostInterface_v1_CredentialBootstrapping credentialBootstrapping;
};
struct HostInterface_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ computerSystems;
    NavigationReference__ kernelAuthRole;
    NavigationReference__ firmwareAuthRole;
    NavigationReference__ authNoneRole;
    NavigationReference__ credentialBootstrappingRole;
};
struct HostInterface_v1_OemActions
{};
#endif
