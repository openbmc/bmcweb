#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "utils/chassis_utils.hpp"

#include <boost/url/format.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <algorithm>
#include <array>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <variant>
#include <vector>

namespace redfish
{

constexpr std::array<std::string_view, 9> chassisAssemblyInterfaces = {
    "xyz.openbmc_project.Inventory.Item.Vrm",
    "xyz.openbmc_project.Inventory.Item.Tpm",
    "xyz.openbmc_project.Inventory.Item.Panel",
    "xyz.openbmc_project.Inventory.Item.Battery",
    "xyz.openbmc_project.Inventory.Item.DiskBackplane",
    "xyz.openbmc_project.Inventory.Item.Board",
    "xyz.openbmc_project.Inventory.Item.Connector",
    "xyz.openbmc_project.Inventory.Item.Drive",
    "xyz.openbmc_project.Inventory.Item.Board.Motherboard"};

inline void doGetAssociatedChassisAssembly(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisPath,
    std::function<void(const std::vector<std::string>& assemblyList)>&&
        callback)
{
    BMCWEB_LOG_DEBUG("Get associated chassis assembly");

    sdbusplus::message::object_path endpointPath{chassisPath};
    endpointPath /= "assembly";

    dbus::utility::getAssociatedSubTreePaths(
        endpointPath,
        sdbusplus::message::object_path("/xyz/openbmc_project/inventory"), 0,
        chassisAssemblyInterfaces,
        [asyncResp, chassisPath, callback{callback}](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreePathsResponse& subtreePaths) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR(
                    "DBUS response error for getAssociatedSubTreePaths {}",
                    ec.value());
                messages::internalError(asyncResp->res);
                return;
            }
            // Pass the empty assemblyList to caller
            callback(std::vector<std::string>());
            return;
        }

        std::vector<std::string> sortedAssemblyList = subtreePaths;
        std::ranges::sort(sortedAssemblyList);

        callback(sortedAssemblyList);
    });
}

/**
 * @brief Get chassis path with given chassis ID
 * @param[in] asyncResp - Shared pointer for asynchronous calls.
 * @param[in] chassisID - Chassis to which the assemblies are
 * associated.
 * @param[in] callback
 *
 * @return None.
 */
inline void getChassisAssembly(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisID,
    std::function<void(const std::optional<std::string>& validChassisPath,
                       const std::vector<std::string>& sortedAssemblyList)>&&
        callback)
{
    BMCWEB_LOG_DEBUG("Get ChassisAssembly");

    // get the chassis path
    redfish::chassis_utils::getValidChassisPath(
        asyncResp, chassisID,
        [asyncResp, callback{callback}](
            const std::optional<std::string>& validChassisPath) {
        if (!validChassisPath)
        {
            // tell the caller as not valid chassisPath
            callback(validChassisPath, std::vector<std::string>());
            return;
        }

        doGetAssociatedChassisAssembly(
            asyncResp, *validChassisPath,
            [asyncResp, validChassisPath, callback{callback}](
                const std::vector<std::string>& sortedAssemblyList) {
            callback(validChassisPath, sortedAssemblyList);
        });
    });
}

/**
 * @brief Get Asset properties on the given assembly.
 * @param[in] asyncResp - Shared pointer for asynchronous calls.
 * @param[in] serviceName - Service in which the assembly is hosted.
 * @param[in] assembly - Assembly object.
 * @param[in] assemblyIndex - Index on the assembly object.
 * @return None.
 */
void getAssemblyAsset(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const auto& serviceName, const auto& assembly,
                      const auto& assemblyIndex)
{
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, serviceName, assembly,
        "xyz.openbmc_project.Inventory.Decorator.Asset",
        [asyncResp, assemblyIndex](
            const boost::system::error_code& ec1,
            const dbus::utility::DBusPropertiesMap& propertiesList) {
        if (ec1)
        {
            BMCWEB_LOG_ERROR("DBUS response error {}", ec1.value());
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string* partNumber = nullptr;
        const std::string* serialNumber = nullptr;
        const std::string* sparePartNumber = nullptr;
        const std::string* model = nullptr;

        const bool success = sdbusplus::unpackPropertiesNoThrow(
            dbus_utils::UnpackErrorPrinter(), propertiesList, "PartNumber",
            partNumber, "SerialNumber", serialNumber, "SparePartNumber",
            sparePartNumber, "Model", model);

        if (!success)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        nlohmann::json& assemblyArray = asyncResp->res.jsonValue["Assemblies"];
        nlohmann::json& assemblyData = assemblyArray.at(assemblyIndex);

        if (partNumber != nullptr)
        {
            assemblyData["PartNumber"] = *partNumber;
        }

        if (serialNumber != nullptr)
        {
            assemblyData["SerialNumber"] = *serialNumber;
        }

        if (sparePartNumber != nullptr)
        {
            assemblyData["SparePartNumber"] = *sparePartNumber;
        }

        if (model != nullptr)
        {
            assemblyData["Model"] = *model;
        }
    });
}

/**
 * @brief Get Location code for the given assembly.
 * @param[in] asyncResp - Shared pointer for asynchronous calls.
 * @param[in] serviceName - Service in which the assembly is hosted.
 * @param[in] assembly - Assembly object.
 * @param[in] assemblyIndex - Index on the assembly object.
 * @return None.
 */
