#ifndef OEMCOMPUTERSYSTEM_V1
#define OEMCOMPUTERSYSTEM_V1

#include "OemComputerSystem_v1.h"

enum class OemComputerSystemV1FirmwareProvisioningStatus
{
    NotProvisioned,
    ProvisionedButNotLocked,
    ProvisionedAndLocked,
};
struct OemComputerSystemV1FirmwareProvisioning
{
    OemComputerSystemV1FirmwareProvisioningStatus provisioningStatus;
};
struct OemComputerSystemV1OpenBmc
{};
struct OemComputerSystemV1Oem
{
    OemComputerSystemV1OpenBmc openBmc;
};
#endif
