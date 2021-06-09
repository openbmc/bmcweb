#ifndef DRIVE_V1
#define DRIVE_V1

#include "Assembly_v1.h"
#include "Drive_v1.h"
#include "NavigationReference__.h"
#include "Protocol_v1.h"
#include "Resource_v1.h"

enum class Drive_v1_EncryptionAbility {
    None,
    SelfEncryptingDrive,
    Other,
};
enum class Drive_v1_EncryptionStatus {
    Unecrypted,
    Unlocked,
    Locked,
    Foreign,
    Unencrypted,
};
enum class Drive_v1_HotspareReplacementModeType {
    Revertible,
    NonRevertible,
};
enum class Drive_v1_HotspareType {
    None,
    Global,
    Chassis,
    Dedicated,
};
enum class Drive_v1_MediaType {
    HDD,
    SSD,
    SMR,
};
enum class Drive_v1_StatusIndicator {
    OK,
    Fail,
    Rebuild,
    PredictiveFailureAnalysis,
    Hotspare,
    InACriticalArray,
    InAFailedArray,
};
struct Drive_v1_Actions
{
    Drive_v1_OemActions oem;
};
struct Drive_v1_Drive
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Drive_v1_StatusIndicator statusIndicator;
    Resource_v1_Resource indicatorLED;
    std::string model;
    std::string revision;
    Resource_v1_Resource status;
    int64_t capacityBytes;
    bool failurePredicted;
    Protocol_v1_Protocol protocol;
    Drive_v1_MediaType mediaType;
    std::string manufacturer;
    std::string SKU;
    std::string serialNumber;
    std::string partNumber;
    std::string assetTag;
    Resource_v1_Resource identifiers;
    Resource_v1_Resource location;
    Drive_v1_HotspareType hotspareType;
    Drive_v1_EncryptionAbility encryptionAbility;
    Drive_v1_EncryptionStatus encryptionStatus;
    double rotationSpeedRPM;
    int64_t blockSizeBytes;
    double capableSpeedGbs;
    double negotiatedSpeedGbs;
    double predictedMediaLifeLeftPercent;
    Drive_v1_Links links;
    Drive_v1_Actions actions;
    Drive_v1_Operations operations;
    Assembly_v1_Assembly assembly;
    Resource_v1_Resource physicalLocation;
    Drive_v1_HotspareReplacementModeType hotspareReplacementMode;
    bool writeCacheEnabled;
    bool multipath;
    bool readyToRemove;
    bool locationIndicatorActive;
};
struct Drive_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ volumes;
    NavigationReference__ endpoints;
    NavigationReference__ chassis;
    NavigationReference__ pCIeFunctions;
    NavigationReference__ storagePools;
};
struct Drive_v1_OemActions
{
};
struct Drive_v1_Operations
{
    std::string operationName;
    int64_t percentageComplete;
    NavigationReference__ associatedTask;
};
#endif
