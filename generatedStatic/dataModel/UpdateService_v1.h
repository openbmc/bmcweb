#ifndef UPDATESERVICE_V1
#define UPDATESERVICE_V1

#include "CertificateCollection_v1.h"
#include "Resource_v1.h"
#include "SoftwareInventoryCollection_v1.h"
#include "UpdateService_v1.h"

#include <chrono>

enum class UpdateServiceV1ApplyTime
{
    Immediate,
    OnReset,
    AtMaintenanceWindowStart,
    InMaintenanceWindowOnReset,
};
enum class UpdateServiceV1TransferProtocolType
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
struct UpdateServiceV1OemActions
{};
struct UpdateServiceV1Actions
{
    UpdateServiceV1OemActions oem;
};
struct UpdateServiceV1HttpPushUriApplyTime
{
    UpdateServiceV1ApplyTime applyTime;
    std::chrono::time_point<std::chrono::system_clock>
        maintenanceWindowStartTime;
    int64_t maintenanceWindowDurationInSeconds;
};
struct UpdateServiceV1HttpPushUriOptions
{
    UpdateServiceV1HttpPushUriApplyTime httpPushUriApplyTime;
};
struct UpdateServiceV1UpdateParameters
{
    std::string targets;
    ResourceV1Resource oem;
};
struct UpdateServiceV1UpdateService
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource status;
    bool serviceEnabled;
    SoftwareInventoryCollectionV1SoftwareInventoryCollection firmwareInventory;
    SoftwareInventoryCollectionV1SoftwareInventoryCollection softwareInventory;
    UpdateServiceV1Actions actions;
    std::string httpPushUri;
    std::string httpPushUriTargets;
    bool httpPushUriTargetsBusy;
    UpdateServiceV1HttpPushUriOptions httpPushUriOptions;
    bool httpPushUriOptionsBusy;
    int64_t maxImageSizeBytes;
    std::string multipartHttpPushUri;
    CertificateCollectionV1CertificateCollection remoteServerCertificates;
    bool verifyRemoteServerCertificate;
};
#endif
