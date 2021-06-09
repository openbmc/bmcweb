#ifndef CHASSIS_V1
#define CHASSIS_V1

#include "Assembly_v1.h"
#include "Chassis_v1.h"
#include "DriveCollection_v1.h"
#include "LogServiceCollection_v1.h"
#include "MediaControllerCollection_v1.h"
#include "MemoryCollection_v1.h"
#include "MemoryDomainCollection_v1.h"
#include "NavigationReference__.h"
#include "NetworkAdapterCollection_v1.h"
#include "PCIeDeviceCollection_v1.h"
#include "PCIeSlots_v1.h"
#include "Power_v1.h"
#include "Resource_v1.h"
#include "Thermal_v1.h"

enum class Chassis_v1_ChassisType {
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
enum class Chassis_v1_EnvironmentalClass {
    A1,
    A2,
    A3,
    A4,
};
enum class Chassis_v1_IndicatorLED {
    Unknown,
    Lit,
    Blinking,
    Off,
};
enum class Chassis_v1_IntrusionSensor {
    Normal,
    HardwareIntrusion,
    TamperingDetected,
};
enum class Chassis_v1_IntrusionSensorReArm {
    Manual,
    Automatic,
};
enum class Chassis_v1_PowerState {
    On,
    Off,
    PoweringOn,
    PoweringOff,
};
struct Chassis_v1_Actions
{
    Chassis_v1_OemActions oem;
};
struct Chassis_v1_Chassis
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Chassis_v1_ChassisType chassisType;
    std::string manufacturer;
    std::string model;
    std::string SKU;
    std::string serialNumber;
    std::string partNumber;
    std::string assetTag;
    Chassis_v1_IndicatorLED indicatorLED;
    Chassis_v1_Links links;
    Chassis_v1_Actions actions;
    Resource_v1_Resource status;
    LogServiceCollection_v1_LogServiceCollection logServices;
    Thermal_v1_Thermal thermal;
    Power_v1_Power power;
    Chassis_v1_PowerState powerState;
    Chassis_v1_PhysicalSecurity physicalSecurity;
    Resource_v1_Resource location;
    double heightMm;
    double widthMm;
    double depthMm;
    double weightKg;
    NetworkAdapterCollection_v1_NetworkAdapterCollection networkAdapters;
    Assembly_v1_Assembly assembly;
    string UUID;
    PCIeSlots_v1_PCIeSlots pCIeSlots;
    Chassis_v1_EnvironmentalClass environmentalClass;
    NavigationReference__ sensors;
    PCIeDeviceCollection_v1_PCIeDeviceCollection pCIeDevices;
    MediaControllerCollection_v1_MediaControllerCollection mediaControllers;
    MemoryCollection_v1_MemoryCollection memory;
    MemoryDomainCollection_v1_MemoryDomainCollection memoryDomains;
    double maxPowerWatts;
    double minPowerWatts;
    bool locationIndicatorActive;
    DriveCollection_v1_DriveCollection drives;
};
struct Chassis_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ computerSystems;
    NavigationReference__ managedBy;
    NavigationReference__ containedBy;
    NavigationReference__ contains;
    NavigationReference__ poweredBy;
    NavigationReference__ cooledBy;
    NavigationReference__ managersInChassis;
    NavigationReference__ drives;
    NavigationReference__ storage;
    NavigationReference__ pCIeDevices;
    NavigationReference__ resourceBlocks;
    NavigationReference__ switches;
    NavigationReference__ processors;
    NavigationReference__ facility;
};
struct Chassis_v1_OemActions
{
};
struct Chassis_v1_PhysicalSecurity
{
    int64_t intrusionSensorNumber;
    Chassis_v1_IntrusionSensor intrusionSensor;
    Chassis_v1_IntrusionSensorReArm intrusionSensorReArm;
};
#endif
