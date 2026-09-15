// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "dbus_utility.hpp"
#include "sensors.hpp"

#include <cstdint>
#include <string>
#include <utility>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

constexpr const char* powerSupplyPath =
    "/xyz/openbmc_project/inventory/powersupply0";
constexpr const char* itemIface = "xyz.openbmc_project.Inventory.Item";
constexpr const char* powerSupplyIface =
    "xyz.openbmc_project.Inventory.Item.PowerSupply";
constexpr const char* assetIface =
    "xyz.openbmc_project.Inventory.Decorator.Asset";
constexpr const char* statusIface =
    "xyz.openbmc_project.State.Decorator.OperationalStatus";
constexpr const char* stateEnabled =
    "xyz.openbmc_project.State.Decorator.OperationalStatus.StateType.Enabled";

void expectConstructorDefaults(const InventoryItem& item)
{
    EXPECT_EQ(item.objectPath, powerSupplyPath);
    EXPECT_EQ(item.name, "powersupply0");
    EXPECT_TRUE(item.isPresent);
    EXPECT_TRUE(item.isFunctional);
    EXPECT_FALSE(item.isPowerSupply);
    EXPECT_EQ(item.powerSupplyEfficiencyPercent, -1);
    EXPECT_TRUE(item.manufacturer.empty());
    EXPECT_TRUE(item.model.empty());
    EXPECT_TRUE(item.partNumber.empty());
    EXPECT_TRUE(item.serialNumber.empty());
    EXPECT_TRUE(item.sensors.empty());
}

TEST(StoreInventoryItemData, EmptyInterfacesMapWritesNothing)
{
    InventoryItem item(powerSupplyPath);
    dbus::utility::DBusInterfacesMap interfaces;

    storeInventoryItemData(item, interfaces);

    expectConstructorDefaults(item);
}

TEST(StoreInventoryItemData, UnknownInterfacesWriteNothing)
{
    InventoryItem item(powerSupplyPath);

    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("FieldReplaceable", true);

    dbus::utility::DBusPropertiesMap lowercaseProps;
    lowercaseProps.emplace_back("Present", false);

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back(
        "xyz.openbmc_project.Inventory.Decorator.Replaceable",
        std::move(props));
    interfaces.emplace_back("xyz.openbmc_project.inventory.item",
                            std::move(lowercaseProps));

    storeInventoryItemData(item, interfaces);

    expectConstructorDefaults(item);
}

TEST(StoreInventoryItemData, ItemPresentFalseClearsIsPresent)
{
    InventoryItem item(powerSupplyPath);

    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("Present", false);

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back(itemIface, std::move(props));

    storeInventoryItemData(item, interfaces);

    EXPECT_FALSE(item.isPresent);
    EXPECT_TRUE(item.isFunctional);
    EXPECT_FALSE(item.isPowerSupply);
    EXPECT_TRUE(item.manufacturer.empty());
    EXPECT_TRUE(item.model.empty());
    EXPECT_TRUE(item.partNumber.empty());
    EXPECT_TRUE(item.serialNumber.empty());
    EXPECT_EQ(item.powerSupplyEfficiencyPercent, -1);
}

TEST(StoreInventoryItemData, ItemPresentTrueRestoresIsPresent)
{
    InventoryItem item(powerSupplyPath);
    item.isPresent = false;

    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("Present", true);

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back(itemIface, std::move(props));

    storeInventoryItemData(item, interfaces);

    EXPECT_TRUE(item.isPresent);
}

TEST(StoreInventoryItemData, ItemPresentAsStringIsIgnored)
{
    InventoryItem item(powerSupplyPath);
    item.isPresent = false;

    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("Present", std::string("true"));

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back(itemIface, std::move(props));

    storeInventoryItemData(item, interfaces);

    EXPECT_FALSE(item.isPresent);
}

TEST(StoreInventoryItemData, ItemPresentAsIntegerIsIgnored)
{
    InventoryItem item(powerSupplyPath);
    item.isPresent = false;

    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("Present", int64_t{1});

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back(itemIface, std::move(props));

    storeInventoryItemData(item, interfaces);

    EXPECT_FALSE(item.isPresent);
}

TEST(StoreInventoryItemData, ItemUnknownPropertyWritesNothing)
{
    InventoryItem item(powerSupplyPath);

    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("PrettyName", "Power Supply 0");

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back(itemIface, std::move(props));

    storeInventoryItemData(item, interfaces);

    expectConstructorDefaults(item);
}

TEST(StoreInventoryItemData, ItemWithoutPropertiesWritesNothing)
{
    InventoryItem item(powerSupplyPath);

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back(itemIface, dbus::utility::DBusPropertiesMap{});

    storeInventoryItemData(item, interfaces);

    expectConstructorDefaults(item);
}

