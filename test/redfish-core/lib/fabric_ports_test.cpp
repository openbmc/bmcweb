// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "fabric_ports.hpp"
#include "http_response.hpp"

#include <boost/asio/error.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/system/errc.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/linux_error.hpp>
#include <nlohmann/json.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

constexpr const char* testPortPath =
    "/xyz/openbmc_project/inventory/system/board/adapter0/port0";

boost::system::error_code ioError()
{
    return boost::system::errc::make_error_code(boost::system::errc::io_error);
}

TEST(AfterGetFabricPortLocation, AbsentPropertyOmitsLocation)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::linux_error::bad_request_descriptor;

    afterGetFabricPortLocation(response, ec, "");

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("Location"));
}

TEST(AfterGetFabricPortLocation, GenericErrorReportsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;

    afterGetFabricPortLocation(response, ec, "");

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetFabricPortLocation, SuccessSetsServiceLabel)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterGetFabricPortLocation(response, ec, "U78DA.ND0-P1-C1-T1");

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    const nlohmann::json& location = response->res.jsonValue["Location"];
    EXPECT_EQ(location["PartLocation"]["ServiceLabel"], "U78DA.ND0-P1-C1-T1");
}

TEST(AfterHandleFabricPortCollectionHead, IoErrorReportsNotFound)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();

    afterHandleFabricPortCollectionHead(response, "adapter0", ioError(), {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::not_found);
}

TEST(AfterHandleFabricPortCollectionHead, GenericErrorReportsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;

    afterHandleFabricPortCollectionHead(response, "adapter0", ec, {});

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterHandleFabricPortCollectionHead, SuccessAddsLinkHeaderOnly)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterHandleFabricPortCollectionHead(response, "adapter0", ec,
                                        {testPortPath});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(
        response->res.getHeaderValue("Link"),
        "</redfish/v1/JsonSchemas/PortCollection/PortCollection.json>; rel=describedby");
    EXPECT_TRUE(response->res.jsonValue.empty());
}

TEST(DoHandleFabricPortCollectionGet, IoErrorReportsNotFound)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();

    doHandleFabricPortCollectionGet(response, "system", "adapter0", ioError(),
                                    {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::not_found);
}

TEST(DoHandleFabricPortCollectionGet, GenericErrorReportsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;

    doHandleFabricPortCollectionGet(response, "system", "adapter0", ec, {});

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(DoHandleFabricPortCollectionGet, NoPortsReturnsEmptyCollection)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    doHandleFabricPortCollectionGet(response, "system", "adapter0", ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(response->res.jsonValue["@odata.type"],
              "#PortCollection.PortCollection");
    EXPECT_EQ(response->res.jsonValue["@odata.id"],
              "/redfish/v1/Systems/system/FabricAdapters/adapter0/Ports");
    EXPECT_EQ(response->res.jsonValue["Name"], "Port Collection");
    EXPECT_TRUE(response->res.jsonValue["Members"].empty());
    EXPECT_EQ(response->res.jsonValue["Members@odata.count"], 0);
}

TEST(DoHandleFabricPortCollectionGet, MembersAreAlphanumericallySorted)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperGetSubTreePathsResponse paths{
        "/xyz/openbmc_project/inventory/system/board/adapter0/port10",
        "/xyz/openbmc_project/inventory/system/board/adapter0/port2",
        "/xyz/openbmc_project/inventory/system/board/adapter0/port1"};

    doHandleFabricPortCollectionGet(response, "system", "adapter0", ec, paths);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(response->res.jsonValue["Members@odata.count"], 3);
    nlohmann::json expected = nlohmann::json::array();
    expected.push_back(
        {{"@odata.id",
          "/redfish/v1/Systems/system/FabricAdapters/adapter0/Ports/port1"}});
    expected.push_back(
        {{"@odata.id",
          "/redfish/v1/Systems/system/FabricAdapters/adapter0/Ports/port2"}});
    expected.push_back(
        {{"@odata.id",
          "/redfish/v1/Systems/system/FabricAdapters/adapter0/Ports/port10"}});
    EXPECT_EQ(response->res.jsonValue["Members"], expected);
}

TEST(AfterGetValidFabricPortPath, GenericErrorDoesNotInvokeCallback)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;
    bool called = false;
    std::function<void(const std::string&, const std::string&)> callback =
        [&called](const std::string&, const std::string&) { called = true; };

    afterGetValidFabricPortPath(response, "port0", callback, ec, {});

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_FALSE(called);
}

TEST(AfterGetValidFabricPortPath, IoErrorInvokesCallbackWithEmptyPath)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    std::optional<std::string> path;
    std::function<void(const std::string&, const std::string&)> callback =
        [&path](const std::string& portPath, const std::string&) {
            path = portPath;
        };

    afterGetValidFabricPortPath(response, "port0", callback, ioError(), {});

    ASSERT_TRUE(path.has_value());
    EXPECT_TRUE(path->empty());
}

TEST(AfterGetValidFabricPortPath, UnknownPortInvokesCallbackWithEmptyPath)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    std::optional<std::string> path;
    std::function<void(const std::string&, const std::string&)> callback =
        [&path](const std::string& portPath, const std::string&) {
            path = portPath;
        };

    afterGetValidFabricPortPath(response, "port1", callback, ec,
                                {testPortPath});

    ASSERT_TRUE(path.has_value());
    EXPECT_TRUE(path->empty());
}

TEST(AfterHandlePortPatch, UnknownPortReportsNotFound)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();

    afterHandlePortPatch(response, "port0", true, "", "");

    EXPECT_EQ(response->res.result(), boost::beast::http::status::not_found);
}

} // namespace
} // namespace redfish
