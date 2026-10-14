// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "http_response.hpp"
#include "snmp_trap_event_clients.hpp"

#include <asm-generic/errno.h>

#include <boost/asio/error.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/system/error_code.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <cstdint>
#include <memory>
#include <string>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(AfterGetSnmpTrapClientdata, ErrorCodeReturnsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::host_unreachable;

    afterGetSnmpTrapClientdata(asyncResp, ec, {});

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetSnmpTrapClientdata, MissingPropertiesReturnsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("Address", std::string("192.168.1.1"));

    afterGetSnmpTrapClientdata(asyncResp, {}, props);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetSnmpTrapClientdata, DestinationIsPopulated)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("Address", std::string("192.168.1.1"));
    props.emplace_back("Port", uint16_t{162});

    afterGetSnmpTrapClientdata(asyncResp, {}, props);

    EXPECT_EQ(asyncResp->res.jsonValue["Destination"],
              "snmp://192.168.1.1:162");
}

TEST(AfterGetSnmpTrapClientdata, HostnameDestinationIsPopulated)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("Address", std::string("example.com"));
    props.emplace_back("Port", uint16_t{162});

    afterGetSnmpTrapClientdata(asyncResp, {}, props);

    EXPECT_EQ(asyncResp->res.jsonValue["Destination"],
              "snmp://example.com:162");
}

TEST(AfterGetSnmpTrapClient, ErrorCodeReturnsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::host_unreachable;

    afterGetSnmpTrapClient(asyncResp, "snmp1", ec, {});

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetSnmpTrapClient, EmptyClientIdReturnsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    dbus::utility::ManagedObjectType managed;
    managed.emplace_back(sdbusplus::object_path("/"),
                         dbus::utility::DBusInterfacesMap{});

    afterGetSnmpTrapClient(asyncResp, "snmp1", {}, managed);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetSnmpTrapClient, UnknownIdReturnsResourceNotFound)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    dbus::utility::ManagedObjectType managed;
    managed.emplace_back(
        sdbusplus::object_path("/xyz/openbmc_project/network/snmp/manager/1"),
        dbus::utility::DBusInterfacesMap{});

    afterGetSnmpTrapClient(asyncResp, "snmp99", {}, managed);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::not_found);

    const nlohmann::json& info =
        asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"][0];
    EXPECT_EQ(info["MessageId"], "Base.1.19.ResourceNotFound");
    EXPECT_EQ(info["MessageArgs"][0], "EventDestination");
    EXPECT_EQ(info["MessageArgs"][1], "snmp99");
}

TEST(AfterSnmpClientCreate, SuccessSetsLocationHeader)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    sdbusplus::message_t msg;

    afterSnmpClientCreate(asyncResp, {}, msg, "192.168.1.1",
                          "/xyz/openbmc_project/network/snmp/manager/1");

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::created);
    EXPECT_EQ(asyncResp->res.getHeaderValue("Location"),
              "/redfish/v1/EventService/Subscriptions/snmp1");
}

TEST(AfterSnmpClientCreate, EmptyClientIdReturnsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    sdbusplus::message_t msg;

    afterSnmpClientCreate(asyncResp, {}, msg, "192.168.1.1", "/");

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterSnmpClientCreate, ErrorCodeWithoutDbusErrorReturnsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::host_unreachable;
    sdbusplus::message_t msg;

    afterSnmpClientCreate(asyncResp, ec, msg, "192.168.1.1",
                          "/xyz/openbmc_project/network/snmp/manager/1");

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_EQ(asyncResp->res.getHeaderValue("Location"), "");
}

TEST(GetSnmpSubscriptionList, MemberIsAppended)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    nlohmann::json memberArray = nlohmann::json::array();

    getSnmpSubscriptionList(asyncResp, "1", memberArray);
    getSnmpSubscriptionList(asyncResp, "2", memberArray);

    EXPECT_EQ(memberArray, R"([
        {"@odata.id": "/redfish/v1/EventService/Subscriptions/snmp1"},
        {"@odata.id": "/redfish/v1/EventService/Subscriptions/snmp2"}
    ])"_json);
    EXPECT_EQ(asyncResp->res.jsonValue["Members@odata.count"], 2);
}

TEST(HandleDeleteSnmpTrapClientResponse, BadRequestDescriptorIsNotFound)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec(EBADR, boost::system::generic_category());

    handleDeleteSnmpTrapClientResponse(asyncResp, "snmp1", ec);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::not_found);

    const nlohmann::json& info =
        asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"][0];
    EXPECT_EQ(info["MessageId"], "Base.1.19.ResourceNotFound");
    EXPECT_EQ(info["MessageArgs"][0], "EventDestination");
    EXPECT_EQ(info["MessageArgs"][1], "snmp1");
}

TEST(HandleDeleteSnmpTrapClientResponse, OtherErrorCodeReturnsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::host_unreachable;

    handleDeleteSnmpTrapClientResponse(asyncResp, "snmp1", ec);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(HandleDeleteSnmpTrapClientResponse, SuccessReturnsOk)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    handleDeleteSnmpTrapClientResponse(asyncResp, "snmp1", {});

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(asyncResp->res.jsonValue["@Message.ExtendedInfo"][0]["MessageId"],
              "Base.1.19.Success");
}

} // namespace
} // namespace redfish