void getAssemblyLocationCode(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const auto& serviceName, const auto& assembly, const auto& assemblyIndex)
{
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, serviceName, assembly,
        "xyz.openbmc_project.Inventory.Decorator.LocationCode", "LocationCode",
        [asyncResp, assemblyIndex](const boost::system::error_code& ec1,
                                   const std::string& value) {
        if (ec1)
        {
            BMCWEB_LOG_ERROR("DBUS response error: {}", ec1.value());
            messages::internalError(asyncResp->res);
            return;
        }

        nlohmann::json& assemblyArray = asyncResp->res.jsonValue["Assemblies"];
        nlohmann::json& assemblyData = assemblyArray.at(assemblyIndex);

        assemblyData["Location"]["PartLocation"]["ServiceLabel"] = value;
    });
}

/**
 * @brief Get properties for the assemblies associated to given chassis
 * @param[in] asyncResp - Shared pointer for asynchronous calls.
 * @param[in] chassisPath - Chassis the assemblies are associated with.
 * @param[in] assemblies - list of all the assemblies associated with the
 * chassis.
 * @return None.
 */
inline void
    getAssemblyProperties(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& chassisPath,
                          const std::vector<std::string>& assemblies)
{
    BMCWEB_LOG_DEBUG("Get properties for assembly associated");

    const std::string& chassis =
        sdbusplus::message::object_path(chassisPath).filename();

    std::size_t assemblyIndex = 0;

    for (const auto& assembly : assemblies)
    {
        nlohmann::json& tempArray = asyncResp->res.jsonValue["Assemblies"];

        boost::urls::url dataID = boost::urls::format(
            "/redfish/v1/Chassis/{}/Assembly#/Assemblies/{}", chassis,
            std::to_string(assemblyIndex));

        nlohmann::json::object_t item;
        item["@odata.type"] = "#Assembly.v1_3_0.AssemblyData";
        item["@odata.id"] = dataID;
        item["MemberId"] = std::to_string(assemblyIndex);

        tempArray.emplace_back(item);

        tempArray.at(assemblyIndex)["Name"] =
            sdbusplus::message::object_path(assembly).filename();

        dbus::utility::getDbusObject(
            assembly, chassisAssemblyInterfaces,
            [asyncResp, assemblyIndex,
             assembly](const boost::system::error_code& ec,
                       const dbus::utility::MapperGetObject& object) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("DBUS response error : {}", ec.value());
                messages::internalError(asyncResp->res);
                return;
            }

            for (const auto& [serviceName, interfaceList] : object)
            {
                for (const auto& interface : interfaceList)
                {
                    if (interface ==
                        "xyz.openbmc_project.Inventory.Decorator.Asset")
                    {
                        getAssemblyAsset(asyncResp, serviceName, assembly,
                                         assemblyIndex);
                    }
                    else if (
                        interface ==
                        "xyz.openbmc_project.Inventory.Decorator.LocationCode")
                    {
                        getAssemblyLocationCode(asyncResp, serviceName,
                                                assembly, assemblyIndex);
                    }
                }
            }
        });

        nlohmann::json& assemblyArray = asyncResp->res.jsonValue["Assemblies"];
        asyncResp->res.jsonValue["Assemblies@odata.count"] =
            assemblyArray.size();

        assemblyIndex++;
    }
}

/**
 * @brief Get chassis path with given chassis ID
 * @param[in] asyncResp - Shared pointer for asynchronous calls.
 * @param[in] chassisID - Chassis to which the assemblies are
 * associated.
 *
 * @return None.
 */
inline void handleChassisAssemblyGet(
    App& /*unused*/, const crow::Request& /*unused*/,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisID)
{
    BMCWEB_LOG_DEBUG("Get chassis path");

    getChassisAssembly(asyncResp, "chassis",
                       [asyncResp, chassisID](
                           const std::optional<std::string>& validChassisPath,
                           const std::vector<std::string>& assemblyList) {
        if (!validChassisPath || assemblyList.empty())
        {
            BMCWEB_LOG_ERROR("Chassis not found");
            messages::resourceNotFound(asyncResp->res, "Chassis", chassisID);
            return;
        }
        const std::string& chassisPath = *validChassisPath;

        asyncResp->res.jsonValue["@odata.type"] = "#Assembly.v1_3_0.Assembly";
        asyncResp->res.jsonValue["@odata.id"] =
            boost::urls::format("/redfish/v1/Chassis/{}/Assembly", chassisID);
        asyncResp->res.jsonValue["Name"] = "Assembly Collection";
        asyncResp->res.jsonValue["Id"] = "Assembly";

        asyncResp->res.jsonValue["Assemblies"] = nlohmann::json::array();
        asyncResp->res.jsonValue["Assemblies@odata.count"] = 0;

        getAssemblyProperties(asyncResp, chassisPath, assemblyList);
    });
}

/**
 * Systems derived class for delivering Assembly Schema.
 */
inline void requestRoutesAssembly(App& app)
{
    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Assembly/")
        .privileges({{"Login"}})
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleChassisAssemblyGet, std::ref(app)));
}

} // namespace redfish
