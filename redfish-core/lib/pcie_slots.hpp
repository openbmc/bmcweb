#pragma once

#include "error_messages.hpp"
#include "utility.hpp"

#include <app.hpp>
#include <pcie.hpp>
#include <registries/privilege_registry.hpp>
#include <utils/json_utils.hpp>

namespace redfish
{

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

    for (const auto& property : propertiesList)
    {
        const std::string& propertyName = property.first;

        if (propertyName == "Generation")
        {
            const std::string* value =
                std::get_if<std::string>(&property.second);
            if (value == nullptr)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            std::optional<std::string> pcieType =
                redfishPcieGenerationFromDbus(*value);
            if (!pcieType)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            slot["PCIeType"] = !pcieType;
        }
        else if (propertyName == "Lanes")
        {
            const size_t* value = std::get_if<size_t>(&property.second);
            if (value == nullptr)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            slot["Lanes"] = *value;
        }
        else if (propertyName == "SlotType")
        {
            const std::string* value =
                std::get_if<std::string>(&property.second);
            if (value == nullptr)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            std::string slotType = dbusSlotTypeToRf(*value);
            if (!slotType.empty())
            {
                messages::internalError(asyncResp->res);
                return;
            }
            slot["SlotType"] = slotType;
        }
        else if (propertyName == "HotPluggable")
        {
            const bool* value = std::get_if<bool>(&property.second);
            if (value == nullptr)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            slot["HotPluggable"] = *value;
        }
    }
    slots.emplace_back(std::move(slot));
}

inline void onMapperAssociationDone(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisID, const std::string& pcieSlotPath,
    const std::string& connectionName, const boost::system::error_code ec,
    const std::variant<std::vector<std::string>>& endpoints)
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

    crow::connections::DBusSingleton::systemBus().async_method_call(
        [asyncResp](const boost::system::error_code ec,
                    const dbus::utility::DBusPropertiesMap& propertiesList) {
        onPcieSlotGetAllDone(asyncResp, ec, propertiesList);
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
        messages::resourceNotFound(asyncResp->res, "#Chassis", chassisID);
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
            crow::connections::DBusSingleton::systemBus().async_method_call(
                [asyncResp, chassisID, pcieSlotPath, connectionName](
                    const boost::system::error_code ec,
                    const std::variant<std::vector<std::string>>& endpoints) {
                onMapperAssociationDone(asyncResp, chassisID, pcieSlotPath,
                                        connectionName, ec, endpoints);
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

    crow::connections::DBusSingleton::systemBus().async_method_call(
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
