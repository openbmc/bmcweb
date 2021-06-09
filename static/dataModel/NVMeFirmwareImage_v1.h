#ifndef NVMEFIRMWAREIMAGE_V1
#define NVMEFIRMWAREIMAGE_V1

#include "NVMeFirmwareImage_v1.h"
#include "Resource_v1.h"

enum class NVMeFirmwareImageV1NVMeDeviceType
{
    Drive,
    JBOF,
    FabricAttachArray,
};
struct NVMeFirmwareImageV1OemActions
{};
struct NVMeFirmwareImageV1Actions
{
    NVMeFirmwareImageV1OemActions oem;
};
struct NVMeFirmwareImageV1Links
{
    ResourceV1Resource oem;
};
struct NVMeFirmwareImageV1NVMeFirmwareImage
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string firmwareVersion;
    std::string vendor;
    NVMeFirmwareImageV1NVMeDeviceType nVMeDeviceType;
    NVMeFirmwareImageV1Actions actions;
};
#endif
