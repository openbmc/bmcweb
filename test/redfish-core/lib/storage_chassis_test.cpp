// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "generated/enums/drive.hpp"
#include "storage_chassis.hpp"

#include <initializer_list>
#include <optional>
#include <string_view>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

constexpr std::string_view encrypted =
    "xyz.openbmc_project.Inventory.Item.Drive.DriveEncryptionState.Encrypted";
constexpr std::string_view unencrypted =
    "xyz.openbmc_project.Inventory.Item.Drive.DriveEncryptionState.Unencrypted";
constexpr std::string_view unknownEncryption =
    "xyz.openbmc_project.Inventory.Item.Drive.DriveEncryptionState.Unknown";
constexpr std::string_view locked =
    "xyz.openbmc_project.Inventory.Item.Drive.DriveLockState.Locked";
constexpr std::string_view unlocked =
    "xyz.openbmc_project.Inventory.Item.Drive.DriveLockState.Unlocked";
constexpr std::string_view unknownLock =
    "xyz.openbmc_project.Inventory.Item.Drive.DriveLockState.Unknown";

TEST(ConvertDriveEncryptionStatus, EncryptedLocked)
{
    EXPECT_EQ(convertDriveEncryptionStatus(encrypted, locked),
              drive::EncryptionStatus::Locked);
}

TEST(ConvertDriveEncryptionStatus, EncryptedUnlocked)
{
    EXPECT_EQ(convertDriveEncryptionStatus(encrypted, unlocked),
              drive::EncryptionStatus::Unlocked);
}

TEST(ConvertDriveEncryptionStatus, EncryptedUnknownLock)
{
    EXPECT_EQ(convertDriveEncryptionStatus(encrypted, unknownLock),
              std::nullopt);
}

TEST(ConvertDriveEncryptionStatus, EncryptedMissingLock)
{
    EXPECT_EQ(convertDriveEncryptionStatus(encrypted, {}), std::nullopt);
}

TEST(ConvertDriveEncryptionStatus, Unencrypted)
{
    for (std::string_view lockState :
         {locked, unlocked, unknownLock, std::string_view{}})
    {
        EXPECT_EQ(convertDriveEncryptionStatus(unencrypted, lockState),
                  drive::EncryptionStatus::Unencrypted);
    }
}

TEST(ConvertDriveEncryptionStatus, UnknownEncryption)
{
    for (std::string_view lockState :
         {locked, unlocked, unknownLock, std::string_view{}})
    {
        EXPECT_EQ(convertDriveEncryptionStatus(unknownEncryption, lockState),
                  std::nullopt);
    }
}

TEST(ConvertDriveEncryptionStatus, InvalidEncryption)
{
    EXPECT_EQ(convertDriveEncryptionStatus("Invalid", locked),
              drive::EncryptionStatus::Invalid);
}

TEST(ConvertDriveEncryptionStatus, InvalidLock)
{
    EXPECT_EQ(convertDriveEncryptionStatus(encrypted, "Invalid"),
              drive::EncryptionStatus::Invalid);
}

} // namespace
} // namespace redfish
