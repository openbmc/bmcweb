#ifndef PCIEFUNCTION_V1
#define PCIEFUNCTION_V1

#include "NavigationReferenceRedfish.h"
#include "PCIeFunction_v1.h"
#include "Resource_v1.h"

enum class PCIeFunctionV1DeviceClass
{
    UnclassifiedDevice,
    MassStorageController,
    NetworkController,
    DisplayController,
    MultimediaController,
    MemoryController,
    Bridge,
    CommunicationController,
    GenericSystemPeripheral,
    InputDeviceController,
    DockingStation,
    Processor,
    SerialBusController,
    WirelessController,
    IntelligentController,
    SatelliteCommunicationsController,
    EncryptionController,
    SignalProcessingController,
    ProcessingAccelerators,
    NonEssentialInstrumentation,
    Coprocessor,
    UnassignedClass,
    Other,
};
enum class PCIeFunctionV1FunctionType
{
    Physical,
    Virtual,
};
struct PCIeFunctionV1OemActions
{};
struct PCIeFunctionV1Actions
{
    PCIeFunctionV1OemActions oem;
};
struct PCIeFunctionV1Links
{
    ResourceV1Resource oem;
    NavigationReferenceRedfish ethernetInterfaces;
    NavigationReferenceRedfish drives;
    NavigationReferenceRedfish storageControllers;
    NavigationReferenceRedfish pCIeDevice;
    NavigationReferenceRedfish networkDeviceFunctions;
};
struct PCIeFunctionV1PCIeFunction
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    int64_t functionId;
    PCIeFunctionV1FunctionType functionType;
    PCIeFunctionV1DeviceClass deviceClass;
    std::string deviceId;
    std::string vendorId;
    std::string classCode;
    std::string revisionId;
    std::string subsystemId;
    std::string subsystemVendorId;
    ResourceV1Resource status;
    PCIeFunctionV1Links links;
    PCIeFunctionV1Actions actions;
    bool enabled;
};
#endif
