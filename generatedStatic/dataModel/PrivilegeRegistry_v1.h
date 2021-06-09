#ifndef PRIVILEGEREGISTRY_V1
#define PRIVILEGEREGISTRY_V1

#include "PrivilegeRegistry_v1.h"
#include "Privileges_v1.h"
#include "Resource_v1.h"

struct PrivilegeRegistryV1OemActions
{};
struct PrivilegeRegistryV1Actions
{
    PrivilegeRegistryV1OemActions oem;
};
struct PrivilegeRegistryV1OperationPrivilege
{
    std::string privilege;
};
struct PrivilegeRegistryV1OperationMap
{
    PrivilegeRegistryV1OperationPrivilege GET;
    PrivilegeRegistryV1OperationPrivilege HEAD;
    PrivilegeRegistryV1OperationPrivilege PATCH;
    PrivilegeRegistryV1OperationPrivilege POST;
    PrivilegeRegistryV1OperationPrivilege PUT;
    PrivilegeRegistryV1OperationPrivilege DELETE;
};
struct PrivilegeRegistryV1TargetPrivilegeMap
{
    std::string targets;
    PrivilegeRegistryV1OperationMap operationMap;
};
struct PrivilegeRegistryV1Mapping
{
    std::string entity;
    PrivilegeRegistryV1TargetPrivilegeMap subordinateOverrides;
    PrivilegeRegistryV1TargetPrivilegeMap resourceURIOverrides;
    PrivilegeRegistryV1TargetPrivilegeMap propertyOverrides;
    PrivilegeRegistryV1OperationMap operationMap;
};
struct PrivilegeRegistryV1PrivilegeRegistry
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    PrivilegesV1Privileges privilegesUsed;
    std::string oEMPrivilegesUsed;
    PrivilegeRegistryV1Mapping mappings;
    PrivilegeRegistryV1Actions actions;
};
#endif
