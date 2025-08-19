// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2018 Intel Corporation
#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/redundancy.hpp"
#include "generated/enums/resource.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "sensors_async_response.hpp"
#include "str_utility.hpp"
#include "utils/chassis_utils.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"
#include "utils/query_param.hpp"
#include "utils/sensor_utils.hpp"

#include <asm-generic/errno.h>

#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <boost/url/url.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace redfish
{

using InventoryItem = sensor_utils::InventoryItem;

/**
 * @brief Finds the inventory item with the specified object path.
 * @param inventoryItems D-Bus inventory items associated with sensors.
 * @param invItemObjPath D-Bus object path of inventory item.
 * @return Inventory item within vector, or nullptr if no match found.
 */
inline InventoryItem* findInventoryItem(
    const std::shared_ptr<std::vector<InventoryItem>>& inventoryItems,
    const std::string& invItemObjPath)
{
    for (InventoryItem& inventoryItem : *inventoryItems)
    {
        if (inventoryItem.objectPath == invItemObjPath)
        {
            return &inventoryItem;
        }
    }
    return nullptr;
}

/**
 * @brief Finds the inventory item associated with the specified led path.
 * @param inventoryItems D-Bus inventory items associated with sensors.
 * @param ledObjPath D-Bus object path of led.
 * @return Inventory item within vector, or nullptr if no match found.
 */
inline sensor_utils::InventoryItem* findInventoryItemForLed(
    std::vector<sensor_utils::InventoryItem>& inventoryItems,
    const std::string& ledObjPath)
{
    for (sensor_utils::InventoryItem& inventoryItem : inventoryItems)
    {
        if (inventoryItem.ledObjectPath == ledObjPath)
        {
            return &inventoryItem;
        }
    }
    return nullptr;
}

/**
 * @brief Adds inventory item and associated sensor to specified vector.
 *
 * Adds a new InventoryItem to the vector if necessary.  Searches for an
 * existing InventoryItem with the specified object path.  If not found, one is
 * added to the vector.
 *
 * Next, the specified sensor is added to the set of sensors associated with the
 * InventoryItem.
 *
 * @param inventoryItems D-Bus inventory items associated with sensors.
 * @param invItemObjPath D-Bus object path of inventory item.
 * @param sensorObjPath D-Bus object path of sensor
 */
inline void addInventoryItem(
    const std::shared_ptr<std::vector<InventoryItem>>& inventoryItems,
    const std::string& invItemObjPath, const std::string& sensorObjPath)
{
    // Look for inventory item in vector
    InventoryItem* inventoryItem =
        findInventoryItem(inventoryItems, invItemObjPath);

    // If inventory item doesn't exist in vector, add it
    if (inventoryItem == nullptr)
    {
        inventoryItems->emplace_back(invItemObjPath);
        inventoryItem = &(inventoryItems->back());
    }

    // Add sensor to set of sensors associated with inventory item
    inventoryItem->sensors.emplace(sensorObjPath);
}

/**
 * @brief Stores D-Bus data in the specified inventory item.
 *
 * Finds D-Bus data in the specified map of interfaces.  Stores the data in the
 * specified InventoryItem.
 *
 * This data is later used to provide sensor property values in the JSON
 * response.
 *
 * @param inventoryItem Inventory item where data will be stored.
 * @param interfacesDict Map containing D-Bus interfaces and their properties
 * for the specified inventory item.
 */
inline void storeInventoryItemData(
    InventoryItem& inventoryItem,
    const dbus::utility::DBusInterfacesMap& interfacesDict)
{
    // Get properties from Inventory.Item interface

    for (const auto& [interface, values] : interfacesDict)
    {
        if (interface == "xyz.openbmc_project.Inventory.Item")
        {
            for (const auto& [name, dbusValue] : values)
            {
                if (name == "Present")
                {
                    const bool* value = std::get_if<bool>(&dbusValue);
                    if (value != nullptr)
                    {
                        inventoryItem.isPresent = *value;
                    }
                }
            }
        }
        // Check if Inventory.Item.PowerSupply interface is present

        if (interface == "xyz.openbmc_project.Inventory.Item.PowerSupply")
        {
            inventoryItem.isPowerSupply = true;
        }

        // Get properties from Inventory.Decorator.Asset interface
        if (interface == "xyz.openbmc_project.Inventory.Decorator.Asset")
        {
            for (const auto& [name, dbusValue] : values)
            {
                if (name == "Manufacturer")
                {
                    const std::string* value =
                        std::get_if<std::string>(&dbusValue);
                    if (value != nullptr)
                    {
                        inventoryItem.manufacturer = *value;
                    }
                }
                if (name == "Model")
                {
                    const std::string* value =
                        std::get_if<std::string>(&dbusValue);
                    if (value != nullptr)
                    {
                        inventoryItem.model = *value;
                    }
                }
                if (name == "SerialNumber")
                {
                    const std::string* value =
                        std::get_if<std::string>(&dbusValue);
                    if (value != nullptr)
                    {
                        inventoryItem.serialNumber = *value;
                    }
                }
                if (name == "PartNumber")
                {
                    const std::string* value =
                        std::get_if<std::string>(&dbusValue);
                    if (value != nullptr)
                    {
                        inventoryItem.partNumber = *value;
                    }
                }
            }
        }

        if (interface ==
            "xyz.openbmc_project.State.Decorator.OperationalStatus")
        {
            for (const auto& [name, dbusValue] : values)
            {
                if (name == "Functional")
                {
                    const bool* value = std::get_if<bool>(&dbusValue);
                    if (value != nullptr)
                    {
                        inventoryItem.isFunctional = *value;
                    }
                }
            }
        }
    }
}

/**
 * @brief Gets D-Bus data for inventory items associated with sensors.
 *
 * Uses the specified connections (services) to obtain D-Bus data for inventory
 * items associated with sensors.  Stores the resulting data in the
 * inventoryItems vector.
 *
 * This data is later used to provide sensor property values in the JSON
 * response.
 *
 * Finds the inventory item data asynchronously.  Invokes callback when data has
 * been obtained.
 *
 * The callback must have the following signature:
 *   @code
 *   callback(void)
 *   @endcode
 *
 * This function is called recursively, obtaining data asynchronously from one
 * connection in each call.  This ensures the callback is not invoked until the
 * last asynchronous function has completed.
 *
 * @param sensorsAsyncResp Pointer to object holding response data.
 * @param inventoryItems D-Bus inventory items associated with sensors.
 * @param invConnections Connections that provide data for the inventory items.
 * implements ObjectManager.
 * @param callback Callback to invoke when inventory data has been obtained.
 * @param invConnectionsIndex Current index in invConnections.  Only specified
 * in recursive calls to this function.
 */
template <typename Callback>
void getInventoryItemsData(
    const std::shared_ptr<SensorsAsyncResp>& sensorsAsyncResp,
    const std::shared_ptr<std::vector<InventoryItem>>& inventoryItems,
    const std::shared_ptr<std::set<std::string>>& invConnections,
    Callback&& callback, size_t invConnectionsIndex = 0)
{
    BMCWEB_LOG_DEBUG("getInventoryItemsData enter");

    // If no more connections left, call callback
    if (invConnectionsIndex >= invConnections->size())
    {
        callback();
        BMCWEB_LOG_DEBUG("getInventoryItemsData exit");
        return;
    }

    // Get inventory item data from current connection
    auto it = invConnections->begin();
    std::advance(it, invConnectionsIndex);
    if (it != invConnections->end())
    {
        const std::string& invConnection = *it;

        // Get all object paths and their interfaces for current connection
        sdbusplus::message::object_path path("/xyz/openbmc_project/inventory");
        dbus::utility::getManagedObjects(
            invConnection, path,
            [sensorsAsyncResp, inventoryItems, invConnections,
             callback = std::forward<Callback>(callback), invConnectionsIndex](
                const boost::system::error_code& ec,
                const dbus::utility::ManagedObjectType& resp) mutable {
                BMCWEB_LOG_DEBUG("getInventoryItemsData respHandler enter");
                if (ec)
                {
                    BMCWEB_LOG_ERROR(
                        "getInventoryItemsData respHandler DBus error {}", ec);
                    messages::internalError(sensorsAsyncResp->asyncResp->res);
                    return;
                }

                // Loop through returned object paths
                for (const auto& objDictEntry : resp)
                {
                    const std::string& objPath =
                        static_cast<const std::string&>(objDictEntry.first);

                    // If this object path is one of the specified inventory
                    // items
                    InventoryItem* inventoryItem =
                        findInventoryItem(inventoryItems, objPath);
                    if (inventoryItem != nullptr)
                    {
                        // Store inventory data in InventoryItem
                        storeInventoryItemData(*inventoryItem,
                                               objDictEntry.second);
                    }
                }

                // Recurse to get inventory item data from next connection
                getInventoryItemsData(sensorsAsyncResp, inventoryItems,
                                      invConnections, std::move(callback),
                                      invConnectionsIndex + 1);

                BMCWEB_LOG_DEBUG("getInventoryItemsData respHandler exit");
            });
    }

    BMCWEB_LOG_DEBUG("getInventoryItemsData exit");
}

/**
 * @brief Gets connections that provide D-Bus data for inventory items.
 *
 * Gets the D-Bus connections (services) that provide data for the inventory
 * items that are associated with sensors.
 *
 * Finds the connections asynchronously.  Invokes callback when information has
 * been obtained.
 *
 * The callback must have the following signature:
 *   @code
 *   callback(std::shared_ptr<std::set<std::string>> invConnections)
 *   @endcode
 *
 * @param sensorsAsyncResp Pointer to object holding response data.
 * @param inventoryItems D-Bus inventory items associated with sensors.
 * @param callback Callback to invoke when connections have been obtained.
 */
template <typename Callback>
void getInventoryItemsConnections(
    const std::shared_ptr<SensorsAsyncResp>& sensorsAsyncResp,
    const std::shared_ptr<std::vector<InventoryItem>>& inventoryItems,
    Callback&& callback)
{
    BMCWEB_LOG_DEBUG("getInventoryItemsConnections enter");

    const std::string path = "/xyz/openbmc_project/inventory";
    constexpr std::array<std::string_view, 4> interfaces = {
        "xyz.openbmc_project.Inventory.Item",
        "xyz.openbmc_project.Inventory.Item.PowerSupply",
        "xyz.openbmc_project.Inventory.Decorator.Asset",
        "xyz.openbmc_project.State.Decorator.OperationalStatus"};

    // Make call to ObjectMapper to find all inventory items
    dbus::utility::getSubTree(
        path, 0, interfaces,
        [callback = std::forward<Callback>(callback), sensorsAsyncResp,
         inventoryItems](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreeResponse& subtree) mutable {
            // Response handler for parsing output from GetSubTree
            BMCWEB_LOG_DEBUG("getInventoryItemsConnections respHandler enter");
            if (ec)
            {
                messages::internalError(sensorsAsyncResp->asyncResp->res);
                BMCWEB_LOG_ERROR(
                    "getInventoryItemsConnections respHandler DBus error {}",
                    ec);
                return;
            }

            // Make unique list of connections for desired inventory items
            std::shared_ptr<std::set<std::string>> invConnections =
                std::make_shared<std::set<std::string>>();

            // Loop through objects from GetSubTree
            for (const std::pair<std::string,
                                 std::vector<std::pair<
                                     std::string, std::vector<std::string>>>>&
                     object : subtree)
            {
                // Check if object path is one of the specified inventory items
                const std::string& objPath = object.first;
                if (findInventoryItem(inventoryItems, objPath) != nullptr)
                {
                    // Store all connections to inventory item
                    for (const std::pair<std::string, std::vector<std::string>>&
                             objData : object.second)
                    {
                        const std::string& invConnection = objData.first;
                        invConnections->insert(invConnection);
                    }
                }
            }

            callback(invConnections);
            BMCWEB_LOG_DEBUG("getInventoryItemsConnections respHandler exit");
        });
    BMCWEB_LOG_DEBUG("getInventoryItemsConnections exit");
}

/**
 * @brief Gets associations from sensors to inventory items.
 *
 * Looks for ObjectMapper associations from the specified sensors to related
 * inventory items. Then finds the associations from those inventory items to
 * their LEDs, if any.
 *
 * Finds the inventory items asynchronously.  Invokes callback when information
 * has been obtained.
 *
 * The callback must have the following signature:
 *   @code
 *   callback(std::shared_ptr<std::vector<InventoryItem>> inventoryItems)
 *   @endcode
 *
 * @param sensorsAsyncResp Pointer to object holding response data.
 * @param sensorNames All sensors within the current chassis.
 * implements ObjectManager.
 * @param callback Callback to invoke when inventory items have been obtained.
 */
template <typename Callback>
void getInventoryItemAssociations(
    const std::shared_ptr<SensorsAsyncResp>& sensorsAsyncResp,
    const std::shared_ptr<std::set<std::string>>& sensorNames,
    Callback&& callback)
{
    BMCWEB_LOG_DEBUG("getInventoryItemAssociations enter");

    // Call GetManagedObjects on the ObjectMapper to get all associations
    sdbusplus::message::object_path path("/");
    dbus::utility::getManagedObjects(
        "xyz.openbmc_project.ObjectMapper", path,
        [callback = std::forward<Callback>(callback), sensorsAsyncResp,
         sensorNames](const boost::system::error_code& ec,
                      const dbus::utility::ManagedObjectType& resp) mutable {
            BMCWEB_LOG_DEBUG("getInventoryItemAssociations respHandler enter");
            if (ec)
            {
                BMCWEB_LOG_ERROR(
                    "getInventoryItemAssociations respHandler DBus error {}",
                    ec);
                messages::internalError(sensorsAsyncResp->asyncResp->res);
                return;
            }

            // Create vector to hold list of inventory items
            std::shared_ptr<std::vector<InventoryItem>> inventoryItems =
                std::make_shared<std::vector<InventoryItem>>();

            // Loop through returned object paths
            std::string sensorAssocPath;
            sensorAssocPath.reserve(128); // avoid memory allocations
            for (const auto& objDictEntry : resp)
            {
                const std::string& objPath =
                    static_cast<const std::string&>(objDictEntry.first);

                // If path is inventory association for one of the specified
                // sensors
                for (const std::string& sensorName : *sensorNames)
                {
                    sensorAssocPath = sensorName;
                    sensorAssocPath += "/inventory";
                    if (objPath == sensorAssocPath)
                    {
                        // Get Association interface for object path
                        for (const auto& [interface, values] :
                             objDictEntry.second)
                        {
                            if (interface == "xyz.openbmc_project.Association")
                            {
                                for (const auto& [valueName, value] : values)
                                {
                                    if (valueName == "endpoints")
                                    {
                                        const std::vector<std::string>*
                                            endpoints = std::get_if<
                                                std::vector<std::string>>(
                                                &value);
                                        if ((endpoints != nullptr) &&
                                            !endpoints->empty())
                                        {
                                            // Add inventory item to vector
                                            const std::string& invItemPath =
                                                endpoints->front();
                                            addInventoryItem(inventoryItems,
                                                             invItemPath,
                                                             sensorName);
                                        }
                                    }
                                }
                            }
                        }
                        break;
                    }
                }
            }

            // Now loop through the returned object paths again, this time to
            // find the leds associated with the inventory items we just found
            std::string inventoryAssocPath;
            inventoryAssocPath.reserve(128); // avoid memory allocations
            for (const auto& objDictEntry : resp)
            {
                const std::string& objPath =
                    static_cast<const std::string&>(objDictEntry.first);

                for (InventoryItem& inventoryItem : *inventoryItems)
                {
                    inventoryAssocPath = inventoryItem.objectPath;
                    inventoryAssocPath += "/leds";
                    if (objPath == inventoryAssocPath)
                    {
                        for (const auto& [interface, values] :
                             objDictEntry.second)
                        {
                            if (interface == "xyz.openbmc_project.Association")
                            {
                                for (const auto& [valueName, value] : values)
                                {
                                    if (valueName == "endpoints")
                                    {
                                        const std::vector<std::string>*
                                            endpoints = std::get_if<
                                                std::vector<std::string>>(
                                                &value);
                                        if ((endpoints != nullptr) &&
                                            !endpoints->empty())
                                        {
                                            // Add inventory item to vector
                                            // Store LED path in inventory item
                                            const std::string& ledPath =
                                                endpoints->front();
                                            inventoryItem.ledObjectPath =
                                                ledPath;
                                        }
                                    }
                                }
                            }
                        }

                        break;
                    }
                }
            }
            callback(inventoryItems);
            BMCWEB_LOG_DEBUG("getInventoryItemAssociations respHandler exit");
        });

    BMCWEB_LOG_DEBUG("getInventoryItemAssociations exit");
}

/**
 * @brief Gets D-Bus data for inventory item leds associated with sensors.
 *
 * Uses the specified connections (services) to obtain D-Bus data for inventory
 * item leds associated with sensors.  Stores the resulting data in the
 * inventoryItems vector.
 *
 * This data is later used to provide sensor property values in the JSON
 * response.
 *
 * Finds the inventory item led data asynchronously.  Invokes callback when data
 * has been obtained.
 *
 * The callback must have the following signature:
 *   @code
 *   callback()
 *   @endcode
 *
 * This function is called recursively, obtaining data asynchronously from one
 * connection in each call.  This ensures the callback is not invoked until the
 * last asynchronous function has completed.
 *
 * @param sensorsAsyncResp Pointer to object holding response data.
 * @param inventoryItems D-Bus inventory items associated with sensors.
 * @param ledConnections Connections that provide data for the inventory leds.
 * @param callback Callback to invoke when inventory data has been obtained.
 * @param ledConnectionsIndex Current index in ledConnections.  Only specified
 * in recursive calls to this function.
 */
template <typename Callback>
void getInventoryLedData(
    const std::shared_ptr<SensorsAsyncResp>& sensorsAsyncResp,
    const std::shared_ptr<std::vector<InventoryItem>>& inventoryItems,
    const std::shared_ptr<std::map<std::string, std::string>>& ledConnections,
    Callback&& callback, size_t ledConnectionsIndex = 0)
{
    BMCWEB_LOG_DEBUG("getInventoryLedData enter");

    // If no more connections left, call callback
    if (ledConnectionsIndex >= ledConnections->size())
    {
        callback();
        BMCWEB_LOG_DEBUG("getInventoryLedData exit");
        return;
    }

    // Get inventory item data from current connection
    auto it = ledConnections->begin();
    std::advance(it, ledConnectionsIndex);
    if (it != ledConnections->end())
    {
        const std::string& ledPath = (*it).first;
        const std::string& ledConnection = (*it).second;
        // Response handler for Get State property
        auto respHandler =
            [sensorsAsyncResp, inventoryItems, ledConnections, ledPath,
             callback = std::forward<Callback>(callback),
             ledConnectionsIndex](const boost::system::error_code& ec,
                                  const std::string& state) mutable {
                BMCWEB_LOG_DEBUG("getInventoryLedData respHandler enter");
                if (ec)
                {
                    BMCWEB_LOG_ERROR(
                        "getInventoryLedData respHandler DBus error {}", ec);
                    messages::internalError(sensorsAsyncResp->asyncResp->res);
                    return;
                }

                BMCWEB_LOG_DEBUG("Led state: {}", state);
                // Find inventory item with this LED object path
                InventoryItem* inventoryItem =
                    findInventoryItemForLed(*inventoryItems, ledPath);
                if (inventoryItem != nullptr)
                {
                    // Store LED state in InventoryItem
                    if (state.ends_with("On"))
                    {
                        inventoryItem->ledState = sensor_utils::LedState::ON;
                    }
                    else if (state.ends_with("Blink"))
                    {
                        inventoryItem->ledState = sensor_utils::LedState::BLINK;
                    }
                    else if (state.ends_with("Off"))
                    {
                        inventoryItem->ledState = sensor_utils::LedState::OFF;
                    }
                    else
                    {
                        inventoryItem->ledState =
                            sensor_utils::LedState::UNKNOWN;
                    }
                }

                // Recurse to get LED data from next connection
                getInventoryLedData(sensorsAsyncResp, inventoryItems,
                                    ledConnections, std::move(callback),
                                    ledConnectionsIndex + 1);

                BMCWEB_LOG_DEBUG("getInventoryLedData respHandler exit");
            };

        // Get the State property for the current LED
        dbus::utility::getProperty<std::string>(
            ledConnection, ledPath, "xyz.openbmc_project.Led.Physical", "State",
            std::move(respHandler));
    }

    BMCWEB_LOG_DEBUG("getInventoryLedData exit");
}

/**
 * @brief Gets LED data for LEDs associated with given inventory items.
 *
 * Gets the D-Bus connections (services) that provide LED data for the LEDs
 * associated with the specified inventory items.  Then gets the LED data from
 * each connection and stores it in the inventory item.
 *
 * This data is later used to provide sensor property values in the JSON
 * response.
 *
 * Finds the LED data asynchronously.  Invokes callback when information has
 * been obtained.
 *
 * The callback must have the following signature:
 *   @code
 *   callback()
 *   @endcode
 *
 * @param sensorsAsyncResp Pointer to object holding response data.
 * @param inventoryItems D-Bus inventory items associated with sensors.
 * @param callback Callback to invoke when inventory items have been obtained.
 */
template <typename Callback>
void getInventoryLeds(
    const std::shared_ptr<SensorsAsyncResp>& sensorsAsyncResp,
    const std::shared_ptr<std::vector<InventoryItem>>& inventoryItems,
    Callback&& callback)
{
    BMCWEB_LOG_DEBUG("getInventoryLeds enter");

    const std::string path = "/xyz/openbmc_project";
    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Led.Physical"};

    // Make call to ObjectMapper to find all inventory items
    dbus::utility::getSubTree(
        path, 0, interfaces,
        [callback = std::forward<Callback>(callback), sensorsAsyncResp,
         inventoryItems](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreeResponse& subtree) mutable {
            // Response handler for parsing output from GetSubTree
            BMCWEB_LOG_DEBUG("getInventoryLeds respHandler enter");
            if (ec)
            {
                messages::internalError(sensorsAsyncResp->asyncResp->res);
                BMCWEB_LOG_ERROR("getInventoryLeds respHandler DBus error {}",
                                 ec);
                return;
            }

            // Build map of LED object paths to connections
            std::shared_ptr<std::map<std::string, std::string>> ledConnections =
                std::make_shared<std::map<std::string, std::string>>();

            // Loop through objects from GetSubTree
            for (const std::pair<std::string,
                                 std::vector<std::pair<
                                     std::string, std::vector<std::string>>>>&
                     object : subtree)
            {
                // Check if object path is LED for one of the specified
                // inventory items
                const std::string& ledPath = object.first;
                if (findInventoryItemForLed(*inventoryItems, ledPath) !=
                    nullptr)
                {
                    // Add mapping from ledPath to connection
                    const std::string& connection =
                        object.second.begin()->first;
                    (*ledConnections)[ledPath] = connection;
                    BMCWEB_LOG_DEBUG("Added mapping {} -> {}", ledPath,
                                     connection);
                }
            }

            getInventoryLedData(sensorsAsyncResp, inventoryItems,
                                ledConnections, std::move(callback));
            BMCWEB_LOG_DEBUG("getInventoryLeds respHandler exit");
        });
    BMCWEB_LOG_DEBUG("getInventoryLeds exit");
}

/**
 * @brief Gets D-Bus data for Power Supply Attributes such as EfficiencyPercent
 *
 * Uses the specified connections (services) (currently assumes just one) to
 * obtain D-Bus data for Power Supply Attributes. Stores the resulting data in
 * the inventoryItems vector. Only stores data in Power Supply inventoryItems.
 *
 * This data is later used to provide sensor property values in the JSON
 * response.
 *
 * Finds the Power Supply Attributes data asynchronously.  Invokes callback
 * when data has been obtained.
 *
 * The callback must have the following signature:
 *   @code
 *   callback(std::shared_ptr<std::vector<InventoryItem>> inventoryItems)
 *   @endcode
 *
 * @param sensorsAsyncResp Pointer to object holding response data.
 * @param inventoryItems D-Bus inventory items associated with sensors.
 * @param psAttributesConnections Connections that provide data for the Power
 *        Supply Attributes
 * @param callback Callback to invoke when data has been obtained.
 */
template <typename Callback>
void getPowerSupplyAttributesData(
    const std::shared_ptr<SensorsAsyncResp>& sensorsAsyncResp,
    const std::shared_ptr<std::vector<InventoryItem>>& inventoryItems,
    const std::map<std::string, std::string>& psAttributesConnections,
    Callback&& callback)
{
    BMCWEB_LOG_DEBUG("getPowerSupplyAttributesData enter");

    if (psAttributesConnections.empty())
    {
        BMCWEB_LOG_DEBUG("Can't find PowerSupplyAttributes, no connections!");
        callback(inventoryItems);
        return;
    }

    // Assuming just one connection (service) for now
    auto it = psAttributesConnections.begin();

    const std::string& psAttributesPath = (*it).first;
    const std::string& psAttributesConnection = (*it).second;

    // Response handler for Get DeratingFactor property
    auto respHandler = [sensorsAsyncResp, inventoryItems,
                        callback = std::forward<Callback>(callback)](
                           const boost::system::error_code& ec,
                           uint32_t value) mutable {
        BMCWEB_LOG_DEBUG("getPowerSupplyAttributesData respHandler enter");
        if (ec)
        {
            BMCWEB_LOG_ERROR(
                "getPowerSupplyAttributesData respHandler DBus error {}", ec);
            messages::internalError(sensorsAsyncResp->asyncResp->res);
            return;
        }

        BMCWEB_LOG_DEBUG("PS EfficiencyPercent value: {}", value);
        // Store value in Power Supply Inventory Items
        for (InventoryItem& inventoryItem : *inventoryItems)
        {
            if (inventoryItem.isPowerSupply)
            {
                inventoryItem.powerSupplyEfficiencyPercent =
                    static_cast<int>(value);
            }
        }

        BMCWEB_LOG_DEBUG("getPowerSupplyAttributesData respHandler exit");
        callback(inventoryItems);
    };

    // Get the DeratingFactor property for the PowerSupplyAttributes
    // Currently only property on the interface/only one we care about
    dbus::utility::getProperty<uint32_t>(
        psAttributesConnection, psAttributesPath,
        "xyz.openbmc_project.Control.PowerSupplyAttributes", "DeratingFactor",
        std::move(respHandler));

    BMCWEB_LOG_DEBUG("getPowerSupplyAttributesData exit");
}

/**
 * @brief Gets the Power Supply Attributes such as EfficiencyPercent
 *
 * Gets the D-Bus connection (service) that provides Power Supply Attributes
 * data. Then gets the Power Supply Attributes data from the connection
 * (currently just assumes 1 connection) and stores the data in the inventory
 * item.
 *
 * This data is later used to provide sensor property values in the JSON
 * response. DeratingFactor on D-Bus is mapped to EfficiencyPercent on Redfish.
 *
 * Finds the Power Supply Attributes data asynchronously. Invokes callback
 * when information has been obtained.
 *
 * The callback must have the following signature:
 *   @code
 *   callback(std::shared_ptr<std::vector<InventoryItem>> inventoryItems)
 *   @endcode
 *
 * @param sensorsAsyncResp Pointer to object holding response data.
 * @param inventoryItems D-Bus inventory items associated with sensors.
 * @param callback Callback to invoke when data has been obtained.
 */
template <typename Callback>
void getPowerSupplyAttributes(
    const std::shared_ptr<SensorsAsyncResp>& sensorsAsyncResp,
    const std::shared_ptr<std::vector<InventoryItem>>& inventoryItems,
    Callback&& callback)
{
    BMCWEB_LOG_DEBUG("getPowerSupplyAttributes enter");

    // Only need the power supply attributes when the Power Schema
    if (sensorsAsyncResp->chassisSubNode != sensors::powerNodeStr)
    {
        BMCWEB_LOG_DEBUG("getPowerSupplyAttributes exit since not Power");
        callback(inventoryItems);
        return;
    }

    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Control.PowerSupplyAttributes"};

    // Make call to ObjectMapper to find the PowerSupplyAttributes service
    dbus::utility::getSubTree(
        "/xyz/openbmc_project", 0, interfaces,
        [callback = std::forward<Callback>(callback), sensorsAsyncResp,
         inventoryItems](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreeResponse& subtree) mutable {
            // Response handler for parsing output from GetSubTree
            BMCWEB_LOG_DEBUG("getPowerSupplyAttributes respHandler enter");
            if (ec)
            {
                messages::internalError(sensorsAsyncResp->asyncResp->res);
                BMCWEB_LOG_ERROR(
                    "getPowerSupplyAttributes respHandler DBus error {}", ec);
                return;
            }
            if (subtree.empty())
            {
                BMCWEB_LOG_DEBUG("Can't find Power Supply Attributes!");
                callback(inventoryItems);
                return;
            }

            // Currently we only support 1 power supply attribute, use this for
            // all the power supplies. Build map of object path to connection.
            // Assume just 1 connection and 1 path for now.
            std::map<std::string, std::string> psAttributesConnections;

            if (subtree[0].first.empty() || subtree[0].second.empty())
            {
                BMCWEB_LOG_DEBUG("Power Supply Attributes mapper error!");
                callback(inventoryItems);
                return;
            }

            const std::string& psAttributesPath = subtree[0].first;
            const std::string& connection = subtree[0].second.begin()->first;

            if (connection.empty())
            {
                BMCWEB_LOG_DEBUG("Power Supply Attributes mapper error!");
                callback(inventoryItems);
                return;
            }

            psAttributesConnections[psAttributesPath] = connection;
            BMCWEB_LOG_DEBUG("Added mapping {} -> {}", psAttributesPath,
                             connection);

            getPowerSupplyAttributesData(sensorsAsyncResp, inventoryItems,
                                         psAttributesConnections,
                                         std::move(callback));
            BMCWEB_LOG_DEBUG("getPowerSupplyAttributes respHandler exit");
        });
    BMCWEB_LOG_DEBUG("getPowerSupplyAttributes exit");
}

/**
 * @brief Retrieves mapping of Redfish URIs to sensor value property to D-Bus
 * path of the sensor.
 *
 * Function builds valid Redfish response for sensor query of given chassis and
 * node. It then builds metadata about Redfish<->D-Bus correlations and provides
 * it to caller in a callback.
 *
 * @param chassis   Chassis for which retrieval should be performed
 * @param node  Node (group) of sensors. See sensor_utils::node for supported
 * values
 * @param mapComplete   Callback to be called with retrieval result
 */
template <typename Callback>
inline void retrieveUriToDbusMap(
    const std::string& chassis, const std::string& node, Callback&& mapComplete)
{
    decltype(sensors::paths)::const_iterator pathIt =
        std::find_if(sensors::paths.cbegin(), sensors::paths.cend(),
                     [&node](auto&& val) { return val.first == node; });
    if (pathIt == sensors::paths.cend())
    {
        BMCWEB_LOG_ERROR("Wrong node provided : {}", node);
        std::map<std::string, std::string> noop;
        mapComplete(boost::beast::http::status::bad_request, noop);
        return;
    }

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    auto callback =
        [asyncResp, mapCompleteCb = std::forward<Callback>(mapComplete)](
            const boost::beast::http::status status,
            const std::map<std::string, std::string>& uriToDbus) {
            mapCompleteCb(status, uriToDbus);
        };

    auto resp = std::make_shared<SensorsAsyncResp>(
        asyncResp, chassis, pathIt->second, node, std::move(callback));
    getChassisData(resp);
}

namespace sensors
{

inline void getChassisCallback(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    std::string_view chassisId, std::string_view chassisSubNode,
    const std::shared_ptr<std::set<std::string>>& sensorNames)
{
    BMCWEB_LOG_DEBUG("getChassisCallback enter ");

    nlohmann::json& entriesArray = asyncResp->res.jsonValue["Members"];
    for (const std::string& sensor : *sensorNames)
    {
        BMCWEB_LOG_DEBUG("Adding sensor: {}", sensor);

        sdbusplus::message::object_path path(sensor);
        std::string sensorName = path.filename();
        if (sensorName.empty())
        {
            BMCWEB_LOG_ERROR("Invalid sensor path: {}", sensor);
            messages::internalError(asyncResp->res);
            return;
        }
        std::string type = path.parent_path().filename();
        std::string id = redfish::sensor_utils::getSensorId(sensorName, type);

        nlohmann::json::object_t member;
        member["@odata.id"] = boost::urls::format(
            "/redfish/v1/Chassis/{}/{}/{}", chassisId, chassisSubNode, id);

        entriesArray.emplace_back(std::move(member));
    }

    asyncResp->res.jsonValue["Members@odata.count"] = entriesArray.size();
    BMCWEB_LOG_DEBUG("getChassisCallback exit");
}

inline void handleSensorCollectionGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId)
{
    query_param::QueryCapabilities capabilities = {
        .canDelegateExpandLevel = 1,
    };
    query_param::Query delegatedQuery;
    if (!redfish::setUpRedfishRouteWithDelegation(app, req, asyncResp,
                                                  delegatedQuery, capabilities))
    {
        return;
    }

    if (delegatedQuery.expandType != query_param::ExpandType::None)
    {
        // we perform efficient expand.
        auto sensorsAsyncResp = std::make_shared<SensorsAsyncResp>(
            asyncResp, chassisId, sensors::dbus::sensorPaths,
            sensors::sensorsNodeStr,
            /*efficientExpand=*/true);
        getChassisData(sensorsAsyncResp);

        BMCWEB_LOG_DEBUG(
            "SensorCollection doGet exit via efficient expand handler");
        return;
    }

    // We get all sensors as hyperlinkes in the chassis (this
    // implies we reply on the default query parameters handler)
    getChassis(asyncResp, chassisId, sensors::sensorsNodeStr, dbus::sensorPaths,
               std::bind_front(sensors::getChassisCallback, asyncResp,
                               chassisId, sensors::sensorsNodeStr));
}

inline void getSensorFromDbus(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& sensorPath,
    const ::dbus::utility::MapperGetObject& mapperResponse)
{
    if (mapperResponse.size() != 1)
    {
        messages::internalError(asyncResp->res);
        return;
    }
    const auto& valueIface = *mapperResponse.begin();
    const std::string& connectionName = valueIface.first;
    BMCWEB_LOG_DEBUG("Looking up {}", connectionName);
    BMCWEB_LOG_DEBUG("Path {}", sensorPath);

    ::dbus::utility::getAllProperties(
        *crow::connections::systemBus, connectionName, sensorPath, "",
        [asyncResp,
         sensorPath](const boost::system::error_code& ec,
                     const ::dbus::utility::DBusPropertiesMap& valuesDict) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            sdbusplus::message::object_path path(sensorPath);
            std::string name = path.filename();
            path = path.parent_path();
            std::string type = path.filename();
            sensor_utils::objectPropertiesToJson(
                name, type, sensor_utils::ChassisSubNode::sensorsNode,
                valuesDict, asyncResp->res.jsonValue, nullptr);
        });
}

