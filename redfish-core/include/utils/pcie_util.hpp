#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http/utility.hpp"

#include <array>
#include <string_view>

namespace redfish
{
namespace pcie_util
{

static constexpr char const* inventoryPath = "/xyz/openbmc_project/inventory";

inline bool checkPCIeSlotDevicePath(
    const std::string& pcieDevicePath,
    const dbus::utility::MapperGetSubTreePathsResponse& pcieDevicePaths)
{
    for (const std::string& devicePath : pcieDevicePaths)
    {
        if (pcieDevicePath == devicePath)
        {
            return true;
        }
    }
    return false;
}

inline void checkPCIeSlotEndpoints(
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
        pcieDevice["@odata.id"] = crow::utility::urlFromPieces(
            "redfish", "v1", "Systems", "system", "PCIeDevices", pcieSlotName);
        pcieDeviceList.push_back(std::move(pcieDevice));
    }
}

/**
 * @brief Populate the PCIe Slot list from a GetSubTreePaths search of
 *        inventory
 *
 * @param[i,o] aResp  Async response object
 * @param[i]   Name   Key to store the list of PCIe Slots in aResp
 *
 * @return void
 */

inline void handleGetEmptyPCIeSlots(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const dbus::utility::MapperGetSubTreePathsResponse& pcieDevicePaths,
    const dbus::utility::MapperGetSubTreePathsResponse& pcieSlotPaths,
    const std::string& name)
{
    nlohmann::json& pcieDeviceList = aResp->res.jsonValue[name];
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
            [aResp, pcieDevicePaths, slotName,
             &pcieDeviceList](const boost::system::error_code& ec1,
                              const dbus::utility::MapperEndPoints& endpoints) {
            if (ec1)
            {
                if (ec1.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR << "DBUS response error: "
                                     << ec1.message();
                    messages::internalError(aResp->res);
                    return;
                }
            }

            checkPCIeSlotEndpoints(endpoints, pcieDevicePaths, slotName,
                                   pcieDeviceList);
            });
    }
    aResp->res.jsonValue[name + "@odata.count"] = pcieDeviceList.size();
}

inline void getEmptyPCIeSlots(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const dbus::utility::MapperGetSubTreePathsResponse& pcieDevicePaths,
    const std::string& name)
{
    constexpr std::array<std::string_view, 1> pcieSlotInterface = {
        "xyz.openbmc_project.Inventory.Item.PCIeSlot"};
    dbus::utility::getSubTreePaths(
        inventoryPath, 0, pcieSlotInterface,
        [aResp, pcieDevicePaths, name](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreePathsResponse& pcieSlotPaths) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "no PCIe Slot paths found ec: " << ec.message();
            return;
        }
        handleGetEmptyPCIeSlots(aResp, pcieDevicePaths, pcieSlotPaths, name);
        });
}

/**
 * @brief Populate the PCIe Device list from a GetSubTreePaths search of
 *        inventory
 *
 * @param[i,o] aResp  Async response object
 * @param[i]   Name   Key to store the list of PCIe devices in aResp
 *
 * @return void
 */

inline void
    getPCIeDeviceList(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& name)
{
    constexpr std::array<std::string_view, 1> pcieDeviceInterface = {
        "xyz.openbmc_project.Inventory.Item.PCIeDevice"};

    dbus::utility::getSubTreePaths(
        inventoryPath, 0, pcieDeviceInterface,
        [asyncResp, name](const boost::system::error_code& ec,
                          const dbus::utility::MapperGetSubTreePathsResponse&
                              pcieDevicePaths) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "no PCIe device paths found ec: "
                             << ec.message();
            // Not an error, system just doesn't have PCIe info
            return;
        }
        nlohmann::json& pcieDeviceList = asyncResp->res.jsonValue[name];
        pcieDeviceList = nlohmann::json::array();
        for (const std::string& pcieDevicePath : pcieDevicePaths)
        {
            size_t devStart = pcieDevicePath.rfind('/');
            if (devStart == std::string::npos)
            {
                continue;
            }

            std::string devName = pcieDevicePath.substr(devStart + 1);
            if (devName.empty())
            {
                continue;
            }
            nlohmann::json::object_t pcieDevice;
            pcieDevice["@odata.id"] = crow::utility::urlFromPieces(
                "redfish", "v1", "Systems", "system", "PCIeDevices", devName);
            pcieDeviceList.push_back(std::move(pcieDevice));
        }
        asyncResp->res.jsonValue[name + "@odata.count"] = pcieDeviceList.size();
        getEmptyPCIeSlots(asyncResp, pcieDevicePaths, name);
        });
}

} // namespace pcie_util
} // namespace redfish
