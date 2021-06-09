#ifndef MEMORY_V1
#define MEMORY_V1

#include "CertificateCollection_v1.h"
#include "MemoryMetrics_v1.h"
#include "Memory_v1.h"
#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"
#include "SoftwareInventory_v1.h"

enum class MemoryV1BaseModuleType
{
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
enum class MemoryV1ErrorCorrection
{
    NoECC,
    SingleBitECC,
    MultiBitECC,
    AddressParity,
};
enum class MemoryV1MemoryClassification
{
    Volatile,
    ByteAccessiblePersistent,
    Block,
};
enum class MemoryV1MemoryDeviceType
{
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
    HBM3,
    GDDR,
    GDDR2,
    GDDR3,
    GDDR4,
    GDDR5,
    GDDR5X,
    GDDR6,
    DDR5,
    OEM,
};
enum class MemoryV1MemoryMedia
{
    DRAM,
    NAND,
    Intel3DXPoint,
    Proprietary,
};
enum class MemoryV1MemoryType
{
    DRAM,
    NVDIMM_N,
    NVDIMM_F,
    NVDIMM_P,
    IntelOptane,
};
enum class MemoryV1OperatingMemoryModes
{
    Volatile,
    PMEM,
    Block,
};
enum class MemoryV1SecurityStates
{
    Enabled,
    Disabled,
    Unlocked,
    Locked,
    Frozen,
    Passphraselimit,
};
struct MemoryV1OemActions
{};
struct MemoryV1Actions
{
    MemoryV1OemActions oem;
};
struct MemoryV1Links
{
    ResourceV1Resource oem;
    NavigationReferenceRedfish chassis;
    NavigationReferenceRedfish processors;
};
struct MemoryV1SecurityCapabilities
{
    bool passphraseCapable;
    int64_t maxPassphraseCount;
    MemoryV1SecurityStates securityStates;
    bool configurationLockCapable;
    bool dataLockCapable;
    int64_t passphraseLockLimit;
};
struct MemoryV1MemoryLocation
{
    int64_t socket;
    int64_t memoryController;
    int64_t channel;
    int64_t slot;
};
struct MemoryV1RegionSet
{
    std::string regionId;
    MemoryV1MemoryClassification memoryClassification;
    int64_t offsetMiB;
    int64_t sizeMiB;
    bool passphraseState;
    bool passphraseEnabled;
};
struct MemoryV1PowerManagementPolicy
{
    bool policyEnabled;
    int64_t maxTDPMilliWatts;
    int64_t peakPowerBudgetMilliWatts;
    int64_t averagePowerBudgetMilliWatts;
};
struct MemoryV1Memory
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    MemoryV1MemoryType memoryType;
    MemoryV1MemoryDeviceType memoryDeviceType;
    MemoryV1BaseModuleType baseModuleType;
    MemoryV1MemoryMedia memoryMedia;
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
    MemoryV1SecurityCapabilities securityCapabilities;
    int64_t spareDeviceCount;
    int64_t rankCount;
    std::string deviceLocator;
    MemoryV1MemoryLocation memoryLocation;
    MemoryV1ErrorCorrection errorCorrection;
    int64_t operatingSpeedMhz;
    int64_t volatileRegionSizeLimitMiB;
    int64_t persistentRegionSizeLimitMiB;
    MemoryV1RegionSet regions;
    MemoryV1OperatingMemoryModes operatingMemoryModes;
    MemoryV1PowerManagementPolicy powerManagementPolicy;
    bool isSpareDeviceEnabled;
    bool isRankSpareEnabled;
    MemoryMetricsV1MemoryMetrics metrics;
    MemoryV1Actions actions;
    ResourceV1Resource status;
    int64_t volatileRegionNumberLimit;
    int64_t persistentRegionNumberLimit;
    int64_t volatileRegionSizeMaxMiB;
    int64_t persistentRegionSizeMaxMiB;
    int64_t allocationIncrementMiB;
    int64_t allocationAlignmentMiB;
    MemoryV1Links links;
    std::string moduleManufacturerID;
    std::string moduleProductID;
    std::string memorySubsystemControllerManufacturerID;
    std::string memorySubsystemControllerProductID;
    int64_t volatileSizeMiB;
    int64_t nonVolatileSizeMiB;
    int64_t cacheSizeMiB;
    int64_t logicalSizeMiB;
    ResourceV1Resource location;
    NavigationReferenceRedfish assembly;
    MemoryV1SecurityStates securityState;
    bool configurationLocked;
    bool locationIndicatorActive;
    std::string sparePartNumber;
    std::string model;
    NavigationReferenceRedfish environmentMetrics;
    CertificateCollectionV1CertificateCollection certificates;
    SoftwareInventoryV1SoftwareInventory measurements;
    bool enabled;
};
#endif
