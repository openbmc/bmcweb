#ifndef PRIVILEGEREGISTRY_V1
#define PRIVILEGEREGISTRY_V1

#include "PrivilegeRegistry_v1.h"
#include "Privileges_v1.h"
#include "Resource_v1.h"

struct PrivilegeRegistry_v1_Actions
{
    PrivilegeRegistry_v1_OemActions oem;
};
struct PrivilegeRegistry_v1_Mapping
{
    std::string entity;
    PrivilegeRegistry_v1_Target_PrivilegeMap subordinateOverrides;
    PrivilegeRegistry_v1_Target_PrivilegeMap resourceURIOverrides;
    PrivilegeRegistry_v1_Target_PrivilegeMap propertyOverrides;
    PrivilegeRegistry_v1_OperationMap operationMap;
};
struct PrivilegeRegistry_v1_OemActions
{
};
struct PrivilegeRegistry_v1_OperationMap
{
    PrivilegeRegistry_v1_OperationPrivilege GET;
    PrivilegeRegistry_v1_OperationPrivilege HEAD;
    PrivilegeRegistry_v1_OperationPrivilege PATCH;
    PrivilegeRegistry_v1_OperationPrivilege POST;
    PrivilegeRegistry_v1_OperationPrivilege PUT;
    PrivilegeRegistry_v1_OperationPrivilege DELETE;
};
struct PrivilegeRegistry_v1_OperationPrivilege
{
    std::string privilege;
};
struct PrivilegeRegistry_v1_PrivilegeRegistry
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Privileges_v1_Privileges privilegesUsed;
    std::string oEMPrivilegesUsed;
    PrivilegeRegistry_v1_Mapping mappings;
    PrivilegeRegistry_v1_Actions actions;
};
struct PrivilegeRegistry_v1_Target_PrivilegeMap
{
    std::string targets;
    PrivilegeRegistry_v1_OperationMap operationMap;
};
#endif
