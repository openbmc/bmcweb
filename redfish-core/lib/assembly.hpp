#pragma once

#include "dbus_utility.hpp"

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
            const boost::system::error_code ec1,
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
        [asyncResp, assemblyIndex](const boost::system::error_code ec1,
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

    constexpr std::array<std::string_view, 5> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Vrm",
        "xyz.openbmc_project.Inventory.Item.Tpm",
        "xyz.openbmc_project.Inventory.Item.Panel",
        "xyz.openbmc_project.Inventory.Item.Battery",
        "xyz.openbmc_project.Inventory.Item.DiskBackplane"};

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
            assembly, interfaces,
            [asyncResp, assemblyIndex,
             assembly](const boost::system::error_code ec,
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
                    else if (interface == "xyz.openbmc_project.Inventory."
                                          "Decorator.LocationCode")
                    {
                        getAssemblyLocationCode(asyncResp, serviceName,
                                                assembly, assemblyIndex);
                    }
                }
            }
        });

        nlohmann::json::array_t assemblyArray =
            asyncResp->res.jsonValue["Assemblies"];
        asyncResp->res.jsonValue["Assemblies@odata.count"] =
            assemblyArray.size();

        assemblyIndex++;
    }
}

/**
 * @brief Api to check if the assemblies fetched from association Json is
 * also implemented in the system. In case the interface for that assembly
 * is not found update the list and fetch properties for only implemented
 * assemblies.
 * @param[in] asyncResp - Shared pointer for asynchronous calls.
 * @param[in] chassisPath - Chassis the assemblies are associated with.
 * @param[in] assemblies - list of all the assemblies associated with the
 * chassis.
 * @return None.
 */
inline void
    checkAssemblyInterface(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& chassisPath,
                           const dbus::utility::MapperEndPoints& assemblies)
{
    constexpr std::array<std::string_view, 5> fruInterface = {
        "xyz.openbmc_project.Inventory.Item.Vrm",
        "xyz.openbmc_project.Inventory.Item.Tpm",
        "xyz.openbmc_project.Inventory.Item.Panel",
        "xyz.openbmc_project.Inventory.Item.Battery",
        "xyz.openbmc_project.Inventory.Item.DiskBackplane"};

    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, fruInterface,
        [asyncResp, chassisPath,
         assemblies](const boost::system::error_code& ec,
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
            BMCWEB_LOG_DEBUG("No object paths found");
            return;
        }
        std::vector<std::string> updatedAssemblyList;
        for (const auto& [objectPath, serviceName] : subtree)
        {
            // This list will store common paths between assemblies fetched
            // from association json and assemblies which are actually
            // implemeted. This is to handle the case in which there is
            // entry in association json but implementation of interface for
            // that particular assembly is missing.
            auto it = std::find(assemblies.begin(), assemblies.end(),
                                objectPath);
            if (it != assemblies.end())
            {
                updatedAssemblyList.emplace(updatedAssemblyList.end(), *it);
            }
        }

        if (!updatedAssemblyList.empty())
        {
            // sorting is required to facilitate patch as the array does not
            // store and data which can be mapped back to Dbus path of
            // assembly.
            std::sort(updatedAssemblyList.begin(), updatedAssemblyList.end());

            getAssemblyProperties(asyncResp, chassisPath, updatedAssemblyList);
        }
    });
}

/**
 * @brief Api to get assembly endpoints from mapper.
 * @param[in] asyncResp - Shared pointer for asynchronous calls.
 * @param[in] chassisPath - Chassis to which the assemblies are
 * associated.
 *
 * @return None.
 */
inline void
    getAssemblyEndpoints(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& chassisPath)
{
    BMCWEB_LOG_DEBUG("Get assembly endpoints");

    sdbusplus::message::object_path assemblyPath(chassisPath);
    assemblyPath /= "assembly";

    // if there is assembly association, look
    // for endpoints
    dbus::utility::getAssociationEndPoints(
        assemblyPath, [asyncResp, chassisPath](
                          const boost::system::error_code& ec,
                          const dbus::utility::MapperEndPoints& endpoints) {
        if (ec)
        {
            BMCWEB_LOG_ERROR("DBUS response error : {}", ec.value());
            messages::internalError(asyncResp->res);
            return;
        }

        auto sortedAssemblyList = endpoints;
        std::sort(sortedAssemblyList.begin(), sortedAssemblyList.end());

        checkAssemblyInterface(asyncResp, chassisPath, sortedAssemblyList);
        return;
    });
}

/**
 * @brief Api to check for assembly associations.
 * @param[in] asyncResp - Shared pointer for asynchronous calls.
 * @param[in] chassisPath - Chassis to which the assemblies are
 * associated.
 *
 * @return None.
 */
