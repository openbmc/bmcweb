#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "common.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/dbus_utils.hpp"

#include <boost/asio/error.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <string>
#include <string_view>
#include <variant>

namespace redfish
{
/**
 * @brief Get Asset properties for the assemblies associated to given chassis
 * @param[in] asyncResp - Shared pointer for asynchronous calls.
 * @param[in] assemblyIndex - Assembly index.
 * @param[in] ec - Error code if any
 * @param[in] propertiesList - List of properties.
 * chassis.
 * @return None.
 */
inline void
    getAssetProperties(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       std::size_t assemblyIndex,
                       const boost::beast::error_code& ec,
                       const dbus::utility::DBusPropertiesMap& propertiesList)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR << "DBUS response error: " << ec.message();
        messages::internalError(asyncResp->res);
        return;
    }

    BMCWEB_LOG_DEBUG << "Got " << propertiesList.size()
                     << " properties for Asset";

    nlohmann::json& assemblyArray = asyncResp->res.jsonValue["Assemblies"];
    nlohmann::json& assemblyData = assemblyArray.at(assemblyIndex);

    const std::string* partNum = nullptr;
    const std::string* serialNum = nullptr;
    const std::string* sparePartNum = nullptr;
    const std::string* modelNum = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), propertiesList, "PartNumber", partNum,
        "SerialNumber", serialNum, "SparePartNumber", sparePartNum, "Model",
        modelNum);

    if (!success)
    {
        BMCWEB_LOG_ERROR << "Failed to get Asset properties";
        messages::internalError(asyncResp->res);
        return;
    }

    if (partNum == nullptr)
    {
        BMCWEB_LOG_ERROR << "Failed to get part number";
        messages::internalError(asyncResp->res);
        return;
    }
    assemblyData["PartNumber"] = *partNum;

    if (serialNum == nullptr)
    {
        BMCWEB_LOG_ERROR << "Failed to get serial number";
        messages::internalError(asyncResp->res);
        return;
    }
    assemblyData["SerialNumber"] = *serialNum;

    if (sparePartNum == nullptr)
    {
        BMCWEB_LOG_ERROR << "Failed to get spare part number";
        messages::internalError(asyncResp->res);
        return;
    }
    assemblyData["SparePartNumber"] = *sparePartNum;

    if (modelNum == nullptr)
    {
        BMCWEB_LOG_ERROR << "Failed to get model number";
        messages::internalError(asyncResp->res);
        return;
    }
    assemblyData["Model"] = *modelNum;
}

/**
 * @brief Callback for the get assembly properties.
 * @param[in] asyncResp - Shared pointer for asynchronous calls.
 * @param[in] assemblyIndex - Assembly index.
 * @param[in] assembly - Assembly information.
 * @param[in] ec - Error code on failure.
 * @param[in] object - Mapper object.
 * @return None.
 */
inline void afterGetAssemblyProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    std::size_t assemblyIndex, const auto& assembly,
    const boost::beast::error_code& ec,
    const dbus::utility::MapperGetObject& object)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR << "DBUS response error: " << ec.message();
        messages::internalError(asyncResp->res);
        return;
    }

    for (const auto& [serviceName, interfaceList] : object)
    {
        BMCWEB_LOG_DEBUG << "ServiceName: " << serviceName;
        for (const std::string& interface : interfaceList)
        {
            BMCWEB_LOG_DEBUG << "interface: " << interface;
            if (interface == "xyz.openbmc_project.Inventory.Decorator.Asset")
            {
                sdbusplus::asio::getAllProperties(
                    *crow::connections::systemBus, serviceName, assembly,
                    "xyz.openbmc_project.Inventory.Decorator.Asset",
                    [asyncResp,
                     assemblyIndex](const boost::beast::error_code& ec2,
                                    const dbus::utility::DBusPropertiesMap&
                                        propertiesList) {
                    getAssetProperties(asyncResp, assemblyIndex, ec2,
                                       propertiesList);
                    });
            }
            else if (interface == "xyz.openbmc_project.Inventory."
                                  "Decorator.LocationCode")
            {
                sdbusplus::asio::getProperty<std::string>(
                    *crow::connections::systemBus, serviceName, assembly,
                    "xyz.openbmc_project.Inventory.Decorator.LocationCode",
                    "LocationCode",
                    [asyncResp,
                     assemblyIndex](const boost::beast::error_code& ec3,
                                    const std::string& location) {
                    if (ec3)
                    {
                        BMCWEB_LOG_ERROR << "DBUS response error: "
                                         << ec3.message();
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    nlohmann::json& assemblyArray =
                        asyncResp->res.jsonValue["Assemblies"];
                    nlohmann::json& assemblyData =
                        assemblyArray.at(assemblyIndex);

                    assemblyData["Location"]["PartLocation"]["ServiceLabel"] =
                        location;
                    });
            }
        }
    }
}

