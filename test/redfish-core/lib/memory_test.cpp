// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "http_response.hpp"
#include "memory.hpp"

#include <boost/asio/error.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/system/linux_error.hpp>
#include <nlohmann/json.hpp>

#include <memory>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(AfterGetDimmChassisLink, ErrorSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;

    afterGetDimmChassisLink(response, ec, {});

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetDimmChassisLink, EbadrOmitsChassis)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::linux_error::bad_request_descriptor;

    afterGetDimmChassisLink(response, ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("Links"));
}

TEST(AfterGetDimmChassisLink, EmptyPathsOmitsChassis)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterGetDimmChassisLink(response, ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("Links"));
}

TEST(AfterGetDimmChassisLink, MultipleChassisOmitChassis)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperGetSubTreePathsResponse chassisPaths = {
        "/xyz/openbmc_project/inventory/system/board/Chassis_0",
        "/xyz/openbmc_project/inventory/system/board/Chassis_1"};

    afterGetDimmChassisLink(response, ec, chassisPaths);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("Links"));
}

TEST(AfterGetDimmChassisLink, MalformedPathSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperGetSubTreePathsResponse chassisPaths = {"/"};

    afterGetDimmChassisLink(response, ec, chassisPaths);

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetDimmChassisLink, SuccessSetsChassisLink)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperGetSubTreePathsResponse chassisPaths = {
        "/xyz/openbmc_project/inventory/system/board/Chassis_0"};

    afterGetDimmChassisLink(response, ec, chassisPaths);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(response->res.jsonValue["Links"]["Chassis"]["@odata.id"],
              "/redfish/v1/Chassis/Chassis_0");
}

TEST(AfterGetDimmProcessorLinks, ErrorSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;

    afterGetDimmProcessorLinks(response, ec, {});

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetDimmProcessorLinks, EbadrOmitsProcessors)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::linux_error::bad_request_descriptor;

    afterGetDimmProcessorLinks(response, ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("Links"));
}

TEST(AfterGetDimmProcessorLinks, EmptyPathsOmitsProcessors)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterGetDimmProcessorLinks(response, ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("Links"));
}

TEST(AfterGetDimmProcessorLinks, MultipleProcessorsSetProcessorLinks)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperGetSubTreePathsResponse processorPaths = {
        "/xyz/openbmc_project/inventory/GPU_0",
        "/xyz/openbmc_project/inventory/GPU_1"};

    afterGetDimmProcessorLinks(response, ec, processorPaths);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(response->res.jsonValue["Links"]["Processors@odata.count"], 2);
    EXPECT_EQ(response->res.jsonValue["Links"]["Processors"][0]["@odata.id"],
              "/redfish/v1/Systems/system/Processors/GPU_0");
    EXPECT_EQ(response->res.jsonValue["Links"]["Processors"][1]["@odata.id"],
              "/redfish/v1/Systems/system/Processors/GPU_1");
    EXPECT_FALSE(response->res.jsonValue["Links"].contains("Chassis"));
}

} // namespace
} // namespace redfish
