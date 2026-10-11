// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "fabric_ports.hpp"
#include "http_response.hpp"

#include <boost/asio/error.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/system/errc.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/linux_error.hpp>
#include <nlohmann/json.hpp>

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

constexpr const char* portPath =
    "/xyz/openbmc_project/inventory/system/chassis/adapter0/port0";

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
    EXPECT_FALSE(response->res.jsonValue.contains("Location"));
}

TEST(AfterGetFabricPortLocation, SuccessSetsServiceLabel)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterGetFabricPortLocation(response, ec, "U78DA.ND0.1234567-P0-C1");

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(
        response->res.jsonValue["Location"]["PartLocation"]["ServiceLabel"],
        "U78DA.ND0.1234567-P0-C1");
}

TEST(GetFabricPortProperties, EmptyPathReportsPortNotFound)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();

    getFabricPortProperties(response, "system", "adapter0", "port0", "", "");

    EXPECT_EQ(response->res.result(), boost::beast::http::status::not_found);
}

TEST(AfterGetValidFabricPortPath, IoErrorInvokesCallbackWithEmptyPath)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::errc::make_error_code(boost::system::errc::io_error);

    bool called = false;
    std::function<void(const std::string&, const std::string&)> callback =
        [&called](const std::string& path, const std::string& service) {
            called = true;
            EXPECT_TRUE(path.empty());
            EXPECT_TRUE(service.empty());
        };

    afterGetValidFabricPortPath(response, "port0", callback, ec, {});

    EXPECT_TRUE(called);
    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
}

TEST(AfterGetValidFabricPortPath, GenericErrorReportsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;

    bool called = false;
    std::function<void(const std::string&, const std::string&)> callback =
        [&called](const std::string&, const std::string&) { called = true; };

    afterGetValidFabricPortPath(response, "port0", callback, ec, {});

    EXPECT_FALSE(called);
    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetValidFabricPortPath, UnknownPortInvokesCallbackWithEmptyPath)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    bool called = false;
    std::function<void(const std::string&, const std::string&)> callback =
        [&called](const std::string& path, const std::string& service) {
            called = true;
            EXPECT_TRUE(path.empty());
            EXPECT_TRUE(service.empty());
        };

    afterGetValidFabricPortPath(response, "port1", callback, ec, {portPath});

    EXPECT_TRUE(called);
    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
}

TEST(AfterHandleFabricPortCollectionHead, IoErrorReportsAdapterNotFound)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::errc::make_error_code(boost::system::errc::io_error);

    afterHandleFabricPortCollectionHead(response, "adapter0", ec, {});

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

TEST(AfterHandleFabricPortCollectionHead, SuccessAddsLinkHeader)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterHandleFabricPortCollectionHead(response, "adapter0", ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(
        response->res.getHeaderValue(boost::beast::http::field::link),
        "</redfish/v1/JsonSchemas/PortCollection/PortCollection.json>; rel=describedby");
}

TEST(DoHandleFabricPortCollectionGet, IoErrorReportsAdapterNotFound)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::errc::make_error_code(boost::system::errc::io_error);

    doHandleFabricPortCollectionGet(response, "system", "adapter0", ec, {});

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

TEST(DoHandleFabricPortCollectionGet, EmptySubTreeReturnsEmptyCollection)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    doHandleFabricPortCollectionGet(response, "system", "adapter0", ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(response->res.jsonValue["@odata.id"],
              "/redfish/v1/Systems/system/FabricAdapters/adapter0/Ports");
    EXPECT_EQ(response->res.jsonValue["Members"], nlohmann::json::array());
    EXPECT_EQ(response->res.jsonValue["Members@odata.count"], 0);
}

TEST(DoHandleFabricPortCollectionGet, MembersAreSortedAlphanumerically)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    doHandleFabricPortCollectionGet(
        response, "system", "adapter0", ec,
        {"/xyz/openbmc_project/inventory/system/chassis/adapter0/port10",
         "/xyz/openbmc_project/inventory/system/chassis/adapter0/port2",
         "/xyz/openbmc_project/inventory/system/chassis/adapter0/port1"});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(response->res.jsonValue["Members@odata.count"], 3);

    nlohmann::json expected = nlohmann::json::array();
    for (const char* portId : {"port1", "port2", "port10"})
    {
        nlohmann::json item;
        item["@odata.id"] =
            "/redfish/v1/Systems/system/FabricAdapters/adapter0/Ports/" +
            std::string(portId);
        expected.emplace_back(std::move(item));
    }
    EXPECT_EQ(response->res.jsonValue["Members"], expected);
}

TEST(AfterHandlePortPatch, EmptyPathReportsPortNotFound)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();

    afterHandlePortPatch(response, "port0", true, "", "");

    EXPECT_EQ(response->res.result(), boost::beast::http::status::not_found);
}

} // namespace
} // namespace redfish
