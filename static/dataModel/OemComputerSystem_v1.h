#ifndef OEMCOMPUTERSYSTEM_V1
#define OEMCOMPUTERSYSTEM_V1

#include "OemComputerSystem_v1.h"

enum class OemComputerSystem_v1_FirmwareProvisioningStatus
{
    NotProvisioned,
    ProvisionedButNotLocked,
    ProvisionedAndLocked,
};
struct OemComputerSystem_v1_FirmwareProvisioning
{
    OemComputerSystem_v1_FirmwareProvisioningStatus provisioningStatus;
};
struct OemComputerSystem_v1_Oem
{
    OemComputerSystem_v1_OpenBmc openBmc;
};
struct OemComputerSystem_v1_OpenBmc
{};
#endif