inline void checkForAssemblyAssociations(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisPath, const std::string& service)
{
    BMCWEB_LOG_DEBUG("Check for assembly association");

    using associationList =
        std::vector<std::tuple<std::string, std::string, std::string>>;

    crow::connections::systemBus->async_method_call(
        [asyncResp,
         chassisPath](const boost::system::error_code ec,
                      const std::variant<associationList>& associations) {
        if (ec)
        {
            BMCWEB_LOG_ERROR("DBUS response error : {}", ec.value());
            messages::internalError(asyncResp->res);
            return;
        }

        const associationList* value =
            std::get_if<associationList>(&associations);
        if (value == nullptr)
        {
            BMCWEB_LOG_ERROR(
                "DBUS response error. Received an empty association list.");
            messages::internalError(asyncResp->res);
            return;
        }

        auto iter = std::ranges::find_if(value->begin(), value->end(),
                                         [](const auto& listOfAssociations) {
            return std::get<0>(listOfAssociations) == "assembly";
        });

        if (iter != value->end())
        {
            getAssemblyEndpoints(asyncResp, chassisPath);
        }
    },
        service, chassisPath, "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Association.Definitions", "Associations");
}

/**
 * @brief Api to check if there is any association.
 * @param[in] asyncResp - Shared pointer for asynchronous calls.
 * @param[in] chassisPath - Chassis to which the assemblies are
 * associated.
 *
 * @return None.
 */
inline void
    checkAssociation(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const std::string& chassisPath,
                     const std::string& chassisID)
{
    BMCWEB_LOG_DEBUG("Check chassis for association");

    std::string chassis =
        sdbusplus::message::object_path(chassisPath).filename();
    if (chassis.empty())
    {
        BMCWEB_LOG_ERROR("Failed to find / in Chassis path");
        messages::internalError(asyncResp->res);
        return;
    }

    asyncResp->res.jsonValue["@odata.type"] = "#Assembly.v1_3_0.Assembly";
    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Chassis/{}/Assembly", chassisID);
    asyncResp->res.jsonValue["Name"] = "Assembly Collection";
    asyncResp->res.jsonValue["Id"] = "Assembly";

    asyncResp->res.jsonValue["Assemblies"] = nlohmann::json::array();
    asyncResp->res.jsonValue["Assemblies@odata.count"] = 0;

    // check if this chassis hosts any association
    dbus::utility::getDbusObject(
        chassisPath, std::array<std::string_view, 0>(),
        [asyncResp, chassisPath](const boost::system::error_code ec,
                                 const dbus::utility::MapperGetObject& object) {
        if (ec)
        {
            BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
            messages::internalError(asyncResp->res);
            return;
        }

        for (const auto& [serviceName, interfaceList] : object)
        {
            for (const auto& interface : interfaceList)
            {
                if (interface == "xyz.openbmc_project.Association.Definitions")
                {
                    checkForAssemblyAssociations(asyncResp, chassisPath,
                                                 serviceName);
                    return;
                }
            }
        }
        return;
    });
}

namespace assembly
{
/**
 * @brief Get chassis path with given chassis ID
 * @param[in] asyncResp - Shared pointer for asynchronous calls.
 * @param[in] chassisID - Chassis to which the assemblies are
 * associated.
 *
 * @return None.
 */
inline void getChassis(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& chassisID)
{
    BMCWEB_LOG_DEBUG("Get chassis path");

    constexpr std::array<std::string_view, 2> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Chassis",
        "xyz.openbmc_project.Inventory.Item.Board"};

    // get the chassis path
    dbus::utility::getSubTreePaths(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [asyncResp, chassisID](
            const boost::system::error_code ec,
            const dbus::utility::MapperGetSubTreePathsResponse& chassisPaths) {
        if (ec)
        {
            BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
            messages::internalError(asyncResp->res);
            return;
        }

        // check if the chassis path belongs to the chassis ID passed
        for (const auto& path : chassisPaths)
        {
            BMCWEB_LOG_DEBUG("Chassis Paths from Mapper ", path);
            std::string chassis =
                sdbusplus::message::object_path(path).filename();
            if (chassis != chassisID)
            {
                // this is not the chassis we are interested in
                continue;
            }

            checkAssociation(asyncResp, path, chassisID);
            return;
        }

        BMCWEB_LOG_ERROR("Chassis not found");
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisID);
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
        .privileges({{"Login"}})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& chassisId) {
        BMCWEB_LOG_DEBUG("chassis =", chassisId);
        assembly::getChassis(asyncResp, chassisId);
    });
}
} // namespace redfish
