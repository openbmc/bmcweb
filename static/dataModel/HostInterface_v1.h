#ifndef HOSTINTERFACE_V1
#define HOSTINTERFACE_V1

#include "EthernetInterfaceCollection_v1.h"
#include "HostInterface_v1.h"
#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

enum class HostInterfaceV1AuthenticationMode
{
    AuthNone,
    BasicAuth,
    RedfishSessionAuth,
    OemAuth,
};
enum class HostInterfaceV1HostInterfaceType
{
    NetworkHostInterface,
};
struct HostInterfaceV1OemActions
{};
struct HostInterfaceV1Actions
{
    HostInterfaceV1OemActions oem;
};
struct HostInterfaceV1CredentialBootstrapping
{
    bool enabled;
    bool enableAfterReset;
    std::string roleId;
};
struct HostInterfaceV1Links
{
    ResourceV1Resource oem;
    NavigationReferenceRedfish computerSystems;
    NavigationReferenceRedfish kernelAuthRole;
    NavigationReferenceRedfish firmwareAuthRole;
    NavigationReferenceRedfish authNoneRole;
    NavigationReferenceRedfish credentialBootstrappingRole;
};
struct HostInterfaceV1HostInterface
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    HostInterfaceV1HostInterfaceType hostInterfaceType;
    ResourceV1Resource status;
    bool interfaceEnabled;
    bool externallyAccessible;
    HostInterfaceV1AuthenticationMode authenticationModes;
    std::string kernelAuthRoleId;
    bool kernelAuthEnabled;
    std::string firmwareAuthRoleId;
    bool firmwareAuthEnabled;
    EthernetInterfaceCollectionV1EthernetInterfaceCollection
        hostEthernetInterfaces;
    NavigationReferenceRedfish managerEthernetInterface;
    NavigationReferenceRedfish networkProtocol;
    HostInterfaceV1Links links;
    HostInterfaceV1Actions actions;
    std::string authNoneRoleId;
    HostInterfaceV1CredentialBootstrapping credentialBootstrapping;
};
#endif
