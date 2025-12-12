#include "ethernet.hpp"
#include "http_response.hpp"

#include <dbus_utility.hpp>

#include <nlohmann/json.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <cstddef>
#include <string>
#include <variant>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace redfish
{
namespace
{

using ::testing::IsEmpty;

TEST(Ethernet, parseAddressesEmpty)
{
    std::vector<std::variant<nlohmann::json::object_t, std::nullptr_t>> addr;
    std::vector<IPv4AddressData> existingAddr;

    std::vector<AddressPatch> addrOut;
    std::string gatewayOut;

    crow::Response res;

    EXPECT_TRUE(parseAddresses(addr, existingAddr, res, addrOut, gatewayOut));
    EXPECT_THAT(addrOut, IsEmpty());
    EXPECT_THAT(gatewayOut, IsEmpty());
}

// Create full entry with all fields
TEST(Ethernet, parseAddressesCreateOne)
{
    std::vector<std::variant<nlohmann::json::object_t, std::nullptr_t>> addr;
    nlohmann::json::object_t eth;
    eth["Address"] = "1.1.1.2";
    eth["Gateway"] = "1.1.1.1";
    eth["SubnetMask"] = "255.255.255.0";
    addr.emplace_back(eth);
    std::vector<IPv4AddressData> existingAddr;

    std::vector<AddressPatch> addrOut;
    std::string gatewayOut;

    crow::Response res;

    EXPECT_TRUE(parseAddresses(addr, existingAddr, res, addrOut, gatewayOut));
    EXPECT_EQ(addrOut.size(), 1);
    EXPECT_EQ(addrOut[0].address, "1.1.1.2");
    EXPECT_EQ(addrOut[0].gateway, "1.1.1.1");
    EXPECT_EQ(addrOut[0].prefixLength, 24);
    EXPECT_EQ(addrOut[0].existingDbusId, "");
    EXPECT_EQ(addrOut[0].operation, AddrChange::Update);
    EXPECT_EQ(gatewayOut, "1.1.1.1");
}

// Missing gateway should default to no gateway
TEST(Ethernet, parseAddressesCreateOneNoGateway)
{
    std::vector<std::variant<nlohmann::json::object_t, std::nullptr_t>> addr;
    nlohmann::json::object_t eth;
    eth["Address"] = "1.1.1.2";
    eth["SubnetMask"] = "255.255.255.0";
    addr.emplace_back(eth);
    std::vector<IPv4AddressData> existingAddr;

    std::vector<AddressPatch> addrOut;
    std::string gatewayOut;

    crow::Response res;

    EXPECT_TRUE(parseAddresses(addr, existingAddr, res, addrOut, gatewayOut));
    EXPECT_EQ(addrOut.size(), 1);
    EXPECT_EQ(addrOut[0].address, "1.1.1.2");
    EXPECT_EQ(addrOut[0].gateway, "");
    EXPECT_EQ(addrOut[0].prefixLength, 24);
    EXPECT_EQ(addrOut[0].existingDbusId, "");
    EXPECT_EQ(addrOut[0].operation, AddrChange::Update);
    EXPECT_EQ(gatewayOut, "");
}

// Create two entries with conflicting gateways.
TEST(Ethernet, conflictingGatewaysNew)
{
    std::vector<std::variant<nlohmann::json::object_t, std::nullptr_t>> addr;
    nlohmann::json::object_t eth;
    eth["Address"] = "1.1.1.2";
    eth["Gateway"] = "1.1.1.1";
    eth["SubnetMask"] = "255.255.255.0";
    addr.emplace_back(eth);
    eth["Gateway"] = "1.1.1.5";
    addr.emplace_back(eth);
    std::vector<IPv4AddressData> existingAddr;

    std::vector<AddressPatch> addrOut;
    std::string gatewayOut;

    crow::Response res;

    EXPECT_FALSE(parseAddresses(addr, existingAddr, res, addrOut, gatewayOut));
}

// Create full entry with all fields
TEST(Ethernet, conflictingGatewaysExisting)
{
    std::vector<std::variant<nlohmann::json::object_t, std::nullptr_t>> addr;
    nlohmann::json::object_t eth;
    addr.emplace_back(eth);
    eth["Address"] = "1.1.1.2";
    eth["Gateway"] = "1.1.1.1";
    eth["SubnetMask"] = "255.255.255.0";
    addr.emplace_back(eth);
    std::vector<IPv4AddressData> existingAddr;
    IPv4AddressData& existing = existingAddr.emplace_back();
    existing.id = "my_ip_id";
    existing.origin = "Static";
    existing.gateway = "192.168.1.1";

    std::vector<AddressPatch> addrOut;
    std::string gatewayOut;

    crow::Response res;

    EXPECT_FALSE(parseAddresses(addr, existingAddr, res, addrOut, gatewayOut));
}

// Missing address should fail
TEST(Ethernet, parseMissingAddress)
{
    std::vector<std::variant<nlohmann::json::object_t, std::nullptr_t>> addr;
    nlohmann::json::object_t eth;
    eth["SubnetMask"] = "255.255.255.0";
    addr.emplace_back(eth);
    std::vector<IPv4AddressData> existingAddr;

    std::vector<AddressPatch> addrOut;
    std::string gatewayOut;

    crow::Response res;

    EXPECT_FALSE(parseAddresses(addr, existingAddr, res, addrOut, gatewayOut));
}

// Missing subnet should fail
TEST(Ethernet, parseAddressesMissingSubnet)
{
    std::vector<std::variant<nlohmann::json::object_t, std::nullptr_t>> addr;
    nlohmann::json::object_t eth;
    eth["Address"] = "1.1.1.2";
    addr.emplace_back(eth);
    std::vector<IPv4AddressData> existingAddr;

    std::vector<AddressPatch> addrOut;
    std::string gatewayOut;

    crow::Response res;

    EXPECT_FALSE(parseAddresses(addr, existingAddr, res, addrOut, gatewayOut));
}

// With one existing address, and a null, it should request deletion
// and clear gateway
TEST(Ethernet, parseAddressesDeleteExistingOnNull)
{
    std::vector<std::variant<nlohmann::json::object_t, std::nullptr_t>> addr;
    addr.emplace_back(nullptr);
    std::vector<IPv4AddressData> existingAddr;
    IPv4AddressData& existing = existingAddr.emplace_back();
    existing.id = "my_ip_id";
    existing.origin = "Static";

    std::vector<AddressPatch> addrOut;
    std::string gatewayOut;

    crow::Response res;

    EXPECT_TRUE(parseAddresses(addr, existingAddr, res, addrOut, gatewayOut));
    EXPECT_EQ(addrOut.size(), 1);
    EXPECT_EQ(addrOut[0].existingDbusId, "my_ip_id");
    EXPECT_EQ(addrOut[0].operation, AddrChange::Delete);
    EXPECT_EQ(gatewayOut, "");
}

TEST(Ethernet, parseAddressesDeleteExistingOnShortLength)
{
    // With one existing address, and an input of len(0) it should request
    // deletion and clear gateway
    std::vector<std::variant<nlohmann::json::object_t, std::nullptr_t>> addr;
    std::vector<IPv4AddressData> existingAddr;
    IPv4AddressData& existing = existingAddr.emplace_back();
    existing.id = "my_ip_id";
    existing.origin = "Static";

    std::vector<AddressPatch> addrOut;
    std::string gatewayOut;

    crow::Response res;

    EXPECT_TRUE(parseAddresses(addr, existingAddr, res, addrOut, gatewayOut));
    EXPECT_EQ(addrOut.size(), 1);
    EXPECT_EQ(addrOut[0].existingDbusId, "my_ip_id");
    EXPECT_EQ(addrOut[0].operation, AddrChange::Delete);
    EXPECT_EQ(gatewayOut, "");
}

// Test that IPv4 Gateway property is correctly extracted from D-Bus data
TEST(Ethernet, extractIPDataWithGateway)
{
    // Create mock D-Bus data simulating phosphor-network's IPv4 object
    dbus::utility::ManagedObjectType dbusData;

    sdbusplus::message::object_path ipPath(
        "/xyz/openbmc_project/network/eth0/ipv4/1a2b3c4d");

    dbus::utility::DBusPropertiesMap properties;
    properties.emplace_back("Address", std::string("192.168.1.100"));
    properties.emplace_back("Gateway", std::string("192.168.1.1"));
    properties.emplace_back("Origin",
                           std::string("xyz.openbmc_project.Network.IP."
                                      "AddressOrigin.Static"));
    properties.emplace_back("PrefixLength", static_cast<uint8_t>(24));
    properties.emplace_back("Type",
                           std::string("xyz.openbmc_project.Network.IP."
                                      "Protocol.IPv4"));

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back("xyz.openbmc_project.Network.IP", properties);

    dbusData.emplace_back(ipPath, interfaces);

    // Extract the IP data
    std::vector<IPv4AddressData> ipv4Config;
    extractIPData("eth0", dbusData, ipv4Config);

    // Verify Gateway was extracted correctly
    ASSERT_EQ(ipv4Config.size(), 1);
    EXPECT_EQ(ipv4Config[0].address, "192.168.1.100");
    EXPECT_EQ(ipv4Config[0].gateway, "192.168.1.1");
    EXPECT_EQ(ipv4Config[0].netmask, "255.255.255.0");
    EXPECT_EQ(ipv4Config[0].origin, "Static");
}

} // namespace
} // namespace redfish
