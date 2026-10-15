// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "http_response.hpp"
#include "pcie_slots.hpp"

#include <boost/asio/error.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/system/linux_error.hpp>
#include <nlohmann/json.hpp>

#include <cstddef>
#include <memory>
#include <string>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

std::shared_ptr<bmcweb::AsyncResp> makeRespWithSlotsArray()
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    asyncResp->res.jsonValue["Slots"] = nlohmann::json::array();
    return asyncResp;
}

TEST(OnPcieSlotGetAllDone, ErrorSetsInternalError)
{
    auto asyncResp = makeRespWithSlotsArray();

    onPcieSlotGetAllDone(asyncResp, boost::asio::error::invalid_argument, {});

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(OnPcieSlotGetAllDone, MissingSlotsArraySetsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    onPcieSlotGetAllDone(asyncResp, {}, {});

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(OnPcieSlotGetAllDone, WrongPropertyTypeSetsInternalError)
{
    auto asyncResp = makeRespWithSlotsArray();
    dbus::utility::DBusPropertiesMap properties = {
        {"Lanes", std::string("x16")}};

    onPcieSlotGetAllDone(asyncResp, {}, properties);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_TRUE(asyncResp->res.jsonValue["Slots"].empty());
}

TEST(OnPcieSlotGetAllDone, PropertiesMapped)
{
    auto asyncResp = makeRespWithSlotsArray();
    dbus::utility::DBusPropertiesMap properties = {
        {"Generation",
         std::string(
             "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Gen4")},
        {"Lanes", static_cast<size_t>(16)},
        {"SlotType",
         std::string(
             "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.FullLength")},
        {"HotPluggable", true}};

    onPcieSlotGetAllDone(asyncResp, {}, properties);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(asyncResp->res.jsonValue["Slots"], R"([{
        "PCIeType": "Gen4",
        "Lanes": 16,
        "SlotType": "FullLength",
        "HotPluggable": true
    }])"_json);
}

TEST(OnPcieSlotGetAllDone, UnknownValuesAndZeroLanesOmitted)
{
    auto asyncResp = makeRespWithSlotsArray();
    dbus::utility::DBusPropertiesMap properties = {
        {"Generation",
         std::string(
             "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Unknown")},
        {"Lanes", static_cast<size_t>(0)},
        {"SlotType",
         std::string(
             "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.Unknown")}};

    onPcieSlotGetAllDone(asyncResp, {}, properties);

    const nlohmann::json& slots = asyncResp->res.jsonValue["Slots"];
    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    ASSERT_EQ(slots.size(), 1);
    EXPECT_FALSE(slots[0].contains("PCIeType"));
    EXPECT_FALSE(slots[0].contains("Lanes"));
    EXPECT_FALSE(slots[0].contains("SlotType"));
    EXPECT_FALSE(slots[0].contains("HotPluggable"));
}

TEST(OnPcieSlotGetAllDone, UnrecognizedGenerationSetsInternalError)
{
    auto asyncResp = makeRespWithSlotsArray();
    dbus::utility::DBusPropertiesMap properties = {
        {"Generation", std::string("not.a.generation")}};

    onPcieSlotGetAllDone(asyncResp, {}, properties);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_TRUE(asyncResp->res.jsonValue["Slots"].empty());
}

TEST(OnPcieSlotGetAllDone, UnrecognizedSlotTypeSetsInternalError)
{
    auto asyncResp = makeRespWithSlotsArray();
    dbus::utility::DBusPropertiesMap properties = {
        {"SlotType", std::string("not.a.slot.type")}};

    onPcieSlotGetAllDone(asyncResp, {}, properties);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_TRUE(asyncResp->res.jsonValue["Slots"].empty());
}

TEST(OnMapperAssociationDone, EbadrIsIgnored)
{
    auto asyncResp = makeRespWithSlotsArray();

    onMapperAssociationDone(
        asyncResp, "chassis0", "/xyz/openbmc_project/inventory/slot0",
        "xyz.openbmc_project.Example",
        boost::system::linux_error::bad_request_descriptor, {});

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(asyncResp->res.jsonValue["Slots"].empty());
}

TEST(OnMapperAssociationDone, OtherErrorSetsInternalError)
{
    auto asyncResp = makeRespWithSlotsArray();

    onMapperAssociationDone(asyncResp, "chassis0",
                            "/xyz/openbmc_project/inventory/slot0",
                            "xyz.openbmc_project.Example",
                            boost::asio::error::invalid_argument, {});

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(OnMapperAssociationDone, NoChassisEndpointSetsInternalError)
{
    auto asyncResp = makeRespWithSlotsArray();

    onMapperAssociationDone(asyncResp, "chassis0",
                            "/xyz/openbmc_project/inventory/slot0",
                            "xyz.openbmc_project.Example", {}, {});

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(OnMapperAssociationDone, MultipleChassisEndpointsSetsInternalError)
{
    auto asyncResp = makeRespWithSlotsArray();
    dbus::utility::MapperEndPoints endpoints = {
        "/xyz/openbmc_project/inventory/chassis0",
        "/xyz/openbmc_project/inventory/chassis1"};

    onMapperAssociationDone(asyncResp, "chassis0",
                            "/xyz/openbmc_project/inventory/slot0",
                            "xyz.openbmc_project.Example", {}, endpoints);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(OnMapperAssociationDone, OtherChassisSlotIsSkipped)
{
    auto asyncResp = makeRespWithSlotsArray();
    dbus::utility::MapperEndPoints endpoints = {
        "/xyz/openbmc_project/inventory/chassis1"};

    onMapperAssociationDone(asyncResp, "chassis0",
                            "/xyz/openbmc_project/inventory/slot0",
                            "xyz.openbmc_project.Example", {}, endpoints);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(asyncResp->res.jsonValue["Slots"].empty());
}

TEST(OnMapperSubtreeDone, ErrorSetsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    onMapperSubtreeDone(asyncResp, "chassis0",
                        boost::asio::error::invalid_argument, {});

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(OnMapperSubtreeDone, EmptySubtreeReturnsNotFound)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    onMapperSubtreeDone(asyncResp, "chassis0", {}, {});

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::not_found);
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("@odata.id"));
}

TEST(OnMapperSubtreeDone, SubtreePopulatesCollection)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::MapperGetSubTreeResponse subtree = {
        {"/xyz/openbmc_project/inventory/slot0", {}}};

    onMapperSubtreeDone(asyncResp, "chassis0", {}, subtree);

    const nlohmann::json& json = asyncResp->res.jsonValue;
    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(json["@odata.type"], "#PCIeSlots.v1_4_1.PCIeSlots");
    EXPECT_EQ(json["@odata.id"], "/redfish/v1/Chassis/chassis0/PCIeSlots");
    EXPECT_EQ(json["Name"], "PCIe Slot Information");
    EXPECT_EQ(json["Id"], "1");
    EXPECT_TRUE(json["Slots"].is_array());
    EXPECT_TRUE(json["Slots"].empty());
}

} // namespace
} // namespace redfish
