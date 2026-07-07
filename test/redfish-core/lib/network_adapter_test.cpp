// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "http_response.hpp"
#include "network_adapter.hpp"

#include <boost/asio/error.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/system/linux_error.hpp>
#include <nlohmann/json.hpp>

#include <memory>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

constexpr std::string_view adapterPath =
    "/xyz/openbmc_project/inventory/system/chassis0/adapter0";

TEST(AfterGetNetworkAdapterLocation, EbadrOmitsLocation)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::linux_error::bad_request_descriptor;

    afterGetNetworkAdapterLocation(response, std::string(adapterPath), ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(response->res.jsonValue.empty());
}

TEST(AfterGetNetworkAdapterLocation, ErrorSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;

    afterGetNetworkAdapterLocation(response, std::string(adapterPath), ec, {});

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetNetworkAdapterLocation, EmptyObjectOmitsLocation)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterGetNetworkAdapterLocation(response, std::string(adapterPath), ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(response->res.jsonValue.empty());
}

TEST(AfterGetNetworkAdapterLocation, EmbeddedSetsLocationType)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperGetObject object = {
        {"xyz.openbmc_project.Service0",
         {"xyz.openbmc_project.Inventory.Connector.Embedded"}}};

    afterGetNetworkAdapterLocation(response, std::string(adapterPath), ec,
                                   object);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(
        response->res.jsonValue["Location"]["PartLocation"]["LocationType"],
        "Embedded");
}

TEST(AfterGetNetworkAdapterLocation, SlotSetsLocationType)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperGetObject object = {
        {"xyz.openbmc_project.Service0",
         {"xyz.openbmc_project.Inventory.Connector.Slot"}}};

    afterGetNetworkAdapterLocation(response, std::string(adapterPath), ec,
                                   object);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(
        response->res.jsonValue["Location"]["PartLocation"]["LocationType"],
        "Slot");
}

} // namespace
} // namespace redfish
