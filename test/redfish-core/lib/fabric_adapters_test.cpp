// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "fabric_adapters.hpp"
#include "http_response.hpp"

#include <boost/asio/error.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/system/errc.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/linux_error.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

constexpr const char* adapterPath =
    "/xyz/openbmc_project/inventory/system/chassis/adapter0";
constexpr const char* adapterService = "xyz.openbmc_project.FabricAdapter";
constexpr const char* adapterInterface =
    "xyz.openbmc_project.Inventory.Item.FabricAdapter";
constexpr const char* adapterId = "adapter0";
constexpr const char* systemName = "system";

boost::system::error_code ioError()
{
    return boost::system::errc::make_error_code(boost::system::errc::io_error);
}

TEST(AfterGetFabricAdapterLocation, EbadrOmitsLocation)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::linux_error::bad_request_descriptor;

    afterGetFabricAdapterLocation(response, ec, "");

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("Location"));
}

TEST(AfterGetFabricAdapterLocation, ErrorSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;

    afterGetFabricAdapterLocation(response, ec, "");

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetFabricAdapterLocation, SuccessSetsServiceLabel)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterGetFabricAdapterLocation(response, ec, "U78DA.ND0.WZS0036-P0-C1");

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(
        response->res.jsonValue["Location"]["PartLocation"]["ServiceLabel"],
        "U78DA.ND0.WZS0036-P0-C1");
}

TEST(AfterGetValidFabricAdapterPath, ErrorForwardsEmptyPath)
{
    bool called = false;
    std::string resultPath = "unset";
    std::string resultService = "unset";
    std::function<void(const boost::system::error_code&, const std::string&,
                       const std::string&)>
        callback = [&](const boost::system::error_code& /*ec*/,
                       const std::string& path, const std::string& service) {
            called = true;
            resultPath = path;
            resultService = service;
        };

    boost::system::error_code ec = boost::asio::error::invalid_argument;

    afterGetValidFabricAdapterPath(adapterId, callback, ec, {});

    EXPECT_TRUE(called);
    EXPECT_TRUE(resultPath.empty());
    EXPECT_TRUE(resultService.empty());
}

TEST(AfterGetValidFabricAdapterPath, UnmatchedAdapterForwardsEmptyPath)
{
    bool called = false;
    std::string resultPath = "unset";
    std::string resultService = "unset";
    std::function<void(const boost::system::error_code&, const std::string&,
                       const std::string&)>
        callback = [&](const boost::system::error_code& /*ec*/,
                       const std::string& path, const std::string& service) {
            called = true;
            resultPath = path;
            resultService = service;
        };

    dbus::utility::MapperGetSubTreeResponse subtree = {
        {adapterPath, {{adapterService, {adapterInterface}}}}};
    boost::system::error_code ec;

    afterGetValidFabricAdapterPath("adapter1", callback, ec, subtree);

    EXPECT_TRUE(called);
    EXPECT_TRUE(resultPath.empty());
    EXPECT_TRUE(resultService.empty());
}

TEST(AfterGetValidFabricAdapterPath, MatchedAdapterForwardsPathAndService)
{
    bool called = false;
    std::string resultPath;
    std::string resultService;
    std::function<void(const boost::system::error_code&, const std::string&,
                       const std::string&)>
        callback = [&](const boost::system::error_code& /*ec*/,
                       const std::string& path, const std::string& service) {
            called = true;
            resultPath = path;
            resultService = service;
        };

    dbus::utility::MapperGetSubTreeResponse subtree = {
        {"/xyz/openbmc_project/inventory/system/chassis/adapter1",
         {{adapterService, {adapterInterface}}}},
        {adapterPath, {{adapterService, {adapterInterface}}}}};
    boost::system::error_code ec;

    afterGetValidFabricAdapterPath(adapterId, callback, ec, subtree);

    EXPECT_TRUE(called);
    EXPECT_EQ(resultPath, adapterPath);
    EXPECT_EQ(resultService, adapterService);
}

