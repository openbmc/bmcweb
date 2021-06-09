#ifndef NVMEFIRMWAREIMAGE_V1
#define NVMEFIRMWAREIMAGE_V1

#include "NVMeFirmwareImage_v1.h"
#include "Resource_v1.h"

enum class NVMeFirmwareImage_v1_NVMeDeviceType {
    Drive,
    JBOF,
    FabricAttachArray,
};
struct NVMeFirmwareImage_v1_Actions
{
    NVMeFirmwareImage_v1_OemActions oem;
};
struct NVMeFirmwareImage_v1_Links
{
    Resource_v1_Resource oem;
};
struct NVMeFirmwareImage_v1_NVMeFirmwareImage
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string firmwareVersion;
    std::string vendor;
    NVMeFirmwareImage_v1_NVMeDeviceType nVMeDeviceType;
    NVMeFirmwareImage_v1_Actions actions;
};
struct NVMeFirmwareImage_v1_OemActions
{
};
#endif
