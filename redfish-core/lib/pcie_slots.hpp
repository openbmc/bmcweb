#pragma once

#include "app.hpp"
#include "assembly.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/pcie_slots.hpp"
#include "led.hpp"
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

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace redfish
{

inline void afterAddLinkedFabricAdapter(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, size_t index,
    const boost::system::error_code& ec,
    const dbus::utility::MapperEndPoints& fabricAdapterPaths)
{
    if (ec)
    {
        if (ec.value() == EBADR)
        {
            BMCWEB_LOG_DEBUG("FabricAdapter Slot association not found");
            return;
        }
        BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }
    if (fabricAdapterPaths.empty())
    {
        // No association to FabricAdapter
        BMCWEB_LOG_DEBUG("FabricAdapter Slot association not found");
        return;
    }

    // Add a link to FabricAdapter
    nlohmann::json::object_t linkOemIbm;
    linkOemIbm["@odata.type"] = "#OemPCIeSlots.v1_0_0.PCIeLinks";
    nlohmann::json& fabricArray = linkOemIbm["UpstreamFabricAdapters"];
    for (const auto& fabricAdapterPath : fabricAdapterPaths)
    {
        std::string fabricAdapterName =
            sdbusplus::message::object_path(fabricAdapterPath).filename();
        nlohmann::json::object_t item;
        item["@odata.id"] = boost::urls::format(
            "/redfish/v1/Systems/system/FabricAdapters/{}", fabricAdapterName);
        fabricArray.emplace_back(std::move(item));
    }
    linkOemIbm["UpstreamFabricAdapters@odata.count"] = fabricArray.size();

    asyncResp->res.jsonValue["Slots"][index]["Links"]["Oem"]["IBM"] =
        std::move(linkOemIbm);
}

inline void
    addLinkedFabricAdapter(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& pcieSlotPath, size_t index)
{
    constexpr std::array<std::string_view, 1> fabricAdapterInterfaces{
        "xyz.openbmc_project.Inventory.Item.FabricAdapter"};
    dbus::utility::getAssociatedSubTreePaths(
        pcieSlotPath + "/contained_by",
        sdbusplus::message::object_path("/xyz/openbmc_project/inventory"), 0,
        fabricAdapterInterfaces,
        std::bind_front(afterAddLinkedFabricAdapter, asyncResp, index));
}

inline void doLinkAssociatedDiskBackplaneToChassis(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& drivePath, size_t index,
    const std::optional<std::string>& validChassisPath,
    const std::vector<std::string>& assemblyList)
{
    if (!validChassisPath || assemblyList.empty())
    {
        BMCWEB_LOG_WARNING("Chassis not found");
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
        return;
    }

    auto it = std::find(assemblyList.begin(), assemblyList.end(), drivePath);
    if (it == assemblyList.end())
    {
        BMCWEB_LOG_ERROR("Drive path {} not found in the assembly list",
                         drivePath);
        messages::internalError(asyncResp->res);
        return;
    }

    nlohmann::json::object_t item;
    item["@odata.id"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/Assembly#/Assemblies/{}", chassisId,
        std::to_string(it - assemblyList.begin()));

    asyncResp->res.jsonValue["Slots"][index]["Links"]["Oem"]["IBM"]
                            ["@odata.type"] = "#OemPCIeSlots.v1_0_0.PCIeLinks";
    asyncResp->res.jsonValue["Slots"][index]["Links"]["Oem"]["IBM"]
                            ["AssociatedAssembly"] = std::move(item);
}

inline void afterLinkAssociatedDiskBackplane(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, size_t index,
    const boost::system::error_code& ec,
    const dbus::utility::MapperEndPoints& endpoints)
{
    if (ec)
    {
        if (ec.value() == EBADR)
        {
            // Disk backplane association not found for this pcie slot.
            BMCWEB_LOG_DEBUG("Disk backplane association not found");
            return;
        }
        BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }

    if (endpoints.empty())
    {
        BMCWEB_LOG_ERROR("No association was found for disk backplane drive");
        messages::internalError(asyncResp->res);
        return;
    }

    // Each slot points to one disk backplane, so picking the top one
    // or the only one we will have instead of looping through.
    const std::string& drivePath = endpoints[0];
    std::string chassisId{"chassis"};
    getChassisAssembly(asyncResp, chassisId,
                       std::bind_front(doLinkAssociatedDiskBackplaneToChassis,
                                       asyncResp, chassisId, drivePath, index));
}

/**
 * @brief Add PCIeSlot to NMVe backplane assembly link
 *
 * @param[in, out]  asyncResp       Async HTTP response.
 * @param[in]       pcieSlotPath    Object path of the PCIeSlot.
 * @param[in]       index           Index.
 */
inline void linkAssociatedDiskBackplane(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& pcieSlotPath, size_t index)
{
    dbus::utility::getAssociationEndPoints(
        pcieSlotPath + "/inventory",
        std::bind_front(afterLinkAssociatedDiskBackplane, asyncResp, index));
}

inline void afterAddLinkedPcieDevices(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, size_t index,
    const boost::system::error_code& ec,
    const dbus::utility::MapperEndPoints& pcieDevicePaths)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("D-Bus response error on GetSubTree {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }
    if (pcieDevicePaths.empty())
    {
        BMCWEB_LOG_DEBUG("Can't find PCIeDevice D-Bus object for given slot");
        return;
    }

    // Assuming only one device path per slot.
    const std::string& pcieDevciePath = pcieDevicePaths.front();
    std::string devName = pcie_util::buildPCIeUniquePath(pcieDevciePath);

    if (devName.empty())
    {
        BMCWEB_LOG_ERROR("Failed to find / in pcie device path");
        messages::internalError(asyncResp->res);
        return;
    }

    nlohmann::json::object_t item;
    nlohmann::json::array_t deviceArray;
    item["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/system/PCIeDevices/{}", devName);
    deviceArray.emplace_back(item);
    asyncResp->res.jsonValue["Slots"][index]["Links"]["PCIeDevice"] =
        std::move(deviceArray);
}

inline void
    addLinkedPcieDevices(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& pcieSlotPath, size_t index)
{
    constexpr std::array<std::string_view, 1> pcieDeviceInterfaces = {
        "xyz.openbmc_project.Inventory.Item.PCIeDevice"};

    dbus::utility::getAssociatedSubTreePaths(
        pcieSlotPath + "/containing",
        sdbusplus::message::object_path("/xyz/openbmc_project/inventory"), 0,
        pcieDeviceInterfaces,
        std::bind_front(afterAddLinkedPcieDevices, asyncResp, index));
}

inline void
    linkAssociatedProcessor(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& pcieSlotPath, size_t index)
{
    constexpr std::array<std::string_view, 1> cpuInterfaces = {
        "xyz.openbmc_project.Inventory.Item.Cpu"};

    dbus::utility::getAssociatedSubTreePaths(
        pcieSlotPath + "/connected_to",
        sdbusplus::message::object_path("/xyz/openbmc_project/inventory"), 0,
        cpuInterfaces,
        [asyncResp, pcieSlotPath,
         index](const boost::system::error_code& ec,
                const dbus::utility::MapperEndPoints& endpoints) {
        if (ec)
        {
            if (ec.value() == EBADR)
            {
                // This PCIeSlot have no processor association.
                BMCWEB_LOG_DEBUG("No processor association found");
                return;
            }
            BMCWEB_LOG_ERROR("DBUS response error", ec.message());
            messages::internalError(asyncResp->res);
            return;
        }

        if (endpoints.empty())
        {
            BMCWEB_LOG_DEBUG("No association found for processor");
            messages::internalError(asyncResp->res);
            return;
        }

        std::string cpuName =
            sdbusplus::message::object_path(endpoints[0]).filename();
        std::string dcmName =
            (sdbusplus::message::object_path(endpoints[0]).parent_path())
                .filename();

        std::string processorName = dcmName + '-' + cpuName;

        nlohmann::json::object_t item;
        item["@odata.id"] = boost::urls::format(
            "/redfish/v1/Systems/system/Processors/{}", processorName);

        nlohmann::json::array_t processorArray = nlohmann::json::array();
        processorArray.emplace_back(std::move(item));

        asyncResp->res
            .jsonValue["Slots"][index]["Links"]["Processors@odata.count"] =
            processorArray.size();
        asyncResp->res.jsonValue["Slots"][index]["Links"]["Processors"] =
            std::move(processorArray);
    });
}

inline void getLocationCode(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const size_t index,
                            const std::string& connectionName,
                            const std::string& pcieSlotPath)
{
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, connectionName, pcieSlotPath,
        "xyz.openbmc_project.Inventory.Decorator.LocationCode", "LocationCode",
        [asyncResp, index](const boost::system::error_code& ec1,
                           const std::string& property) {
        if (ec1)
        {
            if (ec1.value() == EBADR)
            {
                // Don't always have PCIeSlot location codes
                BMCWEB_LOG_DEBUG("No slot location code found");
                return;
            }

            BMCWEB_LOG_ERROR(
                "Can't get location code property for PCIeSlot, Error:{}",
                ec1.value());
            messages::internalError(asyncResp->res);
            return;
        }
        asyncResp->res.jsonValue["Slots"][index]["Location"]["PartLocation"]
                                ["ServiceLabel"] = property;
    });
}

