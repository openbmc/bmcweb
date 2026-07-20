// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "generated/enums/pcie_device.hpp"
#include "utils/pcie_util.hpp"

#include <boost/beast/http/status.hpp>
#include <nlohmann/json.hpp>

#include <cstddef>
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

} // namespace
} // namespace redfish::pcie_util
