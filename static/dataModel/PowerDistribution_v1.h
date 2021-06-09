#ifndef POWERDISTRIBUTION_V1
#define POWERDISTRIBUTION_V1

#include "NavigationReference_.h"
#include "PowerDistribution_v1.h"
#include "Resource_v1.h"

#include <chrono>

enum class PowerDistributionV1PowerEquipmentType
{
    RackPDU,
    FloorPDU,
    ManualTransferSwitch,
    AutomaticTransferSwitch,
    Switchgear,
};
enum class PowerDistributionV1TransferSensitivityType
{
    High,
    Medium,
    Low,
};
struct PowerDistributionV1OemActions
{};
struct PowerDistributionV1Actions
{
    PowerDistributionV1OemActions oem;
};
struct PowerDistributionV1Links
{
    ResourceV1Resource oem;
    NavigationReference_ chassis;
    NavigationReference_ facility;
    NavigationReference_ managedBy;
};
struct PowerDistributionV1TransferConfiguration
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
struct PowerDistributionV1TransferCriteria
{
    PowerDistributionV1TransferSensitivityType transferSensitivity;
    double overVoltageRMSPercentage;
    double underVoltageRMSPercentage;
    double overNominalFrequencyHz;
    double underNominalFrequencyHz;
};
struct PowerDistributionV1PowerDistribution
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    PowerDistributionV1PowerEquipmentType equipmentType;
    std::string model;
    std::string manufacturer;
    std::string serialNumber;
    std::string partNumber;
    std::string version;
    std::string firmwareVersion;
    std::chrono::time_point<std::chrono::system_clock> productionDate;
    std::string assetTag;
    std::string UUID;
    ResourceV1Resource location;
    PowerDistributionV1TransferConfiguration transferConfiguration;
    PowerDistributionV1TransferCriteria transferCriteria;
    NavigationReference_ sensors;
    ResourceV1Resource status;
    NavigationReference_ mains;
    NavigationReference_ branches;
    NavigationReference_ feeders;
    NavigationReference_ subfeeds;
    NavigationReference_ outlets;
    NavigationReference_ outletGroups;
    NavigationReference_ metrics;
    PowerDistributionV1Links links;
    PowerDistributionV1Actions actions;
};
#endif
