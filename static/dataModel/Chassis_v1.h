#ifndef CHASSIS_V1
#define CHASSIS_V1

#include "Assembly_v1.h"
#include "CertificateCollection_v1.h"
#include "Chassis_v1.h"
#include "DriveCollection_v1.h"
#include "LogServiceCollection_v1.h"
#include "MediaControllerCollection_v1.h"
#include "MemoryCollection_v1.h"
#include "MemoryDomainCollection_v1.h"
#include "NavigationReference_.h"
#include "NetworkAdapterCollection_v1.h"
#include "PCIeDeviceCollection_v1.h"
#include "PCIeSlots_v1.h"
#include "PowerSubsystem_v1.h"
#include "Power_v1.h"
#include "Resource_v1.h"
#include "SoftwareInventory_v1.h"
#include "ThermalSubsystem_v1.h"
#include "Thermal_v1.h"

enum class ChassisV1ChassisType
{
    Rack,
    Blade,
    Enclosure,
    StandAlone,
    RackMount,
    Card,
    Cartridge,
    Row,
    Pod,
    Expansion,
    Sidecar,
    Zone,
    Sled,
    Shelf,
    Drawer,
    Module,
    Component,
    IPBasedDrive,
    RackGroup,
    StorageEnclosure,
    Other,
};
enum class ChassisV1EnvironmentalClass
{
    A1,
    A2,
    A3,
    A4,
};
enum class ChassisV1IndicatorLED
{
    Unknown,
    Lit,
    Blinking,
    Off,
};
enum class ChassisV1IntrusionSensor
{
    Normal,
    HardwareIntrusion,
    TamperingDetected,
};
enum class ChassisV1IntrusionSensorReArm
{
    Manual,
    Automatic,
};
enum class ChassisV1PowerState
{
    On,
    Off,
    PoweringOn,
    PoweringOff,
};
struct ChassisV1OemActions
{};
struct ChassisV1Actions
{
    ChassisV1OemActions oem;
};
struct ChassisV1Links
{
    ResourceV1Resource oem;
    NavigationReference_ computerSystems;
    NavigationReference_ managedBy;
    NavigationReference_ containedBy;
    NavigationReference_ contains;
    NavigationReference_ poweredBy;
    NavigationReference_ cooledBy;
    NavigationReference_ managersInChassis;
    NavigationReference_ drives;
    NavigationReference_ storage;
    NavigationReference_ pCIeDevices;
    NavigationReference_ resourceBlocks;
    NavigationReference_ switches;
    NavigationReference_ processors;
    NavigationReference_ facility;
};
struct ChassisV1PhysicalSecurity
{
    int64_t intrusionSensorNumber;
    ChassisV1IntrusionSensor intrusionSensor;
    ChassisV1IntrusionSensorReArm intrusionSensorReArm;
};
struct ChassisV1Chassis
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ChassisV1ChassisType chassisType;
    std::string manufacturer;
    std::string model;
    std::string SKU;
    std::string serialNumber;
    std::string partNumber;
    std::string assetTag;
    ChassisV1IndicatorLED indicatorLED;
    ChassisV1Links links;
    ChassisV1Actions actions;
    ResourceV1Resource status;
    LogServiceCollectionV1LogServiceCollection logServices;
    ThermalV1Thermal thermal;
    PowerV1Power power;
    ChassisV1PowerState powerState;
    ChassisV1PhysicalSecurity physicalSecurity;
    ResourceV1Resource location;
    double heightMm;
    double widthMm;
    double depthMm;
    double weightKg;
    NetworkAdapterCollectionV1NetworkAdapterCollection networkAdapters;
    AssemblyV1Assembly assembly;
    std::string UUID;
    PCIeSlotsV1PCIeSlots pCIeSlots;
    ChassisV1EnvironmentalClass environmentalClass;
    NavigationReference_ sensors;
    PCIeDeviceCollectionV1PCIeDeviceCollection pCIeDevices;
    MediaControllerCollectionV1MediaControllerCollection mediaControllers;
    MemoryCollectionV1MemoryCollection memory;
    MemoryDomainCollectionV1MemoryDomainCollection memoryDomains;
    double maxPowerWatts;
    double minPowerWatts;
    bool locationIndicatorActive;
    DriveCollectionV1DriveCollection drives;
    PowerSubsystemV1PowerSubsystem powerSubsystem;
    ThermalSubsystemV1ThermalSubsystem thermalSubsystem;
    NavigationReference_ environmentMetrics;
    CertificateCollectionV1CertificateCollection certificates;
    SoftwareInventoryV1SoftwareInventory measurements;
    std::string sparePartNumber;
};
#endif
