// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "generated/enums/port.hpp"
#include "generated/enums/protocol.hpp"
#include "http_response.hpp"
#include "switch_port.hpp"

#include <boost/beast/http/status.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

const std::string portIface = "xyz.openbmc_project.Inventory.Connector.Port.";

TEST(DBusSensorPortTypeToRedfish, MapsDbusValues)
{
    EXPECT_EQ(dBusSensorPortTypeToRedfish(portIface + "PortType.Downstream"),
              port::PortType::DownstreamPort);
    EXPECT_EQ(dBusSensorPortTypeToRedfish(portIface + "PortType.Upstream"),
              port::PortType::UpstreamPort);
    EXPECT_EQ(dBusSensorPortTypeToRedfish(portIface + "PortType.Bidirectional"),
              port::PortType::BidirectionalPort);
    EXPECT_EQ(dBusSensorPortTypeToRedfish(portIface + "PortType.Unknown"),
              port::PortType::Invalid);
}

TEST(DBusSensorPortLinkStatusToRedfish, MapsDbusValues)
{
    EXPECT_EQ(
        dBusSensorPortLinkStatusToRedfish(portIface + "LinkStatus.LinkUp"),
        port::LinkStatus::LinkUp);
    EXPECT_EQ(
        dBusSensorPortLinkStatusToRedfish(portIface + "LinkStatus.LinkDown"),
        port::LinkStatus::LinkDown);
    EXPECT_EQ(
        dBusSensorPortLinkStatusToRedfish(portIface + "LinkStatus.NoLink"),
        port::LinkStatus::NoLink);
    EXPECT_EQ(
        dBusSensorPortLinkStatusToRedfish(portIface + "LinkStatus.Starting"),
        port::LinkStatus::Starting);
    EXPECT_EQ(
        dBusSensorPortLinkStatusToRedfish(portIface + "LinkStatus.Training"),
        port::LinkStatus::Training);
    EXPECT_EQ(
        dBusSensorPortLinkStatusToRedfish(portIface + "LinkStatus.Unknown"),
        port::LinkStatus::Invalid);
    EXPECT_EQ(dBusSensorPortLinkStatusToRedfish("LinkUp"),
              port::LinkStatus::Invalid);
    EXPECT_EQ(dBusSensorPortLinkStatusToRedfish(""), port::LinkStatus::Invalid);
}

TEST(DBusSensorPortProtocolToRedfish, MapsDbusValues)
{
    EXPECT_EQ(dBusSensorPortProtocolToRedfish(portIface + "PortProtocol.PCIe"),
              protocol::Protocol::PCIe);
    EXPECT_EQ(
        dBusSensorPortProtocolToRedfish(portIface + "PortProtocol.NVLink"),
        protocol::Protocol::NVLink);
    EXPECT_EQ(
        dBusSensorPortProtocolToRedfish(portIface + "PortProtocol.Unknown"),
        protocol::Protocol::Invalid);
}

TEST(AfterGetFabricSwitchPortInfo, PopulatesPortProperties)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    const dbus::utility::DBusPropertiesMap properties = {
        {"Speed", uint64_t{32'000'000'000}},
        {"Width", size_t{16}},
        {"PortType", portIface + "PortType.Downstream"},
        {"PortProtocol", portIface + "PortProtocol.PCIe"},
        {"LinkStatus", portIface + "LinkStatus.LinkUp"},
    };

    afterGetFabricSwitchPortInfo(asyncResp, {}, properties);

    const nlohmann::json& json = asyncResp->res.jsonValue;
    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(json["CurrentSpeedGbps"], 32.0);
    EXPECT_EQ(json["ActiveWidth"], 16);
    EXPECT_EQ(json["PortType"], "DownstreamPort");
    EXPECT_EQ(json["PortProtocol"], "PCIe");
    EXPECT_EQ(json["LinkStatus"], "LinkUp");
}

TEST(AfterGetFabricSwitchPortInfo, OmitsUnknownValues)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    const dbus::utility::DBusPropertiesMap properties = {
        {"PortType", portIface + "PortType.Unknown"},
        {"PortProtocol", portIface + "PortProtocol.Unknown"},
        {"LinkStatus", portIface + "LinkStatus.Unknown"},
    };

    afterGetFabricSwitchPortInfo(asyncResp, {}, properties);

    const nlohmann::json& json = asyncResp->res.jsonValue;
    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(json.contains("PortType"));
    EXPECT_FALSE(json.contains("PortProtocol"));
    EXPECT_FALSE(json.contains("LinkStatus"));
}

TEST(AfterGetFabricSwitchPortInfo, WrongPropertyTypeReportsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    const dbus::utility::DBusPropertiesMap properties = {
        {"LinkStatus", true},
    };

    afterGetFabricSwitchPortInfo(asyncResp, {}, properties);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

} // namespace
} // namespace redfish
