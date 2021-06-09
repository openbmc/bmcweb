#ifndef ACCELERATIONFUNCTION_V1
#define ACCELERATIONFUNCTION_V1

#include "AccelerationFunction_v1.h"
#include "NavigationReference__.h"
#include "Resource_v1.h"

enum class AccelerationFunction_v1_AccelerationFunctionType
{
    Encryption,
    Compression,
    PacketInspection,
    PacketSwitch,
    Scheduler,
    AudioProcessing,
    VideoProcessing,
    OEM,
};
struct AccelerationFunction_v1_AccelerationFunction
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource status;
    string UUID;
    std::string fpgaReconfigurationSlots;
    AccelerationFunction_v1_AccelerationFunctionType accelerationFunctionType;
    std::string manufacturer;
    std::string version;
    int64_t powerWatts;
    AccelerationFunction_v1_Links links;
    AccelerationFunction_v1_Actions actions;
};
struct AccelerationFunction_v1_Actions
{
    AccelerationFunction_v1_OemActions oem;
};
struct AccelerationFunction_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ endpoints;
    NavigationReference__ pCIeFunctions;
};
struct AccelerationFunction_v1_OemActions
{};
#endif