inline void
    onPcieSlotGetAllDone(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const boost::system::error_code& ec,
                         const dbus::utility::DBusPropertiesMap& propertiesList,
                         const std::string& connectionName,
                         const std::string& pcieSlotPath)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("Can't get PCIeSlot properties!");
        messages::internalError(asyncResp->res);
        return;
    }

    nlohmann::json& slots = asyncResp->res.jsonValue["Slots"];

    nlohmann::json::array_t* slotsPtr =
        slots.get_ptr<nlohmann::json::array_t*>();
    if (slotsPtr == nullptr)
    {
        BMCWEB_LOG_ERROR("Slots key isn't an array???");
        messages::internalError(asyncResp->res);
        return;
    }

    nlohmann::json::object_t slot;

    const std::string* generation = nullptr;
    const size_t* lanes = nullptr;
    const std::string* slotType = nullptr;
    const bool* hotPluggable = nullptr;
    const size_t* busId = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), propertiesList, "Generation",
        generation, "Lanes", lanes, "SlotType", slotType, "HotPluggable",
        hotPluggable, "BusId", busId);

    if (!success)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    if (generation != nullptr)
    {
        std::optional<pcie_device::PCIeTypes> pcieType =
            pcie_util::redfishPcieGenerationFromDbus(*generation);
        if (!pcieType)
        {
            BMCWEB_LOG_WARNING("Unknown PCIe Slot Generation: {}", *generation);
        }
        else
        {
            if (*pcieType == pcie_device::PCIeTypes::Invalid)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            slot["PCIeType"] = *pcieType;
        }
    }

    if (lanes != nullptr && *lanes != 0)
    {
        slot["Lanes"] = *lanes;
    }

    if (slotType != nullptr)
    {
        std::optional<pcie_slots::SlotTypes> redfishSlotType =
            pcie_util::dbusSlotTypeToRf(*slotType);
        if (!redfishSlotType)
        {
            BMCWEB_LOG_WARNING("Unknown PCIe Slot Type: {}", *slotType);
        }
        else
        {
            if (*redfishSlotType == pcie_slots::SlotTypes::Invalid)
            {
                BMCWEB_LOG_ERROR("Unknown PCIe Slot Type: {}", *slotType);
                messages::internalError(asyncResp->res);
                return;
            }
            slot["SlotType"] = *redfishSlotType;
        }
    }

    if (hotPluggable != nullptr)
    {
        slot["HotPluggable"] = *hotPluggable;
    }

    if (busId != nullptr)
    {
        slot["Oem"]["IBM"]["@odata.type"] = "#OemPCIeSlots.v1_0_0.PCIeSlot";
        slot["Oem"]["IBM"]["LinkId"] = *busId;
    }

    size_t index = slots.size();
    slots.emplace_back(std::move(slot));

    // Get and set the location code
    getLocationCode(asyncResp, index, connectionName, pcieSlotPath);

    // Get pcie device link
    addLinkedPcieDevices(asyncResp, pcieSlotPath, index);

    // Get FabricAdapter device link if exists
    addLinkedFabricAdapter(asyncResp, pcieSlotPath, index);

    // Get processor link
    linkAssociatedDiskBackplane(asyncResp, pcieSlotPath, index);

    // Get pcie slot location indicator state
    getLocationIndicatorActive(asyncResp, pcieSlotPath,
                               [asyncResp, index](bool asserted) {
        nlohmann::json& slotArray = asyncResp->res.jsonValue["Slots"];
        nlohmann::json& slotItem = slotArray.at(index);
        slotItem["LocationIndicatorActive"] = asserted;
    });

    // Get processor link
    linkAssociatedProcessor(asyncResp, pcieSlotPath, index);
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

    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, connectionName, pcieSlotPath,
        "xyz.openbmc_project.Inventory.Item.PCIeSlot",
        [asyncResp, connectionName,
         pcieSlotPath](const boost::system::error_code& ec2,
                       const dbus::utility::DBusPropertiesMap& propertiesList) {
        onPcieSlotGetAllDone(asyncResp, ec2, propertiesList, connectionName,
                             pcieSlotPath);
    });
}

