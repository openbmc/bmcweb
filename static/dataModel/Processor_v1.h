#ifndef PROCESSOR_V1
#define PROCESSOR_V1

#include "AccelerationFunctionCollection_v1.h"
#include "Assembly_v1.h"
#include "NavigationReference__.h"
#include "PCIeDevice_v1.h"
#include "Processor_v1.h"
#include "Resource_v1.h"

enum class Processor_v1_BaseSpeedPriorityState
{
    Enabled,
    Disabled,
};
enum class Processor_v1_FpgaType
{
    Integrated,
    Discrete,
};
enum class Processor_v1_ProcessorMemoryType
{
    L1Cache,
    L2Cache,
    L3Cache,
    L4Cache,
    L5Cache,
    L6Cache,
    L7Cache,
    HBM1,
    HBM2,
    HBM3,
    SGRAM,
    GDDR,
    GDDR2,
    GDDR3,
    GDDR4,
    GDDR5,
    GDDR5X,
    GDDR6,
    DDR,
    DDR2,
    DDR3,
    DDR4,
    DDR5,
    SDRAM,
    SRAM,
    Flash,
    OEM,
};
enum class Processor_v1_ProcessorType
{
    CPU,
    GPU,
    FPGA,
    DSP,
    Accelerator,
    Core,
    Thread,
    OEM,
};
enum class Processor_v1_SystemInterfaceType
{
    QPI,
    UPI,
    PCIe,
    Ethernet,
    AMBA,
    CCIX,
    CXL,
    OEM,
};
enum class Processor_v1_TurboState
{
    Enabled,
    Disabled,
};
struct Processor_v1_Actions
{
    Processor_v1_OemActions oem;
};
struct Processor_v1_EthernetInterface
{
    Resource_v1_Resource oem;
    int64_t maxSpeedMbps;
    int64_t maxLanes;
};
struct Processor_v1_FPGA
{
    Processor_v1_FpgaType fpgaType;
    std::string model;
    std::string firmwareId;
    std::string firmwareManufacturer;
    std::string firmwareVersion;
    Processor_v1_ProcessorInterface hostInterface;
    Processor_v1_ProcessorInterface externalInterfaces;
    int64_t pCIeVirtualFunctions;
    bool programmableFromHost;
    Processor_v1_FpgaReconfigurationSlot reconfigurationSlots;
    Resource_v1_Resource oem;
};
struct Processor_v1_FpgaReconfigurationSlot
{
    std::string slotId;
    string UUID;
    bool programmableFromHost;
    NavigationReference__ accelerationFunction;
};
struct Processor_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ chassis;
    NavigationReference__ endpoints;
    NavigationReference__ connectedProcessors;
    NavigationReference__ pCIeDevice;
    NavigationReference__ pCIeFunctions;
};
struct Processor_v1_OemActions
{};
struct Processor_v1_Processor
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string socket;
    Processor_v1_ProcessorType processorType;
    std::string processorArchitecture;
    std::string instructionSet;
    Processor_v1_ProcessorId processorId;
    Resource_v1_Resource status;
    std::string manufacturer;
    std::string model;
    int64_t maxSpeedMHz;
    int64_t totalCores;
    int64_t totalThreads;
    Processor_v1_Links links;
    Processor_v1_Actions actions;
    Resource_v1_Resource location;
    Assembly_v1_Assembly assembly;
    int64_t tDPWatts;
    int64_t maxTDPWatts;
    NavigationReference__ metrics;
    string UUID;
    Processor_v1_ProcessorMemory processorMemory;
    Processor_v1_FPGA FPGA;
    AccelerationFunctionCollection_v1_AccelerationFunctionCollection
        accelerationFunctions;
    int64_t totalEnabledCores;
    std::string serialNumber;
    std::string partNumber;
    std::string version;
    std::string firmwareVersion;
    Processor_v1_ProcessorInterface systemInterface;
    int64_t operatingSpeedMHz;
    int64_t minSpeedMHz;
    Processor_v1_TurboState turboState;
    Processor_v1_BaseSpeedPriorityState baseSpeedPriorityState;
    int64_t highSpeedCoreIDs;
    NavigationReference__ operatingConfigs;
    NavigationReference__ appliedOperatingConfig;
    bool locationIndicatorActive;
    int64_t baseSpeedMHz;
    int64_t speedLimitMHz;
    bool speedLocked;
};
struct Processor_v1_ProcessorId
{
    std::string vendorId;
    std::string identificationRegisters;
    std::string effectiveFamily;
    std::string effectiveModel;
    std::string step;
    std::string microcodeInfo;
    std::string protectedIdentificationNumber;
};
struct Processor_v1_ProcessorInterface
{
    Processor_v1_SystemInterfaceType interfaceType;
    PCIeDevice_v1_PCIeDevice pCIe;
    Processor_v1_EthernetInterface ethernet;
};
struct Processor_v1_ProcessorMemory
{
    bool integratedMemory;
    Processor_v1_ProcessorMemoryType memoryType;
    int64_t capacityMiB;
    int64_t speedMHz;
};
#endif