/**
 * @brief Get properties for the assemblies associated to given chassis
 * @param[in] asyncResp - Shared pointer for asynchronous calls.
 * @param[in] chassisPath - Chassis the assemblies are associated with.
 * @param[in] assemblies - list of all the assemblies associated with
 * the chassis.
 * @return None.
 */
inline void
    getAssemblyProperties(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& chassisPath,
                          const std::vector<std::string>& assemblies)
{
    BMCWEB_LOG_DEBUG << "Get properties for assembly associated";

    const std::string& chassis =
        sdbusplus::message::object_path(chassisPath).filename();

    asyncResp->res.jsonValue["Assemblies@odata.count"] = assemblies.size();

    std::size_t assemblyIndex = 0;
    for (const auto& assembly : assemblies)
    {
        nlohmann::json& tempyArray = asyncResp->res.jsonValue["Assemblies"];

        std::string dataID = "/redfish/v1/Chassis/" + chassis +
                             "/Assembly#/Assemblies/";
        dataID.append(std::to_string(assemblyIndex));

        tempyArray.push_back({{"@odata.type", "#Assembly.v1_3_0.AssemblyData"},
                              {"@odata.id", dataID},
                              {"MemberId", std::to_string(assemblyIndex)}});

        tempyArray.at(assemblyIndex)["Name"] =
            sdbusplus::message::object_path(assembly).filename();

        constexpr std::array<std::string_view, 5> interfaces = {
            "xyz.openbmc_project.Inventory.Item.Vrm",
            "xyz.openbmc_project.Inventory.Item.Tpm",
            "xyz.openbmc_project.Inventory.Item.Panel",
            "xyz.openbmc_project.Inventory.Item.Battery",
            "xyz.openbmc_project.Inventory.Item.DiskBackplane"};

        dbus::utility::getDbusObject(
            assembly, interfaces,
            [asyncResp, assemblyIndex,
             assembly](const boost::beast::error_code& ec,
                       const dbus::utility::MapperGetObject& objInfo) {
            afterGetAssemblyProperties(asyncResp, assemblyIndex, assembly, ec,
                                       objInfo);
            });

        assemblyIndex++;
    }
}

/**
 * @brief Api to check if the assemblies fetched from association Json
 * is also implemented in the system. In case the interface for that
 * assembly is not found update the list and fetch properties for only
 * implemented assemblies.
 * @param[in] asyncResp - Shared pointer for asynchronous calls.
 * @param[in] chassisPath - Chassis the assemblies are associated with.
 * @param[in] assemblies - list of all the assemblies associated with
 * the chassis.
 * @return None.
 */
inline void afterAssemblyInterface(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisPath, const std::vector<std::string>& assemblies,
    const boost::beast::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR << "D-Bus response error on GetSubTree "
                         << ec.message();
        messages::internalError(asyncResp->res);
        return;
    }

    if (subtree.empty())
    {
        BMCWEB_LOG_DEBUG << "No object paths found";
        return;
    }
    std::vector<std::string> updatedAssemblyList = {};
    for (const auto& [objectPath, serviceName] : subtree)
    {
        BMCWEB_LOG_DEBUG << "ObjectPath: " << objectPath;

        // This list will store common paths between assemblies fetched
        // from association json and assemblies which are actually
        // implemeted. This is to handle the case in which there is
        // entry in association json but implementation of interface for
        // that particular assembly is missing.
        auto it = std::find(assemblies.begin(), assemblies.end(), objectPath);
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
}

/**
 * @brief Api to check if the assemblies fetched from association Json
 * is also implemented in the system. In case the interface for that
 * assembly is not found update the list and fetch properties for only
 * implemented assemblies.
 * @param[in] asyncResp - Shared pointer for asynchronous calls.
 * @param[in] chassisPath - Chassis the assemblies are associated with.
 * @param[in] assemblies - list of all the assemblies associated with
 * the chassis.
 * @return None.
 */
