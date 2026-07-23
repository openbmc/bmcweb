// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "generated/enums/pcie_device.hpp"
#include "generated/enums/port.hpp"
#include "generated/enums/protocol.hpp"
#include "utils/pcie_util.hpp"

#include <boost/asio/error.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/system/error_code.hpp>
#include <nlohmann/json.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>

#include <gtest/gtest.h>

namespace redfish::pcie_util
{
namespace
{

TEST(PcieUtil, GenerationFromDbus)
{
    EXPECT_EQ(
        redfishPcieGenerationFromDbus(
            "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Gen5"),
        pcie_device::PCIeTypes::Gen5);
    EXPECT_EQ(redfishPcieGenerationFromDbus(""), std::nullopt);
    EXPECT_EQ(
        redfishPcieGenerationFromDbus(
            "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Unknown"),
        std::nullopt);
    EXPECT_EQ(redfishPcieGenerationFromDbus("garbage"),
              pcie_device::PCIeTypes::Invalid);
}

TEST(PcieUtil, DeviceTypeFromDbus)
{
    EXPECT_EQ(redfishPcieDeviceTypeFromDbus(
                  "xyz.openbmc_project.Inventory.Item."
                  "PCIeDevice.DeviceTypes.SingleFunction"),
              pcie_device::DeviceType::SingleFunction);
    EXPECT_EQ(redfishPcieDeviceTypeFromDbus(
                  "xyz.openbmc_project.Inventory.Item.PCIeDevice.DeviceTypes."
                  "Retimer"),
              pcie_device::DeviceType::Retimer);
    EXPECT_EQ(redfishPcieDeviceTypeFromDbus(""), std::nullopt);
    EXPECT_EQ(redfishPcieDeviceTypeFromDbus(
                  "xyz.openbmc_project.Inventory.Item.PCIeDevice.DeviceTypes."
                  "Unknown"),
              std::nullopt);
    EXPECT_EQ(redfishPcieDeviceTypeFromDbus("garbage"),
              pcie_device::DeviceType::Invalid);
}

TEST(PcieUtil, InterfacePropertiesValid)
{
    auto resp = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::DBusPropertiesMap properties = {
        {"GenerationInUse",
         std::string(
             "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Gen5")},
        {"GenerationSupported",
         std::string(
             "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Gen5")},
        {"LanesInUse", size_t{16}},
        {"MaxLanes", size_t{16}},
    };

    EXPECT_TRUE(addPcieInterfaceProperties(
        resp, nlohmann::json::json_pointer("/PCIeInterface"), properties));
    EXPECT_EQ(resp->res.result(), boost::beast::http::status::ok);

    const auto& iface = resp->res.jsonValue["PCIeInterface"];
    EXPECT_EQ(iface["PCIeType"], pcie_device::PCIeTypes::Gen5);
    EXPECT_EQ(iface["MaxPCIeType"], pcie_device::PCIeTypes::Gen5);
    EXPECT_EQ(iface["LanesInUse"], 16);
    EXPECT_EQ(iface["MaxLanes"], 16);
}

TEST(PcieUtil, InterfacePropertiesDefaultsNulledOrOmitted)
{
    auto resp = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::DBusPropertiesMap properties = {
        {"LanesInUse", std::numeric_limits<size_t>::max()},
        {"MaxLanes", size_t{0}},
    };

    EXPECT_TRUE(addPcieInterfaceProperties(
        resp, nlohmann::json::json_pointer("/PCIeInterface"), properties));

    const auto& iface = resp->res.jsonValue["PCIeInterface"];
    EXPECT_TRUE(iface["LanesInUse"].is_null());
    EXPECT_FALSE(iface.contains("MaxLanes"));
}

TEST(PcieUtil, InterfacePropertiesHonorsJsonPointer)
{
    auto resp = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::DBusPropertiesMap properties = {
        {"GenerationInUse",
         std::string(
             "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Gen5")},
    };

    EXPECT_TRUE(addPcieInterfaceProperties(
        resp, nlohmann::json::json_pointer("/SystemInterface/PCIe"),
        properties));
    EXPECT_EQ(resp->res.jsonValue["SystemInterface"]["PCIe"]["PCIeType"],
              pcie_device::PCIeTypes::Gen5);
}

TEST(PcieUtil, InterfacePropertiesUnknownGenerationOmitted)
{
    auto resp = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::DBusPropertiesMap properties = {
        {"GenerationInUse",
         std::string("xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations."
                     "Unknown")},
    };

    EXPECT_TRUE(addPcieInterfaceProperties(
        resp, nlohmann::json::json_pointer("/PCIeInterface"), properties));
    EXPECT_EQ(resp->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(resp->res.jsonValue["PCIeInterface"].contains("PCIeType"));
}

TEST(PcieUtil, InterfacePropertiesInvalidGenerationFails)
{
    auto resp = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::DBusPropertiesMap properties = {
        {"GenerationInUse", std::string("garbage")},
    };

    EXPECT_FALSE(addPcieInterfaceProperties(
        resp, nlohmann::json::json_pointer("/PCIeInterface"), properties));
    EXPECT_EQ(resp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(PcieUtil, InterfacePropertiesWrongTypeFails)
{
    auto resp = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::DBusPropertiesMap properties = {
        {"LanesInUse", std::string("not-a-number")},
    };

    EXPECT_FALSE(addPcieInterfaceProperties(
        resp, nlohmann::json::json_pointer("/PCIeInterface"), properties));
    EXPECT_EQ(resp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(PcieUtil, InterfacePropertiesEmptyMap)
{
    auto resp = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::DBusPropertiesMap properties = {};

    EXPECT_TRUE(addPcieInterfaceProperties(
        resp, nlohmann::json::json_pointer("/PCIeInterface"), properties));
    EXPECT_FALSE(resp->res.jsonValue.contains("PCIeInterface"));
}

TEST(PcieUtil, PortPropertiesValid)
{
    auto resp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::DBusPropertiesMap properties = {
        {"Width", size_t{16}},
        {"Speed", uint64_t{25000000000}},
        {"PortType",
         std::string(
             "xyz.openbmc_project.Inventory.Connector.Port.PortType.Upstream")},
        {"PortProtocol",
         std::string("xyz.openbmc_project.Inventory.Connector.Port.PortProtocol"
                     ".PCIe")},
    };

    afterGetPortProperties(resp, ec, properties);

    EXPECT_EQ(resp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(resp->res.jsonValue["Width"], 16);
    // D-Bus Speed 25e9 bits/s -> 25 Gbps (decimal).
    EXPECT_EQ(resp->res.jsonValue["CurrentSpeedGbps"], 25.0);
    EXPECT_EQ(resp->res.jsonValue["PortType"], port::PortType::UpstreamPort);
    EXPECT_EQ(resp->res.jsonValue["PortProtocol"], protocol::Protocol::PCIe);
}

TEST(PcieUtil, PortPropertiesDefaultsOmitted)
{
    auto resp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::DBusPropertiesMap properties = {
        {"Width", std::numeric_limits<size_t>::max()},
        {"Speed", uint64_t{0}},
    };

    afterGetPortProperties(resp, ec, properties);

    EXPECT_FALSE(resp->res.jsonValue.contains("Width"));
    EXPECT_FALSE(resp->res.jsonValue.contains("CurrentSpeedGbps"));
}

TEST(PcieUtil, PortPropertiesErrorSetsInternalError)
{
    auto resp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;
    dbus::utility::DBusPropertiesMap properties = {};

    afterGetPortProperties(resp, ec, properties);

    EXPECT_EQ(resp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(PcieUtil, PortPropertiesWrongTypeFails)
{
    auto resp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::DBusPropertiesMap properties = {
        {"Width", std::string("not-a-number")},
    };

    afterGetPortProperties(resp, ec, properties);

    EXPECT_EQ(resp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(PcieUtil, PortProtocolFromDbus)
{
    EXPECT_EQ(
        dbusPortProtocolToRf(
            "xyz.openbmc_project.Inventory.Connector.Port.PortProtocol.PCIe"),
        protocol::Protocol::PCIe);
    EXPECT_EQ(dbusPortProtocolToRf("xyz.openbmc_project.Inventory.Connector."
                                   "Port.PortProtocol.Unknown"),
              std::nullopt);
    EXPECT_EQ(dbusPortProtocolToRf("garbage"), protocol::Protocol::Invalid);
}

TEST(PcieUtil, PortTypeFromDbus)
{
    EXPECT_EQ(
        dbusPortTypeToRf(
            "xyz.openbmc_project.Inventory.Connector.Port.PortType.Upstream"),
        port::PortType::UpstreamPort);
    EXPECT_EQ(
        dbusPortTypeToRf(
            "xyz.openbmc_project.Inventory.Connector.Port.PortType.Downstream"),
        port::PortType::DownstreamPort);
    EXPECT_EQ(dbusPortTypeToRf("xyz.openbmc_project.Inventory.Connector.Port."
                               "PortType.Unknown"),
              std::nullopt);
    EXPECT_EQ(dbusPortTypeToRf("garbage"), port::PortType::Invalid);
}

} // namespace
} // namespace redfish::pcie_util
