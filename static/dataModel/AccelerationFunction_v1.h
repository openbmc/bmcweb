#ifndef ACCELERATIONFUNCTION_V1
#define ACCELERATIONFUNCTION_V1

#include "AccelerationFunction_v1.h"
#include "NavigationReference_.h"
#include "Resource_v1.h"

enum class AccelerationFunctionV1AccelerationFunctionType
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
struct AccelerationFunctionV1Links
{
    ResourceV1Resource oem;
    NavigationReference_ endpoints;
    NavigationReference_ pCIeFunctions;
};
struct AccelerationFunctionV1OemActions
{};
struct AccelerationFunctionV1Actions
{
    AccelerationFunctionV1OemActions oem;
};
struct AccelerationFunctionV1AccelerationFunction
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource status;
    std::string UUID;
    std::string fpgaReconfigurationSlots;
    AccelerationFunctionV1AccelerationFunctionType accelerationFunctionType;
    std::string manufacturer;
    std::string version;
    int64_t powerWatts;
    AccelerationFunctionV1Links links;
    AccelerationFunctionV1Actions actions;
};
#endif