TEST(StoreInventoryItemData, PowerSupplyInterfaceWithoutPropertiesSetsFlag)
{
    InventoryItem item(powerSupplyPath);

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back(powerSupplyIface,
                            dbus::utility::DBusPropertiesMap{});

    storeInventoryItemData(item, interfaces);

    EXPECT_TRUE(item.isPowerSupply);
    EXPECT_TRUE(item.isPresent);
    EXPECT_TRUE(item.isFunctional);
    EXPECT_TRUE(item.manufacturer.empty());
    EXPECT_TRUE(item.model.empty());
    EXPECT_TRUE(item.partNumber.empty());
    EXPECT_TRUE(item.serialNumber.empty());
    EXPECT_EQ(item.powerSupplyEfficiencyPercent, -1);
}

TEST(StoreInventoryItemData, PowerSupplyInterfaceDoesNotMatchItemInterface)
{
    InventoryItem item(powerSupplyPath);

    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("Present", false);

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back(powerSupplyIface, std::move(props));

    storeInventoryItemData(item, interfaces);

    EXPECT_TRUE(item.isPowerSupply);
    EXPECT_TRUE(item.isPresent);
}

TEST(StoreInventoryItemData, AssetPropertiesAreStored)
{
    InventoryItem item(powerSupplyPath);

    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("Manufacturer", "Acme");
    props.emplace_back("Model", "PS-1000");
    props.emplace_back("SerialNumber", "SN123");
    props.emplace_back("PartNumber", "PN456");

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back(assetIface, std::move(props));

    storeInventoryItemData(item, interfaces);

    EXPECT_EQ(item.manufacturer, "Acme");
    EXPECT_EQ(item.model, "PS-1000");
    EXPECT_EQ(item.serialNumber, "SN123");
    EXPECT_EQ(item.partNumber, "PN456");
    EXPECT_TRUE(item.isPresent);
    EXPECT_TRUE(item.isFunctional);
    EXPECT_FALSE(item.isPowerSupply);
    EXPECT_EQ(item.powerSupplyEfficiencyPercent, -1);
    EXPECT_TRUE(item.sensors.empty());
}

TEST(StoreInventoryItemData, AssetNonStringValuesAreIgnored)
{
    InventoryItem item(powerSupplyPath);
    item.manufacturer = "OldMfr";
    item.model = "OldModel";
    item.serialNumber = "OldSerial";
    item.partNumber = "OldPart";

    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("Manufacturer", true);
    props.emplace_back("Model", int64_t{7});
    props.emplace_back("SerialNumber", 1.5);
    props.emplace_back("PartNumber", uint8_t{3});

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back(assetIface, std::move(props));

    storeInventoryItemData(item, interfaces);

    EXPECT_EQ(item.manufacturer, "OldMfr");
    EXPECT_EQ(item.model, "OldModel");
    EXPECT_EQ(item.serialNumber, "OldSerial");
    EXPECT_EQ(item.partNumber, "OldPart");
}

TEST(StoreInventoryItemData, AssetUnknownPropertiesWriteNothing)
{
    InventoryItem item(powerSupplyPath);

    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("BuildDate", "2020-01-01");
    props.emplace_back("SparePartNumber", "SP1");
    props.emplace_back("SubModel", "SM1");

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back(assetIface, std::move(props));

    storeInventoryItemData(item, interfaces);

    expectConstructorDefaults(item);
}

TEST(StoreInventoryItemData, AssetWithoutPropertiesWritesNothing)
{
    InventoryItem item(powerSupplyPath);

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back(assetIface, dbus::utility::DBusPropertiesMap{});

    storeInventoryItemData(item, interfaces);

    expectConstructorDefaults(item);
}

TEST(StoreInventoryItemData, OperationalStatusFunctionalFalseClearsIsFunctional)
{
    InventoryItem item(powerSupplyPath);

    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("Functional", false);

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back(statusIface, std::move(props));

    storeInventoryItemData(item, interfaces);

    EXPECT_FALSE(item.isFunctional);
    EXPECT_TRUE(item.isPresent);
    EXPECT_FALSE(item.isPowerSupply);
    EXPECT_TRUE(item.manufacturer.empty());
    EXPECT_TRUE(item.model.empty());
    EXPECT_TRUE(item.partNumber.empty());
    EXPECT_TRUE(item.serialNumber.empty());
    EXPECT_EQ(item.powerSupplyEfficiencyPercent, -1);
}

TEST(StoreInventoryItemData,
     OperationalStatusFunctionalTrueRestoresIsFunctional)
{
    InventoryItem item(powerSupplyPath);
    item.isFunctional = false;

    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("Functional", true);

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back(statusIface, std::move(props));

    storeInventoryItemData(item, interfaces);

    EXPECT_TRUE(item.isFunctional);
}

TEST(StoreInventoryItemData, OperationalStatusFunctionalAsStringIsIgnored)
{
    InventoryItem item(powerSupplyPath);
    item.isFunctional = false;

    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("Functional", std::string("true"));

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back(statusIface, std::move(props));

    storeInventoryItemData(item, interfaces);

    EXPECT_FALSE(item.isFunctional);
}