inline void handleSensorGet(App& app, const crow::Request& req,
                            const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& chassisId,
                            const std::string& sensorId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    std::pair<std::string, std::string> nameType =
        redfish::sensor_utils::splitSensorNameAndType(sensorId);
    if (nameType.first.empty() || nameType.second.empty())
    {
        messages::resourceNotFound(asyncResp->res, sensorId, "Sensor");
        return;
    }

    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/Sensors/{}", chassisId, sensorId);

    BMCWEB_LOG_DEBUG("Sensor doGet enter");

    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Sensor.Value"};
    std::string sensorPath = "/xyz/openbmc_project/sensors/" + nameType.first +
                             '/' + nameType.second;
    // Get a list of all of the sensors that implement Sensor.Value
    // and get the path and service name associated with the sensor
    ::dbus::utility::getDbusObject(
        sensorPath, interfaces,
        [asyncResp, sensorId,
         sensorPath](const boost::system::error_code& ec,
                     const ::dbus::utility::MapperGetObject& subtree) {
            BMCWEB_LOG_DEBUG("respHandler1 enter");
            if (ec == boost::system::errc::io_error)
            {
                BMCWEB_LOG_WARNING("Sensor not found from getSensorPaths");
                messages::resourceNotFound(asyncResp->res, sensorId, "Sensor");
                return;
            }
            if (ec)
            {
                messages::internalError(asyncResp->res);
                BMCWEB_LOG_ERROR(
                    "Sensor getSensorPaths resp_handler: Dbus error {}", ec);
                return;
            }
            getSensorFromDbus(asyncResp, sensorPath, subtree);
            BMCWEB_LOG_DEBUG("respHandler1 exit");
        });
}

} // namespace sensors

inline void requestRoutesSensorCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Sensors/")
        .privileges(redfish::privileges::getSensorCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(sensors::handleSensorCollectionGet, std::ref(app)));
}

inline void requestRoutesSensor(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Sensors/<str>/")
        .privileges(redfish::privileges::getSensor)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(sensors::handleSensorGet, std::ref(app)));
}

} // namespace redfish