inline void
    checkAssemblyInterface(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& chassisPath,
                           std::vector<std::string>& assemblies)
{
    constexpr std::array<std::string_view, 5> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Vrm",
        "xyz.openbmc_project.Inventory.Item.Tpm",
        "xyz.openbmc_project.Inventory.Item.Panel",
        "xyz.openbmc_project.Inventory.Item.Battery",
        "xyz.openbmc_project.Inventory.Item.DiskBackplane"};

    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [asyncResp, chassisPath,
         assemblies](const boost::beast::error_code& ec,
                     const dbus::utility::MapperGetSubTreeResponse& subtree) {
        afterAssemblyInterface(asyncResp, chassisPath, assemblies, ec, subtree);
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
    BMCWEB_LOG_DEBUG << "Get assembly endpoints";

    sdbusplus::message::object_path assemblyPath(chassisPath);
    assemblyPath /= "assembly";

    // if there is assembly association, look
    // for endpoints
    dbus::utility::getAssociationEndPoints(
        assemblyPath, [asyncResp, chassisPath](
                          const boost::beast::error_code& ec,
                          const dbus::utility::MapperEndPoints& endpoints) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "DBUS response error: " << ec.message();
                messages::internalError(asyncResp->res);
                return;
            }

            if (endpoints.empty())
            {
                BMCWEB_LOG_DEBUG << "There are no assembly endpoints";
                return;
            }

            std::vector<std::string> sortedAssemblyList = endpoints;
            std::sort(sortedAssemblyList.begin(), sortedAssemblyList.end());
            checkAssemblyInterface(asyncResp, chassisPath, sortedAssemblyList);
        });
}

/**
 * @brief Api to check if there is any association.
 * @param[in] asyncResp - Shared pointer for asynchronous calls.
 * @param[in] chassisPath - Chassis to which the assemblies are
 * associated.
 *
 * @return None.
 */
inline void checkAssociationInterface(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisPath)
{
    BMCWEB_LOG_DEBUG << "Check chassis for association";

    std::string chassis =
        sdbusplus::message::object_path(chassisPath).filename();

    asyncResp->res.jsonValue["Assemblies"] = nlohmann::json::array();
    asyncResp->res.jsonValue["Assemblies@odata.count"] = 0;

    // mapper call lambda
    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Association.Definitions"};

    dbus::utility::getDbusObject(
        chassisPath, interfaces,
        [asyncResp, chassisPath](const boost::beast::error_code& ec,
                                 const dbus::utility::MapperGetObject& object) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "DBUS response error: " << ec.message();
            messages::internalError(asyncResp->res);
            return;
        }

        for (const auto& [serviceName, interfaceList] : object)
        {
            BMCWEB_LOG_DEBUG << "serviceName: " << serviceName;
            for (const std::string& interface : interfaceList)
            {
                BMCWEB_LOG_DEBUG << "Interface: " << interface;
                if (interface == "xyz.openbmc_project.Association.Definitions")
                {
                    getAssemblyEndpoints(asyncResp, chassisPath);
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
inline void processChasisPaths(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisID, const boost::beast::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& chassisPaths)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR << "DBUS response error: " << ec.message();
        messages::internalError(asyncResp->res);
        return;
    }

    // check if the chassis path belongs to the chassis ID passed
    for (const std::string& path : chassisPaths)
    {
        BMCWEB_LOG_DEBUG << "Chassis Path from Mapper " << path;
        std::string chassis = sdbusplus::message::object_path(path).filename();
        if (chassis != chassisID)
        {
            // this is not the chassis we are interested in
            continue;
        }

        asyncResp->res.jsonValue["@odata.type"] = "#Assembly.v1_3_0.Assembly";
        asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/Chassis/" +
                                                chassisID + "/Assembly";
        asyncResp->res.jsonValue["Name"] = "Assembly Collection";
        asyncResp->res.jsonValue["Id"] = "Assembly";

        checkAssociationInterface(asyncResp, path);
        return;
    }

    BMCWEB_LOG_ERROR << "Chassis not found";
    messages::resourceNotFound(asyncResp->res, "Chassis", chassisID);
}

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
    BMCWEB_LOG_DEBUG << "Get path for chassis Id:" << chassisID;

    constexpr std::array<std::string_view, 2> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Chassis",
        "xyz.openbmc_project.Inventory.Item.Board"};

    // Use mapper to get chassis paths.
    dbus::utility::getSubTreePaths(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [asyncResp, chassisID](
            const boost::beast::error_code& ec,
            const dbus::utility::MapperGetSubTreePathsResponse& chassisPaths) {
        processChasisPaths(asyncResp, chassisID, ec, chassisPaths);
        });
}
} // namespace assembly

inline void
    handleAssemplyHead(App& app, const crow::Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& /* chassisId */)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/Assembly/Assembly.json>; rel=describedby");
}

/**
 * Systems derived class for delivering Assembly Schema.
 */
inline void requestRoutesAssembly(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Assembly/")
        .privileges(redfish::privileges::privilegeSetLogin)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleAssemplyHead, std::ref(app)));

    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Assembly/")
        .privileges(redfish::privileges::privilegeSetLogin)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& chassisId) {
        assembly::getChassis(asyncResp, chassisId);
        });
}
} // namespace redfish
