#pragma once
/****************************************************************
 *                 READ THIS WARNING FIRST
 * This is an auto-generated header which contains definitions
 * for Redfish DMTF defined messages.
 * DO NOT modify this registry outside of running the
 * parse_registries.py script.  The definitions contained within
 * this file are owned by DMTF.  Any modifications to these files
 * should be first pushed to the relevant registry in the DMTF
 * github organization.
 ***************************************************************/
#include "registries.hpp"

#include <array>

// clang-format off

namespace redfish::registries::storage_device
{
const Header header = {
    "Copyright 2020-2023 DMTF. All rights reserved.",
    "#MessageRegistry.v1_6_2.MessageRegistry",
    "StorageDevice.1.2.1",
    "Storage Device Message Registry",
    "en",
    "This registry defines the messages for storage devices.",
    "StorageDevice",
    "1.2.1",
    "DMTF",
};
constexpr const char* url =
    "https://redfish.dmtf.org/registries/StorageDevice.1.2.1.json";

constexpr std::array registry =
{
    MessageEntry{
        "BatteryCharging",
        {
            "A battery charging condition was detected.",
            "A charging condition for the battery located in '%1' was detected.",
            "Warning",
            1,
            {
                "string",
            },
            "None.  There may be reduced performance and features while the battery is charging.",
        }},
    MessageEntry{
        "BatteryFailure",
        {
            "A battery failure condition was detected.",
            "A failure condition for the battery located in '%1' was detected.",
            "Critical",
            1,
            {
                "string",
            },
            "Ensure all cables are properly and securely connected.  Replace the failed battery.",
        }},
    MessageEntry{
        "BatteryMissing",
        {
            "A battery missing condition was detected.",
            "The battery located in '%1' is missing.",
            "Critical",
            1,
            {
                "string",
            },
            "Attach the battery.  Ensure all cables are properly and securely connected.",
        }},
    MessageEntry{
        "BatteryOK",
        {
            "The health of a battery has changed to OK.",
            "The health of the battery located in '%1' has changed to OK.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "ControllerDegraded",
        {
            "A storage controller degraded condition was detected.",
            "A degraded condition for the storage controller located in '%1' was detected due to reason '%2'.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "Reseat the storage controller in the PCI slot.  Update the controller to the latest firmware version.  If the issue persists, replace the controller.",
        }},
    MessageEntry{
        "ControllerFailure",
        {
            "A storage controller failure was detected.",
            "A failure condition for the storage controller located in '%1' was detected.",
            "Critical",
            1,
            {
                "string",
            },
            "Reseat the storage controller in the PCI slot.  Update the controller to the latest firmware version.  If the issue persists, replace the controller.",
        }},
    MessageEntry{
        "ControllerOK",
        {
            "The storage controller health has changed to OK.",
            "The health of the storage controller located in '%1' has changed to OK.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "ControllerPasswordAccepted",
        {
            "The storage controller password was entered.",
            "A password was entered for the storage controller located in '%1'.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "ControllerPasswordRequired",
        {
            "The storage controller requires a password.",
            "The storage controller located in '%1' requires a password.",
            "Critical",
            1,
            {
                "string",
            },
            "Enter the controller password.",
        }},
    MessageEntry{
        "ControllerPortDegraded",
        {
            "A controller port degraded condition was detected.",
            "A degraded condition for the controller port located in '%1' was detected due to reason '%2'.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "Ensure all cables are properly and securely connected.  Replace faulty cables.",
        }},
    MessageEntry{
        "ControllerPortFailure",
        {
            "A controller port failure condition was detected.",
            "A failure condition for the controller port located in '%1' was detected.",
            "Critical",
            1,
            {
                "string",
            },
            "Ensure all cables are properly and securely connected.  Replace faulty cables.",
        }},
    MessageEntry{
        "ControllerPortOK",
        {
            "The health of a controller port has changed to OK.",
            "The health of the controller port located in '%1' has changed to OK.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "ControllerPreviousError",
        {
            "A storage controller error was detected prior to reboot.",
            "A previous error condition for the storage controller located in '%1' was detected due to '%2'.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "Update the controller to the latest firmware version.  If the issue persists, replace the controller.",
        }},
    MessageEntry{
        "DriveFailure",
        {
            "A drive failure condition was detected.",
            "A failure condition for the drive located in '%1' was detected.",
            "Critical",
            1,
            {
                "string",
            },
            "Ensure all cables are properly and securely connected.  Ensure all drives are fully seated.  Replace the defective cables, drive, or both.",
        }},
    MessageEntry{
        "DriveFailureCleared",
        {
            "A previously detected failure condition on a drive was cleared.",
            "A failure condition for the drive located in '%1' was cleared.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "DriveInserted",
        {
            "A drive was inserted.",
            "The drive located in '%1' was inserted.",
            "OK",
            1,
            {
                "string",
            },
            "If the drive is not properly displayed, attempt to refresh the cached data.",
        }},
    MessageEntry{
        "DriveMissing",
        {
            "A drive missing condition was detected.",
            "A missing condition for the drive located in '%1' was detected.",
            "Critical",
            1,
            {
                "string",
            },
            "Ensure all cables are properly and securely connected.  Ensure all drives are fully seated.  Replace the defective cables, drive, or both.  Delete the volume if it is no longer needed.",
        }},
    MessageEntry{
        "DriveMissingCleared",
        {
            "A previous drive missing condition was cleared.",
            "A missing condition for the drive located in '%1' was cleared.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "DriveOK",
        {
            "The health of a drive has changed to OK.",
            "The health of the drive located in '%1' has changed to OK.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "DriveOffline",
        {
            "A drive offline condition was detected.",
            "An offline condition for the drive located in '%1' was detected.",
            "Critical",
            1,
            {
                "string",
            },
            "If the drive is unconfigured or needs an import, configure the drive.  If the drive operation is in progress, wait for the operation to complete.  If the drive is not supported, replace it.",
        }},
    MessageEntry{
        "DriveOfflineCleared",
        {
            "A drive offline condition was cleared.",
            "An offline condition for the drive located in '%1' was cleared.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "DrivePredictiveFailure",
        {
            "A predictive drive failure condition was detected.",
            "A predictive failure condition for the drive located in '%1' was detected.",
            "Warning",
            1,
            {
                "string",
            },
            "If this drive is not part of a fault-tolerant volume, first back up all data, then replace the drive and restore all data afterward.  If this drive is part of a fault-tolerant volume, replace this drive as soon as possible as long as the volume health is OK.",
        }},
    MessageEntry{
        "DrivePredictiveFailureCleared",
        {
            "A previously detected predictive failure condition on a drive was cleared.",
            "A predictive failure condition for the drive located in '%1' was cleared.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "DriveRemoved",
        {
            "A drive was removed.",
            "The drive located in '%1' was removed.",
            "Critical",
            1,
            {
                "string",
            },
            "If the drive is still displayed, attempt to refresh the cached data.",
        }},
    MessageEntry{
        "VolumeDegraded",
        {
            "The storage controller has detected a degraded volume condition.",
            "The volume '%1' attached to the storage controller located in '%2' is degraded.",
            "Warning",
            2,
            {
                "string",
                "string",
            },
            "Ensure all cables are properly and securely connected.  Replace failed drives.",
        }},
    MessageEntry{
        "VolumeFailure",
        {
            "The storage controller has detected a failed volume condition.",
            "The volume '%1' attached to the storage controller located in '%2' has failed.",
            "Critical",
            2,
            {
                "string",
                "string",
            },
            "Ensure all cables are properly and securely connected.  Ensure all drives are fully seated and operational.",
        }},
    MessageEntry{
        "VolumeOK",
        {
            "A volume health has changed to OK.",
            "The health of volume '%1' that is attached to the storage controller located in '%2' has changed to OK.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "VolumeOffine",
        {
            "The storage controller has detected an offline volume condition.",
            "The volume '%1' attached to the storage controller located in '%2' is offline.",
            "Critical",
            2,
            {
                "string",
                "string",
            },
            "Use storage software to enable, repair, or import the volume.  You may also delete or move volume back to the original controller.",
        }},
    MessageEntry{
        "VolumeOffline",
        {
            "The storage controller has detected an offline volume condition.",
            "The volume '%1' attached to the storage controller located in '%2' is offline.",
            "Critical",
            2,
            {
                "string",
                "string",
            },
            "Use storage software to enable, repair, or import the volume.  You may also delete or move volume back to the original controller.",
        }},
    MessageEntry{
        "VolumeOfflineCleared",
        {
            "The storage controller has detected an online volume condition.",
            "The volume '%1' attached to the storage controller located in '%2' is online.",
            "OK",
            2,
            {
                "string",
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "WriteCacheDataLoss",
        {
            "The write cache is reporting loss of data in posted-writes memory.",
            "The write cache on the storage controller located in '%1' has data loss.",
            "Critical",
            1,
            {
                "string",
            },
            "Check the controller resource properties to determine the cause of the write cache data loss.",
        }},
    MessageEntry{
        "WriteCacheDegraded",
        {
            "The write cache state is degraded.",
            "The write cache state on the storage controller located in '%1' is degraded.",
            "Critical",
            1,
            {
                "string",
            },
            "Check the controller to determine the cause of write cache degraded state, such as a missing battery or hardware failure.",
        }},
    MessageEntry{
        "WriteCacheProtected",
        {
            "A storage controller write cache state is in protected mode.",
            "The write cache state on the storage controller located in '%1' is in protected mode.",
            "OK",
            1,
            {
                "string",
            },
            "None.",
        }},
    MessageEntry{
        "WriteCacheTemporarilyDegraded",
        {
            "The write cache state is temporarily degraded.",
            "The write cache state on the storage controller located in '%1' is temporarily degraded.",
            "Warning",
            1,
            {
                "string",
            },
            "Check the controller to determine the cause of write cache temporarily degraded state, such as a battery is charging or a data recovery rebuild operation is pending.",
        }},

};

enum class Index
{
    batteryCharging = 0,
    batteryFailure = 1,
    batteryMissing = 2,
    batteryOK = 3,
    controllerDegraded = 4,
    controllerFailure = 5,
    controllerOK = 6,
    controllerPasswordAccepted = 7,
    controllerPasswordRequired = 8,
    controllerPortDegraded = 9,
    controllerPortFailure = 10,
    controllerPortOK = 11,
    controllerPreviousError = 12,
    driveFailure = 13,
    driveFailureCleared = 14,
    driveInserted = 15,
    driveMissing = 16,
    driveMissingCleared = 17,
    driveOK = 18,
    driveOffline = 19,
    driveOfflineCleared = 20,
    drivePredictiveFailure = 21,
    drivePredictiveFailureCleared = 22,
    driveRemoved = 23,
    volumeDegraded = 24,
    volumeFailure = 25,
    volumeOK = 26,
    volumeOffine = 27,
    volumeOffline = 28,
    volumeOfflineCleared = 29,
    writeCacheDataLoss = 30,
    writeCacheDegraded = 31,
    writeCacheProtected = 32,
    writeCacheTemporarilyDegraded = 33,
};
} // namespace redfish::registries::storage_device
