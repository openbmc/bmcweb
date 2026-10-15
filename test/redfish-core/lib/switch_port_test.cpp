// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "generated/enums/port.hpp"
#include "http_response.hpp"
#include "switch_port.hpp"

#include <boost/beast/http/status.hpp>
#include <boost/system/error_code.hpp>

#include <memory>
#include <string>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

constexpr const char* linkStatusPrefix =
    "xyz.openbmc_project.Inventory.Connector.Port.LinkStatus.";

TEST(DBusSensorPortLinkStatusToRedfish, MapsKnownValues)
{
    EXPECT_EQ(dBusSensorPortLinkStatusToRedfish(
                  std::string(linkStatusPrefix) + "LinkUp"),
              port::LinkStatus::LinkUp);
    EXPECT_EQ(dBusSensorPortLinkStatusToRedfish(
                  std::string(linkStatusPrefix) + "LinkDown"),
              port::LinkStatus::LinkDown);
    EXPECT_EQ(dBusSensorPortLinkStatusToRedfish(
                  std::string(linkStatusPrefix) + "NoLink"),
              port::LinkStatus::NoLink);
    EXPECT_EQ(dBusSensorPortLinkStatusToRedfish(
                  std::string(linkStatusPrefix) + "Starting"),
              port::LinkStatus::Starting);
    EXPECT_EQ(dBusSensorPortLinkStatusToRedfish(
                  std::string(linkStatusPrefix) + "Training"),
              port::LinkStatus::Training);
}

TEST(DBusSensorPortLinkStatusToRedfish, UnmappedValueReturnsInvalid)
{
    EXPECT_EQ(dBusSensorPortLinkStatusToRedfish(
                  std::string(linkStatusPrefix) + "Unknown"),
              port::LinkStatus::Invalid);
    EXPECT_EQ(dBusSensorPortLinkStatusToRedfish("LinkUp"),
              port::LinkStatus::Invalid);
    EXPECT_EQ(dBusSensorPortLinkStatusToRedfish(""), port::LinkStatus::Invalid);
}

TEST(AfterGetFabricSwitchPortInfo, KnownLinkStatusPopulatesLinkStatus)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::DBusPropertiesMap properties;
    properties.emplace_back("LinkStatus",
                            std::string(linkStatusPrefix) + "Training");

    afterGetFabricSwitchPortInfo(response, boost::system::error_code(),
                                 properties);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(response->res.jsonValue["LinkStatus"],
              port::LinkStatus::Training);
}

TEST(AfterGetFabricSwitchPortInfo, UnmappedLinkStatusOmitsLinkStatus)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::DBusPropertiesMap properties;
    properties.emplace_back("LinkStatus",
                            std::string(linkStatusPrefix) + "Unknown");

    afterGetFabricSwitchPortInfo(response, boost::system::error_code(),
                                 properties);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("LinkStatus"));
}

TEST(AfterGetFabricSwitchPortInfo, WrongLinkStatusTypeReportsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::DBusPropertiesMap properties;
    properties.emplace_back("LinkStatus", true);

    afterGetFabricSwitchPortInfo(response, boost::system::error_code(),
                                 properties);

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

} // namespace
} // namespace redfish
