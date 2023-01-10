#pragma once

#include "app.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/pcie_slots.hpp"
#include "pcie.hpp"
#include "registries/privilege_registry.hpp"
#include "utility.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"

#include <boost/system/error_code.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <string_view>

namespace redfish
{

inline pcie_slot::SlotTypes dbusSlotTypeToRf(const std::string& slotType)
{
    if (slotType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.FullLength")
    {
        return pcie_slot::SlotTypes::FullLength;
    }
    if (slotType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.HalfLength")
    {
        return pcie_slot::SlotTypes::HalfLength;
    }
    if (slotType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.LowProfile")
    {
        return pcie_slot::SlotTypes::LowProfile;
    }
    if (slotType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.Mini")
    {
        return pcie_slot::SlotTypes::Mini;
    }
    if (slotType == "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.M_2")
    {
        return pcie_slot::SlotTypes::M2;
    }
    if (slotType == "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.OEM")
    {
        return pcie_slot::SlotTypes::OEM;
    }
    if (slotType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.OCP3Small")
    {
        return pcie_slot::SlotTypes::OCP3Small;
    }
    if (slotType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.OCP3Large")
    {
        return pcie_slot::SlotTypes::OCP3Large;
    }
    if (slotType == "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.U_2")
    {
        return pcie_slot::SlotTypes::U2;
    }

    // Unknown or others
    return pcie_slot::SlotTypes::Invalid;
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
        std::optional<pcie_device::PCIeTypes> pcieType =
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

    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, connectionName, pcieSlotPath,
        "xyz.openbmc_project.Inventory.Item.PCIeSlot",
        [asyncResp](const boost::system::error_code ec,
                    const dbus::utility::DBusPropertiesMap& propertiesList) {
        onPcieSlotGetAllDone(asyncResp, ec, propertiesList);
        });
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

    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Inventory.Item.PCIeSlot"};
    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [asyncResp,
         chassisID](const boost::system::error_code& ec,
                    const dbus::utility::MapperGetSubTreeResponse& subtree) {
        onMapperSubtreeDone(asyncResp, chassisID, ec, subtree);
        });
}

inline void requestRoutesPCIeSlots(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/PCIeSlots/")
        .privileges(redfish::privileges::getPCIeSlots)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handlePCIeSlotCollectionGet, std::ref(app)));
}

} // namespace redfish
