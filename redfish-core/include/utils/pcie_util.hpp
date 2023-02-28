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
#include <ranges>
#include <string>
#include <string_view>

namespace redfish
{
namespace pcie_util
{

/**
 * @brief Workaround to handle duplicate PCI device list
 *
 * retrieve PCI device endpoint information and if path is
 * ~/chassisN/io_moduleN/slotN/adapterN then, replace redfish
 * PCI device as "chassisN_io_moduleN_slotN_adapterN"
 *
 * @param[i]   fullPath  object path of PCIe device
 *
 * @return string: unique PCIe device name
 */
inline std::string buildPCIeUniquePath(const std::string& fullPath)
{
    sdbusplus::message::object_path path(fullPath);

    // Start it with leaf
    std::string devName = path.filename();

    // walk-thru the parent path upto 3 levels
    for (int depth = 0; depth < 3; depth++)
    {
        path = path.parent_path();

        std::string filename = path.filename();
        if (filename.empty())
        {
            break;
        }
        devName = std::format("{}_{}", filename, devName);
    }
    return devName;
}

inline void afterGetPCIeDeviceList(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const nlohmann::json::json_pointer& jsonKeyName,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& paths)
{
    nlohmann::json::json_pointer jsonCountKeyName = jsonKeyName;
    std::string back = jsonCountKeyName.back();
    jsonCountKeyName.pop_back();
    jsonCountKeyName /= back + "@odata.count";

    nlohmann::json& members = asyncResp->res.jsonValue[jsonKeyName];
    members = nlohmann::json::array();

    if (ec)
    {
        // Not an error, system just doesn't have PCIe info
        BMCWEB_LOG_DEBUG("no PCIe device paths found ec: {}", ec.value());
        asyncResp->res.jsonValue[jsonCountKeyName] = members.size();
        return;
    }

    std::vector<std::string> pathNames;
    for (const auto& pcieDevicePath : paths)
    {
        std::string devName = pcie_util::buildPCIeUniquePath(pcieDevicePath);
        if (devName.empty())
        {
            BMCWEB_LOG_DEBUG("Invalid Name");
            continue;
        }
        pathNames.emplace_back(std::move(devName));
    }
    std::ranges::sort(pathNames, AlphanumLess<std::string>());

    for (const auto& devName : pathNames)
    {
        nlohmann::json::object_t pcieDevice;
        pcieDevice["@odata.id"] = boost::urls::format(
            "/redfish/v1/Systems/system/PCIeDevices/{}", devName);
        members.emplace_back(std::move(pcieDevice));
    }
    asyncResp->res.jsonValue[jsonCountKeyName] = members.size();
}

/**
 * @brief get PCIeDeviceList to resp
 *
 * @param[in,out]   asyncResp   Async HTTP response.
 * @param[in]      jsonKeyName
 */
inline void
    getPCIeDeviceList(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const nlohmann::json::json_pointer& jsonKeyName)
{
    static constexpr std::array<std::string_view, 1> pcieDeviceInterface = {
        "xyz.openbmc_project.Inventory.Item.PCIeDevice"};

    dbus::utility::getSubTreePaths(
        "/xyz/openbmc_project/inventory", 0, pcieDeviceInterface,
        std::bind_front(afterGetPCIeDeviceList, asyncResp, jsonKeyName));
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
