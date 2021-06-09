#ifndef MEMORY_V1
#define MEMORY_V1

#include "Memory_v1.h"
#include "MemoryMetrics_v1.h"
#include "NavigationReference__.h"
#include "Resource_v1.h"

enum class Memory_v1_BaseModuleType {
    RDIMM,
    UDIMM,
    SO_DIMM,
    LRDIMM,
    Mini_RDIMM,
    Mini_UDIMM,
    SO_RDIMM_72b,
    SO_UDIMM_72b,
    SO_DIMM_16b,
    SO_DIMM_32b,
    Die,
};
enum class Memory_v1_ErrorCorrection {
    NoECC,
    SingleBitECC,
    MultiBitECC,
    AddressParity,
};
enum class Memory_v1_MemoryClassification {
    Volatile,
    ByteAccessiblePersistent,
    Block,
};
enum class Memory_v1_MemoryDeviceType {
    DDR,
    DDR2,
    DDR3,
    DDR4,
    DDR4_SDRAM,
    DDR4E_SDRAM,
    LPDDR4_SDRAM,
    DDR3_SDRAM,
    LPDDR3_SDRAM,
    DDR2_SDRAM,
    DDR2_SDRAM_FB_DIMM,
    DDR2_SDRAM_FB_DIMM_PROBE,
    DDR_SGRAM,
    DDR_SDRAM,
    ROM,
    SDRAM,
    EDO,
    FastPageMode,
    PipelinedNibble,
    Logical,
    HBM,
    HBM2,
};
enum class Memory_v1_MemoryMedia {
    DRAM,
    NAND,
    Intel3DXPoint,
    Proprietary,
};
enum class Memory_v1_MemoryType {
    DRAM,
    NVDIMM_N,
    NVDIMM_F,
    NVDIMM_P,
    IntelOptane,
};
enum class Memory_v1_OperatingMemoryModes {
    Volatile,
    PMEM,
    Block,
};
enum class Memory_v1_SecurityStates {
    Enabled,
    Disabled,
    Unlocked,
    Locked,
    Frozen,
    Passphraselimit,
};
struct Memory_v1_Actions
{
    Memory_v1_OemActions oem;
};
struct Memory_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ chassis;
};
struct Memory_v1_Memory
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Memory_v1_MemoryType memoryType;
    Memory_v1_MemoryDeviceType memoryDeviceType;
    Memory_v1_BaseModuleType baseModuleType;
    Memory_v1_MemoryMedia memoryMedia;
    int64_t capacityMiB;
    int64_t dataWidthBits;
    int64_t busWidthBits;
    std::string manufacturer;
    std::string serialNumber;
    std::string partNumber;
    int64_t allowedSpeedsMHz;
    std::string firmwareRevision;
    std::string firmwareApiVersion;
    std::string functionClasses;
    std::string vendorID;
    std::string deviceID;
    std::string subsystemVendorID;
    std::string subsystemDeviceID;
    int64_t maxTDPMilliWatts;
    Memory_v1_SecurityCapabilities securityCapabilities;
    int64_t spareDeviceCount;
    int64_t rankCount;
    std::string deviceLocator;
    Memory_v1_MemoryLocation memoryLocation;
    Memory_v1_ErrorCorrection errorCorrection;
    int64_t operatingSpeedMhz;
    int64_t volatileRegionSizeLimitMiB;
    int64_t persistentRegionSizeLimitMiB;
    Memory_v1_RegionSet regions;
    Memory_v1_OperatingMemoryModes operatingMemoryModes;
    Memory_v1_PowerManagementPolicy powerManagementPolicy;
    bool isSpareDeviceEnabled;
    bool isRankSpareEnabled;
    MemoryMetrics_v1_MemoryMetrics metrics;
    Memory_v1_Actions actions;
    Resource_v1_Resource status;
    int64_t volatileRegionNumberLimit;
    int64_t persistentRegionNumberLimit;
    int64_t volatileRegionSizeMaxMiB;
    int64_t persistentRegionSizeMaxMiB;
    int64_t allocationIncrementMiB;
    int64_t allocationAlignmentMiB;
    Memory_v1_Links links;
    std::string moduleManufacturerID;
    std::string moduleProductID;
    std::string memorySubsystemControllerManufacturerID;
    std::string memorySubsystemControllerProductID;
    int64_t volatileSizeMiB;
    int64_t nonVolatileSizeMiB;
    int64_t cacheSizeMiB;
    int64_t logicalSizeMiB;
    Resource_v1_Resource location;
    NavigationReference__ assembly;
    Memory_v1_SecurityStates securityState;
    bool configurationLocked;
    bool locationIndicatorActive;
};
struct Memory_v1_MemoryLocation
{
    int64_t socket;
    int64_t memoryController;
    int64_t channel;
    int64_t slot;
};
struct Memory_v1_OemActions
{
};
struct Memory_v1_PowerManagementPolicy
{
    bool policyEnabled;
    int64_t maxTDPMilliWatts;
    int64_t peakPowerBudgetMilliWatts;
    int64_t averagePowerBudgetMilliWatts;
};
struct Memory_v1_RegionSet
{
    std::string regionId;
    Memory_v1_MemoryClassification memoryClassification;
    int64_t offsetMiB;
    int64_t sizeMiB;
    bool passphraseState;
    bool passphraseEnabled;
};
struct Memory_v1_SecurityCapabilities
{
    bool passphraseCapable;
    int64_t maxPassphraseCount;
    Memory_v1_SecurityStates securityStates;
    bool configurationLockCapable;
    bool dataLockCapable;
    int64_t passphraseLockLimit;
};
#endif