inline void
    onMapperSubtreeDone(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& chassisID,
                        const boost::system::error_code& ec,
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

    asyncResp->res.jsonValue["@odata.type"] = "#PCIeSlots.v1_5_0.PCIeSlots";
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

// Get all valid  PCIe Slots which are on the given chassis
inline void getValidPCIeSlotList(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisID, const std::string& chassisPath,
    std::function<void(
        const std::vector<std::pair<std::string, std::string>>&)>&& callback)
{
    BMCWEB_LOG_DEBUG("Get properties for PCIeSlots associated to chassis = {}",
                     chassisID);

    // get PCIeSlots that are in chassis
    constexpr std::array<std::string_view, 1> pcieSlotIntf{
        "xyz.openbmc_project.Inventory.Item.PCIeSlot"};

    dbus::utility::getAssociatedSubTree(
        chassisPath + "/inventory",
        sdbusplus::message::object_path("/xyz/openbmc_project/inventory"), 0,
        pcieSlotIntf,
        [asyncResp, chassisID, chassisPath, callback{std::move(callback)}](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            BMCWEB_LOG_ERROR("D-Bus response error on GetSubTree {}",
                             ec.value());
            messages::internalError(asyncResp->res);
            return;
        }
        if (subtree.empty())
        {
            // No PCIeSlot found
            messages::resourceNotFound(asyncResp->res, "Chassis", "PCIeSlot");
            return;
        }

        std::vector<std::pair<std::string, std::string>> slotPathConnNames;
        for (const auto& [pcieSlotPath, serviceName] : subtree)
        {
            if (pcieSlotPath.empty() || serviceName.size() != 1)
            {
                BMCWEB_LOG_ERROR("Error getting PCIeSlot D-Bus object!");
                messages::internalError(asyncResp->res);
                return;
            }
            const std::string& connectionName = serviceName[0].first;
            slotPathConnNames.emplace_back(pcieSlotPath, connectionName);
        }

        // sort by pcieSlotPath
        std::sort(slotPathConnNames.begin(), slotPathConnNames.end(),
                  [](const std::pair<std::string, std::string>& slot1,
                     const std::pair<std::string, std::string>& slot2) {
            return slot1.first < slot2.first;
        });

        callback(slotPathConnNames);
    });
}

