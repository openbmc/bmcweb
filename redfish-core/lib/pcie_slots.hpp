#pragma once

#include "error_messages.hpp"
#include "utility.hpp"

#include <app.hpp>
#include <pcie.hpp>
#include <registries/privilege_registry.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/unpack_properties.hpp>
#include <utils/dbus_utils.hpp>
#include <utils/json_utils.hpp>

namespace redfish
{

inline void
    addPresenceStatus(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& connectionName,
                      const std::string& pcieSlotPath, size_t index)
{

    crow::connections::systemBus->async_method_call(
        [asyncResp, index](const boost::system::error_code ec,
                           const std::variant<bool>& property) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "DBUS response error";
            messages::internalError(asyncResp->res);
            return;
        }
        const bool* value = std::get_if<bool>(&property);
        if (value == nullptr)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        asyncResp->res.jsonValue["Slots"][index]["Status"]["State"] =
            *value ? "Enabled" : "Absent";
        },
        connectionName, pcieSlotPath, "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Inventory.Item", "Present");
}

inline void addLocation(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& connectionName,
                        const std::string& pcieSlotPath, size_t index)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, index](const boost::system::error_code ec,
                           const std::variant<std::string>& property) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "DBUS response error";
            messages::internalError(asyncResp->res);
            return;
        }
        const std::string* value = std::get_if<std::string>(&property);
        if (value == nullptr)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        asyncResp->res.jsonValue["Slots"][index]["Location"]["PartLocation"]
                                ["ServiceLabel"] = *value;
        },
        connectionName, pcieSlotPath, "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Inventory.Decorator.LocationCode", "LocationCode");
}