TEST(AfterHandleFabricAdapterGet, IoErrorReportsResourceNotFound)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();

    afterHandleFabricAdapterGet(response, systemName, adapterId, ioError(), "",
                                "");

    EXPECT_EQ(response->res.result(), boost::beast::http::status::not_found);
}

TEST(AfterHandleFabricAdapterGet, GenericErrorSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;

    afterHandleFabricAdapterGet(response, systemName, adapterId, ec, "", "");

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterHandleFabricAdapterGet, EmptyPathReportsResourceNotFound)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterHandleFabricAdapterGet(response, systemName, adapterId, ec, "",
                                adapterService);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::not_found);
}

TEST(AfterHandleFabricAdapterGet, EmptyServiceReportsResourceNotFound)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterHandleFabricAdapterGet(response, systemName, adapterId, ec,
                                adapterPath, "");

    EXPECT_EQ(response->res.result(), boost::beast::http::status::not_found);
}

TEST(AfterHandleFabricAdapterPatch, IoErrorReportsResourceNotFound)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();

    afterHandleFabricAdapterPatch(response, adapterId, std::nullopt, ioError(),
                                  "", "");

    EXPECT_EQ(response->res.result(), boost::beast::http::status::not_found);
}

TEST(AfterHandleFabricAdapterPatch, GenericErrorSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;

    afterHandleFabricAdapterPatch(response, adapterId, std::nullopt, ec, "",
                                  "");

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterHandleFabricAdapterPatch, EmptyPathReportsResourceNotFound)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterHandleFabricAdapterPatch(response, adapterId, std::nullopt, ec, "",
                                  adapterService);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::not_found);
}

TEST(AfterHandleFabricAdapterPatch, EmptyServiceReportsResourceNotFound)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterHandleFabricAdapterPatch(response, adapterId, std::nullopt, ec,
                                  adapterPath, "");

    EXPECT_EQ(response->res.result(), boost::beast::http::status::not_found);
}

TEST(AfterHandleFabricAdapterPatch, AbsentPropertyLeavesResponseUnchanged)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterHandleFabricAdapterPatch(response, adapterId, std::nullopt, ec,
                                  adapterPath, adapterService);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(response->res.jsonValue.empty());
}

TEST(AfterHandleFabricAdapterHead, IoErrorReportsResourceNotFound)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();

    afterHandleFabricAdapterHead(response, adapterId, ioError(), "", "");

    EXPECT_EQ(response->res.result(), boost::beast::http::status::not_found);
    EXPECT_EQ(response->res.getHeaderValue("Link"), "");
}

TEST(AfterHandleFabricAdapterHead, GenericErrorSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;

    afterHandleFabricAdapterHead(response, adapterId, ec, "", "");

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_EQ(response->res.getHeaderValue("Link"), "");
}

TEST(AfterHandleFabricAdapterHead, EmptyPathReportsResourceNotFound)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterHandleFabricAdapterHead(response, adapterId, ec, "", adapterService);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::not_found);
    EXPECT_EQ(response->res.getHeaderValue("Link"), "");
}

TEST(AfterHandleFabricAdapterHead, EmptyServiceReportsResourceNotFound)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterHandleFabricAdapterHead(response, adapterId, ec, adapterPath, "");

    EXPECT_EQ(response->res.result(), boost::beast::http::status::not_found);
    EXPECT_EQ(response->res.getHeaderValue("Link"), "");
}

TEST(AfterHandleFabricAdapterHead, SuccessAddsSchemaLinkHeader)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterHandleFabricAdapterHead(response, adapterId, ec, adapterPath,
                                 adapterService);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(
        response->res.getHeaderValue("Link"),
        "</redfish/v1/JsonSchemas/FabricAdapter/FabricAdapter.json>; rel=describedby");
}

} // namespace
} // namespace redfish
