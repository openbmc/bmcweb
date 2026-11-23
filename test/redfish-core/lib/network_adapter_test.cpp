// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "http_response.hpp"
#include "network_adapter.hpp"

#include <systemd/sd-bus.h>

#include <boost/asio/error.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/system/errc.hpp>
#include <boost/system/linux_error.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/test/sdbus_mock.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <type_traits>

#include <gmock/gmock.h>
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

TEST(AfterGetNetworkAdapterLocation, IoErrorOmitsLocation)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::errc::make_error_code(boost::system::errc::io_error);

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

TEST(AfterGetNetworkAdapterLocation, UnknownInterfaceOmitsLocation)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperGetObject object = {
        {"xyz.openbmc_project.Service0",
         {"xyz.openbmc_project.Inventory.Item"}}};

    afterGetNetworkAdapterLocation(response, std::string(adapterPath), ec,
                                   object);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(response->res.jsonValue.empty());
}

TEST(AfterGetNetworkAdapterAsset, ResourceNotFoundOmitsAsset)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::errc::make_error_code(boost::system::errc::io_error);
    sdbusplus::SdBusMock sdbusMock;
    sd_bus_error error{
        .name = "xyz.openbmc_project.Common.Error.ResourceNotFound",
        .message = "",
        ._need_free = 0};
    EXPECT_CALL(sdbusMock, sd_bus_message_get_error(testing::_))
        .WillRepeatedly(testing::Return(&error));
    sdbusplus::message_t msg(nullptr, &sdbusMock, std::false_type{});

    afterGetNetworkAdapterAsset(response, std::string(adapterPath), ec, msg,
                                {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(response->res.jsonValue.empty());
}

TEST(AfterGetNetworkAdapterAsset, IoErrorSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::errc::make_error_code(boost::system::errc::io_error);
    sdbusplus::SdBusMock sdbusMock;
    sd_bus_error error{.name = "org.freedesktop.DBus.Error.IOError",
                       .message = "",
                       ._need_free = 0};
    EXPECT_CALL(sdbusMock, sd_bus_message_get_error(testing::_))
        .WillRepeatedly(testing::Return(&error));
    sdbusplus::message_t msg(nullptr, &sdbusMock, std::false_type{});

    afterGetNetworkAdapterAsset(response, std::string(adapterPath), ec, msg,
                                {});

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetNetworkAdapterAsset, UnknownObjectSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::linux_error::bad_request_descriptor;
    sdbusplus::SdBusMock sdbusMock;
    sd_bus_error error{.name = "org.freedesktop.DBus.Error.UnknownObject",
                       .message = "",
                       ._need_free = 0};
    EXPECT_CALL(sdbusMock, sd_bus_message_get_error(testing::_))
        .WillRepeatedly(testing::Return(&error));
    sdbusplus::message_t msg(nullptr, &sdbusMock, std::false_type{});

    afterGetNetworkAdapterAsset(response, std::string(adapterPath), ec, msg,
                                {});

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetNetworkAdapterAsset, ErrorWithoutReplySetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::errc::make_error_code(boost::system::errc::io_error);
    sdbusplus::message_t msg;

    afterGetNetworkAdapterAsset(response, std::string(adapterPath), ec, msg,
                                {});

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetNetworkAdapterAsset, EmptyObjectOmitsAsset)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    sdbusplus::message_t msg;

    afterGetNetworkAdapterAsset(response, std::string(adapterPath), ec, msg,
                                {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(response->res.jsonValue.empty());
}

} // namespace
} // namespace redfish
