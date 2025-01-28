#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "led.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/chassis_utils.hpp"
#include "utils/json_utils.hpp"

#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <sdbusplus/unpack_properties.hpp>
#include <utils/name_utils.hpp>

#include <algorithm>
#include <array>
#include <iterator>
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
        [asyncResp, chassisPath, callback{std::move(callback)}](
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
        [asyncResp, callback{std::move(callback)}](
            const std::optional<std::string>& validChassisPath) {
            if (!validChassisPath)
            {
                // tell the caller as not valid chassisPath
                callback(validChassisPath, std::vector<std::string>());
                return;
            }

            doGetAssociatedChassisAssembly(
                asyncResp, *validChassisPath,
                [asyncResp, validChassisPath,
                 callback](const std::vector<std::string>& sortedAssemblyList) {
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

            nlohmann::json& assemblyArray =
                asyncResp->res.jsonValue["Assemblies"];
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

            nlohmann::json& assemblyArray =
                asyncResp->res.jsonValue["Assemblies"];
            nlohmann::json& assemblyData = assemblyArray.at(assemblyIndex);

            assemblyData["Location"]["PartLocation"]["ServiceLabel"] = value;
        });
}

inline void afterGetReadyToRemoveOfTodBattery(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    std::size_t assemblyIndex, const boost::system::error_code& ec,
    const dbus::utility::MapperGetObject& /*unused*/)
{
    nlohmann::json& assemblyArray = asyncResp->res.jsonValue["Assemblies"];
    if (ec)
    {
        if (ec.value() == boost::system::errc::io_error)
        {
            // Battery voltage is not on DBUS so ADCSensor is not
            // running.
            nlohmann::json& oemOpenBMC =
                assemblyArray.at(assemblyIndex)["Oem"]["OpenBMC"];
            oemOpenBMC["@odata.type"] = "#OpenBMCAssembly.v1_0_0.OpenBMC";
            oemOpenBMC["ReadyToRemove"] = true;
            return;
        }
        BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }
    nlohmann::json& oemOpenBMC =
        assemblyArray.at(assemblyIndex)["Oem"]["OpenBMC"];
    oemOpenBMC["@odata.type"] = "#OpenBMCAssembly.v1_0_0.OpenBMC";
    oemOpenBMC["ReadyToRemove"] = false;
}

inline void getReadyToRemoveOfTodBattery(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    std::size_t assemblyIndex)
{
    dbus::utility::getDbusObject(
        "/xyz/openbmc_project/sensors/voltage/Battery_Voltage", {},
        std::bind_front(afterGetReadyToRemoveOfTodBattery, asyncResp,
                        assemblyIndex));
}

void getAssemblyPresence(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const auto& serviceName, const auto& assembly,
                         const auto& assemblyIndex)
{
    nlohmann::json& assemblyArray = asyncResp->res.jsonValue["Assemblies"];
    nlohmann::json& assemblyData = assemblyArray.at(assemblyIndex);

    assemblyData["Status"]["State"] = "Enabled";

    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, serviceName, assembly,
        "xyz.openbmc_project.Inventory.Item", "Present",
        [asyncResp, assemblyIndex,
         assembly](const boost::system::error_code& ec, const bool value) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("DBUS response error: {}", ec.value());
                messages::internalError(asyncResp->res);
                return;
            }

            if (!value)
            {
                nlohmann::json& array = asyncResp->res.jsonValue["Assemblies"];
                nlohmann::json& data = array.at(assemblyIndex);
                data["Status"]["State"] = "Absent";

                std::string fru =
                    sdbusplus::message::object_path(assembly).filename();
                // Special handling for LCD and base panel CM.
                if (fru == "panel0" || fru == "panel1")
                {
                    data["Oem"]["OpenBMC"]["@odata.type"] =
                        "#OpenBMCAssembly.v1_0_0.Assembly";

                    // if panel is not present, implies it is already removed or
                    // can be placed.
                    data["Oem"]["OpenBMC"]["ReadyToRemove"] = !value;
                }
            }
        });
}

