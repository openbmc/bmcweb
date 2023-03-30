#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/pcie_device.hpp"
#include "generated/enums/pcie_slots.hpp"
#include "http/utility.hpp"
#include "utils/collection.hpp"

#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace redfish
{
namespace pcie_util
{

inline bool checkPCIeSlotDevicePath(
    const std::string& pcieDevicePath,
    const dbus::utility::MapperGetSubTreePathsResponse& pcieDevicePaths)
{
    return std::ranges::find(pcieDevicePaths, pcieDevicePath) !=
           pcieDevicePaths.end();
}

inline void addEmptyPCIeSlot(
    const dbus::utility::MapperEndPoints& endpoints,
    const dbus::utility::MapperGetSubTreePathsResponse& pcieDevicePaths,
    const std::string& pcieSlotName, nlohmann::json& pcieDeviceList)
{
    bool found = false;
    /* There may be multiple different objects with the same
     * association path. Walk through each object and check if the
     * PCIeSlot contains a PCIe Device.*/
    for (const auto& endpoint : endpoints)
    {
        if (checkPCIeSlotDevicePath(endpoint, pcieDevicePaths))
        {
            found = true;
            break;
        }
    }
    /* If the PCIeSlot doesn't contain any PCIeDevice then add the
     * PCIeSlot to pcieDevice list */
    if (!found)
    {
        nlohmann::json::object_t pcieDevice;
        pcieDevice["@odata.id"] = boost::urls::format(
            "/redfish/v1/Systems/system/PCIeDevices/{}", pcieSlotName);
        pcieDeviceList.push_back(std::move(pcieDevice));
    }
}

inline void handleGetEmptyPCIeSlots(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const dbus::utility::MapperGetSubTreePathsResponse& pcieDevicePaths,
    const dbus::utility::MapperGetSubTreePathsResponse& pcieSlotPaths,
    const std::string& name)
{
    nlohmann::json& pcieDeviceList = asyncResp->res.jsonValue[name];
    for (const std::string& pcieSlotPath : pcieSlotPaths)
    {
        std::string slotName =
            sdbusplus::message::object_path(pcieSlotPath).filename();
        if (slotName.empty())
        {
            continue;
        }

        std::string associationPath = pcieSlotPath + "/containing";

        dbus::utility::getAssociationEndPoints(
            associationPath,
            [asyncResp, pcieDevicePaths, slotName,
             &pcieDeviceList](const boost::system::error_code& ec,
                              const dbus::utility::MapperEndPoints& endpoints) {
            if (ec)
            {
                if (ec.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR(
                        "DBUS response error for getAssociationEndPoints: {}",
                        ec.value());
                    messages::internalError(asyncResp->res);
                    return;
                }
                /* No association endpoints indicate an empty PCIeSlot. */
            }

            addEmptyPCIeSlot(endpoints, pcieDevicePaths, slotName,
                             pcieDeviceList);
            });
    }
    asyncResp->res.jsonValue[name + "@odata.count"] = pcieDeviceList.size();
}

inline void getEmptyPCIeSlots(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const dbus::utility::MapperGetSubTreePathsResponse& pcieDevicePaths,
    const std::string& name)
{
    constexpr std::array<std::string_view, 1> pcieSlotInterface = {
        "xyz.openbmc_project.Inventory.Item.PCIeSlot"};
    dbus::utility::getSubTreePaths(
        "/xyz/openbmc_project/inventory", 0, pcieSlotInterface,
        [asyncResp, pcieDevicePaths, name](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreePathsResponse& pcieSlotPaths) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG("no PCIe Slot paths found ec: {}", ec.value());
            return;
        }
        handleGetEmptyPCIeSlots(asyncResp, pcieDevicePaths, pcieSlotPaths,
                                name);
        });
}

/**
 * @brief Populate the PCIe Device list from a GetSubTreePaths search of
 *        inventory
 *
 * @param[i,o] asyncResp  Async response object
 * @param[i]   Name   Key to store the list of PCIe devices in asyncResp
 *
 * @return void
 */

inline void
    getPCIeDeviceList(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& name)
{
    static constexpr std::array<std::string_view, 1> pcieDeviceInterface = {
        "xyz.openbmc_project.Inventory.Item.PCIeDevice"};
    collection_util::getCollectionMembersArray(
        asyncResp, boost::urls::url("/redfish/v1/Systems/system/PCIeDevices"),
        pcieDeviceInterface, name, "/xyz/openbmc_project/inventory");

    dbus::utility::getSubTreePaths(
        "/xyz/openbmc_project/inventory", 0, pcieDeviceInterface,
        [asyncResp, name](const boost::system::error_code& ec,
                          const dbus::utility::MapperGetSubTreePathsResponse&
                              pcieDevicePaths) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG("no PCIe device paths found ec: {}", ec.message());
            // Not an error, system just doesn't have PCIe info
        }
        getEmptyPCIeSlots(asyncResp, pcieDevicePaths, name);
        });
}

inline std::optional<pcie_slots::SlotTypes>
    dbusSlotTypeToRf(const std::string& slotType)
{
    if (slotType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.FullLength")
    {
        return pcie_slots::SlotTypes::FullLength;
    }
    if (slotType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.HalfLength")
    {
        return pcie_slots::SlotTypes::HalfLength;
    }
    if (slotType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.LowProfile")
    {
        return pcie_slots::SlotTypes::LowProfile;
    }
    if (slotType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.Mini")
    {
        return pcie_slots::SlotTypes::Mini;
    }
    if (slotType == "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.M_2")
    {
        return pcie_slots::SlotTypes::M2;
    }
    if (slotType == "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.OEM")
    {
        return pcie_slots::SlotTypes::OEM;
    }
    if (slotType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.OCP3Small")
    {
        return pcie_slots::SlotTypes::OCP3Small;
    }
    if (slotType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.OCP3Large")
    {
        return pcie_slots::SlotTypes::OCP3Large;
    }
    if (slotType == "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.U_2")
    {
        return pcie_slots::SlotTypes::U2;
    }
    if (slotType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.Unknown")
    {
        return std::nullopt;
    }

    return pcie_slots::SlotTypes::Invalid;
}

inline std::optional<pcie_device::PCIeTypes>
    redfishPcieGenerationFromDbus(const std::string& generationInUse)
{
    if (generationInUse ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Gen1")
    {
        return pcie_device::PCIeTypes::Gen1;
    }
    if (generationInUse ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Gen2")
    {
        return pcie_device::PCIeTypes::Gen2;
    }
    if (generationInUse ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Gen3")
    {
        return pcie_device::PCIeTypes::Gen3;
    }
    if (generationInUse ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Gen4")
    {
        return pcie_device::PCIeTypes::Gen4;
    }
    if (generationInUse ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Gen5")
    {
        return pcie_device::PCIeTypes::Gen5;
    }
    if (generationInUse.empty() ||
        generationInUse ==
            "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Unknown")
    {
        return std::nullopt;
    }

    return pcie_device::PCIeTypes::Invalid;
}

} // namespace pcie_util
} // namespace redfish
