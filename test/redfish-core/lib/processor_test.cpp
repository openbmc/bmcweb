// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "http_response.hpp"
#include "processor.hpp"

#include <boost/asio/error.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/system/linux_error.hpp>
#include <nlohmann/json.hpp>

#include <memory>
#include <string>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(AfterGetProcessorLocationCode, ErrorSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;

    afterGetProcessorLocationCode(response, ec, "");

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetProcessorLocationCode, SuccessSetsServiceLabel)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterGetProcessorLocationCode(response, ec, "U3-P0");

    EXPECT_EQ(
        response->res.jsonValue["Location"]["PartLocation"]["ServiceLabel"],
        "U3-P0");
}

TEST(AfterGetProcessorFirmwareVersion, EbadrOmitsFirmwareVersion)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::linux_error::bad_request_descriptor;

    afterGetProcessorFirmwareVersion(response, ec, "");

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("FirmwareVersion"));
}

TEST(AfterGetProcessorFirmwareVersion, ErrorSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;

    afterGetProcessorFirmwareVersion(response, ec, "");

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetProcessorFirmwareVersion, EmptyVersionOmitsFirmwareVersion)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterGetProcessorFirmwareVersion(response, ec, "");

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("FirmwareVersion"));
}

TEST(AfterGetProcessorFirmwareVersion, SuccessSetsFirmwareVersion)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterGetProcessorFirmwareVersion(response, ec, "1.2.3");

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(response->res.jsonValue["FirmwareVersion"], "1.2.3");
}

TEST(AfterGetProcessorFirmwareVersionSubTree, EbadrOmitsFirmwareVersion)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::linux_error::bad_request_descriptor;

    afterGetProcessorFirmwareVersionSubTree(response, ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("FirmwareVersion"));
}

TEST(AfterGetProcessorFirmwareVersionSubTree, ErrorSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;

    afterGetProcessorFirmwareVersionSubTree(response, ec, {});

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetProcessorFirmwareVersionSubTree, EmptySubTreeOmitsFirmwareVersion)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterGetProcessorFirmwareVersionSubTree(response, ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("FirmwareVersion"));
}

TEST(AfterGetProcessorFirmwareVersionSubTree,
     MultipleSoftwareObjectsSetInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperGetSubTreeResponse subtree = {
        {"/xyz/openbmc_project/software/fw0",
         {{"xyz.openbmc_project.Service0",
           {"xyz.openbmc_project.Software.Version"}}}},
        {"/xyz/openbmc_project/software/fw1",
         {{"xyz.openbmc_project.Service0",
           {"xyz.openbmc_project.Software.Version"}}}}};

    afterGetProcessorFirmwareVersionSubTree(response, ec, subtree);

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetProcessorFirmwareVersionSubTree, EmptyServiceMapSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperGetSubTreeResponse subtree = {
        {"/xyz/openbmc_project/software/fw0", {}}};

    afterGetProcessorFirmwareVersionSubTree(response, ec, subtree);

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

} // namespace
} // namespace redfish
