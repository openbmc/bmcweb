#ifndef ROLE_V1
#define ROLE_V1

#include "Privileges_v1.h"
#include "Resource_v1.h"
#include "Role_v1.h"

struct Role_v1_Actions
{
    Role_v1_OemActions oem;
};
struct Role_v1_OemActions
{};
struct Role_v1_Role
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    bool isPredefined;
    Privileges_v1_Privileges assignedPrivileges;
    std::string oemPrivileges;
    Role_v1_Actions actions;
    std::string roleId;
};
#endif