TEST(StoreInventoryItemData, OperationalStatusUnknownPropertyWritesNothing)
{
    InventoryItem item(powerSupplyPath);

    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("State", stateEnabled);

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back(statusIface, std::move(props));

    storeInventoryItemData(item, interfaces);

    expectConstructorDefaults(item);
}

TEST(StoreInventoryItemData, FullPayloadSetsEveryWritableField)
{
    InventoryItem item(powerSupplyPath);

    dbus::utility::DBusPropertiesMap itemProps;
    itemProps.emplace_back("Present", false);

    dbus::utility::DBusPropertiesMap assetProps;
    assetProps.emplace_back("Manufacturer", "Acme");
    assetProps.emplace_back("Model", "PS-1000");
    assetProps.emplace_back("SerialNumber", "SN123");
    assetProps.emplace_back("PartNumber", "PN456");

    dbus::utility::DBusPropertiesMap statusProps;
    statusProps.emplace_back("Functional", false);

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back(itemIface, std::move(itemProps));
    interfaces.emplace_back(powerSupplyIface,
                            dbus::utility::DBusPropertiesMap{});
    interfaces.emplace_back(assetIface, std::move(assetProps));
    interfaces.emplace_back(statusIface, std::move(statusProps));

    storeInventoryItemData(item, interfaces);

    EXPECT_EQ(item.objectPath, powerSupplyPath);
    EXPECT_EQ(item.name, "powersupply0");
    EXPECT_FALSE(item.isPresent);
    EXPECT_FALSE(item.isFunctional);
    EXPECT_TRUE(item.isPowerSupply);
    EXPECT_EQ(item.powerSupplyEfficiencyPercent, -1);
    EXPECT_EQ(item.manufacturer, "Acme");
    EXPECT_EQ(item.model, "PS-1000");
    EXPECT_EQ(item.partNumber, "PN456");
    EXPECT_EQ(item.serialNumber, "SN123");
    EXPECT_TRUE(item.sensors.empty());
}

TEST(StoreInventoryItemData, DuplicatePropertyNamesKeepLastValue)
{
    InventoryItem item(powerSupplyPath);

    dbus::utility::DBusPropertiesMap itemProps;
    itemProps.emplace_back("Present", true);
    itemProps.emplace_back("Present", false);
    itemProps.emplace_back("Model", "ignored");

    dbus::utility::DBusPropertiesMap assetProps;
    assetProps.emplace_back("Model", "First");
    assetProps.emplace_back("Model", "Second");

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back(itemIface, std::move(itemProps));
    interfaces.emplace_back(assetIface, std::move(assetProps));

    storeInventoryItemData(item, interfaces);

    EXPECT_FALSE(item.isPresent);
    EXPECT_EQ(item.model, "Second");
}

TEST(StoreInventoryItemData, DuplicateInterfaceNamesKeepLastValue)
{
    InventoryItem item(powerSupplyPath);

    dbus::utility::DBusPropertiesMap first;
    first.emplace_back("Present", false);

    dbus::utility::DBusPropertiesMap second;
    second.emplace_back("Present", true);

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back(itemIface, std::move(first));
    interfaces.emplace_back(itemIface, std::move(second));

    storeInventoryItemData(item, interfaces);

    EXPECT_TRUE(item.isPresent);
}

TEST(StoreInventoryItemData, AbsentInterfacesLeavePreviousValues)
{
    InventoryItem item(powerSupplyPath);
    item.isPowerSupply = true;
    item.isPresent = false;
    item.manufacturer = "Acme";
    item.model = "PS-1000";
    item.serialNumber = "SN123";
    item.partNumber = "PN456";

    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("Functional", false);

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back(statusIface, std::move(props));

    storeInventoryItemData(item, interfaces);

    EXPECT_FALSE(item.isFunctional);
    EXPECT_TRUE(item.isPowerSupply);
    EXPECT_FALSE(item.isPresent);
    EXPECT_EQ(item.manufacturer, "Acme");
    EXPECT_EQ(item.model, "PS-1000");
    EXPECT_EQ(item.serialNumber, "SN123");
    EXPECT_EQ(item.partNumber, "PN456");
}

TEST(StoreInventoryItemData, SensorsAndEfficiencyAreNeverWritten)
{
    InventoryItem item(powerSupplyPath);
    item.sensors.emplace("/xyz/openbmc_project/sensors/voltage/ps0_vout");
    item.powerSupplyEfficiencyPercent = 90;

    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("Present", false);

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back(itemIface, std::move(props));

    storeInventoryItemData(item, interfaces);

    EXPECT_FALSE(item.isPresent);
    EXPECT_EQ(item.powerSupplyEfficiencyPercent, 90);
    EXPECT_EQ(item.sensors.size(), 1U);
    EXPECT_TRUE(
        item.sensors.contains("/xyz/openbmc_project/sensors/voltage/ps0_vout"));
    EXPECT_EQ(item.objectPath, powerSupplyPath);
    EXPECT_EQ(item.name, "powersupply0");
}

} // namespace
} // namespace redfish
