#ifndef PRIVILEGES_V1
#define PRIVILEGES_V1


enum class Privileges_v1_PrivilegeType {
    Login,
    ConfigureManager,
    ConfigureUsers,
    ConfigureSelf,
    ConfigureComponents,
    NoAuth,
};
#endif
