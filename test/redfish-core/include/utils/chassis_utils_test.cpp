// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "http_response.hpp"
#include "utils/chassis_utils.hpp"

#include <boost/beast/http/status.hpp>
#include <boost/system/errc.hpp>

#include <memory>
#include <optional>
#include <string>

#include <gtest/gtest.h>

namespace redfish::chassis_utils
{
namespace
{

TEST(ChassisUtils, AfterGetValidChassisPathMatchFound)
{
    std::shared_ptr<bmcweb::AsyncResp> asyncResp =
        std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperGetSubTreePathsResponse chassisPaths = {
        "/xyz/openbmc_project/inventory/system/chassis"};

    std::optional<std::string> result;
    auto callback = [&result](const std::optional<std::string>& path) {
        result = path;
    };
    afterGetValidChassisPath(asyncResp, "chassis", callback, ec, chassisPaths);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "/xyz/openbmc_project/inventory/system/chassis");
}

TEST(ChassisUtils, AfterGetValidChassisPathNoMatch)
{
    std::shared_ptr<bmcweb::AsyncResp> asyncResp =
        std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperGetSubTreePathsResponse chassisPaths = {
        "/xyz/openbmc_project/inventory/system/other"};

    std::optional<std::string> result = std::string("unset");
    auto callback = [&result](const std::optional<std::string>& path) {
        result = path;
    };
    afterGetValidChassisPath(asyncResp, "chassis", callback, ec, chassisPaths);

    EXPECT_FALSE(result.has_value());
}

TEST(ChassisUtils, AfterGetValidChassisPathDbusErrorSkipsCallback)
{
    std::shared_ptr<bmcweb::AsyncResp> asyncResp =
        std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::errc::make_error_code(boost::system::errc::timed_out);
    dbus::utility::MapperGetSubTreePathsResponse chassisPaths;

    bool callbackInvoked = false;
    auto callback = [&callbackInvoked](const std::optional<std::string>&) {
        callbackInvoked = true;
    };
    afterGetValidChassisPath(asyncResp, "chassis", callback, ec, chassisPaths);

    EXPECT_FALSE(callbackInvoked);
    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(ChassisUtils, AfterGetValidChassisPathMultiplePathsPicksMatch)
{
    std::shared_ptr<bmcweb::AsyncResp> asyncResp =
        std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperGetSubTreePathsResponse chassisPaths = {
        "/xyz/openbmc_project/inventory/system/board0",
        "/xyz/openbmc_project/inventory/system/chassis",
        "/xyz/openbmc_project/inventory/system/board1"};

    std::optional<std::string> result;
    auto callback = [&result](const std::optional<std::string>& path) {
        result = path;
    };
    afterGetValidChassisPath(asyncResp, "chassis", callback, ec, chassisPaths);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "/xyz/openbmc_project/inventory/system/chassis");
}

} // namespace
} // namespace redfish::chassis_utils
