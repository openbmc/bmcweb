// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "utils/dump_utils.hpp"

#include "dbus_utility.hpp"
#include "generated/enums/log_entry.hpp"

#include <sdbusplus/message/native_types.hpp>

#include <string>

#include <gtest/gtest.h>

namespace redfish::dump_utils
{

TEST(DumpUtils, DumpTypeToStr)
{
    EXPECT_EQ(dumpTypeToStr(DumpType::BMC), "BMC");
    EXPECT_EQ(dumpTypeToStr(DumpType::System), "System");
    EXPECT_EQ(dumpTypeToStr(DumpType::FaultLog), "FaultLog");
}

TEST(DumpUtils, DumpTypeToObjPath)
{
    EXPECT_EQ(dumpTypeToObjPath(DumpType::BMC),
              sdbusplus::object_path{"/xyz/openbmc_project/dump/bmc"});
    EXPECT_EQ(dumpTypeToObjPath(DumpType::System),
              sdbusplus::object_path{"/xyz/openbmc_project/dump/system"});
    EXPECT_EQ(dumpTypeToObjPath(DumpType::FaultLog),
              sdbusplus::object_path{"/xyz/openbmc_project/dump/faultlog"});
}

TEST(DumpUtils, MapDbusStatusToDumpProgress)
{
    EXPECT_EQ(
        mapDbusStatusToDumpProgress(
            "xyz.openbmc_project.Common.Progress.OperationStatus.Completed"),
        DumpCreationProgress::DUMP_CREATE_SUCCESS);

    EXPECT_EQ(mapDbusStatusToDumpProgress(
                  "xyz.openbmc_project.Common.Progress.OperationStatus.Failed"),
              DumpCreationProgress::DUMP_CREATE_FAILED);

    EXPECT_EQ(
        mapDbusStatusToDumpProgress(
            "xyz.openbmc_project.Common.Progress.OperationStatus.Aborted"),
        DumpCreationProgress::DUMP_CREATE_FAILED);

    EXPECT_EQ(mapDbusStatusToDumpProgress(""),
              DumpCreationProgress::DUMP_CREATE_INPROGRESS);
}

TEST(DumpUtils, GetDumpCompletionStatus)
{
    dbus::utility::DBusPropertiesMap values;

    values.emplace_back(
        "Status",
        dbus::utility::DbusVariantType{std::string{
            "xyz.openbmc_project.Common.Progress.OperationStatus.Completed"}});
    EXPECT_EQ(getDumpCompletionStatus(values),
              DumpCreationProgress::DUMP_CREATE_SUCCESS);

    values[0].second =
        "xyz.openbmc_project.Common.Progress.OperationStatus.Failed";
    EXPECT_EQ(getDumpCompletionStatus(values),
              DumpCreationProgress::DUMP_CREATE_FAILED);

    values.clear();
    EXPECT_EQ(getDumpCompletionStatus(values),
              DumpCreationProgress::DUMP_CREATE_INPROGRESS);
}

TEST(DumpUtils, mapDbusOriginatorTypeToRedfish)
{
    EXPECT_EQ(
        mapDbusOriginatorTypeToRedfish(
            "xyz.openbmc_project.Common.OriginatedBy.OriginatorTypes.Client"),
        log_entry::OriginatorTypes::Client);
    EXPECT_EQ(
        mapDbusOriginatorTypeToRedfish(
            "xyz.openbmc_project.Common.OriginatedBy.OriginatorTypes.Internal"),
        log_entry::OriginatorTypes::Internal);
    EXPECT_EQ(
        mapDbusOriginatorTypeToRedfish(
            "xyz.openbmc_project.Common.OriginatedBy.OriginatorTypes.SupportingService"),
        log_entry::OriginatorTypes::SupportingService);
    EXPECT_EQ(mapDbusOriginatorTypeToRedfish(""),
              log_entry::OriginatorTypes::Invalid);
}
} // namespace redfish::dump_utils
