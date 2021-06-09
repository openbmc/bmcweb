#ifndef DRIVE_V1
#define DRIVE_V1

#include "Assembly_v1.h"
#include "CertificateCollection_v1.h"
#include "Drive_v1.h"
#include "NavigationReferenceRedfish.h"
#include "Protocol_v1.h"
#include "Resource_v1.h"
#include "SoftwareInventory_v1.h"

enum class DriveV1EncryptionAbility
{
    None,
    SelfEncryptingDrive,
    Other,
};
enum class DriveV1EncryptionStatus
{
    Unecrypted,
    Unlocked,
    Locked,
    Foreign,
    Unencrypted,
};
enum class DriveV1HotspareReplacementModeType
{
    Revertible,
    NonRevertible,
};
enum class DriveV1HotspareType
{
    None,
    Global,
    Chassis,
    Dedicated,
};
enum class DriveV1MediaType
{
    HDD,
    SSD,
    SMR,
};
enum class DriveV1StatusIndicator
{
    OK,
    Fail,
    Rebuild,
    PredictiveFailureAnalysis,
    Hotspare,
    InACriticalArray,
    InAFailedArray,
};
struct DriveV1OemActions
{};
struct DriveV1Actions
{
    DriveV1OemActions oem;
};
struct DriveV1Links
{
    ResourceV1Resource oem;
    NavigationReferenceRedfish volumes;
    NavigationReferenceRedfish endpoints;
    NavigationReferenceRedfish chassis;
    NavigationReferenceRedfish pCIeFunctions;
    NavigationReferenceRedfish storagePools;
};
struct DriveV1Operations
{
    std::string operationName;
    int64_t percentageComplete;
    NavigationReferenceRedfish associatedTask;
};
struct DriveV1Drive
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    DriveV1StatusIndicator statusIndicator;
    ResourceV1Resource indicatorLED;
    std::string model;
    std::string revision;
    ResourceV1Resource status;
    int64_t capacityBytes;
    bool failurePredicted;
    ProtocolV1Protocol protocol;
    DriveV1MediaType mediaType;
    std::string manufacturer;
    std::string SKU;
    std::string serialNumber;
    std::string partNumber;
    std::string assetTag;
    ResourceV1Resource identifiers;
    ResourceV1Resource location;
    DriveV1HotspareType hotspareType;
    DriveV1EncryptionAbility encryptionAbility;
    DriveV1EncryptionStatus encryptionStatus;
    double rotationSpeedRPM;
    int64_t blockSizeBytes;
    double capableSpeedGbs;
    double negotiatedSpeedGbs;
    double predictedMediaLifeLeftPercent;
    DriveV1Links links;
    DriveV1Actions actions;
    DriveV1Operations operations;
    AssemblyV1Assembly assembly;
    ResourceV1Resource physicalLocation;
    DriveV1HotspareReplacementModeType hotspareReplacementMode;
    bool writeCacheEnabled;
    bool multipath;
    bool readyToRemove;
    bool locationIndicatorActive;
    NavigationReferenceRedfish environmentMetrics;
    CertificateCollectionV1CertificateCollection certificates;
    SoftwareInventoryV1SoftwareInventory measurements;
};
#endif
