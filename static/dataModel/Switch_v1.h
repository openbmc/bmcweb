#ifndef SWITCH_V1
#define SWITCH_V1

#include "CertificateCollection_v1.h"
#include "LogServiceCollection_v1.h"
#include "NavigationReference_.h"
#include "PortCollection_v1.h"
#include "Protocol_v1.h"
#include "Redundancy_v1.h"
#include "Resource_v1.h"
#include "SoftwareInventory_v1.h"
#include "Switch_v1.h"

struct SwitchV1OemActions
{};
struct SwitchV1Actions
{
    SwitchV1OemActions oem;
};
struct SwitchV1Links
{
    ResourceV1Resource oem;
    NavigationReference_ chassis;
    NavigationReference_ managedBy;
    NavigationReference_ endpoints;
    NavigationReference_ pCIeDevice;
};
struct SwitchV1Switch
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ProtocolV1Protocol switchType;
    ResourceV1Resource status;
    std::string manufacturer;
    std::string model;
    std::string SKU;
    std::string serialNumber;
    std::string partNumber;
    std::string assetTag;
    int64_t domainID;
    bool isManaged;
    int64_t totalSwitchWidth;
    ResourceV1Resource indicatorLED;
    ResourceV1Resource powerState;
    PortCollectionV1PortCollection ports;
    RedundancyV1Redundancy redundancy;
    SwitchV1Links links;
    LogServiceCollectionV1LogServiceCollection logServices;
    SwitchV1Actions actions;
    ResourceV1Resource location;
    std::string firmwareVersion;
    ProtocolV1Protocol supportedProtocols;
    std::string UUID;
    bool locationIndicatorActive;
    double currentBandwidthGbps;
    double maxBandwidthGbps;
    CertificateCollectionV1CertificateCollection certificates;
    SoftwareInventoryV1SoftwareInventory measurements;
    bool enabled;
    NavigationReference_ environmentMetrics;
};
#endif
