#ifndef ROLE_V1
#define ROLE_V1

#include "Privileges_v1.h"
#include "Resource_v1.h"
#include "Role_v1.h"

struct RoleV1OemActions
{};
struct RoleV1Actions
{
    RoleV1OemActions oem;
};
struct RoleV1Role
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    bool isPredefined;
    PrivilegesV1Privileges assignedPrivileges;
    std::string oemPrivileges;
    RoleV1Actions actions;
    std::string roleId;
    bool restricted;
    std::string alternateRoleId;
};
#endif