void getAssemblyHeath(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const auto& serviceName, const auto& assembly,
                      const auto& assemblyIndex)
{
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, serviceName, assembly,
        "xyz.openbmc_project.State.Decorator.OperationalStatus", "Functional",
        [asyncResp,
         assemblyIndex](const boost::system::error_code& ec, bool functional) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
                messages::internalError(asyncResp->res);
                return;
            }

            nlohmann::json& assemblyArray =
                asyncResp->res.jsonValue["Assemblies"];
            nlohmann::json& assemblyData = assemblyArray.at(assemblyIndex);

            if (!functional)
            {
                assemblyData["Status"]["Health"] = "Critical";
            }
            else
            {
                assemblyData["Status"]["Health"] = "OK";
            }
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
inline void getAssemblyProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisPath, const std::vector<std::string>& assemblies)
{
    BMCWEB_LOG_DEBUG("Get properties for assembly associated");

    const std::string& chassis =
        sdbusplus::message::object_path(chassisPath).filename();

    std::size_t assemblyIndex = 0;

    for (const auto& assembly : assemblies)
    {
        nlohmann::json& tempArray = asyncResp->res.jsonValue["Assemblies"];

        nlohmann::json::object_t item;
        item["@odata.type"] = "#Assembly.v1_3_0.AssemblyData";
        item["@odata.id"] = boost::urls::format(
            "/redfish/v1/Chassis/{}/Assembly#/Assemblies/{}", chassis,
            std::to_string(assemblyIndex));
        item["MemberId"] = std::to_string(assemblyIndex);

        tempArray.emplace_back(item);

        tempArray.at(assemblyIndex)["Name"] =
            sdbusplus::message::object_path(assembly).filename();

        // Handle special case for tod_battery assembly OEM ReadyToRemove
        // property NOTE: The following method for the special case of the
        // tod_battery ReadyToRemove property only works when there is only ONE
        // adcsensor handled by the adcsensor application.
        if (sdbusplus::message::object_path(assembly).filename() ==
            "tod_battery")
        {
            getReadyToRemoveOfTodBattery(asyncResp, assemblyIndex);
        }

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

                nlohmann::json::json_pointer ptr(
                    "/Assemblies/" + std::to_string(assemblyIndex) + "/Name");

                name_util::getPrettyName(asyncResp, assembly, object, ptr);

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
                        else if (
                            interface ==
                            "xyz.openbmc_project.State.Decorator.OperationalStatus")
                        {
                            getAssemblyHeath(asyncResp, serviceName, assembly,
                                             assemblyIndex);
                        }
                        else if (interface ==
                                 "xyz.openbmc_project.Inventory.Item")
                        {
                            getAssemblyPresence(asyncResp, serviceName,
                                                assembly, assemblyIndex);
                        }
                    }
                }
            });

        getLocationIndicatorActive(
            asyncResp, assembly, [asyncResp, assemblyIndex](bool asserted) {
                nlohmann::json& assemblyArray =
                    asyncResp->res.jsonValue["Assemblies"];
                nlohmann::json& assemblyData = assemblyArray.at(assemblyIndex);
                assemblyData["LocationIndicatorActive"] = asserted;
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

    getChassisAssembly(
        asyncResp, chassisID,
        [asyncResp,
         chassisID](const std::optional<std::string>& validChassisPath,
                    const std::vector<std::string>& assemblyList) {
            if (!validChassisPath || assemblyList.empty())
            {
                BMCWEB_LOG_ERROR("Chassis not found");
                messages::resourceNotFound(asyncResp->res, "Chassis",
                                           chassisID);
                return;
            }
            const std::string& chassisPath = *validChassisPath;

            asyncResp->res.jsonValue["@odata.type"] =
                "#Assembly.v1_3_0.Assembly";
            asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
                "/redfish/v1/Chassis/{}/Assembly", chassisID);
            asyncResp->res.jsonValue["Name"] = "Assembly Collection";
            asyncResp->res.jsonValue["Id"] = "Assembly";

            asyncResp->res.jsonValue["Assemblies"] = nlohmann::json::array();
            asyncResp->res.jsonValue["Assemblies@odata.count"] = 0;

            getAssemblyProperties(asyncResp, chassisPath, assemblyList);
        });
}

