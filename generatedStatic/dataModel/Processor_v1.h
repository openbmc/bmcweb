#ifndef PROCESSOR_V1
#define PROCESSOR_V1

#include "AccelerationFunctionCollection_v1.h"
#include "Assembly_v1.h"
#include "CertificateCollection_v1.h"
#include "MemoryMetrics_v1.h"
#include "NavigationReferenceRedfish.h"
#include "PCIeDevice_v1.h"
#include "Processor_v1.h"
#include "Resource_v1.h"
#include "SoftwareInventory_v1.h"

enum class ProcessorV1BaseSpeedPriorityState
{
    Enabled,
    Disabled,
};
enum class ProcessorV1FpgaType
{
    Integrated,
    Discrete,
};
enum class ProcessorV1ProcessorMemoryType
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
enum class ProcessorV1ProcessorType
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
enum class ProcessorV1SystemInterfaceType
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
enum class ProcessorV1TurboState
{
    Enabled,
    Disabled,
};
struct ProcessorV1OemActions
{};
struct ProcessorV1Actions
{
    ProcessorV1OemActions oem;
};
struct ProcessorV1EthernetInterface
{
    ResourceV1Resource oem;
    int64_t maxSpeedMbps;
    int64_t maxLanes;
};
struct ProcessorV1ProcessorInterface
{
    ProcessorV1SystemInterfaceType interfaceType;
    PCIeDeviceV1PCIeDevice pCIe;
    ProcessorV1EthernetInterface ethernet;
};
struct ProcessorV1FpgaReconfigurationSlot
{
    std::string slotId;
    std::string UUID;
    bool programmableFromHost;
    NavigationReferenceRedfish accelerationFunction;
};
struct ProcessorV1FPGA
{
    ProcessorV1FpgaType fpgaType;
    std::string model;
    std::string firmwareId;
    std::string firmwareManufacturer;
    std::string firmwareVersion;
    ProcessorV1ProcessorInterface hostInterface;
    ProcessorV1ProcessorInterface externalInterfaces;
    int64_t pCIeVirtualFunctions;
    bool programmableFromHost;
    ProcessorV1FpgaReconfigurationSlot reconfigurationSlots;
    ResourceV1Resource oem;
};
struct ProcessorV1Links
{
    ResourceV1Resource oem;
    NavigationReferenceRedfish chassis;
    NavigationReferenceRedfish endpoints;
    NavigationReferenceRedfish connectedProcessors;
    NavigationReferenceRedfish pCIeDevice;
    NavigationReferenceRedfish pCIeFunctions;
    NavigationReferenceRedfish memory;
    NavigationReferenceRedfish graphicsController;
};
struct ProcessorV1MemorySummary
{
    int64_t totalCacheSizeMiB;
    int64_t totalMemorySizeMiB;
    MemoryMetricsV1MemoryMetrics metrics;
};
struct ProcessorV1ProcessorId
{
    std::string vendorId;
    std::string identificationRegisters;
    std::string effectiveFamily;
    std::string effectiveModel;
    std::string step;
    std::string microcodeInfo;
    std::string protectedIdentificationNumber;
};
struct ProcessorV1ProcessorMemory
{
    bool integratedMemory;
    ProcessorV1ProcessorMemoryType memoryType;
    int64_t capacityMiB;
    int64_t speedMHz;
};
struct ProcessorV1Processor
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string socket;
    ProcessorV1ProcessorType processorType;
    std::string processorArchitecture;
    std::string instructionSet;
    ProcessorV1ProcessorId processorId;
    ResourceV1Resource status;
    std::string manufacturer;
    std::string model;
    int64_t maxSpeedMHz;
    int64_t totalCores;
    int64_t totalThreads;
    ProcessorV1Links links;
    ProcessorV1Actions actions;
    ResourceV1Resource location;
    AssemblyV1Assembly assembly;
    int64_t tDPWatts;
    int64_t maxTDPWatts;
    NavigationReferenceRedfish metrics;
    std::string UUID;
    ProcessorV1ProcessorMemory processorMemory;
    ProcessorV1FPGA FPGA;
    AccelerationFunctionCollectionV1AccelerationFunctionCollection
        accelerationFunctions;
    int64_t totalEnabledCores;
    std::string serialNumber;
    std::string partNumber;
    std::string version;
    std::string firmwareVersion;
    ProcessorV1ProcessorInterface systemInterface;
    int64_t operatingSpeedMHz;
    int64_t minSpeedMHz;
    ProcessorV1TurboState turboState;
    ProcessorV1BaseSpeedPriorityState baseSpeedPriorityState;
    int64_t highSpeedCoreIDs;
    NavigationReferenceRedfish operatingConfigs;
    NavigationReferenceRedfish appliedOperatingConfig;
    bool locationIndicatorActive;
    int64_t baseSpeedMHz;
    int64_t speedLimitMHz;
    bool speedLocked;
    ProcessorV1MemorySummary memorySummary;
    NavigationReferenceRedfish environmentMetrics;
    std::string sparePartNumber;
    CertificateCollectionV1CertificateCollection certificates;
    SoftwareInventoryV1SoftwareInventory measurements;
    bool enabled;
};
#endif
