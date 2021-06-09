#ifndef SWITCH_V1
#define SWITCH_V1

#include "LogServiceCollection_v1.h"
#include "NavigationReference__.h"
#include "PortCollection_v1.h"
#include "Protocol_v1.h"
#include "Redundancy_v1.h"
#include "Resource_v1.h"
#include "Switch_v1.h"

struct Switch_v1_Actions
{
    Switch_v1_OemActions oem;
};
struct Switch_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ chassis;
    NavigationReference__ managedBy;
    NavigationReference__ endpoints;
    NavigationReference__ pCIeDevice;
};
struct Switch_v1_OemActions
{
};
struct Switch_v1_Switch
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Protocol_v1_Protocol switchType;
    Resource_v1_Resource status;
    std::string manufacturer;
    std::string model;
    std::string SKU;
    std::string serialNumber;
    std::string partNumber;
    std::string assetTag;
    int64_t domainID;
    bool isManaged;
    int64_t totalSwitchWidth;
    Resource_v1_Resource indicatorLED;
    Resource_v1_Resource powerState;
    PortCollection_v1_PortCollection ports;
    Redundancy_v1_Redundancy redundancy;
    Switch_v1_Links links;
    LogServiceCollection_v1_LogServiceCollection logServices;
    Switch_v1_Actions actions;
    Resource_v1_Resource location;
    std::string firmwareVersion;
    Protocol_v1_Protocol supportedProtocols;
    string UUID;
    bool locationIndicatorActive;
    double currentBandwidthGbps;
    double maxBandwidthGbps;
};
#endif