inline void startOrStopADCSensor(
    const bool start, const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    std::string method{"StartUnit"};
    if (!start)
    {
        method = "StopUnit";
    }

    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code& ec) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("Failed to start or stop ADCSensor:{}",
                                 ec.value());
                messages::internalError(asyncResp->res);
                return;
            }
            messages::success(asyncResp->res);
        },
        "org.freedesktop.systemd1", "/org/freedesktop/systemd1",
        "org.freedesktop.systemd1.Manager", method,
        "xyz.openbmc_project.adcsensor.service", "replace");
}

inline void afterGetDbusObjectDoBatteryCM(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& assembly, const boost::system::error_code& ec,
    const dbus::utility::MapperGetObject& object)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }

    for (const auto& [serviceName, interfaceList] : object)
    {
        auto ifaceIt = std::ranges::find(
            interfaceList,
            "xyz.openbmc_project.State.Decorator.OperationalStatus");

        if (ifaceIt == interfaceList.end())
        {
            continue;
        }

        sdbusplus::asio::setProperty(
            *crow::connections::systemBus, serviceName, assembly,
            "xyz.openbmc_project.State.Decorator."
            "OperationalStatus",
            "Functional", true,
            [asyncResp, assembly](const boost::system::error_code& ec2) {
                if (ec2)
                {
                    BMCWEB_LOG_ERROR(
                        "Failed to set functional property on battery: {} ",
                        ec2.value());
                    messages::internalError(asyncResp->res);
                    return;
                }
                startOrStopADCSensor(true, asyncResp);
            });
        return;
    }

    BMCWEB_LOG_ERROR("No OperationalStatus interface on {}", assembly);
    messages::internalError(asyncResp->res);
}

inline void doBatteryCM(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& assembly, const bool readyToRemove)
{
    if (readyToRemove)
    {
        // Stop the adcsensor service so it doesn't monitor the battery
        startOrStopADCSensor(false, asyncResp);
        return;
    }

    // Find the service that has the OperationalStatus iface, set the
    // Functional property back to true, and then start the adcsensor service.
    std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.State.Decorator.OperationalStatus"};
    dbus::utility::getDbusObject(
        assembly, interfaces,
        std::bind_front(afterGetDbusObjectDoBatteryCM, asyncResp, assembly));
}

/**
 * @brief Set location indicator for the assemblies associated to given chassis
 * @param[in] req - The request data
 * @param[in] asyncResp - Shared pointer for asynchronous calls.
 * @param[in] chassisID - Chassis the assemblies are associated with.
 * @param[in] assemblies - list of all the assemblies associated with the
 * chassis.

 * @return None.
 */
