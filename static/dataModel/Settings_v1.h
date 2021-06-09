#ifndef SETTINGS_V1
#define SETTINGS_V1

#include "Message_v1.h"
#include "NavigationReference_.h"
#include "Settings_v1.h"

#include <chrono>

enum class SettingsV1ApplyTime
{
    Immediate,
    OnReset,
    AtMaintenanceWindowStart,
    InMaintenanceWindowOnReset,
};
enum class SettingsV1OperationApplyTime
{
    Immediate,
    OnReset,
    AtMaintenanceWindowStart,
    InMaintenanceWindowOnReset,
    OnStartUpdateRequest,
};
struct SettingsV1MaintenanceWindow
{
    std::chrono::time_point<std::chrono::system_clock>
        maintenanceWindowStartTime;
    int64_t maintenanceWindowDurationInSeconds;
};
struct SettingsV1OperationApplyTimeSupport
{
    SettingsV1OperationApplyTime supportedValues;
    std::chrono::time_point<std::chrono::system_clock>
        maintenanceWindowStartTime;
    int64_t maintenanceWindowDurationInSeconds;
    NavigationReference_ maintenanceWindowResource;
};
struct SettingsV1PreferredApplyTime
{
    SettingsV1ApplyTime applyTime;
    std::chrono::time_point<std::chrono::system_clock>
        maintenanceWindowStartTime;
    int64_t maintenanceWindowDurationInSeconds;
};
struct SettingsV1Settings
{
    std::chrono::time_point<std::chrono::system_clock> time;
    std::string eTag;
    NavigationReference_ settingsObject;
    MessageV1Message messages;
    SettingsV1ApplyTime supportedApplyTimes;
    NavigationReference_ maintenanceWindowResource;
};
#endif
