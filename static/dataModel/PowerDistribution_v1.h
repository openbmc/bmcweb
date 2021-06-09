#ifndef POWERDISTRIBUTION_V1
#define POWERDISTRIBUTION_V1

#include "NavigationReference__.h"
#include "PowerDistribution_v1.h"
#include "Resource_v1.h"

#include <chrono>

enum class PowerDistribution_v1_PowerEquipmentType
{
    RackPDU,
    FloorPDU,
    ManualTransferSwitch,
    AutomaticTransferSwitch,
    Switchgear,
};
enum class PowerDistribution_v1_TransferSensitivityType
{
    High,
    Medium,
    Low,
};
struct PowerDistribution_v1_Actions
{
    PowerDistribution_v1_OemActions oem;
};
struct PowerDistribution_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ chassis;
    NavigationReference__ facility;
    NavigationReference__ managedBy;
};
struct PowerDistribution_v1_OemActions
{};
struct PowerDistribution_v1_PowerDistribution
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    PowerDistribution_v1_PowerEquipmentType equipmentType;
    std::string model;
    std::string manufacturer;
    std::string serialNumber;
    std::string partNumber;
    std::string version;
    std::string firmwareVersion;
    std::chrono::time_point productionDate;
    std::string assetTag;
    string UUID;
    Resource_v1_Resource location;
    PowerDistribution_v1_TransferConfiguration transferConfiguration;
    PowerDistribution_v1_TransferCriteria transferCriteria;
    NavigationReference__ sensors;
    Resource_v1_Resource status;
    NavigationReference__ mains;
    NavigationReference__ branches;
    NavigationReference__ feeders;
    NavigationReference__ subfeeds;
    NavigationReference__ outlets;
    NavigationReference__ outletGroups;
    NavigationReference__ metrics;
    PowerDistribution_v1_Links links;
    PowerDistribution_v1_Actions actions;
};
struct PowerDistribution_v1_TransferConfiguration
{
    std::string activeMainsId;
    bool autoTransferEnabled;
    bool closedTransitionAllowed;
    int64_t closedTransitionTimeoutSeconds;
    std::string preferredMainsId;
    int64_t retransferDelaySeconds;
    bool retransferEnabled;
    int64_t transferDelaySeconds;
    bool transferInhibit;
};
struct PowerDistribution_v1_TransferCriteria
{
    PowerDistribution_v1_TransferSensitivityType transferSensitivity;
    double overVoltageRMSPercentage;
    double underVoltageRMSPercentage;
    double overNominalFrequencyHz;
    double underNominalFrequencyHz;
};
#endif