inline void setAssemblyLocationIndicators(
    const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisID, const std::vector<std::string>& assemblies)
{
    BMCWEB_LOG_DEBUG(
        "Set LocationIndicatorActive for assembly associated to chassis = {}",
        chassisID);

    std::optional<std::vector<nlohmann::json>> assemblyData;
    if (!json_util::readJsonAction(req, asyncResp->res, "Assemblies",
                                   assemblyData))
    {
        return;
    }
    if (!assemblyData)
    {
        return;
    }

    std::vector<nlohmann::json> items = std::move(*assemblyData);
    std::map<std::string, bool> locationIndicatorActiveMap;
    std::map<std::string, nlohmann::json> oemIndicatorMap;

    for (auto& item : items)
    {
        std::optional<std::string> memberId;
        std::optional<bool> locationIndicatorActive;
        std::optional<nlohmann::json> oem;

        if (!json_util::readJson(
                item, asyncResp->res, "LocationIndicatorActive",
                locationIndicatorActive, "MemberId", memberId, "Oem", oem))
        {
            return;
        }
        if (locationIndicatorActive)
        {
            if (memberId)
            {
                locationIndicatorActiveMap[*memberId] =
                    *locationIndicatorActive;
            }
            else
            {
                BMCWEB_LOG_DEBUG(
                    "Property Missing - MemberId must be included with LocationIndicatorActive ");
                messages::propertyMissing(asyncResp->res, "MemberId");
                return;
            }
        }
        if (oem)
        {
            if (memberId)
            {
                oemIndicatorMap[*memberId] = *oem;
            }
            else
            {
                BMCWEB_LOG_DEBUG(
                    "Property Missing - MemberId must be included with Oem property");
                messages::propertyMissing(asyncResp->res, "MemberId");
                return;
            }
        }
    }

    std::size_t assemblyIndex = 0;
    for (const auto& assembly : assemblies)
    {
        auto iter =
            locationIndicatorActiveMap.find(std::to_string(assemblyIndex));

        if (iter != locationIndicatorActiveMap.end())
        {
            setLocationIndicatorActive(asyncResp, assembly, iter->second);
        }

        auto iter2 = oemIndicatorMap.find(std::to_string(assemblyIndex));

        if (iter2 != oemIndicatorMap.end())
        {
            std::optional<nlohmann::json> openbmc;
            if (!json_util::readJson(iter2->second, asyncResp->res, "OpenBMC",
                                     openbmc))
            {
                BMCWEB_LOG_DEBUG("Property Value Format Error ");
                messages::propertyValueFormatError(
                    asyncResp->res,
                    (*openbmc).dump(2, ' ', true,
                                    nlohmann::json::error_handler_t::replace),
                    "OpenBMC");
                return;
            }

            if (!openbmc)
            {
                BMCWEB_LOG_DEBUG("Property Missing ");
                messages::propertyMissing(asyncResp->res, "OpenBMC");
                return;
            }

            std::optional<bool> readytoremove;
            if (!json_util::readJson(*openbmc, asyncResp->res, "ReadyToRemove",
                                     readytoremove))
            {
                BMCWEB_LOG_DEBUG("Property Value Format Error");
                messages::propertyValueFormatError(
                    asyncResp->res,
                    (*openbmc).dump(2, ' ', true,
                                    nlohmann::json::error_handler_t::replace),
                    "ReadyToRemove");
                return;
            }

            if (!readytoremove)
            {
                BMCWEB_LOG_DEBUG("Property Missing ");
                messages::propertyMissing(asyncResp->res, "ReadyToRemove");
                return;
            }

            // Handle special case for tod_battery assembly OEM ReadyToRemove
            // property. NOTE: The following method for the special case of the
            // tod_battery ReadyToRemove property only works when there is only
            // ONE adcsensor handled by the adcsensor application.
            if (sdbusplus::message::object_path(assembly).filename() ==
                "tod_battery")
            {
                doBatteryCM(asyncResp, assembly, readytoremove.value());
            }

            // Special handling for LCD and base panel. This is required to
            // support concurrent maintenance for base and LCD panel.
            else if (sdbusplus::message::object_path(assembly).filename() ==
                         "panel0" ||
                     sdbusplus::message::object_path(assembly).filename() ==
                         "panel1")
            {
                // Based on the status of readytoremove flag, inventory data
                // like CCIN and present property needs to be updated for this
                // FRU.
                // readytoremove as true implies FRU has been prepared for
                // removal. Set action as "deleteFRUVPD". This is the api
                // exposed by vpd-manager to clear CCIN and set present
                // property as false for the FRU.
                // readytoremove as false implies FRU has been replaced. Set
                // action as "CollectFRUVPD". This is the api exposed by
                // vpd-manager to recollect vpd for a given FRU.
                std::string action =
                    (readytoremove.value()) ? "deleteFRUVPD" : "CollectFRUVPD";

                crow::connections::systemBus->async_method_call(
                    [asyncResp, action](const boost::system::error_code& ec) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR(
                                "Call to Manager failed for action:{} with error:{}",
                                action, ec.value());
                            messages::internalError(asyncResp->res);
                            return;
                        }
                    },
                    "com.ibm.VPD.Manager", "/com/ibm/VPD/Manager",
                    "com.ibm.VPD.Manager", action,
                    sdbusplus::message::object_path(assembly));
            }
            else
            {
                BMCWEB_LOG_DEBUG(
                    "Property Unknown: ReadyToRemove on Assembly with MemberID: {}",
                    assemblyIndex);
                messages::propertyUnknown(asyncResp->res, "ReadyToRemove");
                return;
            }
        }
        assemblyIndex++;
    }
}

