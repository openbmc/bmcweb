#ifndef PRIVILEGES_V1
#define PRIVILEGES_V1

enum class PrivilegesV1PrivilegeType
{
    Login,
    ConfigureManager,
    ConfigureUsers,
    ConfigureSelf,
    ConfigureComponents,
    NoAuth,
    ConfigureCompositionInfrastructure,
};
#endif
