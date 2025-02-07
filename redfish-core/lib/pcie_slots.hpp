#pragma once

#include "app.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/pcie_slots.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utility.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"
#include "utils/pcie_util.hpp"

#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <string_view>

namespace redfish
{

inline void
    processPcieSlot(const dbus::utility::DBusInterfacesMap& interfacesList,
                    nlohmann::json& slots)
{
    nlohmann::json slot = nlohmann::json::object();
    bool slotHasData = false;

    for (const auto& [interface, properties] : interfacesList)
    {
        if (interface == "xyz.openbmc_project.Inventory.Item.PCIeSlot")
        {
            auto it = std::find_if(
                properties.begin(), properties.end(),
                [](const auto& pair) { return pair.first == "Generation"; });
            if (it != properties.end())
            {
                const std::string* generation =
                    std::get_if<std::string>(&it->second);
                if (generation)
                {
                    std::optional<pcie_device::PCIeTypes> pcieType =
                        pcie_util::redfishPcieGenerationFromDbus(*generation);
                    if (pcieType &&
                        *pcieType != pcie_device::PCIeTypes::Invalid)
                    {
                        slot["PCIeType"] = *pcieType;
                        slotHasData = true;
                    }
                }
            }

            it = std::find_if(properties.begin(), properties.end(),
                              [](const auto& pair) {
                                  return pair.first == "Lanes";
                              });
            if (it != properties.end())
            {
                const size_t* lanes = std::get_if<size_t>(&it->second);
                if (lanes)
                {
                    slot["Lanes"] = *lanes;
                    slotHasData = true;
                }
            }

            it = std::find_if(properties.begin(), properties.end(),
                              [](const auto& pair) {
                                  return pair.first == "SlotType";
                              });
            if (it != properties.end())
            {
                const std::string* slotType =
                    std::get_if<std::string>(&it->second);
                if (slotType)
                {
                    std::optional<pcie_slots::SlotTypes> redfishSlotType =
                        pcie_util::dbusSlotTypeToRf(*slotType);
                    if (redfishSlotType &&
                        *redfishSlotType != pcie_slots::SlotTypes::Invalid)
                    {
                        slot["SlotType"] = *redfishSlotType;
                        slotHasData = true;
                    }
                }
            }
        }
        else if (interface ==
                 "xyz.openbmc_project.Inventory.Decorator.LocationCode")
        {
            auto it = std::find_if(
                properties.begin(), properties.end(),
                [](const auto& pair) { return pair.first == "LocationCode"; });
            if (it != properties.end())
            {
                const std::string* locationCode =
                    std::get_if<std::string>(&it->second);
                if (locationCode)
                {
                    slot["Location"]["PartLocation"]["ServiceLabel"] =
                        *locationCode;
                    slotHasData = true;
                }
            }
        }
        else if (interface == "xyz.openbmc_project.Inventory.Item")
        {
            auto it = std::find_if(
                properties.begin(), properties.end(),
                [](const auto& pair) { return pair.first == "Present"; });
            if (it != properties.end())
            {
                const bool* present = std::get_if<bool>(&it->second);
                if (present)
                {
                    slot["Status"]["State"] = *present ? "Enabled" : "Absent";
                    slotHasData = true;
                }
            }
        }
    }

    if (slotHasData)
    {
        slots.push_back(slot);
    }
}

inline void onMapperAssociationDone(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisID, const std::string& pcieSlotPath,
    const std::string& connectionName, const boost::system::error_code& ec,
    const dbus::utility::MapperEndPoints& pcieSlotChassis)
{
    if (ec)
    {
        if (ec.value() == EBADR)
        {
            // This PCIeSlot have no chassis association.
            return;
        }
        BMCWEB_LOG_ERROR("DBUS response error");
        messages::internalError(asyncResp->res);
        return;
    }

    if (pcieSlotChassis.size() != 1)
    {
        BMCWEB_LOG_ERROR("PCIe Slot association error! ");
        messages::internalError(asyncResp->res);
        return;
    }

    sdbusplus::message::object_path path(pcieSlotChassis[0]);
    std::string chassisName = path.filename();
    if (chassisName != chassisID)
    {
        // The pcie slot doesn't belong to the chassisID
        return;
    }

    sdbusplus::message::object_path invPath("/xyz/openbmc_project/inventory");
    dbus::utility::getManagedObjects(
        connectionName, invPath,
        [asyncResp, connectionName,
         pcieSlotPath](const boost::system::error_code& ec1,
                       const dbus::utility::ManagedObjectType& pcieSlotObjs) {
            if (ec1)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            auto& slots = asyncResp->res.jsonValue["Slots"];

            const auto pcieSlotIt = std::ranges::find_if(
                pcieSlotObjs,
                [pcieSlotPath](
                    const std::pair<sdbusplus::message::object_path,
                                    dbus::utility::DBusInterfacesMap>&
                        pcieSlot) { return pcieSlotPath == pcieSlot.first; });

            if (pcieSlotIt == pcieSlotObjs.end())
            {
                messages::resourceNotFound(asyncResp->res, "PCIeSlots",
                                           "PCIeSlots_To_Be_Changed");
                return;
            }

            processPcieSlot(pcieSlotIt->second, slots);
        });
}

inline void onMapperSubtreeDone(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisID, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("D-Bus response error on GetSubTree {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }
    if (subtree.empty())
    {
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisID);
        return;
    }

    BMCWEB_LOG_DEBUG("Get properties for PCIeSlots associated to chassis = {}",
                     chassisID);

    asyncResp->res.jsonValue["@odata.type"] = "#PCIeSlots.v1_4_1.PCIeSlots";
    asyncResp->res.jsonValue["Name"] = "PCIe Slot Information";
    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Chassis/{}/PCIeSlots", chassisID);
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
            dbus::utility::getAssociationEndPoints(
                std::string{pcieSlotAssociationPath},
                [asyncResp, chassisID, pcieSlotPath, connectionName](
                    const boost::system::error_code& ec2,
                    const dbus::utility::MapperEndPoints& endpoints) {
                    onMapperAssociationDone(asyncResp, chassisID, pcieSlotPath,
                                            connectionName, ec2, endpoints);
                });
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
