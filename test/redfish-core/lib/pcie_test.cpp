// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "generated/enums/pcie_function.hpp"
#include "http_response.hpp"
#include "pcie.hpp"

#include <nlohmann/json.hpp>

#include <string>
#include <variant>
#include <vector>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

dbus::utility::DBusPropertiesMap makeProps(const std::string& key,
                                           const std::string& value)
{
    dbus::utility::DBusPropertiesMap props;
    props.emplace_back(key, dbus::utility::DbusVariantType{value});
    return props;
}

TEST(AddPCIeFunctionProperties, FunctionTypePhysicalBareString)
{
    crow::Response resp;
    addPCIeFunctionProperties(resp, 0,
                              makeProps("Function0FunctionType", "Physical"));
    EXPECT_EQ(resp.jsonValue["FunctionType"], "Physical");
}

TEST(AddPCIeFunctionProperties, FunctionTypeVirtualBareString)
{
    crow::Response resp;
    addPCIeFunctionProperties(resp, 0,
                              makeProps("Function0FunctionType", "Virtual"));
    EXPECT_EQ(resp.jsonValue["FunctionType"], "Virtual");
}

TEST(AddPCIeFunctionProperties, FunctionTypeFullyQualifiedDBusString)
{
    crow::Response resp;
    addPCIeFunctionProperties(
        resp, 0,
        makeProps("Function0FunctionType",
                  "xyz.openbmc_project.Inventory.Item.PCIeSlot."
                  "FunctionType.Physical"));
    EXPECT_EQ(resp.jsonValue["FunctionType"], "Physical");
}

TEST(AddPCIeFunctionProperties, FunctionTypeUnknownStringOmitted)
{
    crow::Response resp;
    addPCIeFunctionProperties(resp, 0,
                              makeProps("Function0FunctionType", "Bogus"));
    EXPECT_FALSE(resp.jsonValue.contains("FunctionType"));
}

TEST(AddPCIeFunctionProperties, FunctionTypeEmptyStringOmitted)
{
    crow::Response resp;
    addPCIeFunctionProperties(resp, 0, makeProps("Function0FunctionType", ""));
    EXPECT_FALSE(resp.jsonValue.contains("FunctionType"));
}

TEST(AddPCIeFunctionProperties, DeviceClassNetworkControllerBareString)
{
    crow::Response resp;
    addPCIeFunctionProperties(
        resp, 0, makeProps("Function0DeviceClass", "NetworkController"));
    EXPECT_EQ(resp.jsonValue["DeviceClass"], "NetworkController");
}

TEST(AddPCIeFunctionProperties, DeviceClassBridgeBareString)
{
    crow::Response resp;
    addPCIeFunctionProperties(resp, 0,
                              makeProps("Function0DeviceClass", "Bridge"));
    EXPECT_EQ(resp.jsonValue["DeviceClass"], "Bridge");
}

TEST(AddPCIeFunctionProperties, DeviceClassFullyQualifiedDBusString)
{
    crow::Response resp;
    addPCIeFunctionProperties(
        resp, 0,
        makeProps("Function0DeviceClass",
                  "xyz.openbmc_project.Inventory.Item.PCIeDevice."
                  "DeviceClass.MassStorageController"));
    EXPECT_EQ(resp.jsonValue["DeviceClass"], "MassStorageController");
}

TEST(AddPCIeFunctionProperties, DeviceClassUnknownStringOmitted)
{
    crow::Response resp;
    addPCIeFunctionProperties(resp, 0,
                              makeProps("Function0DeviceClass", "NotAClass"));
    EXPECT_FALSE(resp.jsonValue.contains("DeviceClass"));
}

TEST(AddPCIeFunctionProperties, DeviceClassEmptyStringOmitted)
{
    crow::Response resp;
    addPCIeFunctionProperties(resp, 0, makeProps("Function0DeviceClass", ""));
    EXPECT_FALSE(resp.jsonValue.contains("DeviceClass"));
}

TEST(AddPCIeFunctionProperties, WrongFunctionIndexIgnored)
{
    crow::Response resp;
    addPCIeFunctionProperties(resp, 0,
                              makeProps("Function1FunctionType", "Physical"));
    EXPECT_FALSE(resp.jsonValue.contains("FunctionType"));
}

} // namespace
} // namespace redfish
