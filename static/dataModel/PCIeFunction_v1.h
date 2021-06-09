#ifndef PCIEFUNCTION_V1
#define PCIEFUNCTION_V1

#include "NavigationReference__.h"
#include "PCIeFunction_v1.h"
#include "Resource_v1.h"

enum class PCIeFunction_v1_DeviceClass
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
enum class PCIeFunction_v1_FunctionType
{
    Physical,
    Virtual,
};
struct PCIeFunction_v1_Actions
{
    PCIeFunction_v1_OemActions oem;
};
struct PCIeFunction_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ ethernetInterfaces;
    NavigationReference__ drives;
    NavigationReference__ storageControllers;
    NavigationReference__ pCIeDevice;
    NavigationReference__ networkDeviceFunctions;
};
struct PCIeFunction_v1_OemActions
{};
struct PCIeFunction_v1_PCIeFunction
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    int64_t functionId;
    PCIeFunction_v1_FunctionType functionType;
    PCIeFunction_v1_DeviceClass deviceClass;
    std::string deviceId;
    std::string vendorId;
    std::string classCode;
    std::string revisionId;
    std::string subsystemId;
    std::string subsystemVendorId;
    Resource_v1_Resource status;
    PCIeFunction_v1_Links links;
    PCIeFunction_v1_Actions actions;
};
#endif