inline void handleChassisAssemblyPatch(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisID)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    BMCWEB_LOG_DEBUG("Patch chassis path");

    getChassisAssembly(
        asyncResp, chassisID,
        [req, asyncResp,
         chassisID](const std::optional<std::string>& validChassisPath,
                    const std::vector<std::string>& assemblyList) {
            if (!validChassisPath || assemblyList.empty())
            {
                BMCWEB_LOG_ERROR("Chassis not found");
                messages::resourceNotFound(asyncResp->res, "Chassis",
                                           chassisID);
                return;
            }

            setAssemblyLocationIndicators(req, asyncResp, chassisID,
                                          assemblyList);
        });
}

namespace assembly
{
/**
 * @brief API used to fill the Assembly id of the assembled object that
 *        assembled in the given assembly parent object path.
 *
 *        bmcweb using the sequential numeric value by sorting the
 *        assembled objects instead of the assembled object dbus id
 *        for the Redfish Assembly implementation.
 *
 * @param[in] asyncResp - The redfish response to return.
 * @param[in] assemblyParentServ - The assembly parent dbus service name.
 * @param[in] assemblyParentObjPath - The assembly parent dbus object path.
 * @param[in] assemblyParentIface - The assembly parent dbus interface name
 *                                  to valid the supports in the bmcweb.
 * @param[in] assemblyUriPropPath - The redfish property path to fill with id.
 * @param[in] assembledObjPath - The assembled object that need to fill with
 *                               its id. Used to check in the parent assembly
 *                               associations.
 * @param[in] assembledUriVal - The assembled object redfish uri value that
 *                              need to replace with its id.
 *
 * @return The redfish response with assembled object id in the given
 *         redfish property path if success else returns the error.
 */
inline void fillWithAssemblyId(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& assemblyParentServ,
    const sdbusplus::message::object_path& assemblyParentObjPath,
    const std::string& assemblyParentIface,
    const nlohmann::json::json_pointer& assemblyUriPropPath,
    const sdbusplus::message::object_path& assembledObjPath,
    const std::string& assembledUriVal)
{
    if (assemblyParentIface != "xyz.openbmc_project.Inventory.Item.Chassis")
    {
        // Currently, bmcweb supporting only chassis assembly uri so return
        // error if unsupported assembly uri interface was given
        BMCWEB_LOG_ERROR(
            "Unsupported interface [{}] was given to fill assembly id. Please add support in the bmcweb",
            assemblyParentIface);
        messages::internalError(asyncResp->res);
        return;
    }

    using associationList =
        std::vector<std::tuple<std::string, std::string, std::string>>;

    sdbusplus::asio::getProperty<associationList>( //
        *crow::connections::systemBus, assemblyParentServ,
        assemblyParentObjPath.str,
        "xyz.openbmc_project.Association.Definitions", "Associations",
        [asyncResp, assemblyUriPropPath, assemblyParentObjPath,
         assembledObjPath,
         assembledUriVal](const boost::system::error_code& ec,
                          const associationList& associations) {
            if (ec)
            {
                BMCWEB_LOG_ERROR(
                    "DBUS response error [{}  : {}] when tried to get the Associations from [{}] to fill Assembly id of the assembled object [{}]",
                    ec.value(), ec.message(), assemblyParentObjPath.str,
                    assembledObjPath.str);
                messages::internalError(asyncResp->res);
                return;
            }

            std::vector<std::string> assemblyAssoc;
            for (const auto& association : associations)
            {
                if (std::get<0>(association) != "assembly")
                {
                    continue;
                }
                assemblyAssoc.emplace_back(std::get<2>(association));
            }

            if (assemblyAssoc.empty())
            {
                BMCWEB_LOG_ERROR(
                    "No assembly associations in the [{}] to fill Assembly id of the assembled object [{}]",
                    assemblyParentObjPath.str, assembledObjPath.str);
                messages::internalError(asyncResp->res);
                return;
            }

            // Mak sure whether the retrieved assembly associations are
            // implemented before finding the assembly id as per bmcweb
            // Assembly design.
            crow::connections::systemBus->async_method_call(
                [asyncResp, assemblyUriPropPath, assemblyParentObjPath,
                 assembledObjPath, assemblyAssoc, assembledUriVal](
                    const boost::system::error_code& ec1,
                    const dbus::utility::MapperGetSubTreeResponse& objects) {
                    if (ec1)
                    {
                        BMCWEB_LOG_ERROR(
                            "DBUS response error [{} : {}] when tried to get the subtree to check assembled objects implementation of the [{}] to find assembled object id of the [{}] to fill in the URI property",
                            ec1.value(), ec1.message(),
                            assemblyParentObjPath.str, assembledObjPath.str);
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    if (objects.empty())
                    {
                        BMCWEB_LOG_ERROR(
                            "No objects in the [{}] to check assembled objects implementation to fill the assembled object [{}] id in the URI property",
                            assemblyParentObjPath.str, assembledObjPath.str);
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    std::vector<std::string> implAssemblyAssocs;
                    for (const auto& object : objects)
                    {
                        auto it =
                            std::ranges::find(assemblyAssoc, object.first);
                        if (it != assemblyAssoc.end())
                        {
                            implAssemblyAssocs.emplace_back(*it);
                        }
                    }

                    if (implAssemblyAssocs.empty())
                    {
                        BMCWEB_LOG_ERROR(
                            "The assembled objects of the [{}] are not implemented so unable to fill the assembled object [{}] id in the URI property",
                            assemblyParentObjPath.str, assembledObjPath.str);
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    // sort the implemented assemply object as per bmcweb design
                    // to match with Assembly GET and PATCH handler.
                    std::ranges::sort(implAssemblyAssocs);

                    auto assembledObjectIt = std::ranges::find(
                        implAssemblyAssocs, assembledObjPath.str);

                    if (assembledObjectIt == implAssemblyAssocs.end())
                    {
                        BMCWEB_LOG_ERROR(
                            "The assembled object [{}] in the object [{}] is not implemented so unable to fill assembled object id in the URI property",
                            assembledObjPath.str, assemblyParentObjPath.str);
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    auto assembledObjectId = std::distance(
                        implAssemblyAssocs.begin(), assembledObjectIt);

                    std::string::size_type assembledObjectNamePos =
                        assembledUriVal.rfind(assembledObjPath.filename());

                    if (assembledObjectNamePos == std::string::npos)
                    {
                        BMCWEB_LOG_ERROR(
                            "The assembled object name [{}] is not found in the redfish property value [{}] to replace with assembled object id [{}]",
                            assembledObjPath.filename(), assembledUriVal,
                            assembledObjectId);
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    std::string uriValwithId(assembledUriVal);
                    uriValwithId.replace(assembledObjectNamePos,
                                         assembledObjPath.filename().length(),
                                         std::to_string(assembledObjectId));

                    asyncResp->res.jsonValue[assemblyUriPropPath] =
                        uriValwithId;
                },
                "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/object_mapper",
                "xyz.openbmc_project.ObjectMapper", "GetSubTree",
                "/xyz/openbmc_project/inventory", int32_t(0),
                chassisAssemblyInterfaces);
        });
}

} // namespace assembly

/**
 * Systems derived class for delivering Assembly Schema.
 */
inline void requestRoutesAssembly(App& app)
{
    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Assembly/")
        .privileges(redfish::privileges::getAssembly)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleChassisAssemblyGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Assembly/")
        .privileges(redfish::privileges::patchAssembly)
        .methods(boost::beast::http::verb::patch)(
            std::bind_front(handleChassisAssemblyPatch, std::ref(app)));
}
} // namespace redfish