inline std::string dbusSlotTypeToRf(const std::string& slotType)
{
    if (slotType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.FullLength")
    {
        return "FullLength";
    }
    if (slotType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.HalfLength")
    {
        return "HalfLength";
    }
    if (slotType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.LowProfile")
    {
        return "LowProfile";
    }
    if (slotType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.Mini")
    {
        return "Mini";
    }
    if (slotType == "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.M_2")
    {
        return "M2";
    }
    if (slotType == "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.OEM")
    {
        return "OEM";
    }
    if (slotType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.OCP3Small")
    {
        return "OCP3Small";
    }
    if (slotType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.OCP3Large")
    {
        return "OCP3Large";
    }
    if (slotType == "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.U_2")
    {
        return "U2";
    }

    // Unknown or others
    return "";
}

inline void
    onPcieSlotGetAllDone(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const boost::system::error_code ec,
                         const dbus::utility::DBusPropertiesMap& propertiesList)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR << "Can't get PCIeSlot properties!";
        messages::internalError(asyncResp->res);
        return;
    }

    nlohmann::json& slots = asyncResp->res.jsonValue["Slots"];

    nlohmann::json::array_t* slotsPtr =
        slots.get_ptr<nlohmann::json::array_t*>();
    if (slotsPtr == nullptr)
    {
        BMCWEB_LOG_ERROR << "Slots key isn't an array???";
        messages::internalError(asyncResp->res);
        return;
    }

    nlohmann::json::object_t slot;

    const std::string* generation = nullptr;
    const size_t* lanes = nullptr;
    const std::string* slotType = nullptr;
    const bool* hotPluggable = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), propertiesList, "Generation",
        generation, "Lanes", lanes, "SlotType", slotType, "HotPluggable",
        hotPluggable);

    if (!success)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    if (generation != nullptr)
    {
        std::optional<std::string> pcieType =
            redfishPcieGenerationFromDbus(*generation);
        if (!pcieType)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        slot["PCIeType"] = !pcieType;
    }

    if (lanes != nullptr)
    {

        slot["Lanes"] = *lanes;
    }

    if (slotType != nullptr)
    {
        std::string redfishSlotType = dbusSlotTypeToRf(*slotType);
        if (!slotType.empty())
        {
            messages::internalError(asyncResp->res);
            return;
        }
        slot["SlotType"] = redfishSlotType;
    }

    if (hotPluggable != nullptr)
    {
        slot["HotPluggable"] = *hotPluggable;
    }

    slots.emplace_back(std::move(slot));
}

inline void onMapperAssociationDone(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisID, const std::string& pcieSlotPath,
    const std::string& connectionName, const boost::system::error_code ec,
    const std::variant<std::vector<std::string>>& endpoints, bool findLocation,
    bool findPresence)
{
    if (ec)
    {
        if (ec.value() == EBADR)
        {
            // This PCIeSlot have no chassis association.
            return;
        }
        BMCWEB_LOG_ERROR << "DBUS response error";
        messages::internalError(asyncResp->res);
        return;
    }

    const std::vector<std::string>* pcieSlotChassis =
        std::get_if<std::vector<std::string>>(&(endpoints));

    if (pcieSlotChassis == nullptr)
    {
        BMCWEB_LOG_ERROR << "Error getting PCIe Slot association!";
        messages::internalError(asyncResp->res);
        return;
    }

    if (pcieSlotChassis->size() != 1)
    {
        BMCWEB_LOG_ERROR << "PCIe Slot association error! ";
        messages::internalError(asyncResp->res);
        return;
    }

    sdbusplus::message::object_path path((*pcieSlotChassis)[0]);
    std::string chassisName = path.filename();
    if (chassisName != chassisID)
    {
        // The pcie slot doesn't belong to the chassisID
        return;
    }

    crow::connections::systemBus->async_method_call(
        [asyncResp, connectionName, pcieSlotPath, findLocation,
         findPresence](const boost::system::error_code ec,
                       const dbus::utility::DBusPropertiesMap& propertiesList) {
        onPcieSlotGetAllDone(asyncResp, ec, propertiesList,
                             [asyncResp, connectionName, pcieSlotPath,
                              findLocation, findPresence](size_t index) {
            if (findLocation)
            {
                addLocation(asyncResp, connectionName, pcieSlotPath, index);
            }
            if (findPresence)
            {
                addPresenceStatus(asyncResp, index, connectionName,
                                  pcieSlotPath);
            }
        });
        },
        connectionName, pcieSlotPath, "org.freedesktop.DBus.Properties",
        "GetAll", "xyz.openbmc_project.Inventory.Item.PCIeSlot");
}

inline void
    onMapperSubtreeDone(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& chassisID,
                        const boost::system::error_code ec,
                        const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR << "D-Bus response error on GetSubTree " << ec;
        messages::internalError(asyncResp->res);
        return;
    }
    if (subtree.empty())
    {
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisID);
        return;
    }

    BMCWEB_LOG_DEBUG << "Get properties for PCIeSlots associated to chassis = "
                     << chassisID;

    asyncResp->res.jsonValue["@odata.type"] = "#PCIeSlots.v1_4_1.PCIeSlots";
    asyncResp->res.jsonValue["Name"] = "PCIe Slot Information";
    asyncResp->res.jsonValue["@odata.id"] = crow::utility::urlFromPieces(
        "redfish", "v1", "Chassis", chassisID, "PCIeSlots");
    asyncResp->res.jsonValue["Id"] = "1";
    asyncResp->res.jsonValue["Slots"] = nlohmann::json::array();

    constexpr std::string locationInterface =
        "xyz.openbmc_project.Inventory.Decorator."
        "LocationCode";
    const std::string itemInterface = "xyz.openbmc_project.Inventory.Item";
    for (const auto& pathServicePair : subtree)
    {
        const std::string& pcieSlotPath = pathServicePair.first;
        for (const auto& connectionInterfacePair : pathServicePair.second)
        {
            const std::string& connectionName = connectionInterfacePair.first;
            sdbusplus::message::object_path pcieSlotAssociationPath(
                pcieSlotPath);
            pcieSlotAssociationPath /= "chassis";

            // The association of this PCIeSlot is used to determine whether
            // it belongs to this ChassisID
            crow::connections::systemBus->async_method_call(
                [asyncResp, chassisID, pcieSlotPath, connectionName](
                    const boost::system::error_code ec,
                    const std::variant<std::vector<std::string>>& endpoints) {
                bool findPresence =
                    std::find(interfaceList.begin(), interfaceList.end(),
                              itemInterface) != interfaceList.end();
                bool findLocation =
                    std::find(interfaceList.begin(), interfaceList.end(),
                              locationInterface) != interfaceList.end();
                onMapperAssociationDone(asyncResp, chassisID, pcieSlotPath,
                                        connectionName, ec, endpoints,
                                        findLocation, findPresence);
                },
                "xyz.openbmc_project.ObjectMapper",
                std::string{pcieSlotAssociationPath},
                "org.freedesktop.DBus.Properties", "Get",
                "xyz.openbmc_project.Association", "endpoints");
        }
    }
}

inline void handlePCIeSlotCollectionGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisID)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    crow::connections::systemBus->async_method_call(
        [asyncResp,
         chassisID](const boost::system::error_code ec,
                    const dbus::utility::MapperGetSubTreeResponse& subtree) {
        onMapperSubtreeDone(asyncResp, chassisID, ec, subtree);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", int32_t(0),
        std::array<const char*, 1>{
            "xyz.openbmc_project.Inventory.Item.PCIeSlot"});
}

inline void requestRoutesPCIeSlots(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/PCIeSlots/")
        .privileges(redfish::privileges::getPCIeSlots)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handlePCIeSlotCollectionGet, std::ref(app)));
}

} // namespace redfish
