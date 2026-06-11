#include "dbus_utility.hpp"
#include "ethernet.hpp"
#include "http_response.hpp"

#include <nlohmann/json.hpp>

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

TEST(Ethernet, isInternalInterfacePropertyAbsent)
{
    dbus::utility::DBusPropertiesMap properties;
    properties.emplace_back("InterfaceName", std::string("eth0"));

    EXPECT_FALSE(isInternalInterface(properties));
}

TEST(Ethernet, isInternalInterfaceTrue)
{
    dbus::utility::DBusPropertiesMap properties;
    properties.emplace_back("InterfaceName", std::string("usb0"));
    properties.emplace_back("Internal", true);

    EXPECT_TRUE(isInternalInterface(properties));
}

TEST(Ethernet, isInternalInterfaceFalse)
{
    dbus::utility::DBusPropertiesMap properties;
    properties.emplace_back("InterfaceName", std::string("eth0"));
    properties.emplace_back("Internal", false);

    EXPECT_FALSE(isInternalInterface(properties));
}

TEST(Ethernet, isInternalInterfaceWrongType)
{
    dbus::utility::DBusPropertiesMap properties;
    properties.emplace_back("Internal", std::string("true"));

    EXPECT_FALSE(isInternalInterface(properties));
}

} // namespace
} // namespace redfish