inline void doHandlePCIeSlotPatch(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, size_t total,
    const std::string& chassisID, const std::string& validChassisPath,
    const std::map<unsigned int, bool>& locationIndicatorActiveMap)
{
    getValidPCIeSlotList(
        asyncResp, chassisID, validChassisPath,
        [asyncResp, chassisID, total,
         locationIndicatorActiveMap{locationIndicatorActiveMap}](
            const std::vector<std::pair<std::string, std::string>>&
                slotPathConnNames) {
        if (slotPathConnNames.size() != total)
        {
            BMCWEB_LOG_WARNING(
                "The actual number of PCIeSlots is different from the number of the input slots");
            int64_t totalCount = static_cast<int64_t>(total);
            messages::invalidIndex(asyncResp->res, totalCount);
        }

        unsigned int idx = 0;
        for (const auto& [pcieSlotPath, connectionName] : slotPathConnNames)
        {
            idx++;
            if (!locationIndicatorActiveMap.contains(idx))
            {
                continue;
            }

            auto indicatorOnOff = locationIndicatorActiveMap.at(idx);
            setLocationIndicatorActive(asyncResp, pcieSlotPath, indicatorOnOff);
        }
    });
}

inline void
    handlePCIeSlotsPatch(App& app, const crow::Request& req,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& chassisId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    std::optional<std::vector<nlohmann::json>> slotsData;
    if (!json_util::readJsonPatch(req, asyncResp->res, "Slots", slotsData))
    {
        return;
    }
    if (!slotsData || slotsData->empty())
    {
        return;
    }
    std::vector<nlohmann::json> slots = std::move(*slotsData);
    std::map<unsigned int, bool> locationIndicatorActiveMap;
    unsigned int slotIndex = 0;
    for (auto& slot : slots)
    {
        slotIndex++;
        if (slot.empty())
        {
            continue;
        }

        bool locationIndicatorActive = false;
        if (json_util::readJson(slot, asyncResp->res, "LocationIndicatorActive",
                                locationIndicatorActive))
        {
            locationIndicatorActiveMap[slotIndex] = locationIndicatorActive;
        }
    }

    size_t total = slots.size();
    redfish::chassis_utils::getValidChassisPath(
        asyncResp, chassisId,
        [asyncResp, chassisId, total,
         locationIndicatorActiveMap{std::move(locationIndicatorActiveMap)}](
            const std::optional<std::string>& validChassisPath) {
        if (!validChassisPath)
        {
            BMCWEB_LOG_WARNING("Not a valid chassis ID:{}", chassisId);
            messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
            return;
        }
        doHandlePCIeSlotPatch(asyncResp, total, chassisId, *validChassisPath,
                              locationIndicatorActiveMap);
    });
}

inline void requestRoutesPCIeSlots(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/PCIeSlots/")
        .privileges(redfish::privileges::getPCIeSlots)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handlePCIeSlotCollectionGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/PCIeSlots/")
        .privileges(redfish::privileges::patchPCIeSlots)
        .methods(boost::beast::http::verb::patch)(
            std::bind_front(handlePCIeSlotsPatch, std::ref(app)));
}

} // namespace redfish
