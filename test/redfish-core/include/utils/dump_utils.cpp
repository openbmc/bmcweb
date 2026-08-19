// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "utils/dump_utils.hpp"

#include "bmcweb_config.h"

#include "async_resp.hpp"

#include <boost/beast/http/status.hpp>
#include <boost/url/format.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <memory>
#include <optional>

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
    EXPECT_EQ(dumpTypeToObjPath(static_cast<DumpType>(99)),
              std::nullopt); // NOLINT(clang-analyzer-core.EnumCastOutOfRange)
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
    EXPECT_EQ(getDumpEntriesUrl(static_cast<DumpType>(99)),
              std::nullopt); // NOLINT(clang-analyzer-core.EnumCastOutOfRange)
}
} // namespace redfish::dump_utils
