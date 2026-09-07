// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "utils/dump_utils.hpp"

#include "bmcweb_config.h"

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "generated/enums/log_entry.hpp"

#include <boost/beast/http/status.hpp>
#include <boost/url/format.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include <gtest/gtest.h>

namespace redfish::dump_utils
{
TEST(DumpUtils, GetDumpServiceInfo)
{
    auto shareAsyncResp = std::make_shared<bmcweb::AsyncResp>();
    dump_utils::getDumpServiceInfo(shareAsyncResp, static_cast<DumpType>(99));
    EXPECT_EQ(shareAsyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(DumpUtils, DumpTypeToStr)
{
    EXPECT_EQ(dumpTypeToStr(DumpType::BMC), "BMC");
    EXPECT_EQ(dumpTypeToStr(DumpType::System), "System");
    EXPECT_EQ(dumpTypeToStr(DumpType::FaultLog), "FaultLog");
    EXPECT_EQ(dumpTypeToStr(static_cast<DumpType>(99)), std::nullopt);
}

TEST(DumpUtils, DumpTypeToObjPath)
{
    EXPECT_EQ(dumpTypeToObjPath(DumpType::BMC),
              sdbusplus::object_path{"/xyz/openbmc_project/dump/bmc"});
    EXPECT_EQ(dumpTypeToObjPath(DumpType::System),
              sdbusplus::object_path{"/xyz/openbmc_project/dump/system"});
    EXPECT_EQ(dumpTypeToObjPath(DumpType::FaultLog),
              sdbusplus::object_path{"/xyz/openbmc_project/dump/faultlog"});
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
    EXPECT_EQ(dumpTypeToObjPath(static_cast<DumpType>(99)), std::nullopt);
}

TEST(DumpUtils, GetDumpEntriesUrl)
{
    EXPECT_EQ(
        getDumpEntriesUrl(DumpType::BMC),
        boost::urls::format("/redfish/v1/Managers/{}/LogServices/Dump/Entries",
                            BMCWEB_REDFISH_MANAGER_URI_NAME));
    EXPECT_EQ(getDumpEntriesUrl(DumpType::FaultLog),
              boost::urls::format(
                  "/redfish/v1/Managers/{}/LogServices/FaultLog/Entries",
                  BMCWEB_REDFISH_MANAGER_URI_NAME));
    EXPECT_EQ(
        getDumpEntriesUrl(DumpType::System),
        boost::urls::format("/redfish/v1/Systems/{}/LogServices/Dump/Entries",
                            BMCWEB_REDFISH_SYSTEM_URI_NAME));
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
    EXPECT_EQ(getDumpEntriesUrl(static_cast<DumpType>(99)), std::nullopt);
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
        DumpCreationProgress::DUMP_CREATE_ABORTED);

    EXPECT_EQ(
        mapDbusStatusToDumpProgress(
            "xyz.openbmc_project.Common.Progress.OperationStatus.NotStarted"),
        DumpCreationProgress::DUMP_CREATE_NOTSTARTED);

    EXPECT_EQ(
        mapDbusStatusToDumpProgress(
            "xyz.openbmc_project.Common.Progress.OperationStatus.InProgress"),
        DumpCreationProgress::DUMP_CREATE_INPROGRESS);

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
        "xyz.openbmc_project.Common.Progress.OperationStatus.NotStarted";
    EXPECT_EQ(getDumpCompletionStatus(values),
              DumpCreationProgress::DUMP_CREATE_NOTSTARTED);

    values[0].second =
        "xyz.openbmc_project.Common.Progress.OperationStatus.Failed";
    EXPECT_EQ(getDumpCompletionStatus(values),
              DumpCreationProgress::DUMP_CREATE_FAILED);

    values[0].second =
        "xyz.openbmc_project.Common.Progress.OperationStatus.Aborted";
    EXPECT_EQ(getDumpCompletionStatus(values),
              DumpCreationProgress::DUMP_CREATE_ABORTED);

    values[0].second =
        "xyz.openbmc_project.Common.Progress.OperationStatus.InProgress";
    EXPECT_EQ(getDumpCompletionStatus(values),
              DumpCreationProgress::DUMP_CREATE_INPROGRESS);

    values[0].second = dbus::utility::DbusVariantType{uint64_t{1}};
    EXPECT_EQ(getDumpCompletionStatus(values),
              DumpCreationProgress::DUMP_CREATE_FAILED);

    values.clear();
    EXPECT_EQ(getDumpCompletionStatus(values),
              DumpCreationProgress::DUMP_CREATE_INPROGRESS);
}

TEST(DumpUtils, MapDbusOriginatorTypeToRedfish)
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
