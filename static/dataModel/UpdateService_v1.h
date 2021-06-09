#ifndef UPDATESERVICE_V1
#define UPDATESERVICE_V1

#include "Resource_v1.h"
#include "SoftwareInventoryCollection_v1.h"
#include "UpdateService_v1.h"

#include <chrono>

enum class UpdateService_v1_ApplyTime
{
    Immediate,
    OnReset,
    AtMaintenanceWindowStart,
    InMaintenanceWindowOnReset,
};
enum class UpdateService_v1_TransferProtocolType
{
    CIFS,
    FTP,
    SFTP,
    HTTP,
    HTTPS,
    NSF,
    SCP,
    TFTP,
    OEM,
    NFS,
};
struct UpdateService_v1_Actions
{
    UpdateService_v1_OemActions oem;
};
struct UpdateService_v1_HttpPushUriApplyTime
{
    UpdateService_v1_ApplyTime applyTime;
    std::chrono::time_point maintenanceWindowStartTime;
    int64_t maintenanceWindowDurationInSeconds;
};
struct UpdateService_v1_HttpPushUriOptions
{
    UpdateService_v1_HttpPushUriApplyTime httpPushUriApplyTime;
};
struct UpdateService_v1_OemActions
{};
struct UpdateService_v1_UpdateParameters
{
    std::string targets;
    Resource_v1_Resource oem;
};
struct UpdateService_v1_UpdateService
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource status;
    bool serviceEnabled;
    SoftwareInventoryCollection_v1_SoftwareInventoryCollection
        firmwareInventory;
    SoftwareInventoryCollection_v1_SoftwareInventoryCollection
        softwareInventory;
    UpdateService_v1_Actions actions;
    std::string httpPushUri;
    std::string httpPushUriTargets;
    bool httpPushUriTargetsBusy;
    UpdateService_v1_HttpPushUriOptions httpPushUriOptions;
    bool httpPushUriOptionsBusy;
    int64_t maxImageSizeBytes;
    std::string multipartHttpPushUri;
};
#endif
