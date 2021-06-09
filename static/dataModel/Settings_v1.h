#ifndef SETTINGS_V1
#define SETTINGS_V1

#include "Message_v1.h"
#include "NavigationReference__.h"
#include "Settings_v1.h"

#include <chrono>

enum class Settings_v1_ApplyTime
{
    Immediate,
    OnReset,
    AtMaintenanceWindowStart,
    InMaintenanceWindowOnReset,
};
enum class Settings_v1_OperationApplyTime
{
    Immediate,
    OnReset,
    AtMaintenanceWindowStart,
    InMaintenanceWindowOnReset,
    OnStartUpdateRequest,
};
struct Settings_v1_MaintenanceWindow
{
    std::chrono::time_point maintenanceWindowStartTime;
    int64_t maintenanceWindowDurationInSeconds;
};
struct Settings_v1_OperationApplyTimeSupport
{
    Settings_v1_OperationApplyTime supportedValues;
    std::chrono::time_point maintenanceWindowStartTime;
    int64_t maintenanceWindowDurationInSeconds;
    NavigationReference__ maintenanceWindowResource;
};
struct Settings_v1_PreferredApplyTime
{
    Settings_v1_ApplyTime applyTime;
    std::chrono::time_point maintenanceWindowStartTime;
    int64_t maintenanceWindowDurationInSeconds;
};
struct Settings_v1_Settings
{
    std::chrono::time_point time;
    std::string eTag;
    NavigationReference__ settingsObject;
    Message_v1_Message messages;
    Settings_v1_ApplyTime supportedApplyTimes;
    NavigationReference__ maintenanceWindowResource;
};
#endif
